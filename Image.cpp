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

#include <libraw/libraw.h>
#include "Image.hpp"
#include "Bitmap.hpp"
#include "Histogram.hpp"
using namespace std;
using namespace hdrmerge;


Image::Image(uint16_t * rawImage, const MetaData & md) {
    buildImage(rawImage, new MetaData(md));
}

void Image::buildImage(uint16_t * rawImage, MetaData * md) {
    metaData.reset(md);
    dx = dy = 0;
    width = metaData->width;
    height = metaData->height;
    size_t size = width*height;
    brightness = 0.0;
    uint16_t maxPerColor[4] = {0, 0, 0, 0};
    rawPixels.reset(new uint16_t[size]);
    for (size_t row = 0, rrow = md->topMargin; row < height; ++row, ++rrow) {
        for (size_t col = 0, rcol = md->leftMargin; col < width; ++col, ++rcol) {
            uint16_t v = rawImage[rrow*md->rawWidth + rcol];
            rawPixels[row*width + col] = v;
            brightness += v;
            for (int c = 0; c < 4; ++c) {
                if (v > maxPerColor[md->FC(row, col)]) {
                    maxPerColor[md->FC(row, col)] = v;
                }
            }
        }
    }
    max = metaData->max;
    for (int c = 0; c < 4; ++c) {
        if (maxPerColor[c] < max) {
            max = maxPerColor[c];
        }
    }
    relExp = 65535.0 / max;
    brightness /= size;
    subtractBlack();
    metaData->dumpInfo();
    delta[0] = -width - 1;
    delta[1] = -width;
    delta[2] = -width + 1;
    delta[3] = -1;
    delta[4] = 0;
    delta[5] = 1;
    delta[6] = width - 1;
    delta[7] = width;
    delta[8] = width + 1;
}


Image::Image(const char * f) : rawPixels(nullptr) {
    LibRaw rawProcessor;
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
    // Minimize square error between images:
    // min. C(n) = sum(n*f(x) - g(x))^2  ->  n = sum(f(x)*g(x)) / sum(f(x)^2)
    double numerator = 0, denom = 0, threshold = max * 0.8;
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            size_t pos = (y - dy) * width - dx + x;
            double v = rawPixels[pos];
            if (v > 0 && v < threshold) {
                size_t rpos = (y - r.dy) * width - r.dx + x;
                double nv = r.rawPixels[rpos];
                numerator += v * nv;
                denom += v * v;
            }
        }
    }
    double immExp = numerator / denom;
    relExp = immExp * r.relExp;
}


void Image::alignWith(const Image & r) {
    if (!good() || !r.good()) return;
    dx = dy = 0;
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = width >> (s + 1);
        size_t curHeight = height >> (s + 1);
        size_t minError = curWidth*curHeight;
        Bitmap shiftMtb(curWidth, curHeight), shiftExcl(curWidth, curHeight);
        int curDx = dx, curDy = dy;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                shiftMtb.shift(mtMap[s], curDx + i, curDy + j);
                shiftExcl.shift(excludeMap[s], curDx + i, curDy + j);
                shiftMtb.bitwiseXor(r.mtMap[s]);
                shiftMtb.bitwiseAnd(r.excludeMap[s]);
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


void Image::preAlignSetup(double threshold, double tolerance) {
    size_t curWidth = width;
    size_t curHeight = height;
    uint16_t * r2 = rawPixels.get();
    unique_ptr<uint16_t[]> prev;
    mtMap.reset(new Bitmap[scaleSteps]);
    excludeMap.reset(new Bitmap[scaleSteps]);
    uint16_t tolPixels = (uint16_t)std::floor(32768*tolerance);
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
        Histogram hist(r, r + curWidth*curHeight);
        uint16_t mth = hist.getPercentile(threshold);
        mtMap[s].resize(curWidth, curHeight);
        excludeMap[s].resize(curWidth, curHeight);
        mtMap[s].mtb(r, mth);
        excludeMap[s].exclusion(r, mth, tolPixels);
        r2 = r;
        prev.reset(r);
    }
}
