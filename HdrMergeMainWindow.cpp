#include "HdrMergeMainWindow.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrentRun>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QProgressDialog>
#include <QSettings>
#include "ImageControl.h"
#include <iostream>

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags), images(NULL), rt(NULL), isPickingWB(false) {
	centralwidget = new QWidget(this);
	setCentralWidget(centralwidget);
	QVBoxLayout * layout = new QVBoxLayout(centralwidget);

	previewArea = new DraggableScrollArea(centralwidget);
	previewArea->setAlignment(Qt::AlignCenter);
	layout->addWidget(previewArea);

	preview = new PreviewWidget(previewArea);
	previewArea->setWidget(preview);
	connect(preview, SIGNAL(focus(int, int)), previewArea, SLOT(center(int, int)));
	connect(preview, SIGNAL(imageClicked(QPoint, bool)), this, SLOT(clickImage(QPoint, bool)));

	imageTabs = new QTabWidget(centralwidget);
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(imageTabs->sizePolicy().hasHeightForWidth());
	imageTabs->setSizePolicy(sizePolicy);
	layout->addWidget(imageTabs);

	createActions();
	createMenus();

	//retranslateUi(HdrMergeMainWindow);
	statusbar = new QStatusBar(this);
	setStatusBar(statusbar);
	setWindowTitle(tr("HDRMerge - High dynamic range image fussion"));

	QSettings settings;
	restoreGeometry(settings.value("windowGeometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
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



void MainWindow::createActions() {
	loadImagesAction = new QAction(tr("Load images..."), this);
	connect(loadImagesAction, SIGNAL(triggered()), this, SLOT(loadImages()));

	quitAction = new QAction(tr("Quit"), this);
	quitAction->setShortcut(tr("Ctrl+Q"));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

	aboutAction = new QAction(tr("About..."), this);
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	mergeAction = new QAction(tr("Merge"), this);
	connect(mergeAction, SIGNAL(triggered()), this, SLOT(saveResult()));
}


void MainWindow::createMenus() {
        fileMenu = new QMenu(tr("File"));
        fileMenu->addAction(loadImagesAction);
        fileMenu->addAction(mergeAction);
        fileMenu->addSeparator();
        fileMenu->addAction(quitAction);

        helpMenu = new QMenu(tr("Help"));
        helpMenu->addAction(aboutAction);

	menuBar()->addMenu(fileMenu);
	menuBar()->addMenu(helpMenu);
}


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


void MainWindow::loadImages() {
	QSettings settings;
	QVariant lastDirSetting = settings.value("lastOpenDirectory");
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Load images"),
		lastDirSetting.isNull() ? QDir::currentPath() : QDir(lastDirSetting.toString()).absolutePath(),
		tr("Linear TIFF images (*.tif *.tiff)"));
	if (!files.empty()) {
		// Save last dir
		QString lastDir = QDir(files.front()).absolutePath();
		lastDir.truncate(lastDir.lastIndexOf('/'));
		settings.setValue("lastOpenDirectory", lastDir);

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
		rt = new RenderThread(images, 2.2f, this);
		connect(rt, SIGNAL(renderedImage(unsigned int, unsigned int, unsigned int, unsigned int, QImage)),
			preview, SLOT(paintImage(unsigned int, unsigned int, unsigned int, unsigned int, QImage)));
		connect(preview, SIGNAL(imageViewport(int, int, int, int)),
			rt, SLOT(setImageViewport(int, int, int, int)));
		connect(preview, SIGNAL(scaleBy(int)), rt, SLOT(stepScale(int)));
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
			imageTabs->addTab(ic, tr("Exposure") + " " + QString::number(i));
		}
		// Add white balance widget
		wbw = new WhiteBalanceWidget(images->getWBR(), images->getWBG(), images->getWBB(), imageTabs);
		connect(wbw, SIGNAL(pickerPushed()), this, SLOT(setPickingWB()));
		connect(wbw, SIGNAL(autoWBPushed()), this, SLOT(setAutoWB()));
		connect(rt, SIGNAL(whiteBalanceChanged(double, double, double)),
			wbw, SLOT(changeFactors(double, double, double)));
		imageTabs->addTab(wbw, tr("White Balance"));
	}
}


void MainWindow::saveResult() {
	QString file = QFileDialog::getSaveFileName(this, tr("Save PFS file"), QDir::currentPath(), tr("PFS stream files (*.pfs)"));
	if (!file.isEmpty()) {
		QProgressDialog progress(tr("Saving ") + file, QString(), 0, 1, this);
		progress.setMinimumDuration(0);
		progress.setValue(0);
		QFuture<void> result = QtConcurrent::run(images, &ExposureStack::savePFS, file.toUtf8().constData());
		while (result.isRunning())
			QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
		progress.setValue(1);
	}
}


void MainWindow::setPickingWB() {
	isPickingWB = true;
	preview->toggleCrossCursor(true);
}


void MainWindow::clickImage(QPoint pos, bool left) {
	if (left && isPickingWB) {
		preview->toggleCrossCursor(false);
		isPickingWB = false;
		rt->calculateWB(pos.x(), pos.y(), 5);
	}
}


void MainWindow::setAutoWB() {
	rt->calculateWB(0, 0, 2000000000);
}

