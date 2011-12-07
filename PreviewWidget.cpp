#include "PreviewWidget.h"
#include <QImage>
#include <QWheelEvent>
#include <QCursor>
#include <QPainter>
#include <QTime>
#include <cmath>
#include <iostream>


PreviewWidget::PreviewWidget(QWidget * parent) : QLabel(parent) {
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	setScaledContents(false);
}


void PreviewWidget::toggleCrossCursor(bool toggle) {
	if (toggle)
		setCursor(Qt::CrossCursor);
	else
		unsetCursor();
}


void PreviewWidget::paintImage(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const QImage & image) {
	QTime t;
	t.start();
	if (!pixmap()) {
		setPixmap(QPixmap::fromImage(image));
		resize(pixmap()->size());
	} else {
		// Calculate center point
		QPoint min = mapFromParent(QPoint(0, 0));
		double scale = width / pixmap()->width();
		QPixmap pix(width, height);
		pix.fill();
		QPainter painter(&pix);
		painter.drawImage(x, y, image);
		setPixmap(pix);
		resize(pixmap()->size());
		emit focus((min.x() + parentWidget()->width() / 2) * scale, (min.y() + parentWidget()->height() / 2) * scale);
	}
	update();
	std::cerr << "Setting pixmap at " << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().constData() << ", "
		<< t.elapsed() << " ms elapsed" << std::endl;
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
	if (pixmap()) {
		int steps = event->delta() / 120;
		emit scaleBy(steps);
	}
}


void PreviewWidget::mouseReleaseEvent(QMouseEvent * event) {
	emit imageClicked(event->pos(), event->button() == Qt::LeftButton);
}


void PreviewWidget::moveEvent(QMoveEvent * event) {
	QPoint min = mapFromParent(QPoint(0, 0));
	emit imageViewport(min.x(), min.y(), parentWidget()->width(), parentWidget()->height());
}

