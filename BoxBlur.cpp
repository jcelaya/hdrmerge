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
#include "BoxBlur.hpp"

namespace hdrmerge {

void BoxBlur::blur(size_t radius) {
    // From http://blog.ivank.net/fastest-gaussian-blur.html
    tmp.reset(new float[width*height]);
    size_t hr = std::round(radius*0.39);
    boxBlur(hr);
    boxBlur(hr);
    boxBlur(hr);
    tmp.reset();
}


void BoxBlur::boxBlur(size_t radius) {
    boxBlurH(radius);
    data.swap(tmp);
    boxBlurT(radius);
    data.swap(tmp);
}


void BoxBlur::boxBlurH(size_t r) {
    float iarr = 1.0 / (r+r+1);
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < height; ++i) {
        size_t ti = i * width, li = ti, ri = ti + r;
        float val = data[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += data[li + j];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += data[ri++] - data[li];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = r + 1; j < width - r; ++j) {
            val += data[ri++] - data[li++];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = width - r; j < width; ++j) {
            val += data[ri - 1] - data[li++];
            tmp[ti++] = val*iarr;
        }
    }
}


void BoxBlur::boxBlurT(size_t r) {
    float iarr = 1.0 / (r+r+1);
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < width; ++i) {
        size_t ti = i, li = ti, ri = ti + r*width;
        float val = data[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += data[li + j*width];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += data[ri] - data[li];
            tmp[ti] = val*iarr;
            ri += width;
            ti += width;
        }
        for (size_t j = r + 1; j < height - r; ++j) {
            val += data[ri] - data[li];
            tmp[ti] = val*iarr;
            li += width;
            ri += width;
            ti += width;
        }
        for (size_t j = height - r; j < height; ++j) {
            val += data[ri - width] - data[li];
            tmp[ti] = val*iarr;
            li += width;
            ti += width;
        }
    }
}

} // namespace hdrmerge
