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

#ifndef _CFAPATTERN_HPP_
#define _CFAPATTERN_HPP_

#include <functional>

namespace hdrmerge {

class CFAPattern {
public:
    void setPattern(uint32_t f, std::function<int(int, int)> fcol) {
        filters = f;
        if (filters == 9) {
            // Fujifilm X-Trans sensor
            for (int row = 0; row < 6; ++row) {
                for (int col = 0; col < 6; ++col) {
                    xtrans[row][col] = fcol(row, col);
                }
            }
        }
    }

    bool operator==(const CFAPattern & r) const {
        return filters == r.filters;
    }

    uint8_t operator()(int x, int y) const {
        // (x, y) is relative to the ACTIVE AREA
        if (filters == 9) {
            return xtrans[(y + 6) % 6][(x + 6) % 6];
        } else {
            return (filters >> (((y << 1 & 14) | (x & 1)) << 1) & 3);
        }
    }
    
    bool canAlign() const  {
        uint8_t * f = (uint8_t *)&filters;
        return f[0] == f[1] && f[0] == f[2] && f[0] == f[3];
    }

    uint32_t getFilters() const { return filters; }

    int getRows() const {
        if (filters == 9) return 6;
        else if ((filters & 255) == ((filters >> 8) & 255)) return 2;
        else return 8;
    }
    
    int getColumns() const {
        return filters == 9 ? 6 : 2;
    }

private:
    uint32_t filters;
    uint8_t xtrans[6][6];
};

} // namespace hdrmerge

#endif // _CFAPATTERN_HPP_
