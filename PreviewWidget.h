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
