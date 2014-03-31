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

#include "ImageStack.hpp"

namespace hdrmerge {

class MergeMap {
public:
    MergeMap(size_t w, size_t h) : map(new float[w*h]), width(w), height(h) {}
    MergeMap(const ImageStack & stack);
    float operator[](size_t i) const {
        return map[i];
    }
    float & operator[](size_t i) {
        return map[i];
    }
    void blur(size_t radius);

private:
    void boxBlur_4(size_t radius);
    void boxBlurH_4(size_t radius);
    void boxBlurT_4(size_t radius);

    std::unique_ptr<float[]> map, blurTmp;
    size_t width, height;
};

} // namespace hdrmerge

#endif // _MERGEMAP_HPP_
