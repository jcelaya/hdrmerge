#ifndef _RENDERTHREAD_H_
#define _RENDERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include "Exposure.h"


class RenderThread : public QThread {
	Q_OBJECT

public:
	RenderThread(const ExposureStack * es, float gamma = 1.0f, QObject *parent = 0);
	~RenderThread();

	void render();
	void setGamma(float g);

signals:
	void renderedImage(const QImage & image);

protected:
	void run();

private:
	QMutex mutex;
	QWaitCondition condition;
	bool restart;
	bool abort;
	const ExposureStack * imgs;
	QImage image;
	int gamma[65536];
};

#endif // _RENDERTHREAD_H_
