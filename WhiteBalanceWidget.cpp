#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include "WhiteBalanceWidget.h"


WhiteBalanceWidget::WhiteBalanceWidget(double r, double g, double b, QWidget * parent) : QWidget(parent) {
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

	redLabel = new QLabel(tr("Red:"), valuesContainer);
	valuesLayout->addWidget(redLabel);

	redValue = new QDoubleSpinBox(valuesContainer);
	redValue->setDecimals(3);
	redValue->setMaximum(1.0);
	redValue->setSingleStep(0.001);
	redValue->setValue(r);
	valuesLayout->addWidget(redValue);

	greenLabel = new QLabel(tr("Green:"), valuesContainer);
	valuesLayout->addWidget(greenLabel);

	greenValue = new QDoubleSpinBox(valuesContainer);
	greenValue->setDecimals(3);
	greenValue->setMaximum(1.0);
	greenValue->setSingleStep(0.001);
	greenValue->setValue(g);
	valuesLayout->addWidget(greenValue);

	blueLabel = new QLabel(tr("Blue:"), valuesContainer);
	valuesLayout->addWidget(blueLabel);

	blueValue = new QDoubleSpinBox(valuesContainer);
	blueValue->setDecimals(3);
	blueValue->setMaximum(1.0);
	blueValue->setSingleStep(0.001);
	blueValue->setValue(b);
	valuesLayout->addWidget(blueValue);

	valuesLayout->addStretch(1);

	connect(redValue, SIGNAL(valueChanged(double)), this, SLOT(changeValues()));
	connect(greenValue, SIGNAL(valueChanged(double)), this, SLOT(changeValues()));
	connect(blueValue, SIGNAL(valueChanged(double)), this, SLOT(changeValues()));
	connect(pickerButton, SIGNAL(clicked()), this, SLOT(pushPicker()));
	connect(autoWB, SIGNAL(clicked()), this, SLOT(pushAutoWB()));
}


void WhiteBalanceWidget::changeFactors(double r, double g, double b) {
	redValue->setValue(r);
	greenValue->setValue(g);
	blueValue->setValue(b);
}

