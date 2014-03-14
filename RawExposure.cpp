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

#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <tiff.h>
#include <tiffio.h>
#include <libraw/libraw.h>
#include <pfs-1.2/pfs.h>
#include "RawExposure.hpp"
using namespace std;
using namespace hdrmerge;


bool RawExposure::isWrongFormat(const RawExposure * ref) {
    auto & r = rawData.imgdata;
    return (ref != nullptr &&
        (ref->rawData.imgdata.sizes.raw_width != r.sizes.raw_width
        || ref->rawData.imgdata.sizes.raw_height != r.sizes.raw_height
        || ref->rawData.imgdata.idata.filters != r.idata.filters
        || strcmp(ref->rawData.imgdata.idata.cdesc, r.idata.cdesc)));
}


RawExposure::LoadResult RawExposure::load(const RawExposure * ref) {
    int error = rawData.open_file(fileName.c_str());
    if (error != 0) {
        return LOAD_OPEN_FAIL;
    }
    if (isWrongFormat(ref)) {
        return LOAD_FORMAT_FAIL;
    }

    rawData.unpack();
    auto & r = rawData.imgdata;
    img = r.rawdata.raw_image;
    if (img == nullptr) {
        return LOAD_FORMAT_FAIL;
    }
    max = r.color.maximum;
    subtractBlack();
    computeLogExp();

    unsigned int w = r.sizes.raw_width;
    unsigned int h = r.sizes.raw_height;
    cerr << "Loaded image " << fileName << ", " << w << 'x' << h << ", " << (w * h * 2) << " bytes allocated" << endl;

    return LOAD_SUCCESS;
}


void RawExposure::subtractBlack() {
    unsigned int rowWidth = rawData.imgdata.sizes.raw_width;
    unsigned int rowDisp = 0;
    unsigned int black = rawData.imgdata.color.black;
    unsigned int * cblack = rawData.imgdata.color.cblack;
    for (unsigned int row = 0; row < rawData.imgdata.sizes.raw_height; ++row) {
        for (unsigned int col = 0; col < rowWidth; ++col) {
            img[rowDisp + col] -= black + cblack[rawData.FC(row, col)];
        }
        rowDisp += rowWidth;
    }
    max -= black;
}


void RawExposure::computeLogExp() {
    auto & o = rawData.imgdata.other;
    logExp = log2(o.iso_speed * o.shutter / (100.0 * o.aperture * o.aperture));
}


void RawExposure::dumpInfo() const {
    auto & r = rawData.imgdata;
    // Show idata
    cerr << "Picture by " << r.idata.make << ", model " << r.idata.model << endl;
    cerr << r.idata.colors << " colors with mask " << hex << r.idata.filters << dec << ", " << r.idata.cdesc << endl;
    // Show sizes
    cerr << r.sizes.raw_width << 'x' << r.sizes.raw_height << " (" << r.sizes.width << 'x' << r.sizes.height << '+'
        << r.sizes.left_margin << ',' << r.sizes.top_margin << ") aspect:" << r.sizes.pixel_aspect << ", flip:"
        << r.sizes.flip << ", raw_pitch:" << r.sizes.raw_pitch << endl;
    // Show color
    cerr << r.color.maximum << " max value, black at " << r.color.black << endl;
    // Show other
    cerr << "ISO:" << r.other.iso_speed << " shutter:1/" << (1.0/r.other.shutter) << " aperture:f" << r.other.aperture << endl;
    cerr << "Exposure (log) " << logExp << " steps, pixels saturate at " << max << endl;
    // Show rawdata
}
