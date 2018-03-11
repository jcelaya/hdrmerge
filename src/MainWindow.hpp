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
#include <QString>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QEvent>
#include <QToolBar>
#include <QSpinBox>
#include <QSlider>
#include <QStatusBar>
#include "ImageIO.hpp"


namespace hdrmerge {

class PreviewWidget;
class DraggableScrollArea;

class MainWindow : public QMainWindow {
public:
    MainWindow();

    void showEvent(QShowEvent * event);
    void closeEvent(QCloseEvent * event);
    void preload(const std::vector<QString> & o) {
        preloadFiles = o;
    }

public slots:
    void setStatus(const QString & status) {
        statusLabel->setText(status);
    }
    void showTemporaryStatus(const QString & status) {
        statusBar->showMessage(status, 3000);
    }
    void setPixelStatus(int x, int y);

protected:
    void keyPressEvent(QKeyEvent * event) { setToolFromKey(); }
    void keyReleaseEvent(QKeyEvent * event) { setToolFromKey(); }

private slots:
    void about();
    void loadImages();
    void saveResult();
    void layerSelected(QAction * action);
    void toolSelected(QAction * action) {
        lastTool = action;
    }

private:
    void createWidgets();
    void createActions();
    void createMenus();
    void createToolbars();
    void createLayerSelector();
    void setToolFromKey();

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
    QAction * lastTool;

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
    QStatusBar * statusBar;
    QLabel * statusLabel;

    ImageIO io;
    std::vector<QString> preloadFiles;
};

} // namespace hdrmerge

#endif // UI_HDRMERGEMAINWINDOW_H
