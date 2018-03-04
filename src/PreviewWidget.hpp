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

#ifndef _PREVIEWWIDGET_H_
#define _PREVIEWWIDGET_H_

#include <memory>
#include <list>
#include <QWidget>
#include <QPaintEvent>
#include <QFuture>
#include "ImageStack.hpp"

namespace hdrmerge {

class PreviewWidget : public QWidget {
public:
    static const int maxRadius = 200;

    PreviewWidget(ImageStack & s, QWidget * parent);
    QSize sizeHint() const;

    static QRgb getColor(int layer, int v);

public slots:
    void reload();
    void toggleAddPixelsTool(bool toggled) {
        addPixels = toggled;
        setShowBrush();
    }
    void toggleRmPixelsTool(bool toggled) {
        rmPixels = toggled;
        setShowBrush();
    }
    void selectLayer(int i) { layer = i; }
    void setRadius(int r) {
        radius = r;
        if (radius < 0) radius = 0;
        if (radius > maxRadius) radius = maxRadius;
        setShowBrush();
    }
    void setExposureMultiplier(int e) {
        if (stack.size() > 0) {
            expMult = 1.0 + e * stack.getMaxExposure() / (stack.size() * 1000.0);
            repaintAsync();
        }
    }
    void undo();
    void redo();

signals:
    void radiusChanged(int r);
    void pixelUnderMouse(int x, int y);

protected:
    void paintEvent(QPaintEvent * event);
    void mousePressEvent(QMouseEvent * event) { mouseEvent(event, true); }
    void mouseMoveEvent(QMouseEvent * event) { mouseEvent(event, false); }
    void mouseEvent(QMouseEvent * event, bool pressed);
    void wheelEvent(QWheelEvent * event);
    void enterEvent(QEvent * event) { update(); }
    void leaveEvent(QEvent * event) { update(); }

private slots:
    void paintImage(QPoint where, const QImage & image);

private:
    Q_OBJECT

    std::unique_ptr<QPixmap> pixmap;
    ImageStack & stack;
    size_t width, height;
    int flip;
    bool addPixels, rmPixels;
    int layer;
    int radius;
    int mouseX, mouseY;
    QPixmap brush;
    double expMult;
    QFuture<void> currentRender;
    bool cancelRender;
    uint8_t gamma[65536];

    void render(QRect zone);
    QRgb rgb(int col, int row) const;
    void rotate(int & x, int & y) const;
    QPoint unrotate(QPoint p) const {
        int x = p.x(), y = p.y();
        rotate(x, y); rotate(x, y); rotate(x, y);
        return QPoint(x, y);
    }
    void createBrush(bool plus);
    void setShowBrush();
    void repaintAsync();
};

} // namespace hdrmerge

#endif // _PREVIEWWIDGET_H_
