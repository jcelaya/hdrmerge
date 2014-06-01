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

#include <memory>
#include "Array2D.hpp"
#include "MetaData.hpp"


namespace hdrmerge {

class Image : public Array2D<uint16_t> {
public:
    static const int scaleSteps = 6;

    Image(Array2D<uint16_t> & rawImage);
    Image(const char * f);

    bool good() const {
        return width > 0;
    }
    const MetaData & getMetaData() const {
        return metaData;
    }
    double exposureAt(size_t x, size_t y) const {
        return (*this)(x, y) * relExp;
    }
    bool isSaturated(size_t x, size_t y) const {
        return (*this)(x, y) >= satThreshold;
    }
    bool isSaturatedAround(size_t x, size_t y) const;
    double getRelativeExposure() const {
        return relExp;
    }
    bool isSameFormat(const Image & ref) const;
    size_t alignWith(const Image & r);
    void releaseAlignData() {
        scaled.reset();
    }
    void relativeExposure(const Image & nextImage);

    static bool lBeforeR(const std::unique_ptr<Image> & l, const std::unique_ptr<Image> & r) {
        return l->brightness > r->brightness;
    }

private:
    MetaData metaData;
    std::unique_ptr<Array2D<uint16_t>[]> scaled;
    uint16_t max;
    uint16_t satThreshold;
    double brightness;
    double relExp;
    double halfLightPercent;

    void subtractBlack();
    void buildImage(uint16_t * rawImage);
    void preScale();
};

} // namespace hdrmerge

#endif // _IMAGE_H_
