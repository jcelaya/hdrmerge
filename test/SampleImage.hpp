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
#include <OpenImageIO/imageio.h>

struct SampleImage {
    uint16_t * pixelData;
    hdrmerge::MetaData md;
    SampleImage(const std::string & f) : pixelData(nullptr) {
        OpenImageIO::ImageInput *in = OpenImageIO::ImageInput::create(f);
        if (! in)
            return;
        OpenImageIO::ImageSpec spec;
        in->open(f, spec);
        md.width = spec.width;
        md.height = spec.height;
        md.max = 65535;
        pixelData = new uint16_t[md.width * md.height];
        in->read_image(OpenImageIO::TypeDesc::UINT16, pixelData);
        in->close();
        delete in;
    }
    ~SampleImage() {
        if (pixelData != nullptr) {
            delete[] pixelData;
        }
    }

    void save(const std::string & f) {
        OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create (f);
        if (! out)
            return;
        OpenImageIO::ImageSpec spec (md.width, md.height, 1, OpenImageIO::TypeDesc::UINT8);
        out->open (f, spec);
        out->write_image (OpenImageIO::TypeDesc::UINT16, pixelData);
        out->close ();
        delete out;

    }
};
