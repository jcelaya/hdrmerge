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

#include <ctime>
#include "../Log.hpp"

template <typename Func> double measureTime(Func f) {
    std::clock_t start = std::clock();
    f();
    return (std::clock() - start) / (double) CLOCKS_PER_SEC;
}

class Timer {
public:
    Timer(const char * n) : name(n) {
        start = std::clock();
    }
    ~Timer() {
        double t = (std::clock() - start) / (double) CLOCKS_PER_SEC;
        hdrmerge::Log::msg(hdrmerge::Log::DEBUG, name, " = ", t);
    }

private:
    std::clock_t start;
    const char * name;
};
