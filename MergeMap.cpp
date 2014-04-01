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

#include <cmath>
#include "MergeMap.hpp"
using namespace hdrmerge;

MergeMap::MergeMap(const ImageStack & stack) : MergeMap(stack.getWidth(), stack.getHeight()) {
    for (size_t row = 0, i = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col, ++i) {
            map[i] = stack.getImageAt(col, row);
        }
    }
}


void MergeMap::blur(size_t radius) {
    // From http://blog.ivank.net/fastest-gaussian-blur.html
    size_t hr = std::round(radius*0.39);
    blurTmp.reset(new float[width*height]);
    boxBlur_4(hr);
    boxBlur_4(hr);
    boxBlur_4(hr);
}


void MergeMap::boxBlur_4(size_t radius) {
    boxBlurH_4(radius);
    map.swap(blurTmp);
    boxBlurT_4(radius);
    map.swap(blurTmp);
}


void MergeMap::boxBlurH_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    for (size_t i = 0; i < height; ++i) {
        size_t ti = i * width, li = ti, ri = ti + r;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri++] - map[li];
            blurTmp[ti++] = val*iarr;
        }
        for (size_t j = r + 1; j < width - r; ++j) {
            val += map[ri++] - map[li++];
            blurTmp[ti++] = val*iarr;
        }
        for (size_t j = width - r; j < width; ++j) {
            val += map[ri - 1] - map[li++];
            blurTmp[ti++] = val*iarr;
        }
    }
}


void MergeMap::boxBlurT_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    for (size_t i = 0; i < width; ++i) {
        size_t ti = i, li = ti, ri = ti + r*width;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j*width];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri] - map[li];
            blurTmp[ti] = val*iarr;
            ri += width;
            ti += width;
        }
        for (size_t j = r + 1; j < height - r; ++j) {
            val += map[ri] - map[li];
            blurTmp[ti] = val*iarr;
            li += width;
            ri += width;
            ti += width;
        }
        for (size_t j = height - r; j < height; ++j) {
            val += map[ri - width] - map[li];
            blurTmp[ti] = val*iarr;
            li += width;
            ti += width;
        }
    }
}
