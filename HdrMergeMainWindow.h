#ifndef _HDRMERGEMAINWINDOW_H
#define _HDRMERGEMAINWINDOW_H

#include <list>
#include <QAction>
#include <QActionGroup>
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
public:
    MainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);

    void changeEvent(QEvent * e);

    /// Triggered when the window is first shown
    void showEvent(QShowEvent * event);
    /// Triggered when the window is closed, exit the application
    void closeEvent(QCloseEvent * event);
    /// Preloads a list of images
    void preload(const std::list<char *> & fileNames);
    
private slots:
    void about();
    void loadImages();
    void loadImages(const QStringList & files);
    void saveResult();
    void setTool(QAction * action);
    void painted(int x, int y);
    
private:
    void createGui();
    void createActions();
    void createMenus();

    Q_OBJECT

    QMutex mutex;

    QAction * loadImagesAction;
    QAction * quitAction;
    QAction * aboutAction;
    QAction * mergeAction;
    QAction * dragToolAction;
    QAction * addGhostAction;
    QAction * rmGhostAction;
    QActionGroup * toolActionGroup;

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
};

#endif // UI_HDRMERGEMAINWINDOW_H
