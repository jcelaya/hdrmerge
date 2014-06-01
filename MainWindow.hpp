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
#include <QToolBar>
#include <QSpinBox>
#include <QSlider>
#include "ImageIO.hpp"


namespace hdrmerge {

class PreviewWidget;
class DraggableScrollArea;

class MainWindow : public QMainWindow {
public:
    MainWindow();

    void showEvent(QShowEvent * event);
    void closeEvent(QCloseEvent * event);
    void preload(const std::vector<std::string> & o) {
        preloadFiles = o;
    }

protected:
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

private slots:
    void about();
    void loadImages();
    void saveResult();
    void layerSelected(QAction * action);

private:
    void createWidgets();
    void createActions();
    void createMenus();
    void createToolbars();
    void createLayerSelector();

    Q_OBJECT

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
    QSpinBox * radiusBox;
    QSlider * radiusSlider;
    QActionGroup * layerSelectorGroup;
    QToolBar * layerSelector;
    QSlider * exposureSlider;

    ImageIO io;
    std::vector<std::string> preloadFiles;
    bool shiftPressed, controlPressed;
};

} // namespace hdrmerge

#endif // UI_HDRMERGEMAINWINDOW_H
