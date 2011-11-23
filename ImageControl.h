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
	ImageControl(QWidget * parent, int i, float re, int th);
	void disableREV() { relativeEVSpinBox->setEnabled(false); }

signals:
	void propertiesChanged(int i, float re, int th);

private slots:
	void changedEvent();
};

#endif // _IMAGECONTROL_H
