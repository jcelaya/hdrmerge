#include "PreviewWidget.h"
#include <QWheelEvent>
#include <cmath>
#include <iostream>


PreviewWidget::PreviewWidget(QWidget * parent) : QScrollArea(parent) {
	//setWidgetResizable(true);
	previewLabel = new QLabel(this);
	previewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	previewLabel->setScaledContents(true);
	previewLabel->setAlignment(Qt::AlignCenter);
	setWidget(previewLabel);
	setAlignment(Qt::AlignCenter);
	scale = 1.0;
}


void PreviewWidget::setPixmap(const QPixmap & pixmap) {
	previewLabel->setPixmap(pixmap);
	previewLabel->resize(scale * pixmap.size());
}


void PreviewWidget::wheelEvent(QWheelEvent *event) {
	if (previewLabel->pixmap()) {
		int steps = event->delta() / 120;
		double zoomFactor = pow(2.0, steps);
		double newScale = scale * zoomFactor;
		if (newScale > 1.0)
			newScale = 1.0;
		else if (newScale < 1.0/16.0)
			newScale = 1.0/16.0;
		zoomFactor = newScale / scale;
		scale = newScale;
		previewLabel->resize(scale * previewLabel->pixmap()->size());
		horizontalScrollBar()->setValue(int(zoomFactor * horizontalScrollBar()->value()
			+ ((zoomFactor - 1) * horizontalScrollBar()->pageStep()/2)));
		verticalScrollBar()->setValue(int(zoomFactor * verticalScrollBar()->value()
			+ ((zoomFactor - 1) * verticalScrollBar()->pageStep()/2)));
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

