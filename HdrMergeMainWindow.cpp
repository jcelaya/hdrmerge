#include "HdrMergeMainWindow.h"
#include <QApplication>
#include <QFuture>
#include <QtConcurrentRun>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QProgressDialog>
#include "ImageControl.h"
#include <iostream>

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags), images(NULL), rt(NULL), isGettingWB(false) {
	centralwidget = new QWidget(this);
	setCentralWidget(centralwidget);
	QVBoxLayout * layout = new QVBoxLayout(centralwidget);

	preview = new PreviewWidget(centralwidget);
	connect(preview, SIGNAL(imageClicked(QPoint, bool)), this, SLOT(clickImage(QPoint, bool)));
	layout->addWidget(preview);

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
	resize(640, 480);
	setWindowTitle(tr("HDRMerge - High dynamic range image fussion"));
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
	QStringList files = QFileDialog::getOpenFileNames(this,
		tr("Load images"), QDir::currentPath(), tr("Linear TIFF images (*.tif *.tiff)"));
	if (!files.empty()) {
		unsigned int numImages = files.size();
		// Clean previous state
		while (imageTabs->count() > 0) {
			delete imageTabs->widget(0);
			imageTabs->removeTab(0);
		}
		if (rt != NULL)
			delete rt;
		if (images != NULL)
			delete images;

		// Load and sort images
		images = new ExposureStack();
		QProgressDialog progress(tr("Loading files..."), QString(), 0, numImages + 1, this);
		progress.setMinimumDuration(0);
		for (unsigned int i = 0; i < numImages; i++) {
			progress.setValue(i);
			QFuture<void> result = QtConcurrent::run(images, &ExposureStack::loadImage, files[i].toUtf8().constData());
			while (result.isRunning())
				QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		progress.setValue(numImages);
		QFuture<void> result = QtConcurrent::run(images, &ExposureStack::sort);
		while (result.isRunning())
			QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
		progress.setValue(numImages + 1);

		// Paint and create GUI
		rt = new RenderThread(images, 2.2f, this);
		connect(rt, SIGNAL(renderedImage(QImage)), this, SLOT(updateImage(QImage)));
		rt->start(QThread::LowPriority);
		for (unsigned int i = 0; i < numImages - 1; i++) {
			// Create ImageControl widgets for every exposure except the last one
			ImageControl * ic = new ImageControl(imageTabs, i, images->getRelativeExposure(i), images->getThreshold(i));
			if (i == numImages - 1)
				ic->disableREV();
			connect(ic, SIGNAL(propertiesChanged(int, float, int)), this, SLOT(exposureChange(int, float, int)));
			imageTabs->addTab(ic, tr("Exposure") + " " + QString::number(i));
		}
		// Add white balance widget
		wbw = new WhiteBalanceWidget(images->getWBR(), images->getWBG(), images->getWBB(), imageTabs);
		connect(wbw, SIGNAL(pickerPushed()), this, SLOT(pickWB()));
		imageTabs->addTab(wbw, tr("White Balance"));
	}
}


void MainWindow::saveResult() {
	QString file = QFileDialog::getSaveFileName(this, tr("Save PFS file"), QDir::currentPath(), tr("PFS stream files (*.pfs)"));
	if (!file.isEmpty()) {
		QFuture<void> result = QtConcurrent::run(images, &ExposureStack::savePFS, file.toUtf8().constData());
		while (result.isRunning())
			QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}


void MainWindow::updateImage(const QImage & image) {
	preview->setPixmap(QPixmap::fromImage(image));
}


void MainWindow::exposureChange(int i, float re, int th) {
	images->setRelativeExposure(i, re);
	images->setThreshold(i, th);
	QPoint min, max;
	preview->getViewRect(min, max);
	rt->render(min, max);
}


void MainWindow::pickWB() {
	isGettingWB = true;
	preview->toggleCrossCursor(true);
}


void MainWindow::clickImage(QPoint pos, bool left) {
	if (left && isGettingWB) {
		preview->toggleCrossCursor(false);
		isGettingWB = false;
		unsigned int x = pos.x() > 5 ? pos.x() - 5 : 0;
		unsigned int y = pos.y() > 5 ? pos.y() - 5 : 0;
		unsigned int w = images->getWidth() - pos.x() > 10 ? 10 : images->getWidth() - pos.x();
		unsigned int h = images->getHeight() - pos.y() > 10 ? 10 : images->getHeight() - pos.y();
		images->calculateWB(x, y, w, h);
		QPoint min, max;
		preview->getViewRect(min, max);
		rt->render(min, max);
		wbw->changeFactors(images->getWBR(), images->getWBG(), images->getWBB());
	}
}

