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
#include "Histogram.hpp"
using namespace std;
using namespace hdrmerge;


void Image::buildImage(uint16_t * rawImage, MetaData * md) {
    metaData.reset(md);
    width = metaData->width;
    height = metaData->height;
    max = metaData->max;
    scaledData.emplace_back(new uint16_t[width*height]);
    pixel = scaledData.back().get();
    std::copy_n(rawImage, width*height, pixel);
    subtractBlack();
    logExp = metaData->logExp();
    preScale();
    metaData->dumpInfo();
}


Image::Image(uint16_t * rawImage, const MetaData & md) : dx(0), dy(0), relExp(1.0), immExp(1.0) {
    buildImage(rawImage, new MetaData(md));
}


Image::Image(const char * f) : pixel(nullptr), dx(0), dy(0), max(0), relExp(1.0), immExp(1.0) {
    LibRaw rawData;
    int error = rawData.open_file(f);
    if (error == 0) {
        rawData.unpack();
        uint16_t * rawImage = rawData.imgdata.rawdata.raw_image;
        if (rawImage != nullptr) {
            buildImage(rawImage, new MetaData(f, rawData));
        }
    }
}


bool Image::isSameFormat(const Image & ref) const {
    return metaData.get() && ref.metaData.get() && metaData->isSameFormat(*ref.metaData);
}


void Image::subtractBlack() {
    size_t rowDisp = 0;
    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            pixel[rowDisp + col] -= metaData->blackAt(row, col);
        }
        rowDisp += width;
    }
    max -= metaData->black;
}


void Image::relativeExposure(const Image & r, size_t w, size_t h) {
    Histogram hist;
    for (size_t y1 = -dy, y2 = -r.dy; y1 < h - dy; ++y1, ++y2) {
        for (size_t x1 = -dx, x2= -r.dx; x1 < w - dx; ++x1, ++x2) {
            uint32_t v = pixel[y1*width + x1], nv = r.pixel[y2*width + x2];
            if (v > nv && v < max) {
                hist.addValue((uint16_t)((65536 * nv) / v));
            }
        }
    }
    immExp = hist.getMedian(0.5) / 65536.0;
    relExp = immExp * r.relExp;
}


void Image::alignWith(const Image & r, double threshold, double tolerance) {
    if (!good() || !r.good()) return;
    dx = dy = 0;
    uint16_t tolPixels = (uint16_t)std::floor(32768*tolerance);
    for (size_t s = scaleSteps - 1; s > 0; --s) {
        size_t curWidth = width >> s;
        size_t curHeight = height >> s;
        Histogram hist1(r.scaledData[s].get(), r.scaledData[s].get() + curWidth*curHeight);
        Histogram hist2(scaledData[s].get(), scaledData[s].get() + curWidth*curHeight);
        uint16_t mth1 = hist1.getMedian(threshold);
        uint16_t mth2 = hist2.getMedian(threshold);
        Bitmap mtb1(curWidth, curHeight), mtb2(curWidth, curHeight),
        excl1(curWidth, curHeight), excl2(curWidth, curHeight);
        mtb1.mtb(r.scaledData[s].get(), mth1);
        mtb2.mtb(scaledData[s].get(), mth2);
        excl1.exclusion(r.scaledData[s].get(), mth1, tolPixels);
        excl2.exclusion(scaledData[s].get(), mth2, tolPixels);
        size_t minError = curWidth*curHeight;
        Bitmap shiftMtb(curWidth, curHeight), shiftExcl(curWidth, curHeight);
        int curDx = dx, curDy = dy;
        for (int i : {-1, 0, 1}) {
            for (int j : {-1, 0, 1}) {
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
