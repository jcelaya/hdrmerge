#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include "WhiteBalanceWidget.h"


WhiteBalanceWidget::WhiteBalanceWidget(double rg, double rb, QWidget * parent) : QWidget(parent) {
	QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Minimum);
	sp.setHorizontalStretch(0);
	sp.setVerticalStretch(0);
	sp.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
	this->setSizePolicy(sp);

	QVBoxLayout * vLayout = new QVBoxLayout(this);
	QWidget * buttonsContainer = new QWidget(this);
	QWidget * valuesContainer = new QWidget(this);
	vLayout->addWidget(buttonsContainer);
	vLayout->addWidget(valuesContainer);
	QHBoxLayout * buttonsLayout = new QHBoxLayout(buttonsContainer);
	QHBoxLayout * valuesLayout = new QHBoxLayout(valuesContainer);

	pickerButton = new QPushButton(tr("Select WB"), buttonsContainer);
	buttonsLayout->addWidget(pickerButton);

	autoWB = new QPushButton(tr("Auto WB"), buttonsContainer);
	buttonsLayout->addWidget(autoWB);

	buttonsLayout->addStretch(1);

	redToGreenLabel = new QLabel(tr("Red/Green:"), valuesContainer);
	valuesLayout->addWidget(redToGreenLabel);

	redToGreenValue = new QDoubleSpinBox(valuesContainer);
	redToGreenValue->setDecimals(3);
	redToGreenValue->setMaximum(10.0);
	redToGreenValue->setSingleStep(0.001);
	redToGreenValue->setValue(rg);
	valuesLayout->addWidget(redToGreenValue);

	redToBlueLabel = new QLabel(tr("Red/Blue:"), valuesContainer);
	valuesLayout->addWidget(redToBlueLabel);

	redToBlueValue = new QDoubleSpinBox(valuesContainer);
	redToBlueValue->setDecimals(3);
	redToBlueValue->setMaximum(10.0);
	redToBlueValue->setSingleStep(0.001);
	redToBlueValue->setValue(rb);
	valuesLayout->addWidget(redToBlueValue);

	valuesLayout->addStretch(1);

	connect(redToGreenValue, SIGNAL(valueChanged(double)), this, SLOT(changeValues()));
	connect(redToBlueValue, SIGNAL(valueChanged(double)), this, SLOT(changeValues()));
	connect(pickerButton, SIGNAL(clicked()), this, SLOT(pushPicker()));
	connect(autoWB, SIGNAL(clicked()), this, SLOT(pushAutoWB()));
}


void WhiteBalanceWidget::changeFactors(double rg, double rb) {
	redToGreenValue->setValue(rg);
	redToBlueValue->setValue(rb);
}

