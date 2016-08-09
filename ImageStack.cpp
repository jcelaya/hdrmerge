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
#ifdef __SSE2__
    #include <x86intrin.h>
#endif

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


void ImageStack::calculateSaturationLevel(const RawParameters & params, bool useCustomWl) {
    // Calculate max value of brightest image and assume it is saturated
    uint16_t maxPerColor[4] = { 0, 0, 0, 0 };
    Image & brightest = images.front();
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            uint16_t v = brightest(x, y);
            if (v > maxPerColor[params.FC(x, y)]) {
                maxPerColor[params.FC(x, y)] = v;
            }
        }
    }
    satThreshold = params.max == 0 ? maxPerColor[0] : params.max;
    for (int c = 0; c < 4; ++c) {
        if (maxPerColor[c] < satThreshold) {
            satThreshold = maxPerColor[c];
        }
    }
    if(!useCustomWl) // only scale when no custom white level was specified
        satThreshold *= 0.99;
    else
        Log::debug( "Using custom white level ", params.max );

    for (auto & i : images) {
        i.setSaturationThreshold(satThreshold);
    }
}


void ImageStack::align() {
    if (images.size() > 1) {
        Timer t("Align");
        size_t errors[images.size()];
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < images.size(); ++i) {
            images[i].preScale();
        }
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < images.size() - 1; ++i) {
            errors[i] = images[i].alignWith(images[i + 1]);
        }
        for (size_t i = images.size() - 1; i > 0; --i) {
            images[i - 1].displace(images[i].getDeltaX(), images[i].getDeltaY());
            Log::debug("Image ", i - 1, " displaced to (", images[i - 1].getDeltaX(),
                       ", ", images[i - 1].getDeltaY(), ") with error ", errors[i - 1]);
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


void ImageStack::computeResponseFunctions() {
    Timer t("Compute response functions");
    for (int i = images.size() - 2; i >= 0; --i) {
        images[i].computeResponseFunction(images[i + 1]);
    }
}


void ImageStack::generateMask() {
    Timer t("Generate mask");
    mask.resize(width, height);
    if(images.size() == 1) {
        // single image, fill in zero values
        std::fill_n(&mask[0], width*height, 0);
    } else {
        // multiple images, no need to prefill mask with zeroes. It will be filled correctly on the fly
        #pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                size_t i = 0;
                while (i < images.size() - 1 &&
                    (!images[i].contains(x, y) ||
                    images[i].isSaturatedAround(x, y))) ++i;
                mask(x, y) = i;
            }
        }
    }
    // The mask can be used in compose to get the information about saturated pixels
    // but the mask can be modified in gui, so we have to make a copy to represent the original state
    origMask = mask;
}


double ImageStack::value(size_t x, size_t y) const {
    const Image & img = images[mask(x, y)];
    return img.exposureAt(x, y);
}

#ifndef __SSE2__
// From The GIMP: app/paint-funcs/paint-funcs.c:fatten_region
static Array2D<uint8_t> fattenMask(const Array2D<uint8_t> & mask, int radius) {
    Timer t("Fatten mask");
    size_t width = mask.getWidth(), height = mask.getHeight();
    Array2D<uint8_t> result(width, height);

    int circArray[2 * radius + 1]; // holds the y coords of the filter's mask
    // compute_border(circArray, radius)
    for (int i = 0; i < radius * 2 + 1; i++) {
        double tmp;
        if (i > radius)
            tmp = (i - radius) - 0.5;
        else if (i < radius)
            tmp = (radius - i) - 0.5;
        else
            tmp = 0.0;
        circArray[i] = int(std::sqrt(radius*radius - tmp*tmp));
    }
    // offset the circ pointer by radius so the range of the array
    //     is [-radius] to [radius]
    int * circ = circArray + radius;

    const uint8_t * bufArray[height + 2*radius];
    for (int i = 0; i < radius; i++) {
        bufArray[i] = &mask[0];
    }
    for (size_t i = 0; i < height; i++) {
        bufArray[i + radius] = &mask[i * width];
    }
    for (int i = 0; i < radius; i++) {
        bufArray[i + height + radius] = &mask[(height - 1) * width];
    }
    // offset the buf pointer
    const uint8_t ** buf = bufArray + radius;

    #pragma omp parallel
    {
        unique_ptr<uint8_t[]> buffer(new uint8_t[width * (radius + 1)]);
        unique_ptr<uint8_t *[]> maxArray;  // caches the largest values for each column
        maxArray.reset(new uint8_t *[width + 2 * radius]);
        for (int i = 0; i < radius; i++) {
            maxArray[i] = buffer.get();
        }
        for (size_t i = 0; i < width; i++) {
            maxArray[i + radius] = &buffer[(radius + 1) * i];
        }
        for (int i = 0; i < radius; i++) {
            maxArray[i + width + radius] = &buffer[(radius + 1) * (width - 1)];
        }
        // offset the max pointer
        uint8_t ** max = maxArray.get() + radius;

        #pragma omp for schedule(dynamic)
        for (size_t y = 0; y < height; y++) {
            uint8_t rowMax = 0;
            for (size_t x = 0; x < width; x++) { // compute max array
                max[x][0] = buf[y][x];
                for (int i = 1; i <= radius; i++) {
                    max[x][i] = std::max(std::max(max[x][i - 1], buf[y + i][x]), buf[y - i][x]);
                    rowMax = std::max(max[x][i], rowMax);
                }
            }

            uint8_t last_max = max[0][circ[-1]];
            int last_index = 1;
            for (size_t x = 0; x < width; x++) { // render scan line
                last_index--;
                if (last_index >= 0) {
                    if (last_max == rowMax) {
                        result(x, y) = rowMax;
                    } else {
                        last_max = 0;
                        for (int i = radius; i >= 0; i--)
                            if (last_max < max[x + i][circ[i]]) {
                                last_max = max[x + i][circ[i]];
                                last_index = i;
                            }
                        result(x, y) = last_max;
                    }
                } else {
                    last_index = radius;
                    last_max = max[x + radius][circ[radius]];

                    for (int i = radius - 1; i >= -radius; i--)
                        if (last_max < max[x + i][circ[i]]) {
                            last_max = max[x + i][circ[i]];
                            last_index = i;
                        }
                    result(x, y) = last_max;
                }
            }
        }
    }

    return result;
}
#else // use faster SSE version, crunch 16 bytes at once
// From The GIMP: app/paint-funcs/paint-funcs.c:fatten_region
// SSE version by Ingo Weyrich
static Array2D<uint8_t> fattenMask(const Array2D<uint8_t> & mask, int radius) {
    Timer t("Fatten mask (SSE version)");
    size_t width = mask.getWidth(), height = mask.getHeight();
    Array2D<uint8_t> result(width, height);

    int circArray[2 * radius + 1]; // holds the y coords of the filter's mask
    // compute_border(circArray, radius)
    for (int i = 0; i < radius * 2 + 1; i++) {
        double tmp;
        if (i > radius)
            tmp = (i - radius) - 0.5;
        else if (i < radius)
            tmp = (radius - i) - 0.5;
        else
            tmp = 0.0;
        circArray[i] = int(std::sqrt(radius*radius - tmp*tmp));
    }
    // offset the circ pointer by radius so the range of the array
    //     is [-radius] to [radius]
    int * circ = circArray + radius;

    const uint8_t * bufArray[height + 2*radius];
    for (int i = 0; i < radius; i++) {
        bufArray[i] = &mask[0];
    }
    for (size_t i = 0; i < height; i++) {
        bufArray[i + radius] = &mask[i * width];
    }
    for (int i = 0; i < radius; i++) {
        bufArray[i + height + radius] = &mask[(height - 1) * width];
    }
    // offset the buf pointer
    const uint8_t ** buf = bufArray + radius;

    #pragma omp parallel
    {
        uint8_t buffer[width * (radius + 1)];
        uint8_t *maxArray[radius+1];
        for (int i = 0; i <= radius; i++) {
            maxArray[i] = &buffer[i*width];
        }

        #pragma omp for schedule(dynamic,16)
        for (size_t y = 0; y < height; y++) {
            size_t x = 0;
            for (; x < width-15; x+=16) { // compute max array, use SSE to process 16 bytes at once
                __m128i lmax = _mm_loadu_si128((__m128i*)&buf[y][x]);
                if(radius<2) // max[0] is only used when radius < 2
                    _mm_storeu_si128((__m128i*)&maxArray[0][x],lmax);
                for (int i = 1; i <= radius; i++) {
                    lmax = _mm_max_epu8(_mm_loadu_si128((__m128i*)&buf[y + i][x]),lmax);
                    lmax = _mm_max_epu8(_mm_loadu_si128((__m128i*)&buf[y - i][x]),lmax);
                    _mm_storeu_si128((__m128i*)&maxArray[i][x],lmax);
                }
            }
            for (; x < width; x++) { // compute max array, remaining columns
                uint8_t lmax = buf[y][x];
                if(radius<2) // max[0] is only used when radius < 2
                    maxArray[0][x] = lmax;
                for (int i = 1; i <= radius; i++) {
                    lmax = std::max(std::max(lmax, buf[y + i][x]), buf[y - i][x]);
                    maxArray[i][x] = lmax;
                }
            }

            for (x = 0; (int)x < radius; x++) { // render scan line, first columns without SSE
                uint8_t last_max = maxArray[circ[radius]][x+radius];
                for (int i = radius - 1; i >= -(int)x; i--)
                    last_max = std::max(last_max,maxArray[circ[i]][x + i]);
                result(x, y) = last_max;
            }
            for (; x < width-15-radius+1; x += 16) { // render scan line, use SSE to process 16 bytes at once
                __m128i last_maxv = _mm_loadu_si128((__m128i*)&maxArray[circ[radius]][x+radius]);
                for (int i = radius - 1; i >= -radius; i--)
                    last_maxv = _mm_max_epu8(last_maxv,_mm_loadu_si128((__m128i*)&maxArray[circ[i]][x+i]));
                _mm_storeu_si128((__m128i*)&result(x,y),last_maxv);
            }

            for (; x < width; x++) { // render scan line, last columns without SSE
                int maxRadius = std::min(radius,(int)((int)width-1-(int)x));
                uint8_t last_max = maxArray[circ[maxRadius]][x+maxRadius];
                for (int i = maxRadius-1; i >= -radius; i--)
                    last_max = std::max(last_max,maxArray[circ[i]][x + i]);
                result(x, y) = last_max;
            }
        }
    }

    return result;
}
#endif

Array2D<float> ImageStack::compose(const RawParameters & params, int featherRadius) const {
    int imageMax = images.size() - 1;
    BoxBlur map(fattenMask(mask, featherRadius));
    measureTime("Blur", [&] () {
        map.blur(featherRadius);
    });
    Timer t("Compose");
    Array2D<float> dst(params.rawWidth, params.rawHeight);
    dst.displace(-(int)params.leftMargin, -(int)params.topMargin);
    dst.fillBorders(0.f);

    float max = 0.0;
    double saturatedRange = params.max - satThreshold;
    #pragma omp parallel
    {
        float maxthr = 0.0;
        #pragma omp for schedule(dynamic,16) nowait
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                double v, vv;
                double p = map(x,y);
                p = p < 0.0 ? 0.0 : p;
                int j = p;
                if (images[j].contains(x, y)) {
                    p = p - j;
                    v = images[j].exposureAt(x, y);
                    // Adjust false highlights
                    if (j < origMask(x,y)) { // SaturatedAround
                        v /= params.whiteMultAt(x, y);
                        if(p > 0.0001) {
                            uint16_t rawV = images[j].getMaxAround(x, y);
                            double k = (rawV - satThreshold) / saturatedRange;
                            if (k > 1.0)
                                k = 1.0;
                            p += (1.0 - p) * k;
                        }
                    }
                } else {
                    v = 0.0;
                    p = 1.0;
                }
                if (p > 0.0001 && j < imageMax && images[j + 1].contains(x, y)) {
                    vv = images[j + 1].exposureAt(x, y);
                    if (j + 1 < origMask(x,y)) { // SaturatedAround
                        vv /= params.whiteMultAt(x, y);
                    }
                } else {
                    vv = 0.0;
                    p = 0.0;
                }
                v -= p * (v - vv);
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
    float mult = (params.max - params.maxBlack) / max;
    #pragma omp parallel for
    for (size_t y = 0; y < params.rawHeight; ++y) {
        for (size_t x = 0; x < params.rawWidth; ++x) {
            dst(x, y) *= mult;
            dst(x, y) += params.blackAt(x - params.leftMargin, y - params.topMargin);
        }
    }

    return dst;
}
