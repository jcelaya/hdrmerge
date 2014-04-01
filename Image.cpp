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
#include <iostream>
#include <iomanip>
#include "Image.hpp"
#include "Bitmap.hpp"
#include "Histogram.hpp"
using namespace std;
using namespace hdrmerge;


Image::Image(uint16_t * rawImage, const MetaData & md) : rawPixels(nullptr), image(nullptr), dx(0), dy(0) {
    rawProcessor.imgdata.rawdata.raw_alloc = rawPixels = rawImage;
    metaData.reset(new MetaData(md));
    rwidth = iwidth = metaData->width;
    rheight = iheight = metaData->height;
    size_t size = iwidth*iheight;
    image = rawProcessor.imgdata.image = (uint16_t (*)[4])calloc(size, 4*2);
    for (size_t row = 0; row < iheight; ++row) {
        for (size_t col = 0; col < iwidth; ++col) {
            size_t pos = row*iwidth + col;
            image[pos][md.FC(row, col)] = rawPixels[pos];
        }
    }
    max = metaData->max;
    relExp = 65535.0 / max;
    logExp = metaData->logExp();
    preScale();
    metaData->dumpInfo();
}


Image::Image(const char * f) : rawPixels(nullptr), image(nullptr), dx(0), dy(0) {
    auto & d = rawProcessor.imgdata;
    if (rawProcessor.open_file(f) == LIBRAW_SUCCESS) {
        libraw_decoder_info_t decoder_info;
        rawProcessor.get_decoder_info(&decoder_info);
        if(decoder_info.decoder_flags & LIBRAW_DECODER_FLATFIELD
                && d.idata.colors == 3 && d.idata.filters > 1000
                && rawProcessor.unpack() == LIBRAW_SUCCESS) {
            rawProcessor.raw2image();
            rawProcessor.subtract_black();
            rawPixels = d.rawdata.raw_image;
            rwidth = d.sizes.raw_width;
            rheight = d.sizes.raw_height;
            image = d.image;
            metaData.reset(new MetaData(f, rawProcessor));
            iwidth = metaData->width;
            iheight = metaData->height;
            max = metaData->max;
            relExp = 65535.0 / max;
            logExp = metaData->logExp();
            preScale();
            metaData->dumpInfo();
        }
    }
}


bool Image::isSameFormat(const Image & ref) const {
    return metaData.get() && ref.metaData.get() && metaData->isSameFormat(*ref.metaData);
}


void Image::relativeExposure(const Image & r, size_t w, size_t h) {
    Histogram hist;
    double metaImmExp = 1.0 / (1 << (int)(getMetaData().logExp() - r.getMetaData().logExp()));
    for (size_t y = 0; y < h; ++y) {
        size_t pos = (y - dy) * iwidth - dx, rpos = (y - r.dy) * iwidth - r.dx;
        for (size_t x = 0; x < w; ++x, ++pos, ++rpos) {
            int color = 0;
            while (color < 4 && image[pos][color] == 0) ++color;
            if (color < 4) {
                double v = image[pos][color], nv = r.image[rpos][color];
                double ratio = nv / v;
                if (abs(ratio - metaImmExp) / ratio <= 0.025 && abs(ratio - metaImmExp) / metaImmExp <= 0.025) {
                //if (v > nv && v < max && nv > 0.125*max) {
                    hist.addValue((uint16_t)(65536 * ratio));
                }
            }
        }
    }
    double immExp = hist.getPercentile(0.5) / 65536.0;
    for (double i = 0.0; i <= 1.05; i += 0.1) {
        cout << setprecision(5) << (hist.getPercentile(i) / 65536.0) << ',';
    }
    cout << hist.getNumSamples() << '/' << (rwidth*rheight) << " samples" << endl;
    relExp = immExp * r.relExp;
}


void Image::alignWith(const Image & r, double threshold, double tolerance) {
    if (!good() || !r.good()) return;
    dx = dy = 0;
    uint16_t tolPixels = (uint16_t)std::floor(32768*tolerance);
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = rwidth >> (s + 1);
        size_t curHeight = rheight >> (s + 1);
        Histogram hist1(r.grayscalePics[s].get(), r.grayscalePics[s].get() + curWidth*curHeight);
        Histogram hist2(grayscalePics[s].get(), grayscalePics[s].get() + curWidth*curHeight);
        uint16_t mth1 = hist1.getPercentile(threshold);
        uint16_t mth2 = hist2.getPercentile(threshold);
        Bitmap mtb1(curWidth, curHeight), mtb2(curWidth, curHeight),
        excl1(curWidth, curHeight), excl2(curWidth, curHeight);
        mtb1.mtb(r.grayscalePics[s].get(), mth1);
        mtb2.mtb(grayscalePics[s].get(), mth2);
        excl1.exclusion(r.grayscalePics[s].get(), mth1, tolPixels);
        excl2.exclusion(grayscalePics[s].get(), mth2, tolPixels);
        size_t minError = curWidth*curHeight;
        Bitmap shiftMtb(curWidth, curHeight), shiftExcl(curWidth, curHeight);
        int curDx = dx, curDy = dy;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                shiftMtb.shift(mtb2, curDx + i, curDy + j);
                shiftExcl.shift(excl2, curDx + i, curDy + j);
                shiftMtb.bitwiseXor(mtb1);
                shiftMtb.bitwiseAnd(excl1);
                shiftMtb.bitwiseAnd(shiftExcl);
                size_t err = shiftMtb.count();
                if (err < minError) {
                    dx = curDx + i;
                    dy = curDy + j;
                    minError = err;
                }
            }
        }
        dx <<= 1;
        dy <<= 1;
    }
}


void Image::preScale() {
    size_t curWidth = rwidth;
    size_t curHeight = rheight;
    uint16_t * r2 = rawPixels;
    for (int s = 0; s < scaleSteps; ++s) {
        size_t prevWidth = curWidth;
        curWidth >>= 1;
        curHeight >>= 1;
        uint16_t * r = new uint16_t[curWidth * curHeight];
        for (size_t y = 0, prevY = 0; y < curHeight; ++y, prevY += 2) {
            for (size_t x = 0, prevX = 0; x < curWidth; ++x, prevX += 2) {
                uint32_t value1 = r2[prevY*prevWidth + prevX],
                    value2 = r2[prevY*prevWidth + prevX + 1],
                    value3 = r2[(prevY + 1)*prevWidth + prevX],
                    value4 = r2[(prevY + 1)*prevWidth + prevX + 1];
                r[y*curWidth + x] = (value1 + value2 + value3 + value4) >> 2;
            }
        }
        grayscalePics.emplace_back(r);
        r2 = r;
    }
}
