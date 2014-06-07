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
#include "Log.hpp"
#include "BoxBlur.hpp"
#include "RawParameters.hpp"
using namespace std;
using namespace hdrmerge;


int ImageStack::addImage(Image && i) {
    if (images.empty()) {
        width = i.getWidth();
        height = i.getHeight();
    }
    images.push_back(std::move(i));
    int n = images.size() - 1;
    while (n > 0 && images[n] < images[n - 1]) {
        std::swap(images[n], images[n - 1]);
        --n;
    }
    return n;
}


void ImageStack::align() {
    if (images.size() > 1) {
        size_t errors[images.size()];
        for (int i = images.size() - 2; i >= 0; --i) {
            errors[i] = images[i].alignWith(images[i + 1]);
        }
        for (int i = images.size() - 2; i >= 0; --i) {
            images[i].displace(images[i + 1].getDeltaX(), images[i + 1].getDeltaY());
            Log::msg(Log::DEBUG, "Image ", i, " displaced to (", images[i].getDeltaX(), ", ", images[i].getDeltaY(),
                     ") with error ", errors[i]);
        }
        for (auto & i : images) {
            i.releaseAlignData();
        }
    }
}

void ImageStack::crop() {
    int dx = 0, dy = 0;
    for (auto & i : images) {
        int newDx = max(dx, i.getDeltaX());
        int bound = min(dx + width, i.getDeltaX() + i.getWidth());
        width = bound > newDx ? bound - newDx : 0;
        dx = newDx;
        int newDy = max(dy, i.getDeltaY());
        bound = min(dy + height, i.getDeltaY() + i.getHeight());
        height = bound > newDy ? bound - newDy : 0;
        dy = newDy;
    }
    for (auto & i : images) {
        i.displace(-dx, -dy);
    }
}


void ImageStack::computeRelExposures() {
    for (auto cur = images.rbegin(), next = cur++; cur != images.rend(); next = cur++) {
        cur->relativeExposure(*next);
    }
}


void ImageStack::generateMask() {
    Timer t("Generate mask");
    mask.resize(width, height);
    std::fill_n(&mask[0], width*height, 0);
    for (size_t y = 0, pos = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x, ++pos) {
            int i = mask[pos];
            while (i < images.size() - 1 &&
                (!images[i].contains(x, y) ||
                images[i].isSaturated(x, y) ||
                images[i].isSaturatedAround(x, y))) ++i;
            if (mask[pos] < i) {
                mask[pos] = i;
                mask.traceCircle(x, y, 4, [&] (int col, int row, uint8_t & layer) {
                    if (layer < i && images[i].contains(col, row)) {
                        layer = i;
                    }
                });
            }
        }
    }
}


double ImageStack::value(size_t x, size_t y) const {
    const Image & img = images[mask(x, y)];
    return img.exposureAt(x, y);
}


Array2D<float> ImageStack::compose(const RawParameters & params) const {
    BoxBlur map(mask);
    measureTime("Blur", [&] () { map.blur(3); });
    Timer t("Compose");
    Array2D<float> dst(params.rawWidth, params.rawHeight);
    fill_n(dst.begin(), dst.size(), 0.0);
    dst.displace(-(int)params.leftMargin, -(int)params.topMargin);
    int imageMax = images.size() - 1;
    float max = 0.0;
    #pragma omp parallel
    {
        float maxthr = 0.0;
        #pragma omp for schedule(dynamic)
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                int j = map(x, y) > imageMax ? imageMax : ceil(map(x, y));
                double v = 0.0, vv = 0.0, p;
                if (images[j].contains(x, y)) {
                    v = images[j].exposureAt(x, y);
                    // Adjust false highlights
                    if (j < imageMax && images[j].isSaturatedAround(x, y)) {
                        v /= params.whiteMultAt(x, y);
                    }
                    p = j - map(x, y);
                } else {
                    p = 1.0;
                }
                if (j > 0 && images[j - 1].contains(x, y)) {
                    vv = images[j - 1].exposureAt(x, y);
                    if (images[j - 1].isSaturatedAround(x, y)) {
                        vv /= params.whiteMultAt(x, y);
                    }
                } else {
                    p = 0.0;
                }
                v = v*(1.0 - p) + vv*p;
                dst(x, y) = v;
                if (v > maxthr) {
                    maxthr = v;
                }
            }
        }
        #pragma omp critical
        if (maxthr > max) {
            max = maxthr;
        }
    }
    dst.displace(params.leftMargin, params.topMargin);
    // Scale to params.max and recover the black levels
    size_t oddx = params.leftMargin & 1, oddy = params.topMargin + 1;
    for (size_t y = 0; y < params.rawHeight; ++y) {
        for (size_t x = 0; x < params.rawWidth; ++x) {
            dst(x, y) *= (params.max - params.maxBlack) / max;
            dst(x, y) += params.blackAt(x ^ oddx, y ^ oddy);
        }
    }

    return dst;
}
