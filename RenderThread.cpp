#include <cmath>
#include "RenderThread.h"
#include <iostream>


RenderThread::RenderThread(const ExposureStack * es, float gamma, QObject * parent)
	: QThread(parent), restart(false), abort(false), imgs(es) {
	setGamma(gamma);
	image = QImage(imgs->getWidth(), imgs->getHeight(), QImage::Format_RGB32);
}


RenderThread::~RenderThread() {
	abort = true;
	condition.wakeOne();
	wait();
}


void RenderThread::render() {
	mutex.lock();
	restart = true;
	mutex.unlock();
	condition.wakeOne();
}


void RenderThread::setGamma(float g) {
	g = 1.0f / g;
	for (int i = 0; i < 65536; i++)
		gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g));
}


void RenderThread::run() {
	forever {
		if (abort) return;

		// Iterate through pixels
		for (unsigned int row = 0; row < imgs->getHeight(); row++) {
			if (restart) break;
			if (abort) return;

			QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row));
			for (unsigned int col = 0; col < imgs->getWidth(); col++) {
				float rr, gg, bb;
				imgs->rgb(col, row, rr, gg, bb);
				int r = (int)std::floor(rr), g = (int)std::floor(gg), b = (int)std::floor(bb);
				// Apply contrast compression and gamma correction
				if (r >= 65536 || r < 0) std::cerr << "RValue " << r << " out of range at " << col << "x" << row << std::endl;
				if (g >= 65536 || g < 0) std::cerr << "GValue " << g << " out of range at " << col << "x" << row << std::endl;
				if (b >= 65536 || b < 0) std::cerr << "BValue " << b << " out of range at " << col << "x" << row << std::endl;
				r = gamma[r] >> 8;
				g = gamma[g] >> 8;
				b = gamma[b] >> 8;
				*scanLine++ = qRgb(r, g, b);
			}
		}

		// Wait until render is called
         	mutex.lock();
		if (!restart) {
			emit renderedImage(image);
			condition.wait(&mutex);
		}
		restart = false;
		mutex.unlock();
	}
}


