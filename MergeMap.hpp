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

#ifndef _MERGEMAP_HPP_
#define _MERGEMAP_HPP_

#include <memory>
#include <cstdint>

namespace hdrmerge {

class ImageStack;
class Image;

class MergeMap {
public:
    MergeMap() {}
    MergeMap(size_t w, size_t h) : width(w), height(h), index(new uint8_t[w*h]) {}

    void generateFrom(const ImageStack & images);
    uint8_t operator[](size_t i) const {
        return index[i];
    }
    uint8_t & operator[](size_t i) {
        return index[i];
    }
    std::unique_ptr<float[]> blur() const;
    std::unique_ptr<float[]> blur(size_t radius) const;

private:
    class BoxBlur {
    public:
        BoxBlur(const MergeMap & src, size_t radius);
        std::unique_ptr<float[]> && getResult() {
            return std::move(map);
        }

    private:
        const MergeMap & m;
        void boxBlur_4(size_t radius);
        void boxBlurH_4(size_t radius);
        void boxBlurT_4(size_t radius);
        std::unique_ptr<float[]> map, tmp;
    };
    friend class BoxBlur;

    void paintPixels(int x, int y, size_t radius, uint8_t oldLayer, uint8_t newLayer);
    bool isNotSaturatedAround(const Image & img, size_t col, size_t row) const;

    size_t width, height;
    std::unique_ptr<uint8_t[]> index;
};

} // namespace hdrmerge

#endif // _MERGEMAP_HPP_
