#include <list>
#include <string>
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "HdrMergeMainWindow.h"
#include "Exposure.h"
using std::list;

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

	// Parse the list of images in command line
	list<char *> inFileNames;
	char * outFileName = NULL;
	for (int i = 1; i < argc; ++i) {
		if (std::string("-o") == argv[i]) {
			if (++i < argc)
				outFileName = argv[i];
		} else	
			inFileNames.push_back(argv[i]);
	}

	if (outFileName == NULL || inFileNames.empty()) {
		// Create main window
		MainWindow mw;
		mw.preload(inFileNames);
		mw.show();

		return app.exec();
	} else {
		ExposureStack image;
		for (list<char *>::iterator it = inFileNames.begin(); it != inFileNames.end(); ++it)
			image.loadImage(*it);
		image.sort();
		image.savePFS(outFileName);
	}
}

