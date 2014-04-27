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

public slots:
    void setGamma(float g);
    void setImageStack(ImageStack * s);
    void paintImage(int x, int y, const QImage & image);
    void paintPixels(int x, int y, bool add);
    void toggleAddPixelsTool(bool toggled);
    void toggleRmPixelsTool(bool toggled);
    void selectLayer(int i) { layer = i; }
    void setRadius(int r) { radius = r; }
    void undo();

protected:
    void paintEvent(QPaintEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

private:
    Q_OBJECT

    struct EditAction {
        int layer;
        std::list<QPoint> points;
    };

    std::unique_ptr<QPixmap> pixmap;
    std::unique_ptr<ImageStack> stack;
    int gamma[65536];
    bool addPixels, rmPixels;
    int layer;
    int radius;
    std::list<EditAction> editActions;

    void render(int minx, int miny, int maxx, int maxy);
    QRgb rgb(int col, int row) const;
    QPixmap createCursor(bool plus);
};

} // namespace hdrmerge

#endif // _PREVIEWWIDGET_H_
