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
#include "MetaData.hpp"


namespace hdrmerge {

class Image {
public:
    static const int scaleSteps = 6;

    Image(uint16_t * rawImage, const MetaData & md);
    Image(const char * f);

    bool good() const {
        return pixel != nullptr;
    }
    size_t getWidth() const {
        return width;
    }
    size_t getHeight() const {
        return height;
    }
    int getDeltaX() const {
        return dx;
    }
    int getDeltaY() const {
        return dy;
    }
    bool isSameFormat(const Image & ref) const;
    void alignWith(const Image & r, float threshold, float tolerance);
    void displace(int newDx, int newDy) {
        dx += newDx;
        dy += newDy;
    }
    const uint16_t * getPixels(int i) {
        return scaledData[i].get();
    }

    static bool comparePointers(const std::unique_ptr<Image> & l, const std::unique_ptr<Image> & r) {
        return l->logExp > r->logExp;
    }

private:
    std::unique_ptr<MetaData> metaData;
    uint16_t * pixel;
    std::vector<std::unique_ptr<uint16_t[]>> scaledData;
    size_t width, height;
    int dx, dy;
    uint16_t max;
    double logExp;          ///< Logarithmic exposure, from metadata
    double relExp;          ///< Relative exposure, from data
    double immExp;          ///< Exposure relative to the next image

    void subtractBlack();
    void computeRelExp();
    void preScale();
    void buildImage(uint16_t * rawImage, MetaData * md);
};

} // namespace hdrmerge

#endif // _IMAGE_H_
