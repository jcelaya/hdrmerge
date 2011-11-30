#include "DraggableScrollArea.h"
#include <QScrollBar>
#include <QMouseEvent>
#include <QCursor>


DraggableScrollArea::DraggableScrollArea(QWidget * parent) : QScrollArea(parent) {
	viewport()->setCursor(Qt::OpenHandCursor);
}


/*
void PreviewWidget::getViewRect(QPoint & min, QPoint & max) {
	min = mapFromParent(QPoint(0, 0));
	max = min + QPoint(area->viewport()->width(), area->viewport()->height());
	if (min.x() < 0) min.setX(0);
	if (min.y() < 0) min.setY(0);
	if (max.x() > width()) max.setX(width());
	if (max.y() > height()) max.setY(height());
	min /= scale;
	max /= scale;
}
*/


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

