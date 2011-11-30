#ifndef _PREVIEWWIDGET_H_
#define _PREVIEWWIDGET_H_

#include <QLabel>


class PreviewWidget : public QLabel {
        Q_OBJECT
        
	double scale;

public:
	PreviewWidget(QWidget * parent);

	void getViewRect(QPoint & min, QPoint & max);
	void toggleCrossCursor(bool toggle);

signals:
	void imageClicked(QPoint pos, bool left);
	void focus(int x, int y);

public slots:
	void paintImage(const QImage & image);

protected:
	void wheelEvent(QWheelEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
};


#endif // _PREVIEWWIDGET_H_
