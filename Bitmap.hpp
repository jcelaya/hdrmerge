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
#include <cstdint>
#include <memory>

namespace hdrmerge {

class Bitmap {
public:
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

    void set(size_t x, size_t y) {
        size_t pos = y*rowWidth + x;
        if (pos < numBits) {
            Position p(y*rowWidth + x);
            bits[p.div] |= p.mask;
        }
    }
    void reset(size_t x, size_t y) {
        Position p(y*rowWidth + x);
        bits[p.div] &= ~p.mask;
    }
    bool get(size_t x, size_t y) const {
        Position p(y*rowWidth + x);
        return bits[p.div] & p.mask;
    }
    size_t count() const;
    void resize(size_t w, size_t h);

    std::string dumpInfo();
    void dumpFile();

private:
    struct Position {
        size_t div;
        size_t mod;
        uint32_t mask;
        Position(size_t pos) {
            div = pos >> 5;
            mod = pos & 31;
            mask = 1 << mod;
        }
    };
    static const int ones[256];
    static const uint32_t allOnes = -1;

    std::unique_ptr<uint32_t[]> bits;
    size_t rowWidth, size, numBits;

    void applyRowMask(int dx);
};

} // namespace hdrmerge
