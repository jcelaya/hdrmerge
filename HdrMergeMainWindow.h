#ifndef _HDRMERGEMAINWINDOW_H
#define _HDRMERGEMAINWINDOW_H

#include <list>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QTabWidget>
#include <QEvent>
#include <QMutex>
#include <QStringList>
#include "Exposure.h"
#include "RenderThread.h"
#include "PreviewWidget.h"
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

	QStringList preLoadFiles;

	void createActions();
	void createMenus();

private slots:
	void about();
	void loadImages();
	void loadImages(const QStringList & files);
	void saveResult();

public:
	MainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);

	void changeEvent(QEvent * e);
	void showEvent(QShowEvent * event);
	void closeEvent(QCloseEvent * event);
	void preload(const std::list<char *> & fileNames);
};

#endif // UI_HDRMERGEMAINWINDOW_H
