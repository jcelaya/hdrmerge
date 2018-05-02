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

#include "DraggableScrollArea.hpp"
#include <QScrollBar>
#include <QMouseEvent>
#include <QCursor>
using namespace hdrmerge;


void DraggableScrollArea::show(int x, int y) {
    ensureVisible(x, y, 0, 0);
    ensureVisible(x + viewport()->width(), y + viewport()->height(), 0, 0);
}


void DraggableScrollArea::toggleMoveViewport(bool toggle) {
    moveViewport = toggle;
    if (toggle)
        widget()->setCursor(QCursor(Qt::CrossCursor));
}


void DraggableScrollArea::mousePressEvent(QMouseEvent * event) {
    if (moveViewport && event->button() == Qt::LeftButton) {
        mousePos = QCursor::pos();
        widget()->setCursor(QCursor(Qt::OpenHandCursor));
        dragging = true;
    }
}


void DraggableScrollArea::mouseReleaseEvent(QMouseEvent * event) {
    if (moveViewport && event->button() == Qt::LeftButton) {
        widget()->setCursor(QCursor(Qt::CrossCursor));
        dragging = false;
    }
}


void DraggableScrollArea::mouseMoveEvent(QMouseEvent * event) {
    if (dragging) {
        QPoint delta = QCursor::pos() - mousePos;
        // x5 panning speed
        delta.setX(delta.x() * 5);
        delta.setY(delta.y() * 5);
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        mousePos = QCursor::pos();
    }
}

bool DraggableScrollArea::isDragging() {
    return dragging;
}
