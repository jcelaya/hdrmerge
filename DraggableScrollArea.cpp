#include "DraggableScrollArea.h"
#include <QScrollBar>
#include <QMouseEvent>
#include <QCursor>


DraggableScrollArea::DraggableScrollArea(QWidget * parent) : QScrollArea(parent) {
	viewport()->setCursor(Qt::OpenHandCursor);
}


void DraggableScrollArea::show(int x, int y) {
	ensureVisible(x, y, 0, 0);
	ensureVisible(x + viewport()->width(), y + viewport()->height(), 0, 0);
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

