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
    mw.preload(&options);
    mw.show();

    return QApplication::exec();
}


static std::ostream & operator<<(std::ostream & os, const QString & s) {
    return os << string(s.toUtf8().constData());
}


struct CoutProgressIndicator : public ProgressIndicator {
    CoutProgressIndicator() : currentPercent(0) {}

    virtual void advance(int percent, const char * message, const char * arg) {
        currentPercent = percent;
        if (arg) {
            Log::msg(Log::PROGRESS, '[', setw(3), currentPercent, "%] ", QCoreApplication::translate("LoadSave", message).arg(arg));
        } else {
            Log::msg(Log::PROGRESS, '[', setw(3), currentPercent, "%] ", QCoreApplication::translate("LoadSave", message));
        }
    }
    virtual int getPercent() const {
        return currentPercent;
    }

    int currentPercent;
};


int Launcher::automaticMerge() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("LoadSave", text); };
    CoutProgressIndicator progress;
    ImageStack stack;
    if (stack.load(options, progress)) {
        int i = progress.getPercent() * (options.fileNames.size() + 1) / 100;
        if (i) {
            cerr << tr("Error loading %1").arg(options.fileNames[i].c_str()) << endl;
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
        } else if (string("-n") == argv[i] || string("--no-align") == argv[i]) {
            options.align = false;
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
    cout << endl;
    cout << tr("Options:") << endl;
    cout << "    " << "--help              " << tr("Shows this message.") << endl;
    cout << "    " << "-a                  " << tr("Calculates the output file name automatically.") << endl;
    cout << "    " << "-b BPS              " << tr("Bits per sample, can be 16, 24 or 32.") << endl;
    cout << "    " << "-m MASK_FILE        " << tr("Saves the mask to MASK_FILE as a PNG image.") << endl;
    cout << "    " << "-n|--no-align       " << tr("Do not auto-align source images.") << endl;
    cout << "    " << "-o OUT_FILE         " << tr("Sets OUT_FILE as the output file name.") << endl;
    cout << "    " << "-p full|half|none   " << tr("Preview width.") << endl;
    cout << "    " << "-v                  " << tr("Verbose mode.") << endl;
    cout << "    " << "-vv                 " << tr("Debug mode.") << endl;
    cout << "    " << "RAW_FILES           " << tr("The input raw files.") << endl;
    cout << tr("If similar options are specified, only the last one prevails.") << endl;
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
