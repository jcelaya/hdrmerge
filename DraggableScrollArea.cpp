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


void DraggableScrollArea::show(int x, int y) {
    ensureVisible(x, y, 0, 0);
    ensureVisible(x + viewport()->width(), y + viewport()->height(), 0, 0);
}


void DraggableScrollArea::mousePressEvent(QMouseEvent * event) {
    if (moveViewport && event->button() == Qt::LeftButton) {
        lastDragPos = event->pos();
        lastScrollPos.setX(horizontalScrollBar()->value());
        lastScrollPos.setY(verticalScrollBar()->value());
    } else
        emit drag(widget()->mapFromParent(event->pos()).x(), widget()->mapFromParent(event->pos()).y());
}


void DraggableScrollArea::mouseMoveEvent(QMouseEvent * event) {
    if (moveViewport && event->buttons() & Qt::LeftButton) {
        int deltax = event->pos().x() - lastDragPos.x();
        int deltay = event->pos().y() - lastDragPos.y();
        horizontalScrollBar()->setValue(lastScrollPos.x() - deltax);
        verticalScrollBar()->setValue(lastScrollPos.y() - deltay);
    } else
        emit drag(widget()->mapFromParent(event->pos()).x(), widget()->mapFromParent(event->pos()).y());
}

