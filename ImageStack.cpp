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

#include <tiff.h>
#include <tiffio.h>
#include <libraw/libraw.h>
#include <pfs-1.2/pfs.h>
#include <algorithm>
#include "ImageStack.hpp"
using namespace std;
using namespace hdrmerge;


bool ImageStack::addImage(std::unique_ptr<Image> & i) {
    if (images.empty()) {
        width = i->getWidth();
        height = i->getHeight();
        images.push_back(std::move(i));
        return true;
    } else if (images.front()->isSameFormat(*i)) {
        images.push_back(std::move(i));
        std::sort(images.begin(), images.end(), Image::comparePointers);
        return true;
    }
    return false;
}

void ImageStack::align() {
    if (images.size() > 1) {
        int dx = 0, dy = 0;
        for (auto cur = images.begin(), prev = cur++; cur != images.end(); prev = cur++) {
            dx = (*prev)->getDeltaX();
            dy = (*prev)->getDeltaY();
            (*cur)->alignWith(**prev, 0.5, 1.0/64);
            (*cur)->displace(dx, dy);
        }
        findIntersection();
    }
}

void ImageStack::findIntersection() {
    int dx = 0, dy = 0;
    for (auto & i : images) {
        int newDx = max(dx, i->getDeltaX());
        int bound = min(dx + width, i->getDeltaX() + i->getWidth());
        width = bound > newDx ? bound - newDx : 0;
        dx = newDx;
        int newDy = max(dy, i->getDeltaY());
        bound = min(dy + height, i->getDeltaY() + i->getHeight());
        height = bound > newDy ? bound - newDy : 0;
        dy = newDy;
    }
    for (auto & i : images) {
        i->displace(-dx, -dy);
    }
}
