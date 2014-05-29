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

#ifndef _ARRAY2D_HPP_
#define _ARRAY2D_HPP_

#include <memory>
#include <cstdint>

namespace hdrmerge {

template <typename T>
class Array2D {
public:
    Array2D(size_t w, size_t h) { resize(w, h); }
    Array2D() : Array2D(0, 0) {}
    template <typename Y> Array2D(const Array2D<Y> & copy) {
        resize(copy.getWidth(), copy.getHeight());
        for (size_t i = 0; i < width*height; ++i) {
            data[i] = copy[i];
        }
        displace(copy.getDeltaX(), copy.getDeltaY());
    }
    virtual ~Array2D() {}

    void resize(size_t w, size_t h) {
        width = w;
        height = h;
        dx = dy = 0;
        data.reset(new T[w*h]);
        alignedData = data.get();
    }

    size_t getWidth() const {
        return width;
    }
    size_t getHeight() const {
        return height;
    }
    int getDeltaX() const {
        return dx;
    }
    int getDeltaY() const {
        return dy;
    }
    const T & operator[](size_t i) const {
        return data[i];
    }
    T & operator[](size_t i) {
        return data[i];
    }
    const T & operator()(size_t x, size_t y) const {
        return alignedData[y*width + x];
    }
    T & operator()(size_t x, size_t y) {
        return alignedData[y*width + x];
    }
    bool contains(int x, int y) const {
        return x >= dx && x < width + dx && y >= dy && y < height + dy;
    }
    void displace(int newDx, int newDy) {
        dx += newDx;
        dy += newDy;
        alignedData = &data[-dy*width - dx];
    }
    template <typename F> void traceCircle(int x, int y, int radius, F function) {
        int r2 = radius * radius;
        int ymin = std::max(-y, -radius), ymax = std::min(height - y, radius + 1);
        int xmin = std::max(-x, -radius), xmax = std::min(width - x, radius + 1);
        for (int row = ymin, rrow = y + row; row < ymax; ++row, ++rrow) {
            for (int col = xmin, rcol = x + col; col < xmax; ++col, ++rcol) {
                if (row*row + col*col <= r2) {
                    function(rcol, rrow, operator()(rcol, rrow));
                }
            }
        }
    }

protected:
    std::unique_ptr<T[]> data;
    T * alignedData;
    int width, height;
    int dx, dy;
};

} // namespace hdrmerge

#endif // _ARRAY2D_HPP_
