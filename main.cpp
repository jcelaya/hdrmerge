#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "HdrMergeMainWindow.h"

int main(int argc, char * argv[]) {
	QApplication app(argc, argv);

	// Settings
	QCoreApplication::setOrganizationName("JaviSoft");
	QCoreApplication::setOrganizationDomain("javisoft.com");
	QCoreApplication::setApplicationName("HdrMerge");

	// Translation
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(),
		QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);
 
	QTranslator appTranslator;
	appTranslator.load("hdrmerge_" + QLocale::system().name());
	app.installTranslator(&appTranslator);

	MainWindow mw;
	mw.show();

	return app.exec();
}

