#ifndef _DRAGGABLESCROLLAREA_H_
#define _DRAGGABLESCROLLAREA_H_

#include <QScrollArea>


class DraggableScrollArea : public QScrollArea {
public:
    DraggableScrollArea(QWidget * parent) : QScrollArea(parent), moveViewport(true) {}
    
    void toggleMoveViewport(bool toggle) { moveViewport = toggle; }
    
signals:
    void drag(int x, int y);

public slots:
    void show(int x, int y);

protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
        
private:
    Q_OBJECT
    
    QPoint lastDragPos;
    QPoint lastScrollPos;
    bool moveViewport;
};


#endif // _DRAGGABLESCROLLAREA_H_
