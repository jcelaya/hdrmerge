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
#include <memory>
#include <libraw/libraw.h>
#include "MetaData.hpp"


namespace hdrmerge {

class Image {
public:
    static const int scaleSteps = 6;

    Image(uint16_t * rawImage, const MetaData & md);
    Image(const char * f);

    bool good() const {
        return rawPixels != nullptr;
    }
    const MetaData & getMetaData() const {
        return *metaData;
    }
    size_t getWidth() const {
        return iwidth;
    }
    size_t getHeight() const {
        return iheight;
    }
    int getDeltaX() const {
        return dx;
    }
    int getDeltaY() const {
        return dy;
    }
    uint16_t exposureAt(size_t x, size_t y) const {
        x -= dx; y -= dy;
        return image[y*iwidth + x][metaData->FC(y, x)];
    }
    double relativeValue(uint16_t v) const {
        return v * relExp;
    }
    bool isSaturated(uint16_t v) const {
        return v == max;
    }
    double getRelativeExposure() const {
        return relExp;
    }
    bool isSameFormat(const Image & ref) const;
    void alignWith(const Image & r, double threshold, double tolerance);
    void displace(int newDx, int newDy) {
        dx += newDx;
        dy += newDy;
    }
    void relativeExposure(const Image & nextImage, size_t w, size_t h);

    static bool comparePointers(const std::unique_ptr<Image> & l, const std::unique_ptr<Image> & r) {
        return l->logExp > r->logExp;
    }

private:
    LibRaw rawProcessor;
    std::unique_ptr<MetaData> metaData;
    uint16_t * rawPixels;
    size_t rwidth, rheight;
    std::vector<std::unique_ptr<uint16_t[]>> grayscalePics;
    uint16_t (* image)[4];
    size_t iwidth, iheight;
    int dx, dy;
    uint16_t max;
    double logExp;
    double relExp;          ///< Relative exposure, from data

    void preScale();
};

} // namespace hdrmerge

#endif // _IMAGE_H_
