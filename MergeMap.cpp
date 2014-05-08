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

#include <algorithm>
#include <cmath>
#include "MergeMap.hpp"
#include "ImageStack.hpp"
using namespace hdrmerge;

void MergeMap::generateFrom(const ImageStack & images) {
    width = images.getWidth();
    height = images.getHeight();
    size_t size = width*height;
    index.reset(new uint8_t[size]);

    std::fill_n(index.get(), size, 0);
    for (int i = 0; i < images.size() - 1; ++i) {
        const Image & img = images.getImage(i);
        for (size_t row = 0, pos = 0; row < height; ++row) {
            for (size_t col = 0; col < width; ++col, ++pos) {
                if (index[pos] == i && img.isSaturated(col, row)) {
                    ++index[pos];
                }
            }
        }
    }
}


std::unique_ptr<float[]> MergeMap::blur() const {
    return blur(3);
}


std::unique_ptr<float[]> MergeMap::blur(size_t radius) const {
    BoxBlur b(*this, radius);
    return b.getResult();
}


MergeMap::BoxBlur::BoxBlur(const MergeMap & src, size_t radius) : m(src) {
    // From http://blog.ivank.net/fastest-gaussian-blur.html
    map.reset(new float[m.width*m.height]);
    tmp.reset(new float[m.width*m.height]);
    for (size_t i = 0; i < m.width*m.height; ++i) {
        map[i] = m.index[i];
    }
    size_t hr = std::round(radius*0.39);
    boxBlur_4(hr);
    boxBlur_4(hr);
    boxBlur_4(hr);
}


void MergeMap::BoxBlur::boxBlur_4(size_t radius) {
    boxBlurH_4(radius);
    map.swap(tmp);
    boxBlurT_4(radius);
    map.swap(tmp);
}


void MergeMap::BoxBlur::boxBlurH_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    for (size_t i = 0; i < m.height; ++i) {
        size_t ti = i * m.width, li = ti, ri = ti + r;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri++] - map[li];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = r + 1; j < m.width - r; ++j) {
            val += map[ri++] - map[li++];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = m.width - r; j < m.width; ++j) {
            val += map[ri - 1] - map[li++];
            tmp[ti++] = val*iarr;
        }
    }
}


void MergeMap::BoxBlur::boxBlurT_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    for (size_t i = 0; i < m.width; ++i) {
        size_t ti = i, li = ti, ri = ti + r*m.width;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j*m.width];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri] - map[li];
            tmp[ti] = val*iarr;
            ri += m.width;
            ti += m.width;
        }
        for (size_t j = r + 1; j < m.height - r; ++j) {
            val += map[ri] - map[li];
            tmp[ti] = val*iarr;
            li += m.width;
            ri += m.width;
            ti += m.width;
        }
        for (size_t j = m.height - r; j < m.height; ++j) {
            val += map[ri - m.width] - map[li];
            tmp[ti] = val*iarr;
            li += m.width;
            ti += m.width;
        }
    }
}
