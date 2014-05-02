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

#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "Launcher.hpp"
#include "MainWindow.hpp"
#include "ImageStack.hpp"
#include "DngWriter.hpp"
using namespace std;

namespace hdrmerge {

Launcher::Launcher() : outFileName(NULL), automatic(false) {

}


int Launcher::startGUI() {
    QApplication app(argcGUI, argvGUI);

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

    // Create main window
    MainWindow mw;
    mw.preload(inFileNames);
    mw.show();

    return app.exec();
}


class CoutProgressIndicator : public ProgressIndicator {
public:
    CoutProgressIndicator() : currentPercent(0) {}

    virtual void advance(int percent, const string & message) {
        cout << '[' << setw(3) << (currentPercent = percent) << "%] " << message << endl;
    }
    virtual int getPercent() const {
        return currentPercent;
    }

private:
    int currentPercent;
};


int Launcher::automaticMerge() {
    CoutProgressIndicator pi;
    ImageStack stack;
    if (stack.load(inFileNames, pi)) {
        int i = pi.getPercent() * (inFileNames.size() + 1) / 100;
        for (auto & name : inFileNames) {
            if (!i--) {
                cout << "Error loading " << name << endl;
                return 1;
            }
        }
    }
    string fileName;
    if (outFileName != NULL) {
        fileName = outFileName;
        size_t extPos = fileName.find_last_of('.');
        if (extPos > fileName.length() || fileName.substr(extPos) != ".dng") {
            fileName += ".dng";
        }
    } else {
        fileName = stack.buildOutputFileName() + ".dng";
    }
    cout << "Writing result to " << fileName << endl;
    DngWriter writer(stack, pi);
    writer.write(fileName);
    return 0;
}


void Launcher::parseCommandLine(int argc, char * argv[]) {
    argcGUI = argc;
    argvGUI = argv;
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                outFileName = argv[i];
                automatic = true;
            }
        } else if (string("-a") == argv[i]) {
            automatic = true;
        } else if (argv[i][0] != '-') {
            inFileNames.push_back(argv[i]);
        }
    }
}


int Launcher::run() {
    if (!automatic || inFileNames.empty()) {
        return startGUI();
    } else {
        return automaticMerge();
    }
}

} // namespace hdrmerge
