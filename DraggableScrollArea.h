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


class DraggableScrollArea : public QScrollArea {
public:
    DraggableScrollArea(QWidget * parent) : QScrollArea(parent), moveViewport(true) {}
    
    void toggleMoveViewport(bool toggle) { moveViewport = toggle; }
    
signals:
    void drag(int x, int y);

public slots:
    void show(int x, int y);

protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
        
private:
    Q_OBJECT
    
    QPoint lastDragPos;
    QPoint lastScrollPos;
    bool moveViewport;
};


#endif // _DRAGGABLESCROLLAREA_H_
