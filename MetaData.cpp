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
#include <ctime>
#include <algorithm>
#include <libraw/libraw.h>
#include "Log.hpp"
#include "MetaData.hpp"
using namespace hdrmerge;
using namespace std;


void adobe_cam_xyz(const string & t_make, const string & t_model, float * cam_xyz);


MetaData::MetaData() : width(0), height(0), rawWidth(0), topMargin(0), leftMargin(0), filters(0),
max(0), black(0), cblack{}, camMul{}, camXyz{}, rgbCam{}, isoSpeed(0.0), shutter(0.0), aperture(0.0), colors(0) {}


MetaData::MetaData(const char * f, const LibRaw & rawData) : fileName(f) {
    auto & r = rawData.imgdata;
    width = r.sizes.width;
    height = r.sizes.height;
    rawWidth = r.sizes.raw_width;
    topMargin = r.sizes.top_margin;
    leftMargin = r.sizes.left_margin;
    filters = r.idata.filters;
    cdesc = r.idata.cdesc;
    max = r.color.maximum;
    black = r.color.black;
    copy_n(r.color.cblack, 4, cblack);
    adjustBlack();
    copy_n(r.color.cam_mul, 4, camMul);
    adjustWhite();
    copy_n((float *)r.color.cam_xyz, 3*4, (float *)camXyz);
    copy_n((float *)r.color.rgb_cam, 4*3, (float *)rgbCam);
    isoSpeed = r.other.iso_speed;
    shutter = r.other.shutter;
    aperture = r.other.aperture;
    maker = r.idata.make;
    model = r.idata.model;
    description = r.other.desc;
    char dateTimeTmp[20] = { 0 };
    struct tm * timeinfo;
    timeinfo = localtime (&r.other.timestamp);
    strftime(dateTimeTmp, 20, "%Y:%m:%d %T", timeinfo);
    dateTime = dateTimeTmp;
    colors = r.idata.colors;
    flip = r.sizes.flip;
    switch (flip) {
        case 0: tiffOrientation = 1; break;
        case 3: tiffOrientation = 3; break;
        case 5: tiffOrientation = 8; break;
        case 6: tiffOrientation = 6; break;
        default: tiffOrientation = 9; break;
    }
    // LibRaw does not create this matrix for DNG files!!!
    if (!camXyz[0][0]) {
        adobe_cam_xyz(maker, model, (float *)camXyz);
    }
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


void MetaData::adjustWhite() {
    float green = camMul[1];
    for (int c = 0; c < 3; ++c) {
        camMul[c] /= green;
    }
    camMul[3] = camMul[1];
}


void MetaData::dumpInfo() const {
    // Show idata
    Log::msg(Log::DEBUG, "Image ", fileName, ", ", width, 'x', height, ", by ", maker, ", model ", model);
    Log::msg(Log::DEBUG, colors, " colors with mask ", hex, filters, dec, ", ", cdesc, ", ", max, " max value");
    // Show other
    Log::msg(Log::DEBUG, "ISO:", isoSpeed, " shutter:1/", (1.0/shutter), " aperture:f", aperture, " exposure:", logExp(), " steps");
    // Show matrices
    Log::msg(Log::DEBUG, "White balance: ", camMul[0], ' ', camMul[1], ' ', camMul[2], ' ', camMul[3]);
}
