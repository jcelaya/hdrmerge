/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdlib>
#include "../MetaData.hpp"
#include <QImage>
#include "../Log.hpp"

namespace hdrmerge {

struct SampleImage {
    uint16_t * pixelData;
    hdrmerge::MetaData md;
    SampleImage(const std::string & f) : pixelData(nullptr) {
        QImage image;
        if (!image.load(f.c_str())) {
            Log::msg(Log::DEBUG, "Imposible to load sample image ", f);
            return;
        }
        Log::msg(Log::DEBUG, "Loaded sample image ", f, " with format ", image.format());
        md.width = md.rawWidth = image.width();
        md.height = image.height();
        md.max = 255;
        md.filters = 0x4b4b4b4b;
        pixelData = new uint16_t[md.width * md.height];
        const uchar * data = image.constBits();
        int min = 255, max = 0;
        for (int i = 0; i < md.width * md.height; ++i) {
            pixelData[i] = data[i];
            if (min > data[i]) min = data[i];
            if (max < data[i]) max = data[i];
        }
        Log::msg(Log::DEBUG, "Data in range ", min, " - ", max);
    }
    ~SampleImage() {
        if (pixelData != nullptr) {
            delete[] pixelData;
        }
    }

    void save(const std::string & f) {
        QImage image(md.width, md.height, QImage::Format_RGB32);
        QRgb * data = reinterpret_cast<QRgb *>(image.bits());
        for (int i = 0; i < md.width * md.height; ++i) {
            int v = pixelData[i];
            data[i] = qRgb(v, v, v);
        }
        image.save(f.c_str());
    }
};

} // namespace hdrmerge