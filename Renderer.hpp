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

#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include <memory>
#include <QImage>
#include "MetaData.hpp"

namespace hdrmerge {


class Renderer {
public:
    Renderer(float * rawData, size_t width, size_t height, const MetaData & md);

    void process();
    QImage getScaledVersion(size_t width);

private:
    void convertToRgb();
    void whiteBalance();
    void autoWB();
    void buildResult();

    MetaData md;
    int width, height;
    std::unique_ptr<float[][4]> image;
    QImage halfInterpolated;
};

}

#endif // _RENDERER_HPP_
