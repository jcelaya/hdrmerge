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

#include <cstdint>
#include <list>
#include "Array2D.hpp"

namespace hdrmerge {

class ImageStack;

class EditableMask {
public:
    EditableMask(Array2D<uint8_t> & m) : mask(m), nextAction(editActions.end()) {}

    struct Area {
        int minx, miny, maxx, maxy;
        Area() : minx(0), miny(0), maxx(0), maxy(0) {}
    };

    uint8_t getImageAt(size_t col, size_t row) const {
        return mask(col, row);
    }
    uint8_t & operator[](size_t i) {
        return mask[i];
    }
    void startAction(bool add, int layer);
    void editPixels(const ImageStack & images, int x, int y, size_t radius);
    bool canUndo() const {
        return nextAction != editActions.begin();
    }
    bool canRedo() const {
        return nextAction != editActions.end();
    }
    Area undo();
    Area redo();

private:
    struct Point {
        int x, y;
    };
    struct EditAction {
        int oldLayer, newLayer;
        std::list<Point> points;
    };

    Array2D<uint8_t> & mask;
    std::list<EditAction> editActions;
    std::list<EditAction>::iterator nextAction;

    Area modifyLayer(const std::list<Point> & points, int oldayer);
};

} // namespace hdrmerge

#endif // _EDITABLEMASK_HPP_
