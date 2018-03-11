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
#include "ImageIO.hpp"
#ifndef NO_GUI
#include "MainWindow.hpp"
#endif
#include "Log.hpp"
#include <libraw.h>

using namespace std;

namespace hdrmerge {

Launcher::Launcher(int argc, char * argv[]) : argc(argc), argv(argv), help(false) {
    Log::setOutputStream(cout);
    saveOptions.previewSize = 2;
}


int Launcher::startGUI() {
#ifndef NO_GUI
    // Create main window
    MainWindow mw;
    mw.preload(generalOptions.fileNames);
    mw.show();

    return QApplication::exec();
#else
    return 0;
#endif
}


struct CoutProgressIndicator : public ProgressIndicator {
    virtual void advance(int percent, const char * message, const char * arg) {
        if (arg) {
            Log::progress('[', setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message).arg(arg));
        } else {
            Log::progress('[', setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message));
        }
    }
};


list<LoadOptions> Launcher::getBracketedSets() {
    list<LoadOptions> result;
    list<pair<ImageIO::QDateInterval, QString>> dateNames;
    for (QString & name : generalOptions.fileNames) {
        ImageIO::QDateInterval interval = ImageIO::getImageCreationInterval(name);
        if (interval.start.isValid()) {
            dateNames.emplace_back(interval, name);
        } else {
            // We cannot get time information, process it alone
            result.push_back(generalOptions);
            result.back().fileNames.clear();
            result.back().fileNames.push_back(name);
        }
    }
    dateNames.sort();
    ImageIO::QDateInterval lastInterval;
    for (auto & dateName : dateNames) {
        if (lastInterval.start.isNull() || lastInterval.difference(dateName.first) > generalOptions.batchGap) {
            result.push_back(generalOptions);
            result.back().fileNames.clear();
        }
        result.back().fileNames.push_back(dateName.second);
        lastInterval = dateName.first;
    }
    int setNum = 0;
    for (auto & i : result) {
        Log::progressN("Set ", setNum++, ":");
        for (auto & j : i.fileNames) {
            Log::progressN(" ", j);
        }
        Log::progress();
    }
    return result;
}


int Launcher::automaticMerge() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("LoadSave", text); };
    list<LoadOptions> optionsSet;
    if (generalOptions.batch) {
        optionsSet = getBracketedSets();
    } else {
        optionsSet.push_back(generalOptions);
    }
    ImageIO io;
    int result = 0;
    for (LoadOptions & options : optionsSet) {
        if (!options.withSingles && options.fileNames.size() == 1) {
            Log::progress(tr("Skipping single image %1").arg(options.fileNames.front()));
            continue;
        }
        CoutProgressIndicator progress;
        int numImages = options.fileNames.size();
        int result = io.load(options, progress);
        if (result < numImages * 2) {
            int format = result & 1;
            int i = result >> 1;
            if (format) {
                cerr << tr("Error loading %1, it has a different format.").arg(options.fileNames[i]) << endl;
            } else {
                cerr << tr("Error loading %1, file not found.").arg(options.fileNames[i]) << endl;
            }
            result = 1;
            continue;
        }
        SaveOptions setOptions = saveOptions;
        if (!setOptions.fileName.isEmpty()) {
            setOptions.fileName = io.replaceArguments(setOptions.fileName, "");
            int extPos = setOptions.fileName.lastIndexOf('.');
            if (extPos > setOptions.fileName.length() || setOptions.fileName.mid(extPos) != ".dng") {
                setOptions.fileName += ".dng";
            }
        } else {
            setOptions.fileName = io.buildOutputFileName();
        }
        Log::progress(tr("Writing result to %1").arg(setOptions.fileName));
        io.save(setOptions, progress);
    }
    return result;
}


void Launcher::parseCommandLine() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                saveOptions.fileName = argv[i];
            }
        } else if (string("-m") == argv[i]) {
            if (++i < argc) {
                saveOptions.maskFileName = argv[i];
                saveOptions.saveMask = true;
            }
        } else if (string("-v") == argv[i]) {
            Log::setMinimumPriority(1);
        } else if (string("-vv") == argv[i]) {
            Log::setMinimumPriority(0);
        } else if (string("--no-align") == argv[i]) {
            generalOptions.align = false;
        } else if (string("--no-crop") == argv[i]) {
            generalOptions.crop = false;
        } else if (string("--batch") == argv[i] || string("-B") == argv[i]) {
            generalOptions.batch = true;
        } else if (string("--single") == argv[i]) {
            generalOptions.withSingles = true;
        } else if (string("--help") == argv[i]) {
            help = true;
        } else if (string("-b") == argv[i]) {
            if (++i < argc) {
                try {
                    int value = stoi(argv[i]);
                    if (value == 32 || value == 24 || value == 16) saveOptions.bps = value;
                } catch (std::invalid_argument & e) {
                    cerr << tr("Invalid %1 parameter, using default.").arg(argv[i - 1]) << endl;
                }
            }
        } else if (string("-w") == argv[i]) {
            if (++i < argc) {
                try {
                    generalOptions.customWl = stoi(argv[i]);
                    generalOptions.useCustomWl = true;
                } catch (std::invalid_argument & e) {
                    cerr << tr("Invalid %1 parameter, using default.").arg(argv[i - 1]) << endl;
                    generalOptions.useCustomWl = false;
                }
            }
        } else if (string("-g") == argv[i]) {
            if (++i < argc) {
                try {
                    generalOptions.batchGap = stod(argv[i]);
                } catch (std::invalid_argument & e) {
                    cerr << tr("Invalid %1 parameter, using default.").arg(argv[i - 1]) << endl;
                }
            }
        } else if (string("-r") == argv[i]) {
            if (++i < argc) {
                try {
                    saveOptions.featherRadius = stoi(argv[i]);
                } catch (std::invalid_argument & e) {
                    cerr << tr("Invalid %1 parameter, using default.").arg(argv[i - 1]) << endl;
                }
            }
        } else if (string("-p") == argv[i]) {
            if (++i < argc) {
                string previewWidth(argv[i]);
                if (previewWidth == "full") {
                    saveOptions.previewSize = 2;
                } else if (previewWidth == "half") {
                    saveOptions.previewSize = 1;
                } else if (previewWidth == "none") {
                    saveOptions.previewSize = 0;
                } else {
                    cerr << tr("Invalid %1 parameter, using default.").arg(argv[i - 1]) << endl;
                }
            }
        } else if (argv[i][0] != '-') {
            generalOptions.fileNames.push_back(QString::fromLocal8Bit(argv[i]));
        }
    }
}


void Launcher::showHelp() {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    cout << tr("Usage") << ": HDRMerge [--help] [OPTIONS ...] [RAW_FILES ...]" << endl;
    cout << tr("Merges RAW_FILES into an HDR DNG raw image.") << endl;
#ifndef NO_GUI
    cout << tr("If neither -a nor -o, nor --batch options are given, the GUI will be presented.") << endl;
#endif
    cout << tr("If similar options are specified, only the last one prevails.") << endl;
    cout << endl;
    cout << tr("Options:") << endl;
    cout << "    " << "--help        " << tr("Shows this message.") << endl;
    cout << "    " << "-o OUT_FILE   " << tr("Sets OUT_FILE as the output file name.") << endl;
    cout << "    " << "              " << tr("The following parameters are accepted, most useful in batch mode:") << endl;
    cout << "    " << "              - %if[n]: " << tr("Replaced by the base file name of image n. Image file names") << endl;
    cout << "    " << "                " << tr("are first sorted in lexicographical order. Besides, n = -1 is the") << endl;
    cout << "    " << "                " << tr("last image, n = -2 is the previous to the last image, and so on.") << endl;
    cout << "    " << "              - %iF[n]: " << tr("Replaced by the base file name of image n without the extension.") << endl;
    cout << "    " << "              - %id[n]: " << tr("Replaced by the directory name of image n.") << endl;
    cout << "    " << "              - %in[n]: " << tr("Replaced by the numerical suffix of image n, if it exists.") << endl;
    cout << "    " << "                " << tr("For instance, in IMG_1234.CR2, the numerical suffix would be 1234.") << endl;
    cout << "    " << "              - %%: " << tr("Replaced by a single %.") << endl;
    cout << "    " << "-a            " << tr("Calculates the output file name as") << " %id[-1]/%iF[0]-%in[-1].dng." << endl;
    cout << "    " << "-B|--batch    " << tr("Batch mode: Input images are automatically grouped into bracketed sets,") << endl;
    cout << "    " << "              " << tr("by comparing the creation time. Implies -a if no output file name is given.") << endl;
    cout << "    " << "-g gap        " << tr("Batch gap, maximum difference in seconds between two images of the same set.") << endl;
    cout << "    " << "--single      " << tr("Include single images in batch mode (the default is to skip them.)") << endl;
    cout << "    " << "-b BPS        " << tr("Bits per sample, can be 16, 24 or 32.") << endl;
    cout << "    " << "--no-align    " << tr("Do not auto-align source images.") << endl;
    cout << "    " << "--no-crop     " << tr("Do not crop the output image to the optimum size.") << endl;
    cout << "    " << "-m MASK_FILE  " << tr("Saves the mask to MASK_FILE as a PNG image.") << endl;
    cout << "    " << "              " << tr("Besides the parameters accepted by -o, it also accepts:") << endl;
    cout << "    " << "              - %of: " << tr("Replaced by the base file name of the output file.") << endl;
    cout << "    " << "              - %od: " << tr("Replaced by the directory name of the output file.") << endl;
    cout << "    " << "-r radius     " << tr("Mask blur radius, to soften transitions between images. Default is 3 pixels.") << endl;
    cout << "    " << "-p size       " << tr("Preview size. Can be full, half or none.") << endl;
    cout << "    " << "-v            " << tr("Verbose mode.") << endl;
    cout << "    " << "-vv           " << tr("Debug mode.") << endl;
    cout << "    " << "-w whitelevel " << tr("Use custom white level.") << endl;
    cout << "    " << "RAW_FILES     " << tr("The input raw files.") << endl;
}


bool Launcher::checkGUI() {
    int numFiles = 0;
    bool useGUI = true;
    for (int i = 1; i < argc; ++i) {
        if (string("-o") == argv[i]) {
            if (++i < argc) {
                useGUI = false;
            }
        } else if (string("-a") == argv[i]) {
            useGUI = false;
        } else if (string("--batch") == argv[i]) {
            useGUI = false;
        } else if (string("-B") == argv[i]) {
            useGUI = false;
        } else if (string("--help") == argv[i]) {
            return false;
        } else if (argv[i][0] != '-') {
            numFiles++;
        }
    }
    return useGUI || numFiles == 0;
}


int Launcher::run() {
#ifndef NO_GUI
    bool useGUI = checkGUI();
#else
    bool useGUI = false;
    help = checkGUI();
#endif
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
    Log::debug("Using LibRaw ", libraw_version());

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
