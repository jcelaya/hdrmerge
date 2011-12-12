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
	WhiteBalanceWidget(double gr, double br, QWidget * parent = 0);

signals:
	void factorsChanged(double gr, double br);
	void pickerPushed();
	void autoWBPushed();

public slots:
	void changeFactors(double gr, double br);

private slots:
	void changeValues() {
		emit factorsChanged(redToGreenValue->value(), redToBlueValue->value());
	}

	void pushPicker() { emit pickerPushed(); }

	void pushAutoWB() { emit autoWBPushed(); }
};

#endif // _WHITEBALANCEWIDGET_H
