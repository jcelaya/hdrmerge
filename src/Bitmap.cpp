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

#include <fstream>
#include <sstream>
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
}


void Bitmap::resize(size_t w, size_t h) {
    rowWidth = w;
    numBits = w * h;
    size_t extra = numBits & 31 ? 1 : 0;
    size = (numBits >> 5) + extra;
    bits.reset(new uint32_t[size]);
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
}


void Bitmap::shift(const Bitmap & src, int dx, int dy) {
    int pos = -dy*rowWidth - dx;
    int div = pos >> 5;
    size_t b = pos & 31;
    uint32_t mask1 = allOnes << b, mask2 = ~mask1;
    for (size_t i = 0; i < size; ++i) {
        bits[i] = 0;
        if (div >= 0 && div < (int)size) {
            bits[i] += (src.bits[div] & mask1) >> b;
        }
        ++div;
        if (div >= 0 && div < (int)size) {
            bits[i] += (src.bits[div] & mask2) << (32-b);
        }
    }
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
    applyRowMask(dx);
}


void Bitmap::applyRowMask(int dx) {
    size_t a, b;
    if (dx > 0 && dx <= (int)rowWidth) {
        a = 0;
        b = dx;
    } else if (dx < 0 && dx >= -(int)rowWidth) {
        a = rowWidth + dx;
        b = rowWidth;
    } else return;
    for (size_t disp = 0; disp < numBits; disp += rowWidth) {
        size_t padiv = (disp + a) >> 5, pbdiv = (disp + b) >> 5;
        size_t pamod = (disp + a) & 31, pbmod = (disp + b) & 31;
        uint32_t amask = pamod ? allOnes >> (32 - pamod) : 0;
        uint32_t bmask = allOnes << pbmod;
        if (padiv == pbdiv) {
            uint32_t mask = amask | bmask;
            bits[padiv] &= mask;
        } else {
            bits[padiv] &= amask;
            for (size_t i = padiv + 1; i < pbdiv; ++i)
                bits[i] &= 0;
            if (pbdiv < size)
                bits[pbdiv] &= bmask;
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


void Bitmap::mtb(const uint16_t * pixels, uint16_t mth) {
    size_t i = 0;
    for (iterator p = position(0, 0); p != end(); ++p) {
        p.set(pixels[i++] > mth);
    }
    bits[size-1] &= allOnes >> (32 - (numBits & 31));
}


void Bitmap::exclusion(const uint16_t * pixels, uint16_t mth, uint16_t tolerance) {
    size_t i = 0;
    uint16_t min = mth - tolerance, max = mth + tolerance;
    for (iterator p = position(0, 0); p != end(); ++p) {
        p.set(pixels[i] <= min || pixels[i] > max);
        ++i;
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


std::string Bitmap::dumpInfo() {
    std::ostringstream oss;
    iterator it = position(0, 0);
    int height = numBits / rowWidth;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < (int)rowWidth; ++col, ++it) {
            oss << (it.get() ? '1' : '0');
        }
        oss << std::endl;
    }
    return oss.str();
}

void Bitmap::dumpFile(const std::string & fileName) {
    std::ofstream of(fileName + ".pbm");
    of << "P1\n# Foo\n" << rowWidth << " " << (numBits/rowWidth) << "\n";
    size_t tb = 0;
    for (size_t i = 0; i < size; ++i) {
        uint32_t value = bits[i];
        for (int b = 0; tb < numBits && b < 32; ++tb, ++b) {
            of << ' ' << (value % 2);
            value >>= 1;
        }
        of << "\n";
    }
}

