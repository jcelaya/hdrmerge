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
#include <QImage>
#include "../src/RawParameters.hpp"
#include "../src/Array2D.hpp"
#include "../src/Log.hpp"

namespace hdrmerge {

class SampleImage : public Array2D<uint16_t> {
public:
    RawParameters params;
    SampleImage() : Array2D<uint16_t>() {}
    SampleImage(const std::string & f) : Array2D<uint16_t>() {
        QImage image;
        if (!image.load(f.c_str())) {
            Log::msg(Log::DEBUG, "Imposible to load sample image ", f);
            return;
        }
        Log::msg(Log::DEBUG, "Loaded sample image ", f, " with format ", image.format());
        resize(image.width(), image.height());
        const uchar * data = image.constBits();
        int min = 255, max = 0;
        for (size_t i = 0; i < width * height; ++i) {
            (*this)[i] = data[i];
            if (min > data[i]) min = data[i];
            if (max < data[i]) max = data[i];
        }
        Log::msg(Log::DEBUG, "Data in range ", min, " - ", max);
        params.width = params.rawWidth = width;
        params.height = height;
        params.max = 255;
        params.FC.setPattern(0x4b4b4b4b, (uint8_t (*)(int, int))0);
    }

    void save(const std::string & f) {
        QImage image(width, height, QImage::Format_RGB32);
        QRgb * data = reinterpret_cast<QRgb *>(image.bits());
        for (size_t i = 0; i < width * height; ++i) {
            int v = (*this)[i];
            data[i] = qRgb(v, v, v);
        }
        image.save(f.c_str());
    }
};

} // namespace hdrmerge
