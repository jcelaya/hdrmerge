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

#include "Image.hpp"
#include "Bitmap.hpp"
#include "Histogram.hpp"
#include "Log.hpp"
#include "RawParameters.hpp"
using namespace std;
using namespace hdrmerge;


void Image::ResponseFunction::setLinear(double slope) {
    threshold = 65535;
    linear = slope;
    alglib::real_1d_array x = "[0.0, 0.0]";
    alglib::real_1d_array f = "[0.0, 65535.0]";
    x[1] = 65535.0 / linear;
    alglib::spline1dbuildlinear(x, f, 2, nonLinear);
}


void Image::buildImage(uint16_t * rawImage, const RawParameters & params) {
    resize(params.width, params.height);
    size_t size = width*height;
    brightness = 0.0;
    max = 0;
    for (size_t y = 0, ry = params.topMargin; y < height; ++y, ++ry) {
        for (size_t x = 0, rx = params.leftMargin; x < width; ++x, ++rx) {
            uint16_t v = rawImage[ry*params.rawWidth + rx];
            (*this)(x, y) = v;
            brightness += v;
            if (v > max) max = v;
        }
    }
    brightness /= size;
    response.setLinear(params.max == 0 ? 1.0 : 65535.0 / params.max);
    subtractBlack(params);
}


Image & Image::operator=(Image && move) {
    *static_cast<Array2D<uint16_t> *>(this) = (Array2D<uint16_t> &&)std::move(move);
    scaled.swap(move.scaled);
    satThreshold = move.satThreshold;
    max = move.max;
    brightness = move.brightness;
    response = move.response;
    halfLightPercent = move.halfLightPercent;
    return *this;
}


void Image::setSaturationThreshold(uint16_t sat) {
    satThreshold = sat;
    response.threshold = 0.9*sat;
}


void Image::subtractBlack(const RawParameters & params) {
    if (params.hasBlack()) {
        for (size_t y = 0, pos = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x, ++pos) {
                if ((*this)[pos] > params.blackAt(x, y)) {
                    (*this)[pos] -= params.blackAt(x, y);
                } else {
                    (*this)[pos] = 0;
                }
            }
        }
    }
}


double Image::getRelativeExposure() const {
    return response.linear;
}


void Image::computeResponseFunction(const Image & r) {
    int reldx = dx - std::max(dx, r.dx);
    int relrdx = r.dx - std::max(dx, r.dx);
    int w = width + reldx + relrdx;
    int reldy = dy - std::max(dy, r.dy);
    int relrdy = r.dy - std::max(dy, r.dy);
    int h = height + reldy + relrdy;
    uint16_t * usePixels = &data[-reldy*width - reldx];
    const uint16_t * rUsePixels = &r.data[-relrdy*width - relrdx];

    // Get average relative values between this image and the last one
    std::vector<std::pair<int, double>> histogram(max + 1);
    for (auto & i : histogram) i = { 0, 0.0 };
    #pragma omp parallel
    {
        // use one histogram per thread
        std::vector<std::pair<int, double>> histogramThr(max + 1);
        for (auto & i : histogramThr) i = { 0, 0.0 };
        #pragma omp for nowait
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int pos = y * width + x;
                uint16_t v = usePixels[pos];
                uint16_t nv = rUsePixels[pos];
                if (v >= nv && v < satThreshold) {
                    histogramThr[v].first++;
                    histogramThr[v].second += r.response(nv);
                }
            }
        }
        #pragma omp critical
        {
            // join per thread histogram to global one
            for(int i=0;i<max+1;i++) {
                histogram[i].first += histogramThr[i].first;
                histogram[i].second += histogramThr[i].second;
            }
        }
    }
    alglib::real_1d_array values, adjValues;
    values.setlength(max);
    adjValues.setlength(max);
    values[0] = 0;
    adjValues[0] = 0;
    int i = 1;
    for (int v = max - 1; v >= max*0.75; --v) {
        if (histogram[v].first > 2) {
            values[i] = v;
            adjValues[i] = histogram[v].second / histogram[v].first;
            ++i;
        }
    }
    if (i >= max/8) {
        alglib::ae_int_t info;
        alglib::spline1dfitreport rep;
        alglib::spline1dfitpenalized(values, adjValues, i, 200, 3, info, response.nonLinear, rep);
        response.linear = alglib::spline1dcalc(response.nonLinear, response.threshold) / response.threshold;
    } else {
        response.threshold = 65535;
        // Fallback method for dark images:
        // Minimize square error between images:
        // min. C(n) = sum(n*f(x) - g(x))^2  ->  n = sum(f(x)*g(x)) / sum(f(x)^2)
        double numerator = 0, denom = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int pos = y * width + x;
                double v = usePixels[pos];
                double nv = rUsePixels[pos];
                if (v >= nv && v < satThreshold) {
                    numerator += v * r.response(nv);
                    denom += v * v;
                }
            }
        }
        response.linear = numerator / denom;
    }
}


size_t Image::alignWith(const Image & r) {
    dx = dy = 0;
    const double tolerance = 1.0/16;
    Histogram histFull(begin(), end());
    double halfLightPercent = histFull.getFraction(satThreshold) / 2.0;
    size_t totalError = 0;
    for (int s = scaleSteps - 1; s >= 0; --s) {
        size_t curWidth = width >> (s + 1);
        size_t curHeight = height >> (s + 1);
        size_t minError = curWidth*curHeight;
        Histogram hist1(r.scaled[s].begin(), r.scaled[s].end());
        Histogram hist2(scaled[s].begin(), scaled[s].end());
        uint16_t mth1 = hist1.getPercentile(halfLightPercent);
        uint16_t mth2 = hist2.getPercentile(halfLightPercent);
        uint16_t tolPixels1 = (uint16_t)std::floor(mth1*tolerance);
        uint16_t tolPixels2 = (uint16_t)std::floor(mth2*tolerance);
        Bitmap mtb1(curWidth, curHeight), mtb2(curWidth, curHeight),
        excl1(curWidth, curHeight), excl2(curWidth, curHeight);
        mtb1.mtb(r.scaled[s].begin(), mth1);
        mtb2.mtb(scaled[s].begin(), mth2);
        excl1.exclusion(r.scaled[s].begin(), mth1, tolPixels1);
        excl2.exclusion(scaled[s].begin(), mth2, tolPixels2);
        Bitmap shiftMtb(curWidth, curHeight), shiftExcl(curWidth, curHeight);
        int curDx = dx, curDy = dy;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                shiftMtb.shift(mtb2, curDx + i, curDy + j);
                shiftExcl.shift(excl2, curDx + i, curDy + j);
                shiftMtb.bitwiseXor(mtb1);
                shiftMtb.bitwiseAnd(excl1);
                shiftMtb.bitwiseAnd(shiftExcl);
                size_t err = shiftMtb.count();
                if (err < minError) {
                    dx = curDx + i;
                    dy = curDy + j;
                    minError = err;
                }
            }
        }
        dx <<= 1;
        dy <<= 1;
        totalError += minError;
    }
    return totalError;
}


void Image::preScale() {
    size_t curWidth = width;
    size_t curHeight = height;
    Array2D<uint16_t> * r2 = this;

    scaled.reset(new Array2D<uint16_t>[scaleSteps]);
    for (int s = 0; s < scaleSteps; ++s) {
        scaled[s].resize(curWidth >>= 1, curHeight >>= 1);
        for (size_t y = 0, prevY = 0; y < curHeight; ++y, prevY += 2) {
            for (size_t x = 0, prevX = 0; x < curWidth; ++x, prevX += 2) {
                uint32_t value1 = (*r2)(prevX, prevY),
                    value2 = (*r2)(prevX + 1, prevY),
                    value3 = (*r2)(prevX, prevY + 1),
                    value4 = (*r2)(prevX + 1, prevY + 1);
                scaled[s](x, y) = (value1 + value2 + value3 + value4) >> 2;
            }
        }
        r2 = &scaled[s];
    }
}


uint16_t Image::getMaxAround(size_t x, size_t y) const {
    uint16_t result = 0;
    if ((int)y > dy) {
        if ((int)x > dx) result = std::max(result, (*this)(x - 1, y - 1));
        result = std::max(result, (*this)(x, y - 1));
        if (x < width + dx - 1) result = std::max(result, (*this)(x + 1, y - 1));
    }
    if ((int)x > dx) result = std::max(result, (*this)(x - 1, y));
    result = std::max(result, (*this)(x, y));
    if (x < width + dx - 1) result = std::max(result, (*this)(x + 1, y));
    if (y < height + dy - 1) {
        if ((int)x > dx) result = std::max(result, (*this)(x - 1, y + 1));
        result = std::max(result, (*this)(x, y + 1));
        if (x < width + dx - 1) result = std::max(result, (*this)(x + 1, y + 1));
    }
    return result;
}
