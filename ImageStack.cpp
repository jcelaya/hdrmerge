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

#include <iostream>
#include <list>
#include <tiff.h>
#include <tiffio.h>
#include <libraw/libraw.h>
#include <pfs-1.2/pfs.h>
#include <algorithm>
#include "ImageStack.hpp"
#include "MergeMap.hpp"
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


void ImageStack::computeRelExposures() {
    for (auto cur = images.rbegin(), next = cur++; cur != images.rend(); next = cur++) {
        (*cur)->relativeExposure(**next, width, height);
    }

    imageIndex.reset(new uint8_t[width*height]);
    fill_n(imageIndex.get(), width*height, 0);
    for (int i = 0; i < images.size() - 1; ++i) {
        Image & img = *images[i];
        for (size_t row = 0, pos = 0; row < height; ++row) {
            for (size_t col = 0; col < width; ++col, ++pos) {
                if (img.isSaturated(col, row)) {
                    ++imageIndex[pos];
                }
            }
        }
    }
}


double ImageStack::value(size_t x, size_t y) const {
    Image & img = *images[getImageAt(x, y)];
    return img.exposureAt(x, y);
}


void ImageStack::compose(float * dst) const {
    // TODO: configure radius
    const int radius = width > height ? height / 500 : width / 500;
    MergeMap map(*this);
    map.blur(radius);

    // Apply map
    int imageMax = images.size() - 1;
    float max = 0.0;
    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            size_t pos = row*width + col;
            int j = map[pos] > imageMax ? imageMax : ceil(map[pos]);
            double v = images[j]->exposureAt(col, row);
            if (j > 0) {
                double p = j - map[pos];
                double vv = images[j - 1]->exposureAt(col, row);
                v = v*(1.0 - p) + vv*p;
            }
            dst[pos] = v;
            if (v > max) {
                max = v;
            }
        }
    }
    for (size_t pos = 0; pos < width * height; ++pos) {
        dst[pos] /= max;
    }
}


string ImageStack::buildOutputFileName() const {
    string name;
    std::list<string> names;
    for (auto & image : images) {
        const string & f = image->getMetaData().fileName;
        names.push_back(f.substr(0, f.find_last_of('.')));
    }
    names.sort();
    if (names.size() > 1) {
        string & last = names.back();
        int pos = last.length() - 1;
        while (last[pos] >= '0' && last[pos] <= '9') pos--;
        name = names.front() + '-' + names.back().substr(pos + 1);
    } else {
        name = names.front();
    }
    return name;
}
