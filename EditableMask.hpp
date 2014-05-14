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

#ifndef _EDITABLEMASK_HPP_
#define _EDITABLEMASK_HPP_

#include <memory>
#include <cstdint>
#include <string>
#include <list>

namespace hdrmerge {

class ImageStack;
class Image;

class EditableMask {
public:
    EditableMask() : numLayers(1), nextAction(editActions.end()) {}
    EditableMask(size_t w, size_t h) : numLayers(1), nextAction(editActions.end()),
        width(w), height(h), mask(new uint8_t[w*h])  {}

    struct Area {
        int minx, miny, maxx, maxy;
        Area() : minx(0), miny(0), maxx(0), maxy(0) {}
    };

    void generateFrom(const ImageStack & images);
    uint8_t getImageAt(size_t col, size_t row) const {
        return mask[row*width + col];
    }
    uint8_t & operator[](size_t i) {
        return mask[i];
    }
    void startAction(bool add, int layer);
    void paintPixels(int x, int y, size_t radius);
    Area undo();
    Area redo();
    std::unique_ptr<float[]> blur() const;
    std::unique_ptr<float[]> blur(size_t radius) const;

    void writeMaskImage(const std::string & maskFile);

private:
    class BoxBlur {
    public:
        BoxBlur(const EditableMask & src, size_t radius);
        std::unique_ptr<float[]> && getResult() {
            return std::move(map);
        }

    private:
        const EditableMask & m;
        void boxBlur_4(size_t radius);
        void boxBlurH_4(size_t radius);
        void boxBlurT_4(size_t radius);
        std::unique_ptr<float[]> map, tmp;
    };
    friend class BoxBlur;

    struct Point {
        int x, y;
    };
    struct EditAction {
        int oldLayer, newLayer;
        std::list<Point> points;
    };

    size_t width, height;
    std::unique_ptr<uint8_t[]> mask;
    int numLayers;
    std::list<EditAction> editActions;
    std::list<EditAction>::iterator nextAction;

    bool isNotSaturatedAround(const Image & img, size_t col, size_t row) const;
    Area modifyLayer(const std::list<Point> & points, int oldayer);
    void paintPixels(int x, int y, size_t radius, int oldLayer, int newLayer);
};

} // namespace hdrmerge

#endif // _EDITABLEMASK_HPP_
