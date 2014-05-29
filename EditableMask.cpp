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

#include "EditableMask.hpp"
using namespace hdrmerge;


void EditableMask::startAction(bool add, int layer) {
    editActions.erase(nextAction, editActions.end());
    editActions.emplace_back();
    nextAction = editActions.end();
    editActions.back().oldLayer = add ? layer + 1 : layer;
    editActions.back().newLayer = add ? layer : layer + 1;
}


void EditableMask::editPixels(int x, int y, size_t radius) {
    EditAction & e = editActions.back();
    traceCircle(x, y, radius, [&] (int col, int row, uint8_t & layer) {
        if (layer == e.oldLayer && isLayerValidAt(e.newLayer, col, row)) {
            e.points.push_back({col, row});
            layer = e.newLayer;
        }
    });
}


QRect EditableMask::undo() {
    QRect result;
    if (nextAction != editActions.begin()) {
        --nextAction;
        result = modifyLayer(nextAction->points, nextAction->oldLayer);
    }
    return result;
}


QRect EditableMask::redo() {
    QRect result;
    if (nextAction != editActions.end()) {
        result = modifyLayer(nextAction->points, nextAction->newLayer);
        ++nextAction;
    }
    return result;
}


QRect EditableMask::modifyLayer(const std::list<QPoint> & points, int layer) {
    if (points.empty()) {
        return QRect(0, 0, 0, 0);
    } else {
        QRect a(points.front(), points.front());
        for (auto p : points) {
            operator()(p.x(), p.y()) = layer;
            a = a.unite(QRect(p, p));
        }
        return a;
    }
}
