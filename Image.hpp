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

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <vector>
#include <string>
#include <memory>
//#include "config.h"
#include <libraw/libraw.h>


namespace hdrmerge {

class Image {
public:
    enum LoadResult {
        LOAD_SUCCESS = 0,
        LOAD_OPEN_FAIL = 1,
        LOAD_FORMAT_FAIL = 2,
    };

    Image(const char * f) : fileName(f), pixel(nullptr), max(0), logExp(0.0), relExp(1.0), nextImage(nullptr), immExp(1.0) {}
//     ~Image() {
//         rawData.recycle();
//     }

    bool isWrongFormat(const Image * ref) const;

    LoadResult load(const Image * ref);

    /// Order by brightness
    bool operator<(const Image & r) const { return logExp > r.logExp; }

    void dumpInfo() const;

    void setNextImage(Image * n) {
        nextImage = n;
    }

private:
    std::string fileName;
    LibRaw rawData;
    uint16_t * pixel;
    uint16_t max;
    double logExp;          ///< Logarithmic exposure, from metadata
    double relExp;          ///< Relative exposure, from data
    Image * nextImage;
    double immExp;          ///< Exposure relative to the next image

    void subtractBlack();
    void computeLogExp();
    void computeRelExp();
};

} // namespace hdrmerge

#endif // _IMAGE_H_
