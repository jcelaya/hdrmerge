#include <cmath>
#include "RenderThread.h"
#include <iostream>
#include <QTime>


RenderThread::RenderThread(ExposureStack * es, float gamma, QObject * parent)
	: QThread(parent), restart(false), abort(false), images(es), vpmin(0, 0), vpmax(0, 0) {
	setGamma(gamma);
}


RenderThread::~RenderThread() {
	abort = true;
	condition.wakeOne();
	wait();
	delete images;
}


//void RenderThread::render(QPoint viewportMin, QPoint viewportMax) {
void RenderThread::render() {
	mutex.lock();
	restart = true;
	//vpmin = viewportMin;
	//vpmax = viewportMax;
	mutex.unlock();
	condition.wakeOne();
}


void RenderThread::setGamma(float g) {
	g = 1.0f / g;
	for (int i = 0; i < 65536; i++)
		gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
}


void RenderThread::setExposureThreshold(int i, int th) {
	images->setThreshold(i, th);
	render();
}


void RenderThread::setExposureRelativeEV(int i, double re) {
	images->setRelativeExposure(i, re);
	render();
}


void RenderThread::calculateWB(int x, int y, int w, int h) {
	images->calculateWB(x, y, w, h);
	render();
}


void RenderThread::doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image) {
	QTime t;
	t.start();
	// Iterate through pixels
	for (unsigned int row = miny; !restart && row < maxy; row++) {
		if (abort) return;

		QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row));
		scanLine += minx;
		for (unsigned int col = minx; col < maxx; col++) {
			float rr, gg, bb;
			images->rgb(col, row, rr, gg, bb);
			int r = (int)rr, g = (int)gg, b = (int)bb;
			if (r >= 65536 || r < 0) std::cerr << "RValue " << r << " out of range at " << col << "x" << row << std::endl;
			if (g >= 65536 || g < 0) std::cerr << "GValue " << g << " out of range at " << col << "x" << row << std::endl;
			if (b >= 65536 || b < 0) std::cerr << "BValue " << b << " out of range at " << col << "x" << row << std::endl;
			// Apply gamma correction
			*scanLine++ = qRgb(gamma[r], gamma[g], gamma[b]);
		}
	}
	std::cerr << "Render time " << t.elapsed() << " ms at " << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().constData() << std::endl;
}


void RenderThread::run() {
	unsigned int minx = 0, miny = 0, maxx = 0, maxy = 0;
	forever {
		if (abort) return;

		/*
		QImage a(maxx - minx, maxy - miny, QImage::Format_RGB32);
		doRender(minx, miny, maxx, maxy, a);
		if (!restart && maxy > 0) {
			emit renderedImage(minx, miny, a);
			yieldCurrentThread();
		}
		*/
		
		QImage b(images->getWidth(), images->getHeight(), QImage::Format_RGB32);
		doRender(0, 0, images->getWidth(), images->getHeight(), b);
         	mutex.lock();
		if (!restart) {
			//emit renderedImage(0, 0, b);
			emit renderedImage(b);
			// Wait until render is called
			condition.wait(&mutex);
		}
		restart = false;
		// Check limits, due to roundings in preview label scale
		//minx = vpmin.x() < 0 ? 0 : vpmin.x();
		//miny = vpmin.y() < 0 ? 0 : vpmin.y();
		//maxx = vpmax.x() > images->getWidth() ? imgs->getWidth() : vpmax.x();
		//maxy = vpmax.y() > imgs->getHeight() ? imgs->getHeight() : vpmax.y();
		mutex.unlock();
	}
}


