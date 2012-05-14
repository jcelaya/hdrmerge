#include <list>
#include "HdrMergeMainWindow.h"
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
#include <QToolBar>
#include <QCursor>
#include <QPainter>
#include <QPen>
#include <QBitmap>
#include "ImageControl.h"
#include "config.h"
#include <iostream>
using namespace std;


MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), images(NULL), rt(NULL) {
    createGui();
    createActions();
    createMenus();

    QSettings settings;
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}


void MainWindow::createGui() {
    centralwidget = new QWidget(this);
    setCentralWidget(centralwidget);
    QVBoxLayout * layout = new QVBoxLayout(centralwidget);

    previewArea = new DraggableScrollArea(centralwidget);
    previewArea->setAlignment(Qt::AlignCenter);
    layout->addWidget(previewArea);

    preview = new PreviewWidget(previewArea);
    previewArea->setWidget(preview);
    connect(preview, SIGNAL(focus(int, int)), previewArea, SLOT(show(int, int)));
    connect(previewArea, SIGNAL(drag(int, int)), this, SLOT(painted(int, int)));

    QWidget * toolArea = new QWidget(centralwidget);
    QHBoxLayout * toolLayout = new QHBoxLayout(toolArea);
    
    QToolBar * toolBar = new QToolBar(toolArea);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    connect(toolBar, SIGNAL(actionTriggered(QAction*)), this, SLOT(setTool(QAction*)));
    // Add tools
    dragToolAction = toolBar->addAction(QIcon("images/transform_move.png"), tr("Drag and zoom"));
    addGhostAction = toolBar->addAction(QIcon("images/paintbrush.png"), tr("Add pixels to the current exposure"));
    rmGhostAction = toolBar->addAction(QIcon("images/draw_eraser.png"), tr("Remove pixels from the current exposure"));
    dragToolAction->setCheckable(true);
    addGhostAction->setCheckable(true);
    rmGhostAction->setCheckable(true);
    toolActionGroup = new QActionGroup(this);
    toolActionGroup->addAction(dragToolAction);
    toolActionGroup->addAction(addGhostAction);
    toolActionGroup->addAction(rmGhostAction);
    dragToolAction->setChecked(true);
    preview->setCursor(Qt::OpenHandCursor);
    toolLayout->addWidget(toolBar);
    
    imageTabs = new QTabWidget(toolArea);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(imageTabs->sizePolicy().hasHeightForWidth());
    imageTabs->setSizePolicy(sizePolicy);
    toolLayout->addWidget(imageTabs);
    
    layout->addWidget(toolArea);

    //retranslateUi(HdrMergeMainWindow);
    //statusbar = new QStatusBar(this);
    //setStatusBar(statusbar);
    setWindowTitle(tr("HDRMerge v%1.%2 - High dynamic range image fussion").arg(HDRMERGE_VERSION_MAJOR).arg(HDRMERGE_VERSION_MINOR));
}


void MainWindow::createActions() {
    loadImagesAction = new QAction(tr("&Open exposures..."), this);
    loadImagesAction->setShortcut(tr("Ctrl+O"));
    connect(loadImagesAction, SIGNAL(triggered()), this, SLOT(loadImages()));

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    aboutAction = new QAction(tr("&About..."), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    mergeAction = new QAction(tr("&Save HDR..."), this);
    mergeAction->setShortcut(tr("Ctrl+S"));
    connect(mergeAction, SIGNAL(triggered()), this, SLOT(saveResult()));
}


void MainWindow::createMenus() {
    fileMenu = new QMenu(tr("&File"));
    fileMenu->addAction(loadImagesAction);
    fileMenu->addAction(mergeAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    helpMenu = new QMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(helpMenu);
}


void MainWindow::closeEvent(QCloseEvent * event) {
    QSettings settings;
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}



/*
    void retranslateUi(QMainWindow *HdrMergeMainWindow)
    {
        HdrMergeMainWindow->setWindowTitle(QApplication::translate("HdrMergeMainWindow", "HDRMerge - Fusi\303\263n de im\303\241genes para alto rango din\303\241mico", 0, QApplication::UnicodeUTF8));
        loadImagesAction->setText(QApplication::translate("HdrMergeMainWindow", "Cargar im\303\241genes", 0, QApplication::UnicodeUTF8));
        quitAction->setText(QApplication::translate("HdrMergeMainWindow", "Salir", 0, QApplication::UnicodeUTF8));
        quitAction->setShortcut(QApplication::translate("HdrMergeMainWindow", "Ctrl+Q", 0, QApplication::UnicodeUTF8));
        aboutAction->setText(QApplication::translate("HdrMergeMainWindow", "Acerca de...", 0, QApplication::UnicodeUTF8));
        mergeAction->setText(QApplication::translate("HdrMergeMainWindow", "Fusionar", 0, QApplication::UnicodeUTF8));
        previewLabel->setText(QString());
        fileMenu->setTitle(QApplication::translate("HdrMergeMainWindow", "Archivo", 0, QApplication::UnicodeUTF8));
        helpMenu->setTitle(QApplication::translate("HdrMergeMainWindow", "Ayuda", 0, QApplication::UnicodeUTF8));
    } // retranslateUi
*/



void MainWindow::changeEvent(QEvent * e) {
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        //retranslateUi(this);
        break;
    default:
        break;
    }
}


void MainWindow::about() {
    QMessageBox::about(this, tr("About HDRMerge"),
        tr("<p><b>HDR Merge tool</b></p>"));
}


void MainWindow::preload(const list<char *> & fileNames) {
    for (list<char *>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it)
        preLoadFiles << QString(*it);
}


void MainWindow::showEvent(QShowEvent * event) {
    if (!preLoadFiles.empty()) {
        loadImages(preLoadFiles);
        preLoadFiles.clear();
    }
}


void MainWindow::loadImages() {
    QSettings settings;
    QVariant lastDirSetting = settings.value("lastOpenDirectory");
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open exposures"),
        lastDirSetting.isNull() ? QDir::currentPath() : QDir(lastDirSetting.toString()).absolutePath(),
        tr("Linear TIFF images (*.tif *.tiff)"), NULL, QFileDialog::DontUseNativeDialog);
    if (!files.empty()) {
        // Save last dir
        QString lastDir = QDir(files.front()).absolutePath();
        lastDir.truncate(lastDir.lastIndexOf('/'));
        settings.setValue("lastOpenDirectory", lastDir);
        loadImages(files);
    }
}


void MainWindow::loadImages(const QStringList & files) {
    if (!files.empty()) {
        unsigned int numImages = files.size();
        // Clean previous state
        while (imageTabs->count() > 0) {
            delete imageTabs->widget(0);
            imageTabs->removeTab(0);
        }
        if (rt != NULL)
            delete rt;

        // Load and sort images
        images = new ExposureStack();
        QFuture<void> result;
        QProgressDialog progress(tr("Loading files..."), QString(), 0, numImages + 2, this);
        progress.setMinimumDuration(0);
        for (unsigned int i = 0; i < numImages; i++) {
            progress.setValue(i);
            QByteArray fileName = QDir::toNativeSeparators(files[i]).toUtf8();
            result = QtConcurrent::run(images, &ExposureStack::loadImage, fileName.constData());
            while (result.isRunning())
                QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        progress.setValue(numImages);
        progress.setLabelText(tr("Sorting..."));
        result = QtConcurrent::run(images, &ExposureStack::sort);
        while (result.isRunning())
            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        progress.setValue(numImages + 1);
        progress.setLabelText(tr("Prescaling..."));
        result = QtConcurrent::run(images, &ExposureStack::preScale);
        while (result.isRunning())
            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
        progress.setValue(numImages + 2);

        // Render
        preview->resetScale();
        rt = new RenderThread(images, 2.2f, this);
        connect(rt, SIGNAL(renderedImage(unsigned int, unsigned int, unsigned int, unsigned int, QImage)),
                preview, SLOT(paintImage(unsigned int, unsigned int, unsigned int, unsigned int, QImage)));
        connect(preview, SIGNAL(imageViewport(int, int, int, int, int)),
                rt, SLOT(setImageViewport(int, int, int, int, int)));
        rt->start(QThread::LowPriority);

        // Create GUI
        for (unsigned int i = 0; i < numImages - 1; i++) {
            // Create ImageControl widgets for every exposure except the last one
            ImageControl * ic =
                new ImageControl(imageTabs, i, images->getRelativeExposure(i), images->getThreshold(i));
            connect(ic, SIGNAL(relativeEVChanged(int, double)),
                    rt, SLOT(setExposureRelativeEV(int, double)));
            connect(ic, SIGNAL(thresholdChanged(int, int)),
                    rt, SLOT(setExposureThreshold(int, int)));
            imageTabs->addTab(ic, tr("Exposure %1").arg(i));
        }
    }
}


void MainWindow::saveResult() {
    if (images) {
        // Take the prefix and add the first and last suffix
        QString name;
        if (images->size() > 1) {
            list<string> names;
            for (unsigned int i = 0; i < images->size(); i++)
                names.push_back(images->getFileName(i).substr(0, images->getFileName(i).find_last_of('.')));
            names.sort();
            int pos = 0;
            while (names.front()[pos] == names.back()[pos]) pos++;
            name = (names.front() + '-' + names.back().substr(pos) + ".pfs").c_str();
        } else name = images->getFileName(0).c_str();

        QString file = QFileDialog::getSaveFileName(this, tr("Save PFS file"), name,
            tr("PFS stream files (*.pfs)"), NULL, QFileDialog::DontUseNativeDialog);
        if (!file.isEmpty()) {
            QProgressDialog progress(tr("Saving %1").arg(file), QString(), 0, 1, this);
            progress.setMinimumDuration(0);
            progress.setValue(0);
            QByteArray fileName = QDir::toNativeSeparators(file).toUtf8();
            QFuture<void> result = QtConcurrent::run(images, &ExposureStack::savePFS, fileName.constData());
            while (result.isRunning())
                QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
            progress.setValue(1);
        }
    }
}


void MainWindow::setTool(QAction * action) {
    if (action == dragToolAction) {
        preview->setCursor(Qt::OpenHandCursor);
        previewArea->toggleMoveViewport(true);
    } else if (action == addGhostAction) {
        QPixmap cursor(32, 32);
        cursor.fill(Qt::white);
        {
            QPainter painter(&cursor);
            painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawEllipse(16 - 5, 16 - 5, 5*2, 5*2);
            painter.drawLine(14, 16, 18, 16);
            painter.drawLine(16, 14, 16, 18);
        }
        cursor.setMask(cursor.createMaskFromColor(Qt::white));
        preview->setCursor(QCursor(cursor, 16, 16));
        previewArea->toggleMoveViewport(false);
    } else if (action == rmGhostAction) {
        QPixmap cursor(32, 32);
        cursor.fill(Qt::white);
        {
            QPainter painter(&cursor);
            painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawEllipse(16 - 5, 16 - 5, 5*2, 5*2);
            painter.drawLine(14, 16, 18, 16);
        }
        cursor.setMask(cursor.createMaskFromColor(Qt::white));
        preview->setCursor(QCursor(cursor, 16, 16));
        previewArea->toggleMoveViewport(false);
    }
}


void MainWindow::painted(int x, int y) {
    // TODO: configure radius
    if (rt != NULL) {
        if (addGhostAction->isChecked())
            rt->addPixels(imageTabs->currentIndex(), x, y, 5);
        else
            rt->removePixels(imageTabs->currentIndex(), x, y, 5);
    }
}
