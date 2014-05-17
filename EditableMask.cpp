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
#include <cmath>
#include <QImage>
#include "EditableMask.hpp"
#include "ImageStack.hpp"
#include "Log.hpp"
using namespace hdrmerge;

void EditableMask::writeMaskImage(const std::string & maskFile) {
    QImage maskImage(width, height, QImage::Format_Indexed8);
    int numColors = numLayers - 1;
    for (int c = 0; c < numColors; ++c) {
        int gray = (256 * c) / numColors;
        maskImage.setColor(c, qRgb(gray, gray, gray));
    }
    maskImage.setColor(numColors, qRgb(255, 255, 255));
    for (size_t y = 0, pos = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x, ++pos) {
            maskImage.setPixel(x, y, mask[pos]);
        }
    }
    if (!maskImage.save(QString(maskFile.c_str()))) {
        Log::msg(Log::PROGRESS, "Cannot save mask image to ", maskFile);
    }
}


void EditableMask::generateFrom(const ImageStack & images) {
    width = images.getWidth();
    height = images.getHeight();
    numLayers = images.size();
    editActions.clear();
    nextAction = editActions.end();
    size_t size = width*height;

    Timer t("Generate mask");
    mask.reset(new uint8_t[size]);
    std::fill_n(mask.get(), size, 0);
    for (size_t y = 0, pos = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x, ++pos) {
            int i = mask[pos];
            while (i < numLayers - 1 &&
                (!images.getImage(i).contains(x, y) ||
                images.getImage(i).isSaturated(x, y) ||
                images.getImage(i).isSaturatedAround(x, y))) ++i;
            if (mask[pos] < i) {
                mask[pos] = i;
                //if (!images.getImage(i - 1).isSaturatedAround(x, y)) {
                    paintCircle(x, y, 4, [&] (int col, int row) {
                        size_t pos = row*width + col;
                        if (mask[pos] < i && images.getImage(i).contains(col, row)) {
                            mask[pos] = i;
                        }
                    });
                //}
            }
        }
    }
}


void EditableMask::startAction(bool add, int layer) {
    editActions.erase(nextAction, editActions.end());
    editActions.emplace_back();
    nextAction = editActions.end();
    editActions.back().oldLayer = add ? layer + 1 : layer;
    editActions.back().newLayer = add ? layer : layer + 1;
}


void EditableMask::paintPixels(const ImageStack & images, int x, int y, size_t radius) {
    EditAction & e = editActions.back();
    paintCircle(x, y, radius, [&] (int col, int row) {
        size_t pos = row*width + col;
        if (mask[pos] == e.oldLayer && images.getImage(e.newLayer).contains(col, row)) {
            e.points.push_back({col, row});
            mask[pos] = e.newLayer;
        }
    });
}


EditableMask::Area EditableMask::undo() {
    Area result;
    if (nextAction != editActions.begin()) {
        --nextAction;
        result = modifyLayer(nextAction->points, nextAction->oldLayer);
    }
    return result;
}


EditableMask::Area EditableMask::redo() {
    Area result;
    if (nextAction != editActions.end()) {
        result = modifyLayer(nextAction->points, nextAction->newLayer);
        ++nextAction;
    }
    return result;
}


EditableMask::Area EditableMask::modifyLayer(const std::list<Point> & points, int layer) {
    Area a;
    a.minx = width + 1;
    a.miny = height + 1;
    a.maxx = -1;
    a.maxy = -1;
    for (auto p : points) {
        mask[p.y * width + p.x] = layer;
        if (p.x < a.minx) a.minx = p.x;
        if (p.x > a.maxx) a.maxx = p.x;
        if (p.y < a.miny) a.miny = p.y;
        if (p.y > a.maxy) a.maxy = p.y;
    }
    return a;
}


std::unique_ptr<float[]> EditableMask::blur() const {
    return blur(3);
}


std::unique_ptr<float[]> EditableMask::blur(size_t radius) const {
    BoxBlur b(*this, radius);
    return b.getResult();
}


EditableMask::BoxBlur::BoxBlur(const EditableMask & src, size_t radius) : m(src) {
    // From http://blog.ivank.net/fastest-gaussian-blur.html
    map.reset(new float[m.width*m.height]);
    tmp.reset(new float[m.width*m.height]);
    for (size_t i = 0; i < m.width*m.height; ++i) {
        map[i] = m.mask[i];
    }
    size_t hr = std::round(radius*0.39);
    boxBlur_4(hr);
    boxBlur_4(hr);
    boxBlur_4(hr);
}


void EditableMask::BoxBlur::boxBlur_4(size_t radius) {
    boxBlurH_4(radius);
    map.swap(tmp);
    boxBlurT_4(radius);
    map.swap(tmp);
}


void EditableMask::BoxBlur::boxBlurH_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < m.height; ++i) {
        size_t ti = i * m.width, li = ti, ri = ti + r;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri++] - map[li];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = r + 1; j < m.width - r; ++j) {
            val += map[ri++] - map[li++];
            tmp[ti++] = val*iarr;
        }
        for (size_t j = m.width - r; j < m.width; ++j) {
            val += map[ri - 1] - map[li++];
            tmp[ti++] = val*iarr;
        }
    }
}


void EditableMask::BoxBlur::boxBlurT_4(size_t r) {
    float iarr = 1.0 / (r+r+1);
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < m.width; ++i) {
        size_t ti = i, li = ti, ri = ti + r*m.width;
        float val = map[li] * (r + 1);
        for (size_t j = 0; j < r; ++j) {
            val += map[li + j*m.width];
        }
        for (size_t j = 0; j <= r; ++j) {
            val += map[ri] - map[li];
            tmp[ti] = val*iarr;
            ri += m.width;
            ti += m.width;
        }
        for (size_t j = r + 1; j < m.height - r; ++j) {
            val += map[ri] - map[li];
            tmp[ti] = val*iarr;
            li += m.width;
            ri += m.width;
            ti += m.width;
        }
        for (size_t j = m.height - r; j < m.height; ++j) {
            val += map[ri - m.width] - map[li];
            tmp[ti] = val*iarr;
            li += m.width;
            ti += m.width;
        }
    }
}
