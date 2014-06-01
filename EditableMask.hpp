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
#include <QRect>
#include "Array2D.hpp"

namespace hdrmerge {

class EditableMask : public Array2D<uint8_t> {
public:
    EditableMask() : nextAction(editActions.end()) {}
    void reset() {
        editActions.clear();
        nextAction = editActions.end();
    }

    void startAction(bool add, int layer);
    void editPixels(int x, int y, size_t radius);
    bool canUndo() const {
        return nextAction != editActions.begin();
    }
    bool canRedo() const {
        return nextAction != editActions.end();
    }
    QRect undo();
    QRect redo();

private:
    struct EditAction {
        int oldLayer, newLayer;
        std::list<QPoint> points;
    };

    std::list<EditAction> editActions;
    std::list<EditAction>::iterator nextAction;

    QRect modifyLayer(const std::list<QPoint> & points, int oldayer);
    virtual bool isLayerValidAt(int layer, int x, int y) const = 0;
};

} // namespace hdrmerge

#endif // _EDITABLEMASK_HPP_
