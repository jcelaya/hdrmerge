#ifndef _PREVIEWWIDGET_H_
#define _PREVIEWWIDGET_H_

#include <QLabel>


class PreviewWidget : public QLabel {
        Q_OBJECT
        
	double scale;

public:
	PreviewWidget(QWidget * parent);

	void toggleCrossCursor(bool toggle);

signals:
	void imageClicked(QPoint pos, bool left);
	void focus(int x, int y);
	void imageViewport(int x, int y, int w, int h);

public slots:
	void paintImage(unsigned int x, unsigned int y, const QImage & image);

protected:
	void wheelEvent(QWheelEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void moveEvent(QMoveEvent * event);
};


#endif // _PREVIEWWIDGET_H_
