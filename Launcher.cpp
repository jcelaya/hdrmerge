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

Launcher::Launcher() : outFileName(NULL), automatic(false), help(false), bps(16) {}


int Launcher::startGUI() {
    // Create main window
    MainWindow mw;
    mw.preload(inFileNames);
    mw.show();

    return QApplication::exec();
}


static std::ostream & operator<<(std::ostream & os, const QString & s) {
    return os << string(s.toUtf8().constData());
}


class CoutProgressIndicator : public ProgressIndicator {
public:
    CoutProgressIndicator() : currentPercent(0) {}

    virtual void advance(int percent, const char * message, const char * arg) {
        cout << '[' << setw(3) << (currentPercent = percent) << "%] ";
        if (arg) {
            cout << QCoreApplication::translate("LoadSave", message).arg(arg) << endl;
        } else {
            cout << QCoreApplication::translate("LoadSave", message) << endl;
        }
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
                cout << QCoreApplication::translate("LoadSave", "Error loading %1").arg(name.c_str()) << endl;
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
    cout << QCoreApplication::translate("LoadSave", "Writing result to %1").arg(fileName.c_str()) << endl;
    DngWriter writer(stack, pi);
    writer.setBitsPerSample(bps);
    writer.setPreviewWidth(stack.getWidth());
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
        } else if (string("--help") == argv[i]) {
            help = true;
        } else if (string("-b") == argv[i]) {
            if (++i < argc) {
                int value = stoi(argv[i]);
                if (value == 32 || value == 24 || value == 16) bps = value;
            }
        } else if (argv[i][0] != '-') {
            inFileNames.push_back(argv[i]);
        }
    }
}


void Launcher::showHelp() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    cout << tr("Usage") << ": HDRMerge [--help] [-o OUT_FILE] [-a] [-b BPS] [RAW_FILES ...]" << endl;
    cout << tr("Merges RAW_FILES into OUT_FILE, to obtain an HDR raw image.") << endl;
    cout << endl;
    cout << tr("Options:") << endl;
    cout << "    " << "--help        " << tr("Shows this message.") << endl;
    cout << "    " << "-o OUT_FILE   " << tr("Sets OUT_FILE as the output file name.") << endl;
    cout << "    " << "-a            " << tr("Calculates the output file name automatically. Ignores -o.") << endl;
    cout << "    " << "-b BPS        " << tr("Bits per sample, can be 16, 24 or 32.") << endl;
    cout << "    " << "RAW_FILES     " << tr("The input raw files.") << endl;
}


int Launcher::run() {
    bool useGUI = !help && (!automatic || inFileNames.empty());

    QApplication app(argcGUI, argvGUI, useGUI);

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
    appTranslator.load("hdrmerge_" + QLocale::system().name(), ":/translators");
    app.installTranslator(&appTranslator);

    if (help) {
        showHelp();
        return 0;
    } else if (useGUI) {
        return startGUI();
    } else {
        return automaticMerge();
    }
}

} // namespace hdrmerge
