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

#include <list>
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


double ImageStack::value(size_t x, size_t y) const {
    for (auto & i : images) {
        uint16_t v = i->exposureAt(x, y);
        if (!i->isSaturated(v))
            return i->relativeValue(v);
    }
    return images.back()->relativeValue(images.back()->exposureAt(x, y));
}


int ImageStack::getImageAt(size_t x, size_t y) const {
    for (int i = 0; i < images.size(); ++i) {
        uint16_t v = images[i]->exposureAt(x, y);
        if (!images[i]->isSaturated(v))
            return i;
    }
    return images.size() - 1;
}


void ImageStack::compose(float (* dst)[4]) const {
    // Create merge map
    unique_ptr<float[]> map(new float[width * height]);
    for (size_t row = 0, i = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col, ++i) {
            map[i] = getImageAt(col, row);
        }
    }

    // Progressive merge: gaussian blur
    // TODO: configure radius and sigma
    const int radius = width > height ? height / 200 : width / 200;
    const float sigma = radius / 3.0f;
    gaussianBlur(map.get(), radius, sigma);

    // Apply map
    const MetaData & md = images.front()->getMetaData();
    for (size_t row = 0, i = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col, ++i) {
            size_t pos = row*width + col;
            int j = ceil(map[i]);
            double v = images[j]->relativeValue(images[j]->exposureAt(col, row));
            if (j > 0) {
                double p = j - map[i];
                double vv = images[j - 1]->relativeValue(images[j - 1]->exposureAt(col, row));
                v = v*(1.0 -p) + vv*p;
            }
            dst[pos][md.FC(row, col)] = v;
        }
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
        int pos = 0;
        while (names.front()[pos] == names.back()[pos]) pos++;
        name = names.front() + '-' + names.back().substr(pos);
    } else {
        name = names.front().substr(0, names.front().find_last_of('.'));
    }
    return name;
}


void ImageStack::gaussianBlur(float * m, int radius, float sigma) const {
    const float pi = 3.14159265358979323846;
    int size = width * height;
    int samples = radius * 2 + 1;
    vector<float> weight(samples);
    float tss = 2.0 * sigma * sigma;
    float div = sqrt(pi * tss);

    // Calculate weights
    weight[radius] = 1.0 / div;
    for (int i = 1; i <= radius; i++)
        weight[radius - i] = weight[radius + i] = exp(-i*i / tss) / div;
    float norm = 0.0;
    for (int i = 0; i < samples; i++)
        norm += weight[i];
    for (int i = 0; i < samples; i++)
        weight[i] /= norm;

    vector<float> m2(size, 0.0);
    // Horizontal blur
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < samples; k++) {
                int kk = j + k - radius;
                if (kk < 0) kk = 0;
                else if (kk >= width) kk = width - 1;
                m2[i * width + j] += m[i * width + kk] * weight[k];
            }
        }
    }

    fill_n(m, size, 0.0);
    // Vertical blur
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < samples; k++) {
                int kk = i + k - radius;
                if (kk < 0) kk = 0;
                else if (kk >= height) kk = height - 1;
                m[i * width + j] += m2[kk * width + j] * weight[k];
            }
        }
    }
}
