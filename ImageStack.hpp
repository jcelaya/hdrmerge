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

#ifndef _IMAGESTACK_H_
#define _IMAGESTACK_H_

#include <vector>
#include <string>
#include <memory>
#include "config.h"
#include "Image.hpp"


namespace hdrmerge {

class ImageStack {
public:
    ImageStack() : width(0), height(0), currentScale(0) {}

    size_t size() const { return images.size(); }

    size_t getWidth() const {
        return width >> currentScale;
    }

    size_t getHeight() const {
        return height >> currentScale;
    }

    Image & getImage(unsigned int i) {
        return *images[i];
    }
    const Image & getImage(unsigned int i) const {
        return *images[i];
    }

    bool addImage(std::unique_ptr<Image> & i);
    void align();
    void computeRelExposures() {
        for (auto cur = images.rbegin(), next = cur++; cur != images.rend(); next = cur++) {
            (*cur)->relativeExposure(**next, width, height);
        }
    }
    std::string buildOutputFileName();
    double value(size_t x, size_t y);
    void demosaic();

private:
    std::vector<std::unique_ptr<Image>> images;   ///< Images, from most to least exposed
    size_t width;     ///< Size of a row
    size_t height;    ///< Size of a column
    unsigned int currentScale;     ///< Current scale factor

    void findIntersection();
};

} // namespace hdrmerge

#endif // _IMAGESTACK_H_
