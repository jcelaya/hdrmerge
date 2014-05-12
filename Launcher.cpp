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
using namespace std;

namespace hdrmerge {

Launcher::Launcher() : automatic(false), help(false), previewWidth(nullptr) {}


int Launcher::startGUI() {
    // Create main window
    MainWindow mw;
    mw.preload(&options);
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
    if (stack.load(options, pi)) {
        int i = pi.getPercent() * (options.fileNames.size() + 1) / 100;
        if (i) {
            cout << QCoreApplication::translate("LoadSave", "Error loading %1").arg(options.fileNames[i].c_str()) << endl;
            return 1;
        }
    }
    if (!wOptions.fileName.empty()) {
        size_t extPos = wOptions.fileName.find_last_of('.');
        if (extPos > wOptions.fileName.length() || wOptions.fileName.substr(extPos) != ".dng") {
            wOptions.fileName += ".dng";
        }
    } else {
        wOptions.fileName = stack.buildOutputFileName() + ".dng";
    }
    cout << QCoreApplication::translate("LoadSave", "Writing result to %1").arg(wOptions.fileName.c_str()) << endl;
    wOptions.previewSize = stack.getWidth();
    if (previewWidth != nullptr) {
        if (string("half") == previewWidth) {
            wOptions.previewSize /= 2;
        } else if (string("none") == previewWidth) {
            wOptions.previewSize = 0;
        }
    }
    stack.save(wOptions, pi);
    return 0;
}


void Launcher::parseCommandLine(int argc, char * argv[]) {
    argcGUI = argc;
    argvGUI = argv;
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                wOptions.fileName = argv[i];
                automatic = true;
            }
        } else if (string("-m") == argv[i]) {
            if (++i < argc) {
                wOptions.maskFileName = argv[i];
            }
        } else if (string("-a") == argv[i]) {
            automatic = true;
        } else if (string("-n") == argv[i] || string("--no-align") == argv[i]) {
            options.align = false;
        } else if (string("--help") == argv[i]) {
            help = true;
        } else if (string("-b") == argv[i]) {
            if (++i < argc) {
                int value = stoi(argv[i]);
                if (value == 32 || value == 24 || value == 16) wOptions.bps = value;
            }
        } else if (string("-p") == argv[i]) {
            if (++i < argc) {
                previewWidth = argv[i];
            }
        } else if (argv[i][0] != '-') {
            options.fileNames.push_back(argv[i]);
        }
    }
}


void Launcher::showHelp() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    cout << tr("Usage") << ": HDRMerge [--help] [OPTIONS ...] [RAW_FILES ...]" << endl;
    cout << tr("Merges RAW_FILES into an HDR DNG raw image.") << endl;
    cout << endl;
    cout << tr("Options:") << endl;
    cout << "    " << "--help              " << tr("Shows this message.") << endl;
    cout << "    " << "-a                  " << tr("Calculates the output file name automatically. Ignores -o.") << endl;
    cout << "    " << "-b BPS              " << tr("Bits per sample, can be 16, 24 or 32.") << endl;
    cout << "    " << "-m MASK_FILE        " << tr("Saves the mask to MASK_FILE as a PNG image.") << endl;
    cout << "    " << "-n|--no-align       " << tr("Do not auto-align source images.") << endl;
    cout << "    " << "-o OUT_FILE         " << tr("Sets OUT_FILE as the output file name.") << endl;
    cout << "    " << "-p full|half|none   " << tr("Preview width.") << endl;
    cout << "    " << "RAW_FILES           " << tr("The input raw files.") << endl;
}


int Launcher::run() {
    bool useGUI = !help && (!automatic || options.fileNames.empty());

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
