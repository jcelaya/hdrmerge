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

#ifndef _RAWEXPOSURESTACK_H_
#define _RAWEXPOSURESTACK_H_

#include <vector>
#include <string>
#include <memory>
//#include "config.h"
#include <libraw/libraw.h>


namespace hdrmerge {

class RawExposureStack {
public:
    enum LoadResult {
        LOAD_SUCCESS = 0,
        LOAD_OPEN_FAIL = 1,
        LOAD_PARAM_FAIL = 2,
        LOAD_FORMAT_FAIL = 3,
    };

    struct Exposure {
        std::string fileName;
        LibRaw rawData;
        uint16_t * img;
        uint16_t max;
        double logExp;          ///< Logarithmic exposure, from metadata
        double relExp;          ///< Relative exposure, from data
        double immExp;          ///< Exposure relative to the next image

        Exposure(const char * f) : fileName(f), img(nullptr), max(0), logExp(0.0), relExp(1.0), immExp(1.0) {}
        ~Exposure() {
            rawData.recycle();
        }

        LoadResult load(const Exposure * ref);

        /// Order by brightness
        bool operator<(const Exposure & r) const { return logExp > r.logExp; }

        void dumpInfo() const;

    private:
        void subtractBlack();
        void computeLogExp();
    };

    RawExposureStack() : width(0), height(0), currentScale(0) {}

    LoadResult loadImage(const char * fileName);

    /// Sort images and calculate relative exposure
    void sort();

    unsigned int size() const { return exps.size(); }

    unsigned int getWidth() const {
        return width >> currentScale;
    }

    unsigned int getHeight() const {
        return height >> currentScale;
    }

    float getRelativeExposure(unsigned int i) const {
        return exps[i].immExp;
    }

    const std::string & getFileName(unsigned int i) const {
        return exps[i].fileName;
    }

    void setRelativeExposure(unsigned int i, double re);

private:
    std::vector<Exposure> exps;   ///< Exposures, from top to bottom
    unsigned int width;     ///< Size of a row
    unsigned int height;    ///< Size of a column
    unsigned int currentScale;     ///< Current scale factor
};

} // namespace hdrmerge

#endif // _EXPOSURE_H_
