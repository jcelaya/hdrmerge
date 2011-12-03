#ifndef _RENDERTHREAD_H_
#define _RENDERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include "Exposure.h"


class RenderThread : public QThread {
	Q_OBJECT

	QMutex mutex;
	QWaitCondition condition;
	bool restart;
	bool abort;
	ExposureStack * images;
	QPoint vpmin, vpmax;
	int gamma[65536];

	void doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image);

protected:
	void run();

public:
	RenderThread(ExposureStack * es, float gamma = 1.0f, QObject *parent = 0);
	~RenderThread();

public slots:
	//void render(QPoint viewportMin, QPoint viewportMax);
	void render();
	void setExposureThreshold(int i, int th);
	void setExposureRelativeEV(int i, double re);
	void setGamma(float g);
	void calculateWB(int x, int y, int w, int h);

signals:
	//void renderedImage(unsigned int x, unsigned int y, const QImage & image);
	void renderedImage(const QImage & image);
	void whiteBalanceChanged(double wbr, double wbg, double wbb);
};

#endif // _RENDERTHREAD_H_
