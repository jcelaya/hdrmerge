#include "DraggableScrollArea.h"
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
        emit drag(event->pos().x(), event->pos().y());
}


void DraggableScrollArea::mouseMoveEvent(QMouseEvent * event) {
    if (moveViewport && event->buttons() & Qt::LeftButton) {
        int deltax = event->pos().x() - lastDragPos.x();
        int deltay = event->pos().y() - lastDragPos.y();
        horizontalScrollBar()->setValue(lastScrollPos.x() - deltax);
        verticalScrollBar()->setValue(lastScrollPos.y() - deltay);
    } else
        emit drag(event->pos().x(), event->pos().y());
}

