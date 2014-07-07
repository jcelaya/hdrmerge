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

#include <list>
#include <cmath>
#include "MainWindow.hpp"
#include <QApplication>
#include <QFuture>
#include <QtConcurrentRun>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QProgressDialog>
#include <QSettings>
#include "config.h"
#include "AboutDialog.hpp"
#include "DngFloatWriter.hpp"
#include "ImageStack.hpp"
#include "PreviewWidget.hpp"
#include "DraggableScrollArea.hpp"
#include "DngPropertiesDialog.hpp"
#include "LoadOptionsDialog.hpp"
using namespace std;
using namespace hdrmerge;


class ProgressDialog : public QProgressDialog , public ProgressIndicator {
public:
    ProgressDialog(QWidget * parent = 0) : QProgressDialog(parent) {
        setMaximum(100);
        setMinimum(0);
        setMinimumDuration(0);
        setCancelButtonText(QString());
    }

    virtual void advance(int percent, const char * message, const char * arg) {
        QString translatedMessage = QCoreApplication::translate("LoadSave", message);
        if (arg) {
            translatedMessage = translatedMessage.arg(arg);
        }
        QMetaObject::invokeMethod(this, "setValue", Qt::QueuedConnection, Q_ARG(int, percent));
        QMetaObject::invokeMethod(this, "setLabelText", Qt::QueuedConnection, Q_ARG(QString, translatedMessage));
    }
};


MainWindow::MainWindow() : QMainWindow() {
    createWidgets();
    createActions();
    createToolbars();
    createMenus();

    setWindowTitle(tr("HDRMerge %1 - Raw image fusion").arg(HDRMERGE_VERSION_STRING));
    setWindowIcon(QIcon(":/images/icon.png"));

    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}


void MainWindow::createWidgets() {
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusLabel = new QLabel(statusBar);
    statusBar->addWidget(statusLabel);

    previewArea = new DraggableScrollArea(this);
    setCentralWidget(previewArea);
    preview = new PreviewWidget(io.getImageStack(), previewArea);
    previewArea->setWidget(preview);
    connect(preview, SIGNAL(pixelUnderMouse(int, int)), this, SLOT(setPixelStatus(int, int)));

    radiusBox = new QSpinBox();
    radiusBox->setRange(0, 200);
    radiusBox->findChild<QLineEdit*>()->setReadOnly(true);
    radiusBox->setToolTip(tr("Brush radius of the add/remove tool."));
    radiusSlider = new QSlider(Qt::Horizontal);
    radiusSlider->setRange(0, PreviewWidget::maxRadius);
    radiusSlider->setMaximumWidth(200);
    radiusSlider->setToolTip(tr("Brush radius of the add/remove tool."));
    connect(radiusBox, SIGNAL(valueChanged(int)), preview, SLOT(setRadius(int)));
    connect(radiusSlider, SIGNAL(valueChanged(int)), preview, SLOT(setRadius(int)));
    connect(preview, SIGNAL(radiusChanged(int)), radiusSlider, SLOT(setValue(int)));
    connect(radiusBox, SIGNAL(valueChanged(int)), radiusSlider, SLOT(setValue(int)));
    connect(radiusSlider, SIGNAL(valueChanged(int)), radiusBox, SLOT(setValue(int)));
    radiusBox->setValue(5);

    exposureSlider = new QSlider(Qt::Horizontal);
    exposureSlider->setRange(0, 1000);
    exposureSlider->setMaximumWidth(200);
    exposureSlider->setToolTip(tr("Preview brightness. It does NOT affect the HDR result."));
    connect(exposureSlider, SIGNAL(valueChanged(int)), preview, SLOT(setExposureMultiplier(int)));
}


void MainWindow::createActions() {
    loadImagesAction = new QAction(tr("&Open raw images..."), this);
    loadImagesAction->setShortcut(tr("Ctrl+O"));
    connect(loadImagesAction, SIGNAL(triggered()), this, SLOT(loadImages()));

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    undoAction = new QAction(tr("Undo"), this);
    undoAction->setShortcut(QString("Ctrl+z"));
    connect(undoAction, SIGNAL(triggered()), preview, SLOT(undo()));

    redoAction = new QAction(tr("Redo"), this);
    redoAction->setShortcut(QString("Ctrl+Shift+z"));
    connect(redoAction, SIGNAL(triggered()), preview, SLOT(redo()));

    aboutAction = new QAction(tr("&About..."), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    mergeAction = new QAction(tr("&Save HDR..."), this);
    mergeAction->setShortcut(tr("Ctrl+S"));
    mergeAction->setEnabled(false);
    connect(mergeAction, SIGNAL(triggered()), this, SLOT(saveResult()));

    dragToolAction = new QAction(QIcon(":/images/transform-move.png"), tr("Pan"), nullptr);
    dragToolAction->setCheckable(true);
    connect(dragToolAction, SIGNAL(toggled(bool)), previewArea, SLOT(toggleMoveViewport(bool)));

    addGhostAction = new QAction(QIcon(":/images/draw-brush.png"), tr("Add pixels to the current image"), nullptr);
    addGhostAction->setCheckable(true);
    addGhostAction->setDisabled(true);
    connect(addGhostAction, SIGNAL(toggled(bool)), preview, SLOT(toggleAddPixelsTool(bool)));

    rmGhostAction = new QAction(QIcon(":/images/draw-eraser.png"), tr("Remove pixels from the current image"), nullptr);
    rmGhostAction->setCheckable(true);
    rmGhostAction->setDisabled(true);
    connect(rmGhostAction, SIGNAL(toggled(bool)), preview, SLOT(toggleRmPixelsTool(bool)));
}


void MainWindow::createMenus() {
    fileMenu = new QMenu(tr("&File"));
    fileMenu->addAction(loadImagesAction);
    fileMenu->addAction(mergeAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    editMenu = new QMenu(tr("&Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);

    helpMenu = new QMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(editMenu);
    menuBar()->addMenu(helpMenu);
}


void MainWindow::createToolbars() {
    QToolBar * toolBar = addToolBar("Tools");
    toolBar->setObjectName("Tools");
    toolBar->setOrientation(Qt::Horizontal);
    toolBar->setFloatable(false);
    toolBar->setMovable(true);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    QActionGroup * toolActionGroup = new QActionGroup(toolBar);
    toolBar->addAction(toolActionGroup->addAction(dragToolAction));
    toolBar->addAction(toolActionGroup->addAction(addGhostAction));
    toolBar->addAction(toolActionGroup->addAction(rmGhostAction));
    dragToolAction->setChecked(true);
    lastTool = dragToolAction;
    toolBar->addSeparator();
    toolBar->addWidget(new QLabel(" " + tr("Radius:"), toolBar));
    toolBar->addWidget(radiusBox);
    toolBar->addWidget(radiusSlider);
    toolBar->addSeparator();
    toolBar->addWidget(new QLabel(" " + tr("Brightness:"), toolBar));
    toolBar->addWidget(exposureSlider);
    connect(toolActionGroup, SIGNAL(triggered(QAction *)), this, SLOT(toolSelected(QAction *)));

    layerSelector = addToolBar("Layers");
    layerSelector->setObjectName("Layers");
    layerSelector->setFloatable(false);
    layerSelector->setMovable(true);
    toolBar->setAllowedAreas(Qt::AllToolBarAreas);
    layerSelector->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    layerSelectorGroup = new QActionGroup(layerSelector);
    connect(layerSelectorGroup, SIGNAL(triggered(QAction *)), this, SLOT(layerSelected(QAction *)));
}


void MainWindow::setPixelStatus(int x, int y) {
    int l = io.getImageStack().getImageAt(x, y);
    Image & img = io.getImageStack().getImage(l);
    setStatus(tr("Layer %1: displaced %2,%3,%4 cropped | src. value = %5 ; result = %6")
        .arg(l + 1)
        .arg(img.getDeltaX())
        .arg(img.getDeltaY())
        .arg(io.getImageStack().isCropped() ? "" : " not")
        .arg(io.getImageStack().getImage(l)(x, y))
        .arg(io.getImageStack().value(x, y)));
}


void MainWindow::closeEvent(QCloseEvent * event) {
    QSettings settings;
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}


void MainWindow::about() {
    AboutDialog dialog(this);
    dialog.exec();
}


void MainWindow::showEvent(QShowEvent * event) {
    loadImages();
}


void MainWindow::loadImages() {
    LoadOptionsDialog lod(this);
    if (!preloadFiles.empty()) {
        lod.fileNames = preloadFiles;
    }
    if (lod.exec() && !lod.fileNames.empty()) {
        unsigned int numImages = lod.fileNames.size();
        ProgressDialog progress(this);
        progress.setWindowTitle(tr("Open raw images"));
        QFuture<int> error = QtConcurrent::run([&] () { return io.load(lod, progress); });
        while (error.isRunning())
            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        int result = error.result();
        if (result < numImages * 2) {
            int i = result >> 1;
            QString message = result & 1 ?
            tr("File %1 has not the same format as the previous ones.").arg(lod.fileNames[i].c_str()) :
            tr("Unable to open file %1.").arg(lod.fileNames[i].c_str());
            QMessageBox::warning(this, tr("Error opening file"), message);
        }

        numImages = io.getImageStack().size();
        // Create GUI
        preview->reload();
        mergeAction->setEnabled(numImages > 0);
        addGhostAction->setEnabled(numImages > 1);
        rmGhostAction->setEnabled(numImages > 1);
        radiusSlider->setValue(5);
        exposureSlider->setValue(0);
        createLayerSelector();
    }
    setToolFromKey();
}


static QPixmap getColorIcon(int i) {
    QImage colorBlock(20, 20, QImage::Format_ARGB32);
    QColor color(PreviewWidget::getColor(i - 1, 255));
    colorBlock.fill(color);
    color.setAlpha(0);
    int x[] = { 0, 1, 18, 19, 0, 19, 0, 19, 0, 1, 18, 19 };
    int y[] = { 0, 0, 0, 0, 1, 1, 18, 18, 19, 19, 19, 19 };
    for (int i = 0; i < 12; ++i) {
        colorBlock.setPixel(x[i], y[i], color.rgba());
    }
    return QPixmap::fromImage(colorBlock);
}


void MainWindow::createLayerSelector() {
    ImageStack & images = io.getImageStack();
    unsigned int numImages = images.size();
    layerSelector->clear();
    for (auto action : layerSelectorGroup->actions()) {
        layerSelectorGroup->removeAction(action);
        delete action;
    }
    if (numImages > 1) {
        double logLeastExp = std::log2(images.getImage(numImages - 1).getRelativeExposure());
        for (unsigned int i = 1; i < numImages; i++) {
            QAction * action = new QAction(QIcon(getColorIcon(i)), std::to_string(i).c_str(), layerSelectorGroup);
            action->setCheckable(true);
            double logExp = logLeastExp - std::log2(images.getImage(i - 1).getRelativeExposure());
            action->setToolTip(QString("+%1 EV").arg(logExp, 0, 'f', 2));
            if (i < 10)
                action->setShortcut(Qt::Key_0 + i);
            else if (i == 10)
                action->setShortcut(Qt::Key_0);
            layerSelector->addAction(action);
        }
        QAction * firstAction = layerSelectorGroup->actions().first();
        firstAction->setChecked(true);
        preview->selectLayer(0);
    }
    if (numImages > 0) {
        QWidget * lastLayer = new QWidget();
        lastLayer->setContentsMargins(0, 1, 0, 0);
        lastLayer->setLayout(new QHBoxLayout());
        QLabel * lastIcon = new QLabel(lastLayer);
        lastIcon->setPixmap(getColorIcon(numImages));
        lastLayer->layout()->addWidget(lastIcon);
        lastLayer->layout()->addWidget(new QLabel(std::to_string(numImages).c_str()));
        //lastLayer->setMinimumHeight(layerSelector->widgetForAction(firstAction)->height());
        layerSelector->addWidget(lastLayer);
    }
}


void MainWindow::saveResult() {
    if (io.getImageStack().size() > 0) {
        QSettings settings;
        QVariant lastDirSetting = settings.value("lastSaveDirectory");
        // Take the prefix and add the first and last suffix
        QString name = io.buildOutputFileName().c_str();
        if (!lastDirSetting.isNull()) {
            size_t slashPos = name.lastIndexOf('/');
            if (slashPos > name.length()) slashPos = 0;
            name = QDir(lastDirSetting.toString()).absolutePath() + "/" + name.right(name.size() - slashPos);
        }
        QString file = QFileDialog::getSaveFileName(this, tr("Save DNG file"), name,
            tr("Digital Negatives (*.dng)"), NULL, QFileDialog::DontUseNativeDialog);
        if (!file.isEmpty()) {
            std::string fileName = QDir::toNativeSeparators(file).toUtf8().constData();
            size_t extPos = fileName.find_last_of('.');
            if (extPos > fileName.length() || fileName.substr(extPos) != ".dng") {
                fileName += ".dng";
            }
            DngPropertiesDialog dpd(this);
            if (dpd.exec()) {
                QString lastDir = QDir(file).absolutePath();
                lastDir.truncate(lastDir.lastIndexOf('/'));
                settings.setValue("lastSaveDirectory", lastDir);
                dpd.fileName = fileName;
                ProgressDialog pd(this);
                pd.setWindowTitle(tr("Save DNG file"));
                QFuture<void> result = QtConcurrent::run([&]() {
                    io.save(dpd, pd);
                });
                while (result.isRunning())
                    QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
    }
    setToolFromKey();
}


void MainWindow::layerSelected(QAction * action) {
    int i = 0;
    for (auto a : layerSelectorGroup->actions()) {
        if (action == a) {
            preview->selectLayer(i);
            return;
        }
        ++i;
    }
}


void MainWindow::setToolFromKey() {
    Qt::KeyboardModifiers mods = QApplication::queryKeyboardModifiers();
    if (mods & Qt::ShiftModifier && addGhostAction->isEnabled()) addGhostAction->setChecked(true);
    else if (mods & Qt::ControlModifier && rmGhostAction->isEnabled()) rmGhostAction->setChecked(true);
    else lastTool->setChecked(true);
}
