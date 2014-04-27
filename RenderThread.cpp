/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cmath>
#include <QTime>
#include "RenderThread.hpp"
#include "ImageStack.hpp"
using namespace hdrmerge;


RenderThread::RenderThread(ImageStack * es, float gamma, QObject * parent)
    : QThread(parent), restart(false), abort(false), images(es), minx(0), miny(0), maxx(0), maxy(0) {
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


void RenderThread::setImageViewport(int x, int y, int w, int h) {
    mutex.lock();
    minx = x;
    miny = y;
    maxx = x + w;
    maxy = y + h;
    mutex.unlock();
}


void RenderThread::addPixels(int i, int x, int y, int radius) {
    mutex.lock();
    //images->addPixels(i, x, y, radius);
    QImage a(2*radius + 1, 2*radius + 1, QImage::Format_RGB32);
    doRender(x - radius, y - radius, x + radius + 1, y + radius + 1, a, true);
    emit renderedImage(x - radius, y - radius, images->getWidth(), images->getHeight(), a);
    mutex.unlock();
}


void RenderThread::removePixels(int i, int x, int y, int radius) {
    mutex.lock();
    //images->removePixels(i, x, y, radius);
    QImage a(2*radius + 1, 2*radius + 1, QImage::Format_RGB32);
    doRender(x - radius, y - radius, x + radius + 1, y + radius + 1, a, true);
    emit renderedImage(x - radius, y - radius, images->getWidth(), images->getHeight(), a);
    mutex.unlock();
}


QRgb RenderThread::rgb(size_t col, size_t row) const {
    int v = gamma[(int) images->value(col, row)];
    int color = images->getImageAt(col, row);
    QRgb pixel;
    switch (color) {
        case 0:
            pixel = qRgb(v*7/10, v, v*7/10); break;
        case 1:
            pixel = qRgb(v*7/10, v*7/10, v); break;
        case 2:
            pixel = qRgb(v, v*7/10, v*7/10); break;
        default:
            pixel = qRgb(v, v, v); break;
    }
}


void RenderThread::doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image, bool ignoreRestart) {
    QTime t;
    t.start();
    // Iterate through pixels
    for (size_t row = miny; !abort && (!restart || ignoreRestart) && row < maxy; row++) {
        QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - miny));
        for (size_t col = minx; col < maxx; col++) {
            *scanLine++ = rgb(col, row);
        }
    }
}


void RenderThread::run() {
    unsigned int _minx = 0, _miny = 0, _maxx = 0, _maxy = 0;
    forever {
        if (abort) return;

        if (_maxy > 0) {
            QImage a(_maxx - _minx, _maxy - _miny, QImage::Format_RGB32);
            doRender(_minx, _miny, _maxx, _maxy, a, true);
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

