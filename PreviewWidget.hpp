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
#include "ImageStack.hpp"

namespace hdrmerge {

class PreviewWidget : public QWidget {
public:
    PreviewWidget(QWidget * parent);
    QSize sizeHint() const;

    static QRgb getColor(int layer, int v);

public slots:
    void setImageStack(ImageStack * s);
    void toggleAddPixelsTool(bool toggled) { addPixels = toggled; setShowBrush(); }
    void toggleRmPixelsTool(bool toggled) { rmPixels = toggled; setShowBrush(); }
    void selectLayer(int i) { layer = i; }
    void setRadius(int r) { radius = r; }
    void undo();
    void redo();

protected:
    void paintEvent(QPaintEvent * event);
    void mousePressEvent(QMouseEvent * event) { mouseEvent(event, true); }
    void mouseMoveEvent(QMouseEvent * event) { mouseEvent(event, false); }
    void mouseEvent(QMouseEvent * event, bool pressed);
    void enterEvent(QEvent * event) { update(); }
    void leaveEvent(QEvent * event) { update(); }

private slots:
    void paintImage(int x, int y, const QImage & image);

private:
    Q_OBJECT

    std::unique_ptr<QPixmap> pixmap;
    std::unique_ptr<ImageStack> stack;
    size_t width, height;
    int flip;
    bool addPixels, rmPixels;
    int layer;
    int radius;
    int mouseX, mouseY;
    QPixmap brush;

    void render(int minx, int miny, int maxx, int maxy);
    QRgb rgb(int col, int row) const;
    void rotate(int & x, int & y) const;
    void createBrush(bool plus);
    void setShowBrush();
};

} // namespace hdrmerge

#endif // _PREVIEWWIDGET_H_
