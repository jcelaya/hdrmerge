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

#include <iostream>
#include <cmath>
#include "Bitmap.hpp"
using namespace hdrmerge;

const int Bitmap::ones[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};


Bitmap::Bitmap(size_t w, size_t h) {
    resize(w, h);
    for (size_t i = 0; i < size; ++i) {
        bits[i] = 0;
    }
}


void Bitmap::resize(size_t w, size_t h) {
    rowWidth = w;
    numBits = w * h;
    size_t extra = numBits & 31 ? 1 : 0;
    size = (numBits >> 5) + extra;
    bits.reset(new uint32_t[size]);
}


void Bitmap::shift(const Bitmap & src, int dx, int dy) {
    rowWidth = src.rowWidth;
    size = src.size;
    numBits = src.numBits;
    bits.reset(new uint32_t[size]);
    int pos = -dy*rowWidth - dx;
    int div = pos >> 5;
    size_t b = pos & 31;
    uint32_t mask1 = allOnes << b, mask2 = ~mask1;
    for (size_t i = 0; i < size; ++i) {
        bits[i] = 0;
        if (div >= 0 && div < size) {
            bits[i] += (src.bits[div] & mask1) >> b;
        }
        ++div;
        if (div >= 0 && div < size) {
            bits[i] += (src.bits[div] & mask2) << (32-b);
        }
    }
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
    applyRowMask(dx);
}


void Bitmap::applyRowMask(int dx) {
    size_t a, b;
    if (dx > 0 && dx <= rowWidth) {
        a = 0;
        b = dx;
    } else if (dx < 0 && dx >= -(int)rowWidth) {
        a = rowWidth + dx;
        b = rowWidth;
    } else return;
    for (size_t disp = 0; disp < numBits; disp += rowWidth) {
        Position pa(disp + a), pb(disp + b);
        if (pa.div == pb.div) {
            uint32_t mask = (allOnes >> (32 - pa.mod)) | (allOnes << pb.mod);
            bits[pa.div] &= mask;
        } else {
            bits[pa.div] &= allOnes >> (32 - pa.mod);
            bits[pb.div] &= allOnes << pb.mod;
        }
    }
}


void Bitmap::bitwiseXor(const Bitmap & r) {
    for (size_t i = 0; i < size; ++i) {
        bits[i] ^= r.bits[i];
    }
}


void Bitmap::bitwiseAnd(const Bitmap & r) {
    for (size_t i = 0; i < size; ++i) {
        bits[i] &= r.bits[i];
    }
}


void Bitmap::mtb(const uint16_t * pixels, size_t w, size_t h, uint16_t mth) {
    resize(w, h);
    uint32_t mask = 1;
    for (size_t i = 0, pos = 0; i < numBits; ++i) {
        if (pixels[i] > mth) {
            bits[pos] |= mask;
        } else {
            bits[pos] &= ~mask;
        }
        mask <<= 1;
        if (!mask) {
            mask = 1;
            ++pos;
        }
    }
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
}


void Bitmap::exclusion(const uint16_t * pixels, size_t w, size_t h, uint16_t mth, uint16_t tolerance) {
    resize(w, h);
    uint32_t mask = 1;
    uint16_t min = mth - tolerance, max = mth + tolerance;
    for (size_t i = 0, pos = 0; i < numBits; ++i) {
        if (pixels[i] < min || pixels[i] > max) {
            bits[pos] |= mask;
        } else {
            bits[pos] &= ~mask;
        }
        mask <<= 1;
        if (!mask) {
            mask = 1;
            ++pos;
        }
    }
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
}


size_t Bitmap::count() const {
    size_t c = 0;
    for (size_t i = 0; i < size; ++i) {
        if (bits[i]) {
            uint8_t * b = (uint8_t *)&bits[i];
            c += ones[b[0]] + ones[b[1]] + ones[b[2]] + ones[b[3]];
        }
    }
    return c;
}


void Bitmap::dumpInfo() {
    size_t tb = 0;
    for (size_t i = 0; i < size; ++i) {
        uint32_t value = bits[i];
        for (int b = 0; tb < numBits && b < 32; ++tb, ++b) {
            std::cerr << (value % 2);
            value >>= 1;
        }
    }
    std::cerr << std::endl;
}
