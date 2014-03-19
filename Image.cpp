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
#include "Bitmap.hpp"
using namespace std;
using namespace hdrmerge;


Image::Image(const char * f) : fileName(f), pixel(nullptr), dx(0), dy(0),
        max(0), logExp(0.0), relExp(1.0), nextImage(nullptr), immExp(1.0) {
    LibRaw rawData;
    int error = rawData.open_file(fileName.c_str());
    if (error == 0) {
        rawData.unpack();
        auto & r = rawData.imgdata;
        if (r.rawdata.raw_image != nullptr) {
            width = r.sizes.raw_width;
            height = r.sizes.raw_height;
            scaledData.emplace_back(new uint16_t[width*height]);
            pixel = scaledData.back().get();
            std::copy_n(r.rawdata.raw_image, width*height, pixel);
            filter = r.idata.filters;
            cdesc = r.idata.cdesc;
            max = r.color.maximum;
            subtractBlack(rawData);
            computeLogExp(rawData);
            preScale();
            cerr << "Loaded image " << fileName << ", " << (width * height * 2) << " bytes allocated" << endl;
            dumpInfo(rawData);
        }
    }
}


bool Image::isWrongFormat(const Image & ref) const {
    return (width != ref.width
        || height != ref.height
        || filter != ref.filter
        || cdesc != ref.cdesc);
}


void Image::subtractBlack(const LibRaw & rawData) {
    unsigned int rowDisp = 0;
    unsigned int black = rawData.imgdata.color.black;
    const unsigned int * cblack = rawData.imgdata.color.cblack;
    for (unsigned int row = 0; row < height; ++row) {
        for (unsigned int col = 0; col < width; ++col) {
            pixel[rowDisp + col] -= black + cblack[FC(row, col)];
        }
        rowDisp += width;
    }
    max -= black;
}


void Image::computeLogExp(const LibRaw & rawData) {
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
        uint16_t * rpix = nextImage->pixel, * end = pixel + width*height;
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


void Image::alignWith(const Image & r, float threshold, float tolerance) {
    dx = dy = 0;
    size_t curWidth = width >> (scaleSteps - 1);
    size_t curHeight = height >> (scaleSteps - 1);
    uint16_t tolPixels = (uint16_t)std::floor(32768*tolerance);
    for (size_t s = scaleSteps - 1; s > 0; --s) {
        uint16_t mth1 = getMedian(scaledData[s].get(), curWidth*curHeight, threshold);
        uint16_t mth2 = getMedian(r.scaledData[s].get(), curWidth*curHeight, threshold);
        Bitmap mtb1, mtb2, excl1, excl2;
        mtb1.mtb(scaledData[s].get(), curWidth, curHeight, mth1);
        mtb2.mtb(r.scaledData[s].get(), curWidth, curHeight, mth2);
        excl1.exclusion(scaledData[s].get(), curWidth, curHeight, mth1, tolPixels);
        excl2.exclusion(r.scaledData[s].get(), curWidth, curHeight, mth2, tolPixels);
        size_t minError = curWidth*curHeight;
        int curdx = dx, curdy = dy;
        for (int i : {-1, 0, 1}) {
            for (int j : {-1, 0, 1}) {
                Bitmap shiftMtb, shiftExcl;
                shiftMtb.shift(mtb2, curdx + i, curdy + j);
                shiftExcl.shift(excl2, curdx + i, curdy + j);
                shiftMtb.bitwiseXor(mtb1);
                shiftMtb.bitwiseAnd(excl1);
                shiftMtb.bitwiseAnd(shiftExcl);
                size_t err = shiftMtb.count();
                if (err < minError) {
                    dx = curdx + i;
                    dy = curdy + j;
                    minError = err;
                }
            }
        }
        dx <<= 1;
        dy <<= 1;
    }
}


void Image::preScale() {
    scaledData.resize(1);
    size_t curWidth = width;
    size_t curHeight = height;
    for (size_t s = 1; s < scaleSteps; ++s) {
        size_t prevWidth = curWidth;
        curWidth >>= 1;
        curHeight >>= 1;
        uint16_t * r = new uint16_t[curWidth * curHeight], * r2 = scaledData.back().get();
        for (size_t y = 0, prevY = 0; y < curHeight; ++y, prevY += 2) {
            for (size_t x = 0, prevX = 0; x < curWidth; ++x, prevX += 2) {
                uint32_t value1 = r2[prevY*prevWidth + prevX],
                    value2 = r2[prevY*prevWidth + prevX + 1],
                    value3 = r2[(prevY + 1)*prevWidth + prevX],
                    value4 = r2[(prevY + 1)*prevWidth + prevX + 1];
                r[y*curWidth + x] = (value1 + value2 + value3 + value4) >> 2;
            }
        }
        scaledData.emplace_back(r);
    }
}


uint16_t Image::getMedian(const uint16_t * values, size_t size, float threshold) {
    size_t histogram[65536] = {};
    for (size_t i = 0; i < size; ++i) {
        ++histogram[values[i]];
    }
    size_t current = histogram[0], limit = std::floor(size * threshold);
    uint16_t result;
    for (result = 0; current < limit; current += histogram[++result]);
    return result;
}


void Image::dumpInfo(const LibRaw & rawData) const {
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
