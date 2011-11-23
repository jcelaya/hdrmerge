#ifndef _HDRMERGEMAINWINDOW_H
#define _HDRMERGEMAINWINDOW_H

#include <QtGui/QAction>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QScrollArea>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QWidget>
#include <QEvent>
#include <QMutex>
#include "Exposure.h"
#include "RenderThread.h"
#include "PreviewWidget.h"


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
	PreviewWidget * preview;
	QTabWidget * imageTabs;
	QStatusBar * statusbar;

	ExposureStack * images;
	RenderThread * rt;

	void createActions();
	void createMenus();

private slots:
	void about();
	void loadImages();
	void saveResult();
	void updateImage(const QImage & image);
	void exposureChange(int i, float re, int th);

public:
	MainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);

	void changeEvent(QEvent * e);
};

#endif // UI_HDRMERGEMAINWINDOW_H
