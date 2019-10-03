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
#include <iterator>
#include <string>
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "Launcher.hpp"
#include "ImageIO.hpp"
#ifndef NO_GUI
#include "MainWindow.hpp"
#endif
#include "Log.hpp"
#include <libraw.h>

using namespace std;

namespace hdrmerge {

Launcher::Launcher(int argc, char * argv[]) : argc(argc), argv(argv), useGui(true) {
    Log::setOutputStream(cout);
    saveOptions.previewSize = 2;
}


int Launcher::startGUI() {
#ifndef NO_GUI
    // Create main window
    MainWindow mw;
    mw.preload(generalOptions.fileNames);
    mw.show();
    QMetaObject::invokeMethod(&mw, "loadImages", Qt::QueuedConnection);

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
    if (generalOptions.imagesPerBracket > 0) {
        if (generalOptions.fileNames.size() % generalOptions.imagesPerBracket != 0) {
            cerr << QCoreApplication::translate("LoadSave", "Number of files not a multiple of number per bracketed set (-s). Aborting.");
            exit(EXIT_FAILURE);
        }

        while(!generalOptions.fileNames.empty()) {
            LoadOptions opt = generalOptions;
            auto oIt = opt.fileNames.begin();
            auto goIt = generalOptions.fileNames.begin();
            std::advance(oIt, generalOptions.imagesPerBracket);
            std::advance(goIt, generalOptions.imagesPerBracket);
            opt.fileNames.erase(oIt, opt.fileNames.end());
            generalOptions.fileNames.erase(generalOptions.fileNames.begin(), goIt);
            result.push_back(opt);
        }
    } else {
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

    QCommandLineParser parser;
    parser.setApplicationDescription(
            tr("Merges raw_files into an HDR DNG raw image.\n") +
#ifndef NO_GUI
            tr("If neither -o nor --batch options are given, the GUI will be presented.\n") +
#endif
            tr("If similar options are specified, only the last one prevails.\n")
    );

    parser.addPositionalArgument("files", "The input raw files", "[raw_files...]");

    QCommandLineOption outputOpt("o",
            tr("Sets <file> as the output file name. "
            "If not set it falls back to '%1'.\n"
            "The following parameters are accepted, most useful in batch mode:").arg("%id[-1]/%iF[0]-%in[-1].dng") +
            "\n- %if[n]: " + tr("Replaced by the base file name of image n. Image file names "
            "are first sorted in lexicographical order. Besides, n = -1 is the "
            "last image, n = -2 is the previous to the last image, and so on.") +
            "\n- %iF[n]: " + tr("Replaced by the base file name of image n without the extension.") +
            "\n- %id[n]: " + tr("Replaced by the directory name of image n.") +
            "\n- %in[n]: " + tr("Replaced by the numerical suffix of image n, if it exists."
            "For instance, in IMG_1234.CR2, the numerical suffix would be 1234.") +
            "\n- %%: " + tr("Replaced by a single %."),
            "file");
    parser.addOption(outputOpt);

    QCommandLineOption maskFileOpt("m",
            tr("Saves the mask to <file> as a PNG image.\n"
            "Besides the parameters accepted by -o, it also accepts:") +
            "\n- %of: " + tr("Replaced by the base file name of the output file.") +
            "\n- %od: " + tr("Replaced by the directory name of the output file."),
            "file");
    parser.addOption(maskFileOpt);

    QCommandLineOption logLevelOpt({"l", "loglevel"},
            tr("The level of logging output you want to recieve."),
            "verbose|debug");
    parser.addOption(logLevelOpt);

    QCommandLineOption noAlignOpt("no-align",
            tr("Do not auto-align source images."));
    parser.addOption(noAlignOpt);

    QCommandLineOption noCropOpt("no-crop",
            tr("Do not crop the output image to the optimum size."));
    parser.addOption(noCropOpt);

    QCommandLineOption batchOpt({"B","batch"},
            tr("Batch mode: Input images are automatically grouped into bracketed sets, "
            "by comparing the creation time.")
            );
    parser.addOption(batchOpt);

    QCommandLineOption bracketSizeOpt({"s", "bracket-size"},
            tr("Fixed number of images per bracket set. Use together with -B. "
               "Creation time and --single option will be ignored."),
            "integer", QString::number(-1));
    parser.addOption(bracketSizeOpt);

    QCommandLineOption singleOpt("single",
            tr("Include single images in batch mode (the default is to skip them.)"));
    parser.addOption(singleOpt);

    QCommandLineOption helpOpt = parser.addHelpOption();

    QCommandLineOption bitOpt("b",
            tr("Bits per sample."),
            "16|24|32", QString::number(16));
    parser.addOption(bitOpt);

    QCommandLineOption whiteLevelOpt("w",
            tr("Use custom white level."), "integer", "16383");
    parser.addOption(whiteLevelOpt);

    QCommandLineOption gapOpt("g",
            tr("Batch gap, maximum difference in seconds between two images of the same set."),
            "double", QString::number(2.0));
    parser.addOption(gapOpt);

    QCommandLineOption featherRadiusOpt("r",
            tr("Mask blur radius, to soften transitions between images. Default is 3 pixels."),
            "integer", QString::number(3));
    parser.addOption(featherRadiusOpt);

    QCommandLineOption previewSizeOpt("p",
            tr("Size of the preview. Default is none."),
            "full|half|none", "none");
    parser.addOption(previewSizeOpt);

    parser.process(QCoreApplication::arguments());

    bool ok; // to check conversions from QString

    char const * errorStart = ">>> ERROR >>> ERROR >>> ERROR >>>\n";
    char const * errorStop  = "\n<<< ERROR <<< ERROR <<< ERROR <<<";

    /**
     * Use this when the user passes an invalid parameter, like "-b=17" (b:16;24;32)
     */
    auto invalidParamHandler = [&] (const QCommandLineOption& opt) {
        cerr << errorStart <<
                tr("Invalid parameter '%1' for option '-%2'. "
                   "Please read the help for valid parameters.")
                  .arg(parser.value(opt))
                  .arg(opt.names().at(0))
             << errorStop << endl;
        // exit right away, creating HDR takes a lot of time
        // and just falling back to default is probably not what the user wants...
        parser.showHelp(EXIT_FAILURE);
    };

    /**
     * Use this when the user passes an invalid type, like "-b=sixteen" (b:integer)
     */
    auto invalidParamTypeHandler = [&] (const QCommandLineOption& opt) {
        cerr << errorStart <<
                tr("Invalid parameter type for option '-%1'. "
                   "Please read the help for the valid type.")
                  .arg(opt.names().at(0))
             << errorStop << endl;
        parser.showHelp(EXIT_FAILURE);
    };

    generalOptions.fileNames = parser.positionalArguments();

    saveOptions.fileName = parser.value(outputOpt);
    if (parser.isSet(maskFileOpt)) {
        saveOptions.saveMask = true;
        saveOptions.maskFileName = parser.value(outputOpt);
    }
    if (parser.isSet(logLevelOpt)) {
        QString value = parser.value(logLevelOpt);
        if (value == "verbose" || value == "v") {
            Log::setMinimumPriority(1);
        } else if (value == "debug" || value == "d") {
            Log::setMinimumPriority(0);
        } else {
            invalidParamHandler(logLevelOpt);
        }
    }
    generalOptions.align = !parser.isSet(noAlignOpt);
    generalOptions.crop = !parser.isSet(noCropOpt);
    generalOptions.batch = parser.isSet(batchOpt);
    {
        QString value = parser.value(bracketSizeOpt);
        int iVal = value.toInt(&ok);
        if (ok) {
            generalOptions.imagesPerBracket = iVal;
        } else {
            invalidParamTypeHandler(bracketSizeOpt);
        }
    }
    generalOptions.withSingles = parser.isSet(singleOpt);
    {
        QString value = parser.value(gapOpt);
        double dVal = value.toDouble(&ok);
        if (ok) {
            generalOptions.batchGap = dVal;
        } else {
            invalidParamTypeHandler(gapOpt);
        }
    }
    {
        QString value = parser.value(featherRadiusOpt);
        int iVal = value.toInt(&ok);
        if (ok) {
            saveOptions.featherRadius = iVal;
        } else {
            invalidParamTypeHandler(featherRadiusOpt);
        }
    }
    {
        generalOptions.useCustomWl = parser.isSet(whiteLevelOpt);
        QString value = parser.value(whiteLevelOpt);
        uint iVal = value.toUInt(&ok);
        if (ok) {
            generalOptions.customWl = iVal;
        } else {
            invalidParamTypeHandler(whiteLevelOpt);
        }
    }
    {
        QString previewSize = parser.value(previewSizeOpt);
        if (previewSize == "none" || previewSize == "n") {
            saveOptions.previewSize = 0;
        } else if (previewSize == "half" || previewSize =="h") {
            saveOptions.previewSize = 1;
        } else if (previewSize == "full" || previewSize == "f") {
            saveOptions.previewSize = 2;
        } else {
            invalidParamHandler(previewSizeOpt);
        }
    }
    {
        QString value = parser.value(bitOpt);
        int iVal = value.toInt(&ok);
        if (ok) {
            if (iVal == 32 || iVal == 24 || iVal == 16) {
                saveOptions.bps = iVal;
            } else {
                invalidParamHandler(bitOpt);
            }
        } else {
            invalidParamTypeHandler(bitOpt);
        }
    }

#ifdef NO_GUI
    useGui = false;
#else
    if (parser.isSet(outputOpt) || parser.isSet(batchOpt)) {
        useGui = false;
    }
#endif
    if (!useGui && generalOptions.fileNames.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }
}

int Launcher::run() {
    QApplication app(argc, argv);

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

    if (useGui) {
        return startGUI();
    } else {
        return automaticMerge();
    }
}

} // namespace hdrmerge
