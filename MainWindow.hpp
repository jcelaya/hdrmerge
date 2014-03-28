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
#include "ImageStack.hpp"
#include "RenderThread.hpp"
#include "PreviewWidget.hpp"
#include "DraggableScrollArea.hpp"


namespace hdrmerge {

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

protected:
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

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

    ImageStack * images;
    RenderThread * rt;

    QStringList preLoadFiles;
    bool shiftPressed, controlPressed;
};

} // namespace hdrmerge

#endif // UI_HDRMERGEMAINWINDOW_H
