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

#include "PreviewWidget.hpp"
#include <QImage>
#include <QPainter>
#include <QFuture>
#include <QtConcurrentRun>
#include <QApplication>
#include <QBitmap>
#include <QAction>
using namespace hdrmerge;


PreviewWidget::PreviewWidget(QWidget * parent) : QWidget(parent), width(0), height(0), flip(0),
addPixels(false), rmPixels(false), layer(0), radius(5), nextAction(editActions.end()) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}


void PreviewWidget::setImageStack(ImageStack * s) {
    stack.reset(s);
    flip = stack->getImage(0).getMetaData().flip;
    if (flip == 5 || flip == 6) {
        width = stack->getHeight();
        height = stack->getWidth();
    } else {
        width = stack->getWidth();
        height = stack->getHeight();
    }
    pixmap.reset();
    editActions.clear();
    nextAction = editActions.end();
    QtConcurrent::run(this, &PreviewWidget::render, 0, 0, width, height);
}


void PreviewWidget::rotate(int & x, int & y) const {
    int tmp;
    switch (flip) {
        case 3:
            x = width - x;
            y = height - y;
            break;
        case 5:
            tmp = x;
            x = height - y;
            y = tmp;
            break;
        case 6:
            tmp = x;
            x = y;
            y = width - tmp;
            break;
    }
}


void PreviewWidget::setGamma(float g) {
    g = 1.0f / g;
    for (int i = 0; i < 65536; i++)
        gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
}


QSize PreviewWidget::sizeHint() const {
    return pixmap.get() ? pixmap->size() : QSize(0, 0);
}


void PreviewWidget::paintEvent(QPaintEvent * event) {
    if (pixmap.get()) {
        QPainter painter(this);
        painter.drawPixmap(0, 0, *pixmap);
    }
}


QRgb PreviewWidget::rgb(int col, int row) const {
    rotate(col, row);
    int v = stack->value(col, row);
    if (v < 0) v = 0;
    else if (v > 65535) v = 65535;
    v = gamma[v];
    int color = stack->getImageAt(col, row);
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
    return pixel;
}


void PreviewWidget::render(int minx, int miny, int maxx, int maxy) {
    if (!stack.get()) return;
    QRect area = QRect(0, 0, width, height).intersected(QRect(minx, miny, maxx - minx, maxy - miny));
    if (area.isEmpty()) return;
    area.getCoords(&minx, &miny, &maxx, &maxy);
    QImage image(area.width() - 1, area.height() - 1, QImage::Format_RGB32);
    for (int row = miny; row < maxy; row++) {
        QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - miny));
        for (int col = minx; col < maxx; col++) {
            *scanLine++ = rgb(col, row);
        }
    }
    QMetaObject::invokeMethod(this, "paintImage", Qt::AutoConnection,
                              Q_ARG(int, minx), Q_ARG(int, miny), Q_ARG(const QImage &, image));
}


void PreviewWidget::paintImage(int x, int y, const QImage & image) {
    if (!pixmap.get()) {
        pixmap.reset(new QPixmap);
        *pixmap = QPixmap::fromImage(image);
        resize(pixmap->size());
    } else {
        QPainter painter(pixmap.get());
        painter.drawImage(x, y, image);
    }
    update(x, y, image.width(), image.height());
}


QPixmap PreviewWidget::createCursor(bool plus) {
    QPixmap cursor(radius*2 + 1, radius*2 + 1);
    cursor.fill(Qt::white);
    {
        QPainter painter(&cursor);
        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(0, 0, radius*2, radius*2);
        painter.drawLine(radius - 2, radius, radius + 2, radius);
        if (plus)
            painter.drawLine(radius, radius - 2, radius, radius + 2);
    }
    cursor.setMask(cursor.createMaskFromColor(Qt::white));
    return cursor;
}


void PreviewWidget::toggleAddPixelsTool(bool toggled) {
    addPixels = toggled;
    if (toggled) {
        setCursor(QCursor(createCursor(true), radius, radius));
    }
}


void PreviewWidget::toggleRmPixelsTool(bool toggled) {
    rmPixels = toggled;
    if (toggled) {
        setCursor(QCursor(createCursor(false), radius, radius));
    }
}


void PreviewWidget::mousePressEvent(QMouseEvent * event) {
    if (addPixels || rmPixels) {
        event->accept();
        editActions.erase(nextAction, editActions.end());
        editActions.emplace_back();
        nextAction = editActions.end();
        editActions.back().oldLayer = addPixels ? layer + 1 : layer;
        editActions.back().newLayer = addPixels ? layer : layer + 1;
        paintPixels(event->x(), event->y());
    } else
        event->ignore();
}
void PreviewWidget::mouseMoveEvent(QMouseEvent * event) {
    if (addPixels || rmPixels) {
        event->accept();
        paintPixels(event->x(), event->y());
    } else
        event->ignore();
}


void PreviewWidget::paintPixels(int x, int y) {
    if (!stack.get()) return;
    EditAction & e = editActions.back();
    int r2 = radius * radius;
    for (int row = -radius; row <= radius; ++row) {
        for (int col = -radius; col <= radius; ++col) {
            if (row*row + col*col <= r2) {
                size_t pos = (y + row)*width + (x + col);
                int rcol = x + col, rrow = y + row;
                rotate(rcol, rrow);
                if (pos < width*height && stack->getImageAt(rcol, rrow) == e.oldLayer) {
                    e.points.push_back(QPoint(x + col, y + row));
                    stack->setImageAt(rcol, rrow, e.newLayer);
                }
            }
        }
    }
    render(x - radius, y - radius, x + radius + 1, y + radius + 1);
}


void PreviewWidget::modifyLayer(const std::list<QPoint> & points, int layer) {
    int minx = width + 1, miny = height + 1, maxx = -1, maxy = -1;
    for (auto p : points) {
        int rcol = p.x(), rrow = p.y();
        rotate(rcol, rrow);
        stack->setImageAt(rcol, rrow, layer);
        if (p.x() < minx) minx = p.x();
        if (p.x() > maxx) maxx = p.x();
        if (p.y() < miny) miny = p.y();
        if (p.y() > maxy) maxy = p.y();
    }
    render(minx, miny, maxx + 1, maxy + 1);
}


void PreviewWidget::undo() {
    if (nextAction != editActions.begin()) {
        --nextAction;
        modifyLayer(nextAction->points, nextAction->oldLayer);
    }
}


void PreviewWidget::redo() {
    if (nextAction != editActions.end()) {
        modifyLayer(nextAction->points, nextAction->newLayer);
        ++nextAction;
    }
}
