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


PreviewWidget::PreviewWidget(QWidget * parent) : QWidget(parent), width(0), height(0), flip(0),
addPixels(false), rmPixels(false), layer(0), radius(5), expMult(1.0) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setMouseTracking(true);
}


void PreviewWidget::setImageStack(ImageStack * s) {
    layer = 0;
    expMult = 1.0;
    stack.reset(s);
    mask.reset(new EditableMask(stack->getMask()));
    flip = stack->getImage(0).getMetaData().flip;
    if (flip == 5 || flip == 6) {
        width = stack->getHeight();
        height = stack->getWidth();
    } else {
        width = stack->getWidth();
        height = stack->getHeight();
    }
    pixmap.reset();
    repaintAsync();
}


void PreviewWidget::repaintAsync() {
    cancelRender = true;
    currentRender.waitForFinished();
    currentRender = QtConcurrent::run(this, &PreviewWidget::render, 0, 0, width, height);
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
    double v = stack->value(col, row) * expMult;
    if (v < 0.0) v = 0.0;
    else if (v > 65535.0) v = 65535.0;
    return getColor(stack->getImageAt(col, row), stack->toneMap(v));
}


void PreviewWidget::render(int minx, int miny, int maxx, int maxy) {
    if (!stack.get()) return;
    QRect area = QRect(0, 0, width, height).intersected(QRect(minx, miny, maxx - minx, maxy - miny));
    if (area.isEmpty()) return;
    cancelRender = false;
    area.getCoords(&minx, &miny, &maxx, &maxy);
    QImage image(area.width() - 1, area.height() - 1, QImage::Format_RGB32);
    #pragma omp parallel for schedule(dynamic)
    for (int row = miny; row < maxy; row++) {
        QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - miny));
        for (int col = minx; !cancelRender && col < maxx; col++) {
            *scanLine++ = rgb(col, row);
        }
    }
    if (!cancelRender) {
        QMetaObject::invokeMethod(this, "paintImage", Qt::AutoConnection,
                                Q_ARG(int, minx), Q_ARG(int, miny), Q_ARG(const QImage &, image));
    }
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
    if (event->buttons() & Qt::LeftButton && (addPixels || rmPixels)) {
        event->accept();
        if (pressed) {
            mask->startAction(addPixels, layer);
        }
        rotate(rx, ry);
        mask->editPixels(*stack, rx, ry, radius);
        render(mouseX - radius, mouseY - radius, mouseX + radius + 1, mouseY + radius + 1);
    } else {
        event->ignore();
    }
    update();
}


void PreviewWidget::undo() {
    if (mask.get() && mask->canUndo()) {
        EditableMask::Area a = mask->undo();
        render(a.minx, a.miny, a.maxx + 1, a.maxy + 1);
    }
}


void PreviewWidget::redo() {
    if (mask.get() && mask->canRedo()) {
        EditableMask::Area a = mask->redo();
        render(a.minx, a.miny, a.maxx + 1, a.maxy + 1);
    }
}
