#include "PreviewWidget.h"
#include <QImage>
#include <QWheelEvent>
#include <QCursor>
#include <QPainter>
#include <QTime>
#include <cmath>
#include <iostream>


PreviewWidget::PreviewWidget(QWidget * parent) : QLabel(parent), scale(1.0) {
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	setScaledContents(true);
}


void PreviewWidget::toggleCrossCursor(bool toggle) {
	if (toggle)
		setCursor(Qt::CrossCursor);
	else
		unsetCursor();
}


void PreviewWidget::paintImage(unsigned int x, unsigned int y, const QImage & image) {
	QTime t;
	t.start();
	if (!pixmap()) {
		setPixmap(QPixmap::fromImage(image));
	} else {
		QPixmap pix(pixmap()->width(), pixmap()->height());
		pix.fill();
		QPainter painter(&pix);
		painter.drawImage(x, y, image);
		setPixmap(pix);
	}
	resize(scale * pixmap()->size());
	update();
	QPoint min = mapFromParent(QPoint(0, 0));
	emit imageViewport(min.x() / scale, min.y() / scale,
		parentWidget()->width() / scale, parentWidget()->height() / scale);
	std::cerr << "Setting pixmap at " << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().constData() << ", "
		<< t.elapsed() << " ms elapsed" << std::endl;
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
	if (pixmap()) {
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
			resize(scale * pixmap()->size());
			emit focus(event->pos().x() * zoomFactor, event->pos().y() * zoomFactor);
		}
	}
}


void PreviewWidget::mouseReleaseEvent(QMouseEvent * event) {
	emit imageClicked(event->pos() / scale, event->button() == Qt::LeftButton);
}


void PreviewWidget::moveEvent(QMoveEvent * event) {
	QPoint min = mapFromParent(QPoint(0, 0));
	std::cout << "PreviewWidget: Image viewport changed to " << (min.x() / scale) << ',' << (min.y() / scale)
		<< ':' << (parentWidget()->width() / scale) << 'x' << (parentWidget()->height() / scale) << std::endl;
	emit imageViewport(min.x() / scale, min.y() / scale,
		parentWidget()->width() / scale, parentWidget()->height() / scale);
}

