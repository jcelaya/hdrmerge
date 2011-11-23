#ifndef _PREVIEWWIDGET_H_
#define _PREVIEWWIDGET_H_

#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QPixmap>

class PreviewWidget : public QScrollArea {
        Q_OBJECT
        
	QLabel * previewLabel;
	//QPixmap image;
	double scale;
	QPoint lastDragPos;
	QPoint lastScrollPos;

public:
	PreviewWidget(QWidget * parent);

	void setPixmap(const QPixmap & pixmap);

protected:
	void wheelEvent(QWheelEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
};


#endif // _PREVIEWWIDGET_H_
