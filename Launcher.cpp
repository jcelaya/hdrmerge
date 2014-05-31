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
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "Launcher.hpp"
#include "MainWindow.hpp"
#include "Log.hpp"
using namespace std;

namespace hdrmerge {

Launcher::Launcher(int argc, char * argv[]) : argc(argc), argv(argv), help(false) {
    Log::setOutputStream(cout);
    wOptions.previewSize = 2;
}


int Launcher::startGUI() {
    // Create main window
    MainWindow mw;
    mw.preload(options.fileNames);
    mw.show();

    return QApplication::exec();
}


struct CoutProgressIndicator : public ProgressIndicator {
    CoutProgressIndicator() {}

    virtual void advance(int percent, const char * message, const char * arg) {
        if (arg) {
            Log::msg(Log::PROGRESS, '[', setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message).arg(arg));
        } else {
            Log::msg(Log::PROGRESS, '[', setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message));
        }
    }
};


int Launcher::automaticMerge() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("LoadSave", text); };
    CoutProgressIndicator progress;
    ImageStack stack;
    int numImages = options.fileNames.size();
    int result = stack.load(options, progress);
    if (result < numImages * 2) {
        int format = result & 1;
        int i = result >> 1;
        if (format) {
            cerr << tr("Error loading %1, it has a different format.").arg(options.fileNames[i].c_str()) << endl;
        } else {
            cerr << tr("Error loading %1, file not found.").arg(options.fileNames[i].c_str()) << endl;
        }
        return 1;
    }
    if (!wOptions.fileName.empty()) {
        size_t extPos = wOptions.fileName.find_last_of('.');
        if (extPos > wOptions.fileName.length() || wOptions.fileName.substr(extPos) != ".dng") {
            wOptions.fileName += ".dng";
        }
    } else {
        wOptions.fileName = stack.buildOutputFileName() + ".dng";
    }
    Log::msg(Log::PROGRESS, tr("Writing result to %1").arg(wOptions.fileName.c_str()));
    stack.save(wOptions, progress);
    return 0;
}


void Launcher::parseCommandLine() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                wOptions.fileName = argv[i];
            }
        } else if (string("-m") == argv[i]) {
            if (++i < argc) {
                wOptions.maskFileName = argv[i];
            }
        } else if (string("-v") == argv[i]) {
            Log::setMinimumPriority(1);
        } else if (string("-vv") == argv[i]) {
            Log::setMinimumPriority(0);
        } else if (string("--no-align") == argv[i]) {
            options.align = false;
        } else if (string("--no-crop") == argv[i]) {
            options.crop = false;
        } else if (string("--help") == argv[i]) {
            help = true;
        } else if (string("-b") == argv[i]) {
            if (++i < argc) {
                try {
                    int value = stoi(argv[i]);
                    if (value == 32 || value == 24 || value == 16) wOptions.bps = value;
                } catch (std::invalid_argument & e) {
                    cerr << tr("Invalid %1 parameter, using default.").arg("-b") << endl;
                }
            }
        } else if (string("-p") == argv[i]) {
            if (++i < argc) {
                string previewWidth(argv[i]);
                if (previewWidth == "full") {
                    wOptions.previewSize = 2;
                } else if (previewWidth == "half") {
                    wOptions.previewSize = 1;
                } else if (previewWidth == "none") {
                    wOptions.previewSize = 0;
                } else {
                    cerr << tr("Invalid %1 parameter, using default.").arg("-p") << endl;
                }
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
    cout << tr("If neither -a nor -o options are given, the GUI will be presented.") << endl;
    cout << tr("If similar options are specified, only the last one prevails.") << endl;
    cout << endl;
    cout << tr("Options:") << endl;
    cout << "    " << "--help              " << tr("Shows this message.") << endl;
    cout << "    " << "-a                  " << tr("Calculates the output file name automatically.") << endl;
    cout << "    " << "-b BPS              " << tr("Bits per sample, can be 16, 24 or 32.") << endl;
    cout << "    " << "-m MASK_FILE        " << tr("Saves the mask to MASK_FILE as a PNG image.") << endl;
    cout << "    " << "--no-align          " << tr("Do not auto-align source images.") << endl;
    cout << "    " << "--no-crop           " << tr("Do not crop the output image to the optimum size.") << endl;
    cout << "    " << "-o OUT_FILE         " << tr("Sets OUT_FILE as the output file name.") << endl;
    cout << "    " << "-p full|half|none   " << tr("Preview width.") << endl;
    cout << "    " << "-v                  " << tr("Verbose mode.") << endl;
    cout << "    " << "-vv                 " << tr("Debug mode.") << endl;
    cout << "    " << "RAW_FILES           " << tr("The input raw files.") << endl;
}


bool Launcher::checkGUI() {
    int numFiles = 0;
    bool result = true;
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                result = false;
            }
        } else if (string("-a") == argv[i]) {
            result = false;
        } else if (string("--help") == argv[i]) {
            return false;
        } else if (argv[i][0] != '-') {
            numFiles++;
        }
    }
    return result || numFiles == 0;
}


int Launcher::run() {
    bool useGUI = checkGUI();

    QApplication app(argc, argv, useGUI);

    // Settings
    QCoreApplication::setOrganizationName("J.Celaya");
    QCoreApplication::setApplicationName("HdrMerge");

    // Translation
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load("hdrmerge_" + QLocale::system().name(), ":/translators");
    app.installTranslator(&appTranslator);

    parseCommandLine();

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
