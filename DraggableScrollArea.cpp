#include "DraggableScrollArea.h"
#include <QScrollBar>
#include <QMouseEvent>
#include <QCursor>


DraggableScrollArea::DraggableScrollArea(QWidget * parent) : QScrollArea(parent) {
	viewport()->setCursor(Qt::OpenHandCursor);
}


void DraggableScrollArea::center(int x, int y) {
	ensureVisible(x, y, viewport()->width() / 2, viewport()->height() / 2);
}


void DraggableScrollArea::mousePressEvent(QMouseEvent * event) {
	if (event->button() == Qt::LeftButton) {
		lastDragPos = event->pos();
		lastScrollPos.setX(horizontalScrollBar()->value());
		lastScrollPos.setY(verticalScrollBar()->value());
	}
}


void DraggableScrollArea::mouseMoveEvent(QMouseEvent * event) {
	if (event->buttons() & Qt::LeftButton) {
		int deltax = event->pos().x() - lastDragPos.x();
		int deltay = event->pos().y() - lastDragPos.y();
		horizontalScrollBar()->setValue(lastScrollPos.x() - deltax);
		verticalScrollBar()->setValue(lastScrollPos.y() - deltay);
	}
}

