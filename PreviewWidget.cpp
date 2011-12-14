#include "PreviewWidget.h"
#include <QImage>
#include <QWheelEvent>
#include <QCursor>
#include <QPainter>


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


void PreviewWidget::paintImage(unsigned int x, unsigned int y,
	unsigned int width, unsigned int height, const QImage & image) {
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
			for (; currentScale > 0 && currentWidth < width; currentWidth <<= 1) currentScale--;
			for (; currentWidth > width; currentWidth >>= 1) currentScale++;
			// If scale changed, move to new viewport
			emit focus(x, y);
		}
	}
	update();
}


void PreviewWidget::zoom(int steps) {
	if (steps == 0) return;
	// Positive steps is zoom in, negative is zoom out, the reverse of scale... so pay attention
	if (pixmap()) {
		// Zoom steps
		int minWidth = pixmap()->width(), minHeight = pixmap()->height();
		int minSteps = 0;
		while (minWidth > parentWidget()->width() || minHeight > parentWidget()->height()) {
			minWidth >>= 1;
			minHeight >>= 1;
			minSteps--;
		}
		// Steps are referred to newScale, but need reference to currentScale
		steps += newScale - currentScale;
		if (steps > currentScale) steps = currentScale;
		else if (steps < minSteps) steps = minSteps;
		newScale = currentScale - steps;

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

		emit imageViewport(newminx, newminy, w, h, newScale);
	}
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
	zoom(event->delta() / 120);
}


void PreviewWidget::moveEvent(QMoveEvent * event) {
	QPoint min = mapFromParent(QPoint(0, 0));
	emit imageViewport(min.x(), min.y(), parentWidget()->width(), parentWidget()->height(), newScale);
}

