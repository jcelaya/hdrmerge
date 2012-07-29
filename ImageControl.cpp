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

#include "ImageControl.h"
#include <iostream>


ImageControl::ImageControl(QWidget * parent, int i, double re, int th) : QWidget(parent), imageNumber(i) {
	QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Minimum);
	sp.setHorizontalStretch(0);
	sp.setVerticalStretch(0);
	sp.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
	this->setSizePolicy(sp);

	gridLayout = new QGridLayout(this);
	relativeEVLabel = new QLabel(tr("Relative EV"), this);
	gridLayout->addWidget(relativeEVLabel, 0, 0, 1, 1);

	relativeEVSpinBox = new QDoubleSpinBox(this);
	relativeEVSpinBox->setDecimals(3);
	relativeEVSpinBox->setMaximum(1.0);
	relativeEVSpinBox->setSingleStep(0.001);
	relativeEVSpinBox->setValue(re);
	gridLayout->addWidget(relativeEVSpinBox, 0, 1, 1, 1);

	thresholdLabel = new QLabel(tr("Threshold"), this);
	gridLayout->addWidget(thresholdLabel, 1, 0, 1, 1);

	thresholdSpinBox = new QSpinBox(this);
	thresholdSpinBox->setMaximum(255);
	thresholdSpinBox->setValue(th / 256);
	gridLayout->addWidget(thresholdSpinBox, 1, 1, 1, 1);

	thresholdSlider = new QSlider(this);
	thresholdSlider->setMaximum(256);
	thresholdSlider->setValue(th / 256);
	thresholdSlider->setOrientation(Qt::Horizontal);
	gridLayout->addWidget(thresholdSlider, 1, 2, 1, 1);

	//retranslateUi(imageControl);
	connect(thresholdSlider, SIGNAL(valueChanged(int)), thresholdSpinBox, SLOT(setValue(int)));
	connect(thresholdSpinBox, SIGNAL(valueChanged(int)), thresholdSlider, SLOT(setValue(int)));
	connect(thresholdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(thresholdEvent(int)));
	connect(relativeEVSpinBox, SIGNAL(valueChanged(double)), this, SLOT(relativeEVEvent(double)));
}


/*
void retranslateUi(QWidget *imageControl)
    {
        imageControl->setWindowTitle(QApplication::translate("imageControl", "Form", 0, QApplication::UnicodeUTF8));
        relativeEVLabel->setText(QApplication::translate("imageControl", "Exposici\303\263n relativa:", 0, QApplication::UnicodeUTF8));
        thresholdLabel->setText(QApplication::translate("imageControl", "Maxima exposici\303\263n (%):", 0, QApplication::UnicodeUTF8));
    } // retranslateUi
*/

