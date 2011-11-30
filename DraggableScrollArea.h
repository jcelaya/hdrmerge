#ifndef _DRAGGABLESCROLLAREA_H_
#define _DRAGGABLESCROLLAREA_H_

#include <QScrollArea>


class DraggableScrollArea : public QScrollArea {
        Q_OBJECT
        
	QPoint lastDragPos;
	QPoint lastScrollPos;

public:
	DraggableScrollArea(QWidget * parent);

public slots:
	void center(int x, int y);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
};


#endif // _DRAGGABLESCROLLAREA_H_
