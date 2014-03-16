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

#ifndef _RAWEXPOSURE_H_
#define _RAWEXPOSURE_H_

#include <vector>
#include <string>
#include <memory>
//#include "config.h"
#include <libraw/libraw.h>


namespace hdrmerge {

class RawExposure {
public:
    enum LoadResult {
        LOAD_SUCCESS = 0,
        LOAD_OPEN_FAIL = 1,
        LOAD_FORMAT_FAIL = 2,
    };

    RawExposure(const char * f) : fileName(f), img(nullptr), max(0), logExp(0.0), relExp(1.0), nextExposure(nullptr), immExp(1.0) {}
    ~RawExposure() {
        rawData.recycle();
    }

    bool isWrongFormat(const RawExposure * ref);

    LoadResult load(const RawExposure * ref);

    /// Order by brightness
    bool operator<(const RawExposure & r) const { return logExp > r.logExp; }

    void dumpInfo() const;

private:
    std::string fileName;
    LibRaw rawData;
    uint16_t * img;
    uint16_t max;
    double logExp;          ///< Logarithmic exposure, from metadata
    double relExp;          ///< Relative exposure, from data
    RawExposure * nextExposure;
    double immExp;          ///< Exposure relative to the next image

    void subtractBlack();
    void computeLogExp();
};

} // namespace hdrmerge

#endif // _RAWEXPOSURE_H_
