#include "PreviewWidget.h"
#include <QImage>
#include <QWheelEvent>
#include <QCursor>
#include <QPainter>
#include <QTime>
#include <QDebug>


PreviewWidget::PreviewWidget(QWidget * parent) : QLabel(parent), currentScale(0), newScale(0) {
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
	if (!pixmap()) {
		setPixmap(QPixmap::fromImage(image));
		resize(pixmap()->size());
	} else {
		QPixmap pix(width, height);
		pix.fill();
		QPainter painter(&pix);
		painter.drawImage(x, y, image);
		setPixmap(pix);
		unsigned int currentWidth = this->width();
		resize(pixmap()->size());
		if (width != currentWidth) {
			for (; currentWidth < width; currentWidth <<= 1) currentScale--;
			for (; currentWidth > width; currentWidth >>= 1) currentScale++;
			// If scale changed, move to new viewport
			emit focus(x, y);
		}
	}
	update();
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
	if (pixmap()) {
		// Zoom steps
		int minWidth = pixmap()->width(), minHeight = pixmap()->height();
		int minSteps = newScale - currentScale;
		while (minWidth > parentWidget()->width() || minHeight > parentWidget()->height()) {
			minWidth >>= 1;
			minHeight >>= 1;
			minSteps--;
		}
		int steps = event->delta() / 120;
		if (steps > newScale) steps = newScale;
		else if (steps < minSteps) steps = minSteps;
		if (steps == 0) return;

		// Compute new viewport
		QPoint currentMin = mapFromParent(QPoint(0, 0));
		int imageWidth = steps < 0 ? pixmap()->width() >> -steps : pixmap()->width() << steps;
		int newminx, w = parentWidget()->width(), halfw = w >> 1;
		if (w > imageWidth) {
                        newminx = 0;
                        w = imageWidth;
                } else {
                        int centerx = steps < 0 ? (currentMin.x() + halfw) >> -steps : (currentMin.x() + halfw) << steps;
                        newminx = centerx - halfw > 0 ? centerx - halfw : 0;
                        if (newminx + w > imageWidth)
				newminx = imageWidth - w;
                }
		int imageHeight = steps < 0 ? pixmap()->height() >> -steps : pixmap()->height() << steps;
		int newminy, h = parentWidget()->height(), halfh = h >> 1;
		if (h > imageHeight) {
                        newminy = 0;
                        h = imageHeight;
                } else {
                        int centery = steps < 0 ? (currentMin.y() + halfh) >> -steps : (currentMin.y() + halfh) << steps;
                        newminy = centery - halfh > 0 ? centery - halfh : 0;
                        if (newminy + h > imageHeight)
				newminy = imageHeight - h;
                }
                qDebug() << "New viewport will be " << newminx << ',' << newminy << ':' << (newminx + w) << ',' << (newminy + h);

		newScale = newScale - steps;
		emit imageViewport(newminx, newminy, w, h, newScale);
	}
}


void PreviewWidget::mouseReleaseEvent(QMouseEvent * event) {
	emit imageClicked(event->pos(), event->button() == Qt::LeftButton);
}


void PreviewWidget::moveEvent(QMoveEvent * event) {
	QPoint min = mapFromParent(QPoint(0, 0));
	emit imageViewport(min.x(), min.y(), parentWidget()->width(), parentWidget()->height(), newScale);
}

