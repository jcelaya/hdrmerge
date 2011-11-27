#include "PreviewWidget.h"
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <QTime>


PreviewWidget::PreviewWidget(QWidget * parent) : QScrollArea(parent) {
	previewLabel = new QLabel(this);
	previewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	previewLabel->setScaledContents(true);
	previewLabel->setAlignment(Qt::AlignCenter);
	setWidget(previewLabel);
	setAlignment(Qt::AlignCenter);
	scale = 1.0;
}


void PreviewWidget::setPixmap(const QPixmap & pixmap) {
	std::cerr << "Seting pixmap at " << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().constData() << std::endl;
	previewLabel->setPixmap(pixmap);
	previewLabel->resize(scale * pixmap.size());
	previewLabel->repaint();
}


void PreviewWidget::getViewRect(QPoint & min, QPoint & max) {
	min = previewLabel->mapFromParent(QPoint(0, 0));
	max = min + QPoint(viewport()->width(), viewport()->height());
	if (min.x() < 0) min.setX(0);
	if (min.y() < 0) min.setY(0);
	if (max.x() > previewLabel->width()) max.setX(previewLabel->width());
	if (max.y() > previewLabel->height()) max.setY(previewLabel->height());
	min /= scale;
	max /= scale;
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
	if (previewLabel->pixmap()) {
		int steps = event->delta() / 120;
		double zoomFactor = pow(2.0, steps);
		double newScale = scale * zoomFactor;
		if (newScale > 1.0)
			newScale = 1.0;
		else if (newScale < 1.0/16.0)
			newScale = 1.0/16.0;
		zoomFactor = newScale / scale;
		if (zoomFactor != 1.0) {
			scale = newScale;
			QPoint pos = zoomFactor * previewLabel->mapFromParent(viewport()->mapFromParent(event->pos()));
			previewLabel->resize(scale * previewLabel->pixmap()->size());
			ensureVisible(pos.x(), pos.y(), viewport()->width() / 2, viewport()->height() / 2);
		}
	}
}


void PreviewWidget::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		lastDragPos = event->pos();
		lastScrollPos.setX(horizontalScrollBar()->value());
		lastScrollPos.setY(verticalScrollBar()->value());
	}
}


void PreviewWidget::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton) {
		int deltax = event->pos().x() - lastDragPos.x();
		int deltay = event->pos().y() - lastDragPos.y();
		horizontalScrollBar()->setValue(lastScrollPos.x() - deltax);
		verticalScrollBar()->setValue(lastScrollPos.y() - deltay);
	}
}


void PreviewWidget::mouseReleaseEvent(QMouseEvent *event) {
}

