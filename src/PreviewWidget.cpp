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
#include "Log.hpp"
using namespace hdrmerge;


PreviewWidget::PreviewWidget(ImageStack & s, QWidget * parent) : QWidget(parent), stack(s),
width(0), height(0), flip(0), addPixels(false), rmPixels(false), layer(0), radius(5), expMult(1.0) {
    float g = 1.0f / 2.2f;
    for (int i = 0; i < 65536; i++) {
        gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
    }
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setMouseTracking(true);
}


void PreviewWidget::reload() {
    layer = 0;
    expMult = 1.0;
    flip = stack.size() ? stack.getFlip() : 0;
    if (flip == 5 || flip == 6) {
        width = stack.getHeight();
        height = stack.getWidth();
    } else {
        width = stack.getWidth();
        height = stack.getHeight();
    }
    pixmap.reset();
    resize(QSize(0, 0));
    repaintAsync();
}


void PreviewWidget::repaintAsync() {
    cancelRender = true;
    currentRender.waitForFinished();
    currentRender = QtConcurrent::run(this, &PreviewWidget::render, QRect(0, 0, width, height));
}


void PreviewWidget::rotate(int & x, int & y) const {
    int tmp;
    switch (flip) {
        case 3:
            x = width - 1 - x;
            y = height - 1 - y;
            break;
        case 5:
            tmp = x;
            x = height - 1 - y;
            y = tmp;
            break;
        case 6:
            tmp = x;
            x = y;
            y = width - 1 - tmp;
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
        if ((addPixels || rmPixels) && underMouse()) {
            painter.drawPixmap(mouseX - radius, mouseY - radius, brush);
        }
    }
}


QRgb PreviewWidget::getColor(int layer, int v) {
    int v70 = v*7/10;
    switch (layer % 7) {
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
    int v = (int)stack.value(col, row) * expMult;
    if (v < 0) v = 0;
    else if (v > 65535) v = 65535;
    return getColor(stack.getImageAt(col, row), gamma[v]);
}


void PreviewWidget::render(QRect zone) {
    if (!stack.size()) return;
    zone = zone.intersect(QRect(0, 0, width, height));
    if (zone.isNull()) return;
    cancelRender = false;
    QImage image(zone.width(), zone.height(), QImage::Format_RGB32);
    #pragma omp parallel for schedule(dynamic)
    for (int row = zone.top(); row <= zone.bottom(); row++) {
        QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - zone.top()));
        for (int col = zone.left(); !cancelRender && col <= zone.right(); col++) {
            *scanLine++ = rgb(col, row);
        }
    }
    if (!cancelRender) {
        QMetaObject::invokeMethod(this, "paintImage", Qt::AutoConnection,
                                Q_ARG(QPoint, zone.topLeft()), Q_ARG(const QImage &, image));
    }
}


void PreviewWidget::paintImage(QPoint where, const QImage & image) {
    if (!pixmap.get()) {
        pixmap.reset(new QPixmap);
        *pixmap = QPixmap::fromImage(image);
        resize(pixmap->size());
    } else {
        QPainter painter(pixmap.get());
        painter.drawImage(where, image);
    }
    update(where.x(), where.y(), image.width(), image.height());
}


void PreviewWidget::createBrush(bool plus) {
    brush = QPixmap(radius*2 + 1, radius*2 + 1);
    brush.fill(Qt::white);
    {
        QPainter painter(&brush);
        painter.setPen(QPen(QColor(64, 64, 64), 2, Qt::SolidLine));
        painter.drawEllipse(0, 0, radius*2, radius*2);
        QPen dashed(QColor(128, 128, 128), 2, Qt::CustomDashLine);
        dashed.setDashPattern(QVector<qreal>({3, 4}));
        painter.setPen(dashed);
        painter.drawEllipse(0, 0, radius*2, radius*2);
        painter.drawLine(radius - 2, radius, radius + 2, radius);
        if (plus)
            painter.drawLine(radius, radius - 2, radius, radius + 2);
    }
    brush.setMask(brush.createMaskFromColor(Qt::white));
}


void PreviewWidget::setShowBrush() {
    if (addPixels || rmPixels) {
        createBrush(addPixels);
        setCursor(QCursor(Qt::BlankCursor));
    }
    update();
}


void PreviewWidget::mouseEvent(QMouseEvent * event, bool pressed) {
    int rx = mouseX = event->x(), ry = mouseY = event->y();
    rotate(rx, ry);
    if (rx >= 0 && rx < (int)stack.getWidth() && ry >= 0 && ry < (int)stack.getHeight())
        emit pixelUnderMouse(rx, ry);
    if (event->buttons() & Qt::LeftButton && (addPixels || rmPixels)) {
        event->accept();
        if (pressed) {
            stack.getMask().startAction(addPixels, layer);
        }
        stack.getMask().editPixels(rx, ry, radius);
        render(QRect(mouseX - radius, mouseY - radius, 2*radius + 1, 2*radius + 1));
    } else {
        event->ignore();
    }
    update();
}


void PreviewWidget::wheelEvent(QWheelEvent * event) {
    Qt::KeyboardModifiers mods = QApplication::queryKeyboardModifiers();
    if (mods & Qt::AltModifier) {
        event->accept();
        int step = event->delta() / 24;
        if (step == 0) step = event->delta() > 0 ? 1 : -1;
        setRadius(radius - step);
        emit radiusChanged(radius);
    } else {
        event->ignore();
    }
}


void PreviewWidget::undo() {
    if (stack.getMask().canUndo()) {
        QRect undoRect = stack.getMask().undo();
        render(QRect(unrotate(undoRect.topLeft()), unrotate(undoRect.bottomRight())));
    }
}


void PreviewWidget::redo() {
    if (stack.getMask().canRedo()) {
        QRect redoRect = stack.getMask().redo();
        render(QRect(unrotate(redoRect.topLeft()), unrotate(redoRect.bottomRight())));
    }
}
