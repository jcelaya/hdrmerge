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
#include <string>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QEvent>
#include <QMutex>
#include <QToolBar>


namespace hdrmerge {

class ImageStack;
class PreviewWidget;
class DraggableScrollArea;
class LoadOptions;

class MainWindow : public QMainWindow {
public:
    MainWindow();

    /// Triggered when the window is first shown
    void showEvent(QShowEvent * event);
    /// Triggered when the window is closed, exit the application
    void closeEvent(QCloseEvent * event);
    /// Preloads a list of images
    void preload(const LoadOptions * o) {
        preloadOptions = o;
    }

protected:
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

private slots:
    void about();
    void loadImages();
    void loadImages(const LoadOptions & options);
    void saveResult();
    void layerSelected(QAction * action);

private:
    void createGui();
    void createActions();
    void createMenus();
    void createLayerSelector();

    Q_OBJECT

    QMutex mutex;

    QAction * loadImagesAction;
    QAction * quitAction;
    QAction * undoAction;
    QAction * redoAction;
    QAction * aboutAction;
    QAction * mergeAction;
    QAction * dragToolAction;
    QAction * addGhostAction;
    QAction * rmGhostAction;

    QMenu * fileMenu;
    QMenu * editMenu;
    QMenu * helpMenu;

    DraggableScrollArea * previewArea;
    PreviewWidget * preview;
    QToolBar * layerSelector;
    QActionGroup * layerSelectorGroup;
    QWidget * lastLayer;

    ImageStack * images;
    const LoadOptions * preloadOptions;
    bool shiftPressed, controlPressed;
};

} // namespace hdrmerge

#endif // UI_HDRMERGEMAINWINDOW_H
