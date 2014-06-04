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
#include <QDateTime>
#include <libraw/libraw.h>
#include "Log.hpp"
#include "RawParameters.hpp"
using namespace hdrmerge;
using namespace std;


void adobe_cam_xyz(const string & t_make, const string & t_model, float * cam_xyz);


RawParameters::RawParameters() : width(0), height(0), rawWidth(0), topMargin(0), leftMargin(0), filters(0),
max(0), black(0), cblack{}, camMul{}, camXyz{}, rgbCam{}, isoSpeed(0.0), shutter(0.0), aperture(0.0), colors(0) {}


void RawParameters::fromLibRaw(const LibRaw & rawData) {
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
    if (camMul[0] == 0 || camMul[1] == 0 || camMul[2] == 0) {
        copy_n(r.color.pre_mul, 4, camMul);
    }
    adjustWhite();
    copy_n((float *)r.color.cam_xyz, 3*4, (float *)camXyz);
    copy_n((float *)r.color.rgb_cam, 4*3, (float *)rgbCam);
    isoSpeed = r.other.iso_speed;
    shutter = r.other.shutter;
    aperture = r.other.aperture;
    maker = r.idata.make;
    model = r.idata.model;
    description = r.other.desc;
    QDateTime dateTimeTmp = QDateTime::fromTime_t(r.other.timestamp);
    QString dateTimeTmpText = dateTimeTmp.toString("yyyy:MM:dd hh:mm:ss");
    dateTime = dateTimeTmpText.toAscii().constData();
    colors = r.idata.colors;
    flip = r.sizes.flip;
    switch ((flip + 3600) % 360) {
        case 270: flip = 5; break;
        case 180: flip = 3; break;
        case  90: flip = 6; break;
    }
    switch (flip) {
        case 0: tiffOrientation = 1; break;
        case 3: tiffOrientation = 3; break;
        case 5: tiffOrientation = 8; break;
        case 6: tiffOrientation = 6; break;
        default: tiffOrientation = 9; break;
    }
    // LibRaw does not create this matrix for DNG files!!!
    if (!camXyz[0][0]) {
        Log::msg(Log::DEBUG, "Generating cam_xyz matrix for ", maker, " ", model);
        adobe_cam_xyz(maker, model, (float *)camXyz);
    }
}


double RawParameters::logExp() const {
    return std::log2(isoSpeed * shutter / (100.0 * aperture * aperture));
}


void RawParameters::adjustBlack() {
    uint16_t minb = cblack[0] + black;
    for (int i = 0; i < 4; ++i) {
        cblack[i] += black;
        if (minb > cblack[i]) {
            minb = cblack[i];
        }
    }
    black = minb;
}


void RawParameters::adjustWhite() {
    // FIXME: Set to 1, because I don't know how to
    // compute the correct white balance (if at all posible)
    if (camMul[0] == 0 || camMul[1] == 0 || camMul[2] == 0) {
        camMul[0] = camMul[1] = camMul[2] = 1;
    }
    float green = camMul[1];
    for (int c = 0; c < 3; ++c) {
        camMul[c] /= green;
    }
    camMul[3] = camMul[1];
}


void RawParameters::dumpInfo() const {
    // Show idata
    Log::msg(Log::DEBUG, width, 'x', height, ", by ", maker, ' ' , model, ", ", hex, filters, dec, ' ', cdesc, ", ", max, " sat, flip ", flip);
    // Show other
    Log::msg(Log::DEBUG, isoSpeed, "ISO 1/", (1.0/shutter), "sec f", aperture, " EV:", logExp(), " wb: ", camMul[0], ' ', camMul[1], ' ', camMul[2], ' ', camMul[3]);
}
