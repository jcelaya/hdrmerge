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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QProgressDialog>
#include <QSettings>
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QBitmap>
#include <QKeyEvent>
#include <QSpinBox>
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
    ProgressDialog(QWidget * parent = 0) : QProgressDialog(parent), currentPercent(0) {
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
        QMetaObject::invokeMethod(this, "setValue", Qt::QueuedConnection, Q_ARG(int, (currentPercent = percent)));
        QMetaObject::invokeMethod(this, "setLabelText", Qt::QueuedConnection, Q_ARG(QString, translatedMessage));
    }

    virtual int getPercent() const {
        return currentPercent;
    }

private:

    int currentPercent;
};


MainWindow::MainWindow()
    : QMainWindow(), images(NULL), shiftPressed(false), controlPressed(false) {
    createGui();
    createActions();
    createMenus();

    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}


void MainWindow::createGui() {
    QIcon moveIcon = QIcon(":/images/transform-move.png");
    QIcon brushIcon = QIcon(":/images/draw-brush.png");
    QIcon eraserIcon = QIcon(":/images/draw-eraser.png");

    QWidget * centralwidget = new QWidget(this);
    setCentralWidget(centralwidget);
    QVBoxLayout * layout = new QVBoxLayout(centralwidget);
    layout->setContentsMargins(0, 0, 0, 0);

    previewArea = new DraggableScrollArea(centralwidget);
    preview = new PreviewWidget(previewArea);
    previewArea->setWidget(preview);

    QWidget * toolArea = new QWidget(centralwidget);
    QHBoxLayout * toolLayout = new QHBoxLayout(toolArea);
    toolLayout->setContentsMargins(5, 5, 0, 0);

    QToolBar * toolBar = new QToolBar(toolArea);
    toolBar->setStyleSheet("QToolBar { border: 0px }");
    toolBar->setOrientation(Qt::Horizontal);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    // Add tools
    // TODO: load default icons from KDE
    QActionGroup * toolActionGroup = new QActionGroup(toolBar);
    dragToolAction = new QAction(moveIcon, tr("Pan"), toolActionGroup);
    connect(dragToolAction, SIGNAL(toggled(bool)), previewArea, SLOT(toggleMoveViewport(bool)));
    addGhostAction = new QAction(brushIcon, tr("Add pixels to the current image"), toolActionGroup);
    addGhostAction->setDisabled(true);
    connect(addGhostAction, SIGNAL(toggled(bool)), preview, SLOT(toggleAddPixelsTool(bool)));
    rmGhostAction = new QAction(eraserIcon, tr("Remove pixels from the current image"), toolActionGroup);
    rmGhostAction->setDisabled(true);
    connect(rmGhostAction, SIGNAL(toggled(bool)), preview, SLOT(toggleRmPixelsTool(bool)));
    for (auto action : toolActionGroup->actions()) {
        action->setCheckable(true);
        toolBar->addAction(action);
    }
    dragToolAction->setChecked(true);
    toolBar->addSeparator();
    toolLayout->addWidget(toolBar);

    QSpinBox * radiusBox = new QSpinBox(toolBar);
    radiusBox->setRange(0, 200);
    radiusBox->findChild<QLineEdit*>()->setReadOnly(true);
    QSlider * radiusSlider = new QSlider(Qt::Horizontal, toolBar);
    radiusSlider->setRange(0, 200);
    radiusSlider->setMaximumWidth(200);
    connect(radiusBox, SIGNAL(valueChanged(int)), preview, SLOT(setRadius(int)));
    connect(radiusSlider, SIGNAL(valueChanged(int)), preview, SLOT(setRadius(int)));
    connect(radiusBox, SIGNAL(valueChanged(int)), radiusSlider, SLOT(setValue(int)));
    connect(radiusSlider, SIGNAL(valueChanged(int)), radiusBox, SLOT(setValue(int)));
    radiusBox->setValue(5);
    toolLayout->addWidget(new QLabel(" " + tr("Radius:"), toolBar));
    toolLayout->addWidget(radiusBox);
    toolLayout->addWidget(radiusSlider, 1);

    layerSelector = new QToolBar(toolArea);
    layerSelector->setStyleSheet("QToolBar { border: 0px }");
    layerSelector->setOrientation(Qt::Horizontal);
    layerSelector->setFloatable(false);
    layerSelector->setMovable(false);
    layerSelector->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    layerSelectorGroup = new QActionGroup(layerSelector);
    connect(layerSelectorGroup, SIGNAL(triggered(QAction *)), this, SLOT(layerSelected(QAction *)));
    toolLayout->addWidget(layerSelector);

    lastLayer = new QWidget(toolArea);
    lastLayer->setLayout(new QHBoxLayout());
    lastLayer->layout()->setContentsMargins(0, 0, 0, 0);
    lastLayer->layout()->addWidget(new QLabel());
    lastLayer->layout()->addWidget(new QLabel());
    toolLayout->addWidget(lastLayer, 0, Qt::AlignVCenter);
    toolLayout->addStretch(1);

    layout->addWidget(toolArea);
    layout->addWidget(previewArea);

    setWindowTitle(tr("HDRMerge v%1.%2 - Raw image fusion").arg(HDRMERGE_VERSION_MAJOR).arg(HDRMERGE_VERSION_MINOR));
    setWindowIcon(QIcon(":/images/logo.png"));
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
    if (preloadOptions && !preloadOptions->fileNames.empty()) {
        loadImages(*preloadOptions);
        preloadOptions = nullptr;
    }
}


void MainWindow::loadImages() {
    LoadOptionsDialog lod;
    if (lod.exec()) {
        loadImages(lod);
    }
    shiftPressed = controlPressed = false;
    dragToolAction->trigger();
}


void MainWindow::loadImages(const LoadOptions & options) {
    if (!options.fileNames.empty()) {
        unsigned int numImages = options.fileNames.size();
        ImageStack * newImages = new ImageStack();
        ProgressDialog progress(this);
        progress.setWindowTitle(tr("Open raw images"));
        QFuture<int> error = QtConcurrent::run([&] () { return newImages->load(options, progress); });
        while (error.isRunning())
            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        int result = error.result();
        if (result < numImages * 2) {
            int i = result >> 1;
            QString message = result & 1 ?
            tr("File %1 has not the same format as the previous ones.").arg(options.fileNames[i].c_str()) :
            tr("Unable to open file %1.").arg(options.fileNames[i].c_str());
            QMessageBox::warning(this, tr("Error opening file"), message);
            delete newImages;
            return;
        }
        images = newImages;
        preview->setImageStack(images);

        // Create GUI
        mergeAction->setEnabled(true);
        addGhostAction->setEnabled(numImages > 1);
        rmGhostAction->setEnabled(numImages > 1);
        createLayerSelector();
    }
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
    unsigned int numImages = images->size();
    layerSelector->clear();
    for (auto action : layerSelectorGroup->actions()) {
        layerSelectorGroup->removeAction(action);
        delete action;
    }
    layerSelector->addSeparator();
    if (numImages > 1) {
        double logLeastExp = std::log2(images->getImage(numImages - 1).getRelativeExposure());
        for (unsigned int i = 1; i < numImages; i++) {
            QAction * action = new QAction(QIcon(getColorIcon(i)), std::to_string(i).c_str(), layerSelectorGroup);
            action->setCheckable(true);
            double logExp = logLeastExp - std::log2(images->getImage(i - 1).getRelativeExposure());
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
        QLabel * lastIcon = (QLabel *)lastLayer->layout()->itemAt(0)->widget();
        QLabel * lastText = (QLabel *)lastLayer->layout()->itemAt(1)->widget();
        lastIcon->setPixmap(getColorIcon(numImages));
        lastText->setText(std::to_string(numImages).c_str());
    } else {
        QLabel * lastIcon = (QLabel *)lastLayer->layout()->itemAt(0)->widget();
        QLabel * lastText = (QLabel *)lastLayer->layout()->itemAt(1)->widget();
        lastIcon->setText("");
        lastText->setText("");
    }
}


void MainWindow::saveResult() {
    if (images) {
        // Take the prefix and add the first and last suffix
        QString name = (images->buildOutputFileName() + ".dng").c_str();
        QString file = QFileDialog::getSaveFileName(this, tr("Save DNG file"), name,
            tr("Digital Negatives (*.dng)"), NULL, QFileDialog::DontUseNativeDialog);
        if (!file.isEmpty()) {
            std::string fileName = QDir::toNativeSeparators(file).toUtf8().constData();
            size_t extPos = fileName.find_last_of('.');
            if (extPos > fileName.length() || fileName.substr(extPos) != ".dng") {
                fileName += ".dng";
            }
            DngPropertiesDialog dpd(this);
            dpd.setMaskFileName((images->buildOutputFileName() + "_mask.png").c_str());
            if (dpd.exec()) {
                dpd.fileName = fileName;
                ProgressDialog pd(this);
                pd.setWindowTitle(tr("Save DNG file"));
                QFuture<void> result = QtConcurrent::run([&]() {
                    images->save(dpd, pd);
                });
                while (result.isRunning())
                    QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
    }
    shiftPressed = controlPressed = false;
    dragToolAction->trigger();
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


void MainWindow::keyPressEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_Shift)
        shiftPressed = true;
    else if (event->key() == Qt::Key_Control)
        controlPressed = true;
    else QMainWindow::keyPressEvent(event);
    if (shiftPressed && addGhostAction->isEnabled()) addGhostAction->trigger();
    else if (controlPressed && rmGhostAction->isEnabled()) rmGhostAction->trigger();
    else dragToolAction->trigger();
}


void MainWindow::keyReleaseEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_Shift)
        shiftPressed = false;
    else if (event->key() == Qt::Key_Control)
        controlPressed = false;
    else QMainWindow::keyReleaseEvent(event);
    if (shiftPressed && addGhostAction->isEnabled()) addGhostAction->trigger();
    else if (controlPressed && rmGhostAction->isEnabled()) rmGhostAction->trigger();
    else dragToolAction->trigger();
}
