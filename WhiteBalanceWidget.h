#ifndef _WHITEBALANCEWIDGET_H
#define _WHITEBALANCEWIDGET_H

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>


class WhiteBalanceWidget : public QWidget {
	Q_OBJECT

	QLabel * redLabel, * greenLabel, * blueLabel;
	QDoubleSpinBox * redValue, * greenValue, * blueValue;
	QPushButton * pickerButton, * autoWB;

public:
	WhiteBalanceWidget(double r, double g, double b, QWidget * parent = 0);

signals:
	void factorsChanged(double r, double g, double b);
	void pickerPushed();
	void autoWBPushed();

public slots:
	void changeFactors(double r, double g, double b);

private slots:
	void changeValues() {
		emit factorsChanged(redValue->value(), greenValue->value(), blueValue->value());
	}

	void pushPicker() { emit pickerPushed(); }

	void pushAutoWB() { emit autoWBPushed(); }
};

#endif // _WHITEBALANCEWIDGET_H
