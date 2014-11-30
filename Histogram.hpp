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

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#include <cmath>
#include <cstdint>
#include <vector>

namespace hdrmerge {

class Histogram {
public:
    Histogram() : bins(65536), numSamples(0) {}
    template <typename Iterator> Histogram(Iterator start, Iterator end) : Histogram() {
        while (start != end) {
            addValue((uint16_t)*start++);
        }
    }

    void addValue(uint16_t v) {
        ++bins[v];
        ++numSamples;
    }
    std::size_t getNumSamples() const {
        return numSamples;
    }
    uint16_t getPercentile(double frac) const {
        std::size_t current = bins[0], limit = std::floor(numSamples * frac);
        uint16_t result = 0;
        while (current < limit)
                current += bins[++result];
        return result;
    }
    double getFraction(uint16_t value) const {
        double result = 0.0;
        for (uint16_t i = 0; i <= value; ++i) {
            result += bins[i];
        }
        return result / numSamples;
    }

private:
    std::vector<unsigned int> bins; // 65536 elements
    std::size_t numSamples;
};

}

#endif // _HISTOGRAM_H_
