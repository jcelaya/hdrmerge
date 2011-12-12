#include <cmath>
#include "RenderThread.h"
#include <QTime>
#include <QDebug>


RenderThread::RenderThread(ExposureStack * es, float gamma, QObject * parent)
	: QThread(parent), restart(false), abort(false), images(es), minx(0), miny(0), maxx(0), maxy(0), scale(0) {
	setGamma(gamma);
}


RenderThread::~RenderThread() {
	abort = true;
	condition.wakeOne();
	wait();
	delete images;
}


void RenderThread::setGamma(float g) {
	mutex.lock();
	g = 1.0f / g;
	for (int i = 0; i < 65536; i++)
		gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
	mutex.unlock();
}


void RenderThread::setExposureThreshold(int i, int th) {
	mutex.lock();
	images->setThreshold(i, ((th + 1) << 8) - 1);
	restart = true;
	mutex.unlock();
	condition.wakeOne();
}


void RenderThread::setExposureRelativeEV(int i, double re) {
	mutex.lock();
	images->setRelativeExposure(i, re);
	restart = true;
	mutex.unlock();
	condition.wakeOne();
}


void RenderThread::calculateWB(int x, int y, int radius) {
	mutex.lock();
	unsigned int w = images->getWidth() - x > 2*radius ? 2*radius : images->getWidth() - x;
	unsigned int h = images->getHeight() - y > 2*radius ? 2*radius : images->getHeight() - y;
	x = x > radius ? x - radius : 0;
	y = y > radius ? y - radius : 0;
	images->calculateWB(x, y, w, h);
	restart = true;
	emit whiteBalanceChanged(images->getWBRG(), images->getWBRB());
	mutex.unlock();
	condition.wakeOne();
}


void RenderThread::setImageViewport(int x, int y, int w, int h) {
	mutex.lock();
	// Check limits, due to roundings in preview label scale
	minx = x > 0 ? x : 0;
	miny = y > 0 ? y : 0;
	maxx = x + w <= images->getWidth() ? x + w : images->getWidth();
	maxy = y + h <= images->getHeight() ? y + h : images->getHeight();
	qDebug() << "Viewport set to " << minx << ',' << miny << ':' << maxx << ',' << maxy ;
	mutex.unlock();
}


void RenderThread::stepScale(int steps) {
	mutex.lock();
	int newScale = scale - steps;
	if (newScale < 0) newScale = 0;
	else if (newScale > 4) newScale = 4;
	if (newScale != scale) {
		images->setScale(newScale);

		// Recalculate viewport
		unsigned int w = maxx - minx;
		if (w > images->getWidth()) {
			minx = 0;
			maxx = images->getWidth();
		} else {
			unsigned int centerx = minx + maxx;
			if (newScale > scale) {
				centerx >>= (newScale - scale + 1);
			} else {
				centerx <<= (scale - newScale - 1);
			}
			int x = centerx - w / 2;
			minx = x > 0 ? x : 0;
			if (minx + w > images->getWidth()) {
				maxx = images->getWidth();
				minx = maxx - w;
			} else maxx = minx + w;
		}
		unsigned int h = maxy - miny;
		if (h > images->getHeight()) {
			miny = 0;
			maxy = images->getHeight();
		} else {
			unsigned int centery = miny + maxy;
			if (newScale > scale) {
				centery >>= (newScale - scale + 1);
			} else {
				centery <<= (scale - newScale - 1);
			}
			int y = centery - h / 2;
			miny = y > 0 ? y : 0;
			if (miny + h > images->getHeight()) {
				maxy = images->getHeight();
				miny = maxy - h;
			} else maxy = miny + h;
		}
		qDebug() << "New viewport set to " << minx << ',' << miny << ':' << maxx << ',' << maxy ;

		restart = true;
		scale = newScale;
		mutex.unlock();
		condition.wakeOne();
	} else mutex.unlock();
}


void RenderThread::doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image) {
	QTime t;
	t.start();
	// Iterate through pixels
	for (unsigned int row = miny; !restart && row < maxy; row++) {
		if (abort) return;

		QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - miny));
		for (unsigned int col = minx; col < maxx; col++) {
			double rr, gg, bb;
			images->rgb(col, row, rr, gg, bb);
			int r = (int)rr, g = (int)gg, b = (int)bb;
			if (r >= 65536 || r < 0) std::cerr << "RValue " << r << " out of range at " << col << "x" << row << std::endl;
			if (g >= 65536 || g < 0) std::cerr << "GValue " << g << " out of range at " << col << "x" << row << std::endl;
			if (b >= 65536 || b < 0) std::cerr << "BValue " << b << " out of range at " << col << "x" << row << std::endl;
			// Apply gamma correction
			*scanLine++ = qRgb(gamma[r], gamma[g], gamma[b]);
			//*scanLine++ = qRgb(r, g, b);
		}
	}
	qDebug() << "Render time " << t.elapsed() << " ms at " << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().constData();
}


void RenderThread::run() {
	unsigned int _minx = 0, _miny = 0, _maxx = 0, _maxy = 0;
	forever {
		if (abort) return;

		QImage a(_maxx - _minx, _maxy - _miny, QImage::Format_RGB32);
		doRender(_minx, _miny, _maxx, _maxy, a);
		if (!restart && _maxy > 0) {
			emit renderedImage(_minx, _miny, images->getWidth(), images->getHeight(), a);
			yieldCurrentThread();
		}
		
		QImage b(images->getWidth(), images->getHeight(), QImage::Format_RGB32);
		doRender(0, 0, images->getWidth(), images->getHeight(), b);
         	mutex.lock();
		if (!restart) {
			emit renderedImage(0, 0, images->getWidth(), images->getHeight(), b);
			// Wait until render is called
			condition.wait(&mutex);
		}
		restart = false;
		_minx = minx;
		_miny = miny;
		_maxx = maxx;
		_maxy = maxy;
		mutex.unlock();
	}
}

