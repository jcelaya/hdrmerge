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


Image::Image(uint16_t * rawImage, const MetaData & md) {
    rawProcessor.imgdata.rawdata.raw_alloc = rawImage;
    buildImage(rawImage, new MetaData(md));
}


void Image::buildImage(uint16_t * rawImage, MetaData * md) {
    rawPixels = rawImage;
    metaData.reset(md);
    dx = dy = 0;
    width = metaData->width;
    height = metaData->height;
    size_t size = width*height;
    max = metaData->max;
    relExp = 65535.0 / max;
    logExp = metaData->logExp();
    subtractBlack();
    preScale();
    metaData->dumpInfo();
}


Image::Image(const char * f) : rawPixels(nullptr) {
    auto & d = rawProcessor.imgdata;
    if (rawProcessor.open_file(f) == LIBRAW_SUCCESS) {
        libraw_decoder_info_t decoder_info;
        rawProcessor.get_decoder_info(&decoder_info);
        if(decoder_info.decoder_flags & LIBRAW_DECODER_FLATFIELD
                && d.idata.colors == 3 && d.idata.filters > 1000
                && rawProcessor.unpack() == LIBRAW_SUCCESS) {
            buildImage(d.rawdata.raw_image, new MetaData(f, rawProcessor));
        }
    }
}


void Image::subtractBlack() {
    if (metaData->hasBlack()) {
        for (size_t row = 0; row < height; ++row) {
            for (size_t col = 0; col < width; ++col) {
                if (rawPixels[row*width + col] > metaData->blackAt(row, col)) {
                    rawPixels[row*width + col] -= metaData->blackAt(row, col);
                } else {
                    rawPixels[row*width + col] = 0;
                }
            }
        }
        max -= metaData->black;
    }
}


bool Image::isSameFormat(const Image & ref) const {
    return metaData.get() && ref.metaData.get() && metaData->isSameFormat(*ref.metaData);
}


void Image::relativeExposure(const Image & r, size_t w, size_t h) {
    Histogram hist;
    uint16_t metaImmExp = std::round(65536.0 / (1 << (int)(getMetaData().logExp() - r.getMetaData().logExp())));
    int margin = std::floor(metaImmExp * 0.025);
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            size_t pos = (y - dy) * width - dx + x, rpos = (y - r.dy) * width - r.dx + x;
            uint32_t v = rawPixels[pos], nv = r.rawPixels[rpos];
            if (v) {
                uint16_t ratio = (nv << 16) / v;
                int diff = ratio - metaImmExp;
                if (diff <= margin && diff >= -margin) {
                    hist.addValue(ratio);
                }
            }
        }
    }
    double immExp = hist.getPercentile(0.5) / 65536.0;
    for (double i = 0.0; i <= 1.05; i += 0.1) {
        cout << setprecision(5) << (hist.getPercentile(i) / 65536.0) << ',';
    }
    cout << hist.getNumSamples() << '/' << (width*height) << " samples" << endl;
    relExp = immExp * r.relExp;
}


void Image::alignWith(const Image & r, double threshold, double tolerance) {
    if (!good() || !r.good()) return;
    dx = dy = 0;
    uint16_t tolPixels = (uint16_t)std::floor(32768*tolerance);
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = width >> (s + 1);
        size_t curHeight = height >> (s + 1);
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
    size_t curWidth = width;
    size_t curHeight = height;
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
