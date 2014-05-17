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
    alignedPixels = rawPixels.get();
    for (size_t y = 0, ry = md->topMargin; y < height; ++y, ++ry) {
        for (size_t x = 0, rx = md->leftMargin; x < width; ++x, ++rx) {
            uint16_t v = rawImage[ry*md->rawWidth + rx];
            rawPixels[y*width + x] = v;
            brightness += v;
            if (v > maxPerColor[md->FC(x, y)]) {
                maxPerColor[md->FC(x, y)] = v;
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
    metaData->max = max;
    satThreshold = 0.99*max;
    preScale();
    metaData->dumpInfo();
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
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                if (rawPixels[y*width + x] > metaData->blackAt(x, y)) {
                    rawPixels[y*width + x] -= metaData->blackAt(x, y);
                } else {
                    rawPixels[y*width + x] = 0;
                }
            }
        }
        max -= metaData->black;
    }
}


bool Image::isSameFormat(const Image & ref) const {
    return metaData.get() && ref.metaData.get() && metaData->isSameFormat(*ref.metaData);
}


void Image::relativeExposure(const Image & r) {
    int reldx = dx - std::max(dx, r.dx);
    int relrdx = r.dx - std::max(dx, r.dx);
    int w = width + reldx + relrdx;
    int reldy = dy - std::max(dy, r.dy);
    int relrdy = r.dy - std::max(dy, r.dy);
    int h = height + reldy + relrdy;
    uint16_t * usePixels = &rawPixels[-reldy*width - reldx];
    uint16_t * rusePixels = &r.rawPixels[-relrdy*width - relrdx];
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


void Image::alignWith(const Image & r) {
    if (!good() || !r.good()) return;
    dx = dy = 0;
    const double tolerance = 1.0/16;
    Histogram histFull(rawPixels.get(), rawPixels.get() + width*height);
    double halfLightPercent = histFull.getFraction(max * 0.9) / 2.0;
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = width >> (s + 1);
        size_t curHeight = height >> (s + 1);
        size_t minError = curWidth*curHeight;
        Histogram hist1(r.scaled[s].get(), r.scaled[s].get() + curWidth*curHeight);
        Histogram hist2(scaled[s].get(), scaled[s].get() + curWidth*curHeight);
        uint16_t mth1 = hist1.getPercentile(halfLightPercent);
        uint16_t mth2 = hist2.getPercentile(halfLightPercent);
        uint16_t tolPixels1 = (uint16_t)std::floor(mth1*tolerance);
        uint16_t tolPixels2 = (uint16_t)std::floor(mth2*tolerance);
        Bitmap mtb1(curWidth, curHeight), mtb2(curWidth, curHeight),
        excl1(curWidth, curHeight), excl2(curWidth, curHeight);
        mtb1.mtb(r.scaled[s].get(), mth1);
        mtb2.mtb(scaled[s].get(), mth2);
        excl1.exclusion(r.scaled[s].get(), mth1, tolPixels1);
        excl2.exclusion(scaled[s].get(), mth2, tolPixels2);
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
    uint16_t * r2 = rawPixels.get();

    scaled.reset(new unique_ptr<uint16_t[]>[scaleSteps]);
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
        r2 = r;
        scaled[s].reset(r);
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
