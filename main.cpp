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

