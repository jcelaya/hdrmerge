/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
