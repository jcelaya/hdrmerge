#ifndef _WHITEBALANCEWIDGET_H
#define _WHITEBALANCEWIDGET_H

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>


class WhiteBalanceWidget : public QWidget {
	Q_OBJECT

	QLabel * redToGreenLabel, * redToBlueLabel;
	QDoubleSpinBox * redToGreenValue, * redToBlueValue;
	QPushButton * pickerButton, * autoWB;

public:
	WhiteBalanceWidget(double rg, double rb, QWidget * parent = 0);

signals:
	void factorsChanged(double rg, double rb);
	void pickerPushed();
	void autoWBPushed();

public slots:
	void changeFactors(double rg, double rb);

private slots:
	void changeValues() {
		emit factorsChanged(redToGreenValue->value(), redToBlueValue->value());
	}

	void pushPicker() { emit pickerPushed(); }

	void pushAutoWB() { emit autoWBPushed(); }
};

#endif // _WHITEBALANCEWIDGET_H
