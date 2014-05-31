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
#include "Log.hpp"
using namespace std;
using namespace hdrmerge;


Image::Image(Array2D<uint16_t> & rawImage) {
    // For testing purposes
    metaData.width = metaData.rawWidth = rawImage.getWidth();
    metaData.height = rawImage.getHeight();
    metaData.max = 255;
    metaData.filters = 0x4b4b4b4b;
    buildImage(&rawImage[0]);
}


void Image::buildImage(uint16_t * rawImage) {
    resize(metaData.width, metaData.height);
    size_t size = width*height;
    brightness = 0.0;
    uint16_t maxPerColor[4] = {0, 0, 0, 0};
    for (size_t y = 0, ry = metaData.topMargin; y < height; ++y, ++ry) {
        for (size_t x = 0, rx = metaData.leftMargin; x < width; ++x, ++rx) {
            uint16_t v = rawImage[ry*metaData.rawWidth + rx];
            (*this)(x, y) = v;
            brightness += v;
            if (v > maxPerColor[metaData.FC(x, y)]) {
                maxPerColor[metaData.FC(x, y)] = v;
            }
        }
    }
    max = metaData.max;
    for (int c = 0; c < 4; ++c) {
        if (maxPerColor[c] < max) {
            max = maxPerColor[c];
        }
    }
    relExp = 65535.0 / max;
    brightness /= size;
    subtractBlack();
    metaData.max = max;
    satThreshold = 0.99*max;
    preScale();
    metaData.dumpInfo();
}


Image::Image(const char * f) {
    LibRaw rawProcessor;
    auto & d = rawProcessor.imgdata;
    if (rawProcessor.open_file(f) == LIBRAW_SUCCESS) {
        libraw_decoder_info_t decoder_info;
        rawProcessor.get_decoder_info(&decoder_info);
        if(decoder_info.decoder_flags & LIBRAW_DECODER_FLATFIELD
                && d.idata.colors == 3 && d.idata.filters > 1000
                && rawProcessor.unpack() == LIBRAW_SUCCESS) {
            metaData.fromLibRaw(f, rawProcessor);
            buildImage(d.rawdata.raw_image);
        }
    }
}


void Image::subtractBlack() {
    if (metaData.hasBlack()) {
        for (size_t y = 0, pos = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x, ++pos) {
                if ((*this)[pos] > metaData.blackAt(x, y)) {
                    (*this)[pos] -= metaData.blackAt(x, y);
                } else {
                    (*this)[pos] = 0;
                }
            }
        }
        max -= metaData.black;
    }
}


bool Image::isSameFormat(const Image & ref) const {
    return metaData.isSameFormat(ref.metaData);
}


void Image::relativeExposure(const Image & r) {
    int reldx = dx - std::max(dx, r.dx);
    int relrdx = r.dx - std::max(dx, r.dx);
    int w = width + reldx + relrdx;
    int reldy = dy - std::max(dy, r.dy);
    int relrdy = r.dy - std::max(dy, r.dy);
    int h = height + reldy + relrdy;
    uint16_t * usePixels = &data[-reldy*width - reldx];
    const uint16_t * rusePixels = &r.data[-relrdy*width - relrdx];
    // Minimize square error between images:
    // min. C(n) = sum(n*f(x) - g(x))^2  ->  n = sum(f(x)*g(x)) / sum(f(x)^2)
    double numerator = 0, denom = 0, threshold = max * 0.9;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int pos = y * width + x;
            double v = usePixels[pos];
            if (v > 0 && v < threshold) {
                double nv = rusePixels[pos];
                numerator += v * nv;
                denom += v * v;
            }
        }
    }
    double immExp = numerator / denom;
    relExp = immExp * r.relExp;
}


size_t Image::alignWith(const Image & r) {
    dx = dy = 0;
    const double tolerance = 1.0/16;
    Histogram histFull(begin(), end());
    double halfLightPercent = histFull.getFraction(max * 0.9) / 2.0;
    size_t totalError = 0;
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = width >> (s + 1);
        size_t curHeight = height >> (s + 1);
        size_t minError = curWidth*curHeight;
        Histogram hist1(r.scaled[s].begin(), r.scaled[s].end());
        Histogram hist2(scaled[s].begin(), scaled[s].end());
        uint16_t mth1 = hist1.getPercentile(halfLightPercent);
        uint16_t mth2 = hist2.getPercentile(halfLightPercent);
        uint16_t tolPixels1 = (uint16_t)std::floor(mth1*tolerance);
        uint16_t tolPixels2 = (uint16_t)std::floor(mth2*tolerance);
        Bitmap mtb1(curWidth, curHeight), mtb2(curWidth, curHeight),
        excl1(curWidth, curHeight), excl2(curWidth, curHeight);
        mtb1.mtb(r.scaled[s].begin(), mth1);
        mtb2.mtb(scaled[s].begin(), mth2);
        excl1.exclusion(r.scaled[s].begin(), mth1, tolPixels1);
        excl2.exclusion(scaled[s].begin(), mth2, tolPixels2);
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
        totalError += minError;
    }
    return totalError;
}


void Image::preScale() {
    size_t curWidth = width;
    size_t curHeight = height;
    Array2D<uint16_t> * r2 = this;

    scaled.reset(new Array2D<uint16_t>[scaleSteps]);
    for (int s = 0; s < scaleSteps; ++s) {
        scaled[s].resize(curWidth >>= 1, curHeight >>= 1);
        for (size_t y = 0, prevY = 0; y < curHeight; ++y, prevY += 2) {
            for (size_t x = 0, prevX = 0; x < curWidth; ++x, prevX += 2) {
                uint32_t value1 = (*r2)(prevX, prevY),
                    value2 = (*r2)(prevX + 1, prevY),
                    value3 = (*r2)(prevX, prevY + 1),
                    value4 = (*r2)(prevX + 1, prevY + 1);
                scaled[s](x, y) = (value1 + value2 + value3 + value4) >> 2;
            }
        }
        r2 = &scaled[s];
    }
}


bool Image::isSaturatedAround(size_t x, size_t y) const {
    if (y > dy) {
        if ((x > dx && !isSaturated(x - 1, y - 1)) ||
            !isSaturated(x, y - 1) ||
            (x < width + dx - 1 && !isSaturated(x + 1, y - 1))) {
            return false;
        }
    }
    if ((x > dx && !isSaturated(x - 1, y)) ||
        (x < width + dx - 1 && !isSaturated(x + 1, y))) {
        return false;
    }
    if (y < height + dy - 1) {
        if ((x > dx && !isSaturated(x - 1, y + 1)) ||
            !isSaturated(x, y + 1) ||
            (x < width + dx - 1 && !isSaturated(x + 1, y + 1))) {
            return false;
        }
    }
    return true;
}
