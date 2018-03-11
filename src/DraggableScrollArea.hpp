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

#ifndef _DRAGGABLESCROLLAREA_H_
#define _DRAGGABLESCROLLAREA_H_

#include <QScrollArea>
#include <QMouseEvent>

namespace hdrmerge {

class DraggableScrollArea : public QScrollArea {
public:
    DraggableScrollArea(QWidget * parent) : QScrollArea(parent), moveViewport(true) {}

public slots:
    void show(int x, int y);
    void toggleMoveViewport(bool toggle);

protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

private:
    Q_OBJECT

    QPoint mousePos;
    QPoint lastScrollPos;
    bool moveViewport;
};

} // namespace hdrmerge

#endif // _DRAGGABLESCROLLAREA_H_
