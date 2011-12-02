#ifndef _HDRMERGEMAINWINDOW_H
#define _HDRMERGEMAINWINDOW_H

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QTabWidget>
#include <QEvent>
#include <QMutex>
#include "Exposure.h"
#include "RenderThread.h"
#include "PreviewWidget.h"
#include "WhiteBalanceWidget.h"
#include "DraggableScrollArea.h"


class MainWindow : public QMainWindow {
	Q_OBJECT

	QMutex mutex;

	QAction * loadImagesAction;
	QAction * quitAction;
	QAction * aboutAction;
	QAction * mergeAction;

	QMenu * fileMenu;
	QMenu * helpMenu;

	QWidget * centralwidget;
	DraggableScrollArea * previewArea;
	PreviewWidget * preview;
	QTabWidget * imageTabs;
	QStatusBar * statusbar;

	ExposureStack * images;
	RenderThread * rt;
	WhiteBalanceWidget * wbw;

	bool isPickingWB;

	void createActions();
	void createMenus();

private slots:
	void about();
	void loadImages();
	void saveResult();
	void setExposureParams(int i, float re, int th);
	void setPickingWB();
	void setAutoWB();
	void clickImage(QPoint pos, bool left);

public:
	MainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);

	void changeEvent(QEvent * e);
};

#endif // UI_HDRMERGEMAINWINDOW_H
