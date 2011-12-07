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
	unsigned int minx, miny, maxx, maxy;
	int gamma[65536];
	int scale;

	void doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image);

protected:
	void run();

public:
	RenderThread(ExposureStack * es, float gamma = 1.0f, QObject *parent = 0);
	~RenderThread();

public slots:
	void setExposureThreshold(int i, int th);
	void setExposureRelativeEV(int i, double re);
	void setGamma(float g);
	void calculateWB(int x, int y, int radius);
	void setImageViewport(int x, int y, int w, int h);
	void stepScale(int steps);

signals:
	void renderedImage(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const QImage & image);
	void whiteBalanceChanged(double rg, double rb);
};

#endif // _RENDERTHREAD_H_
