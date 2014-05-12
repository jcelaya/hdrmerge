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

#include <algorithm>
#include "ImageStack.hpp"
#include "DngFloatWriter.hpp"
using namespace std;
using namespace hdrmerge;


void ImageStack::setGamma(float g) {
    g = 1.0f / g;
    for (int i = 0; i < 65536; i++) {
        toneCurve[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
    }
}


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


int ImageStack::load(const LoadOptions & options, ProgressIndicator & progress) {
    int step = 100 / (options.fileNames.size() + 1);
    int p = -step;
    for (auto & name : options.fileNames) {
        progress.advance(p += step, "Loading %1", name.c_str());
        std::unique_ptr<Image> image(new Image(name.c_str()));
        if (image.get() == nullptr || !image->good()) {
            return 1;
        }
        if (!addImage(image)) {
            return 2;
        }
    }
    if (options.align) {
        progress.advance(p += step, "Aligning");
        align();
    }
    computeRelExposures();
    imageIndex.generateFrom(*this);
    progress.advance(100, "Done loading!");
    return 0;
}


int ImageStack::save(const SaveOptions & options, ProgressIndicator & progress) {
    DngFloatWriter writer(*this, progress);
    writer.setBitsPerSample(options.bps);
    writer.setPreviewWidth(options.previewSize);
    writer.setIndexFileName(options.maskFileName.c_str());
    writer.write(options.fileName);
}


void ImageStack::align() {
    if (images.size() > 1) {
        int dx = 0, dy = 0;
        for (auto cur = images.begin(), prev = cur++; cur != images.end(); prev = cur++) {
            dx = (*prev)->getDeltaX();
            dy = (*prev)->getDeltaY();
            (*cur)->alignWith(**prev);
            (*cur)->displace(dx, dy);
        }
        for (auto & i : images) {
            i->releaseAlignData();
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
}


double ImageStack::value(size_t x, size_t y) const {
    Image & img = *images[getImageAt(x, y)];
    return img.exposureAt(x, y);
}


void ImageStack::compose(float * dst) const {
    unique_ptr<float[]> map = imageIndex.blur();
    const MetaData & md = images.front()->getMetaData();
    int imageMax = images.size() - 1;
    float max = 0.0;
    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            size_t pos = row*width + col;
            int j = map[pos] > imageMax ? imageMax : ceil(map[pos]);
            double v = images[j]->exposureAt(col, row);
            // Adjust false highlights
            if (j < imageMax && images[j]->isSaturated(col, row)) {
                v /= md.whiteMultAt(row, col);
            }
            if (j > 0) {
                double p = j - map[pos];
                double vv = images[j - 1]->exposureAt(col, row);
                if (images[j - 1]->isSaturated(col, row)) {
                    vv /= md.whiteMultAt(row, col);
                }
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
    std::vector<string> names;
    for (auto & image : images) {
        const string & f = image->getMetaData().fileName;
        names.push_back(f.substr(0, f.find_last_of('.')));
    }
    sort(names.begin(), names.end());
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
