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

#ifndef _BITMAP_HPP_
#define _BITMAP_HPP_

#include <cstdint>
#include <memory>

namespace hdrmerge {

class Bitmap {
public:
    Bitmap() : Bitmap(0, 0) {}
    Bitmap(size_t w, size_t h);

    void shift(const Bitmap & src, int dx, int dy);
    void bitwiseXor(const Bitmap & r);
    void bitwiseAnd(const Bitmap & r);
    void mtb(const uint16_t * pixels, uint16_t mth);
    void exclusion(const uint16_t * pixels, uint16_t mth, uint16_t tolerance);
    void reset() {
        for (size_t i = 0; i < size; ++i) {
            bits[i] = 0;
        }
    }

    class iterator {
    public:
        void set(bool v = true) const {
            *div = v ? *div | mask : *div & ~mask;
        }
        void reset() {
            *div &= ~mask;
        }
        bool get() const {
            return *div & mask;
        }
        bool operator==(const iterator & r) const {
            return div == r.div && mask == r.mask;
        }
        bool operator!=(const iterator & r) const {
            return !(*this == r);
        }
        iterator & operator++() {
            mask <<= 1;
            if (!mask) {
                mask = 1;
                div++;
            }
            return *this;
        }
        iterator & operator+=(size_t h) {
            size_t rem = h & 31;
            div += h >> 5;
            uint32_t newmask = mask << rem;
            if (newmask) {
                mask = newmask;
            } else {
                mask >>= 32 - rem;
                ++div;
            }
            return *this;
        }
    private:
        friend class Bitmap;
        uint32_t * div;
        uint32_t mask;
        iterator(Bitmap & b, size_t pos) {
            div = &b.bits[pos >> 5];
            mask = 1 << (pos & 31);
        }
    };

    iterator position(size_t x, size_t y) {
        return iterator(*this, y*rowWidth + x);
    }
    const iterator position(size_t x, size_t y) const {
        return iterator(*const_cast<Bitmap *>(this), y*rowWidth + x);
    }
    const iterator end() const {
        return iterator(*const_cast<Bitmap *>(this), numBits);
    }

    size_t count() const;
    size_t getWidth() const {
        return rowWidth;
    }
    void resize(size_t w, size_t h);

    std::string dumpInfo();
    void dumpFile(const std::string & fileName);

private:
    static const int ones[256];
    static const uint32_t allOnes = -1;

    std::unique_ptr<uint32_t[]> bits;
    size_t rowWidth, size, numBits;

    void applyRowMask(int dx);
};

} // namespace hdrmerge

#endif // _BITMAP_HPP_
