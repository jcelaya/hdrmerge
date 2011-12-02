#ifndef _IMAGECONTROL_H
#define _IMAGECONTROL_H

#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

class ImageControl : public QWidget {
	Q_OBJECT

	QGridLayout *gridLayout;
	QLabel *relativeEVLabel;
	QDoubleSpinBox *relativeEVSpinBox;
	QLabel *thresholdLabel;
	QSpinBox *thresholdSpinBox;
	QSlider *thresholdSlider;
	int imageNumber;

public:
	ImageControl(QWidget * parent, int i, double re, int th);

signals:
	void relativeEVChanged(int i, double re);
	void thresholdChanged(int i, int th);

private slots:
	void thresholdEvent(int th) {
		emit thresholdChanged(imageNumber, th);
	}

	void relativeEVEvent(double re) {
		emit relativeEVChanged(imageNumber, re);
	}
};

#endif // _IMAGECONTROL_H
