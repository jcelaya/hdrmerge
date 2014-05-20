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
addPixels(false), rmPixels(false), layer(0), radius(5) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setMouseTracking(true);
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


QSize PreviewWidget::sizeHint() const {
    return pixmap.get() ? pixmap->size() : QSize(0, 0);
}


void PreviewWidget::paintEvent(QPaintEvent * event) {
    if (pixmap.get()) {
        QPainter painter(this);
        painter.drawPixmap(0, 0, *pixmap);
    }
}


QRgb PreviewWidget::getColor(int layer, int v) {
    int v70 = v*7/10;
    switch (layer) {
        case 0:
            return qRgb(v70, v, v70); break;
        case 1:
            return qRgb(v70, v70, v); break;
        case 2:
            return qRgb(v, v70, v70); break;
        case 3:
            return qRgb(v, v, v70); break;
        case 4:
            return qRgb(v, v70, v); break;
        case 5:
            return qRgb(v70, v, v); break;
        default:
            return qRgb(v, v, v); break;
    }
}


QRgb PreviewWidget::rgb(int col, int row) const {
    rotate(col, row);
    double v = stack->value(col, row);
    if (v < 0) v = 0;
    else if (v > 65535) v = 65535;
    return getColor(stack->getImageAt(col, row), stack->toneMap(v));
}


void PreviewWidget::render(int minx, int miny, int maxx, int maxy) {
    if (!stack.get()) return;
    QRect area = QRect(0, 0, width, height).intersected(QRect(minx, miny, maxx - minx, maxy - miny));
    if (area.isEmpty()) return;
    area.getCoords(&minx, &miny, &maxx, &maxy);
    QImage image(area.width() - 1, area.height() - 1, QImage::Format_RGB32);
    #pragma omp parallel for schedule(dynamic)
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
        painter.setPen(QPen(QColor(64, 64, 64), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(0, 0, radius*2, radius*2);
        QPen dashed(QColor(128, 128, 128), 2, Qt::CustomDashLine, Qt::RoundCap, Qt::RoundJoin);
        dashed.setDashPattern(QVector<qreal>({3, 4}));
        painter.setPen(dashed);
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
    if (event->buttons() & Qt::LeftButton && (addPixels || rmPixels)) {
        event->accept();
        stack->startEditAction(addPixels, layer);
        int x = event->x(), y = event->y(), rx = x, ry = y;
        rotate(rx, ry);
        stack->editPixels(rx, ry, radius);
        render(x - radius, y - radius, x + radius + 1, y + radius + 1);
    } else
        event->ignore();
}


void PreviewWidget::mouseMoveEvent(QMouseEvent * event) {
    int x = event->x(), y = event->y(), rx = x, ry = y;
    if (event->buttons() & Qt::LeftButton && (addPixels || rmPixels)) {
        event->accept();
        rotate(rx, ry);
        stack->editPixels(rx, ry, radius);
        render(x - radius, y - radius, x + radius + 1, y + radius + 1);
    } else if (addPixels || rmPixels) {
        update(x - radius, y - radius, 2*radius + 1, 2*radius + 1);
    } else
        event->ignore();
}


void PreviewWidget::undo() {
    EditableMask::Area a = stack->undo();
    render(a.minx, a.miny, a.maxx + 1, a.maxy + 1);
}


void PreviewWidget::redo() {
    EditableMask::Area a = stack->redo();
    render(a.minx, a.miny, a.maxx + 1, a.maxy + 1);
}
