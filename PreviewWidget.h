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

#include <QWidget>


class PreviewWidget : public QWidget {
public:
    PreviewWidget(QWidget * parent);
    QSize sizeHint() const;

signals:
    void focus(int x, int y);
    void imageViewport(int x, int y, int w, int h, int scale);

public slots:
    void resetScale() { currentScale = newScale = 0; }
    void paintImage(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const QImage & image);
    void zoom(int steps);

protected:
    void wheelEvent(QWheelEvent * event);
    void moveEvent(QMoveEvent * event);
    void paintEvent(QPaintEvent * event);
        
private:
    Q_OBJECT
    
    int currentScale, newScale;
    QPixmap * pixmap;
};


#endif // _PREVIEWWIDGET_H_
