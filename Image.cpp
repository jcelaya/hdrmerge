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
#include "Image.hpp"
using namespace std;
using namespace hdrmerge;


bool Image::isWrongFormat(const Image * ref) const {
    auto & r = rawData.imgdata, & rr = ref->rawData.imgdata;
    return (ref != nullptr &&
        (rr.sizes.raw_width != r.sizes.raw_width
        || rr.sizes.raw_height != r.sizes.raw_height
        || rr.idata.filters != r.idata.filters
        || strcmp(rr.idata.cdesc, r.idata.cdesc)));
}


Image::LoadResult Image::load(const Image * ref) {
    int error = rawData.open_file(fileName.c_str());
    if (error != 0) {
        return LOAD_OPEN_FAIL;
    }
    if (isWrongFormat(ref)) {
        return LOAD_FORMAT_FAIL;
    }

    rawData.unpack();
    auto & r = rawData.imgdata;
    pixel = r.rawdata.raw_image;
    if (pixel == nullptr) {
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


void Image::subtractBlack() {
    unsigned int rowWidth = rawData.imgdata.sizes.raw_width;
    unsigned int rowDisp = 0;
    unsigned int black = rawData.imgdata.color.black;
    unsigned int * cblack = rawData.imgdata.color.cblack;
    for (unsigned int row = 0; row < rawData.imgdata.sizes.raw_height; ++row) {
        for (unsigned int col = 0; col < rowWidth; ++col) {
            pixel[rowDisp + col] -= black + cblack[rawData.FC(row, col)];
        }
        rowDisp += rowWidth;
    }
    max -= black;
}


void Image::computeLogExp() {
    auto & o = rawData.imgdata.other;
    logExp = log2(o.iso_speed * o.shutter / (100.0 * o.aperture * o.aperture));
}


void Image::computeRelExp() {
    if (nextImage == nullptr) {
        relExp = immExp = 1.0;
    } else {
        // Calculate median relative exposure
        uint16_t min = (uint16_t)floor(max * 0.2);
        vector<float> samples;
        uint16_t * rpix = nextImage->pixel, * end = pixel + rawData.imgdata.sizes.raw_width * rawData.imgdata.sizes.raw_height;
        for (uint16_t * pix = pixel; pix < end; rpix++, pix++) {
            // Only sample those pixels that are in the linear zone
            if (*rpix < max && *rpix > min && *pix < max && *pix > min)
                samples.push_back((float)*rpix / *pix);
        }
        std::sort(samples.begin(), samples.end());
        immExp = samples[samples.size() / 2];
        relExp = immExp * nextImage->relExp;
        cerr << "Relative exposure: " << (1.0/relExp) << '(' << log2(1.0/relExp) << " EV)" << endl;
    }
}


void Image::dumpInfo() const {
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
