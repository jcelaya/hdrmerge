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

    unsigned int size() const { return images.size(); }

    unsigned int getWidth() const {
        return width >> currentScale;
    }

    unsigned int getHeight() const {
        return height >> currentScale;
    }

    Image & getImage(unsigned int i) {
        return *images[i];
    }
    
    bool addImage(std::unique_ptr<Image> & i);

private:
    std::vector<std::unique_ptr<Image>> images;   ///< Images, from most to least exposed
    unsigned int width;     ///< Size of a row
    unsigned int height;    ///< Size of a column
    unsigned int currentScale;     ///< Current scale factor
};

} // namespace hdrmerge

#endif // _IMAGESTACK_H_
