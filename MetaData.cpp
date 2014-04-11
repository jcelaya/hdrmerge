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

#include <iostream>
#include <cmath>
#include <algorithm>
#include "MetaData.hpp"
using namespace hdrmerge;
using namespace std;


MetaData::MetaData() : width(0), height(0), filters(0), max(0), black(0), cblack{},
preMul{}, isoSpeed(0.0), shutter(0.0), aperture(0.0), colors(0) {}


MetaData::MetaData(const char * f, const LibRaw & rawData) : fileName(f) {
    auto & r = rawData.imgdata;
    width = r.sizes.raw_width;
    height = r.sizes.raw_height;
    filters = r.idata.filters;
    cdesc = r.idata.cdesc;
    max = r.color.maximum;
    black = r.color.black;
    copy_n(r.color.cblack, 4, cblack);
    adjustBlack();
    copy_n((uint16_t *)r.color.white, 8*8, (uint16_t *)white);
    copy_n(r.color.pre_mul, 4, preMul);
    copy_n(r.color.cam_mul, 4, camMul);
    copy_n((float *)r.color.rgb_cam, 3*4, (float *)rgbCam);
    isoSpeed = r.other.iso_speed;
    shutter = r.other.shutter;
    aperture = r.other.aperture;
    maker = r.idata.make;
    model = r.idata.model;
    colors = r.idata.colors;
}


double MetaData::logExp() const {
    return std::log2(isoSpeed * shutter / (100.0 * aperture * aperture));
}


void MetaData::adjustBlack() {
    uint16_t minb = cblack[0] + black;
    for (int i = 0; i < 4; ++i) {
        cblack[i] += black;
        if (minb > cblack[i]) {
            minb = cblack[i];
        }
    }
    black = minb;
}


void MetaData::dumpInfo() const {
    // Show idata
    cerr << "Image " << fileName << ", " << width << 'x' << height << ", by " << maker << ", model " << model << endl;
    cerr << colors << " colors with mask " << hex << filters << dec << ", " << cdesc << ", " << max << " max value" << endl;
    // Show other
    cerr << "ISO:" << isoSpeed << " shutter:1/" << (1.0/shutter) << " aperture:f" << aperture << " exposure:" << logExp() << " steps" << endl;
    // Show rawdata
}
