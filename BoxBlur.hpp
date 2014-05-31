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

#ifndef _BOXBLUR_HPP_
#define _BOXBLUR_HPP_

#include <memory>
#include "Array2D.hpp"

namespace hdrmerge {

class BoxBlur : public Array2D<float>{
public:
    template <typename T> BoxBlur(const Array2D<T> & src) : Array2D<float>(src) {}
    void blur(size_t radius);

private:
    void boxBlur(size_t radius);
    void boxBlurH(size_t radius);
    void boxBlurT(size_t radius);
    std::unique_ptr<float[]> tmp;
};
} // namespace hdrmerge

#endif // _BOXBLUR_HPP_
