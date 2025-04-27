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

namespace hdrmerge {

Launcher::Launcher(int argc, char * argv[]) : argc(argc), argv(argv), useGui(true) {
    Log::setOutputStream(std::cout);
    saveOptions.previewSize = 2;
}


int Launcher::startGUI() const {
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
            Log::progress('[', std::setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message).arg(arg));
        } else {
            Log::progress('[', std::setw(3), percent, "%] ", QCoreApplication::translate("LoadSave", message));
        }
    }
};


std::list<LoadOptions> Launcher::getBracketedSets() const {
    std::list<LoadOptions> result;
    if (generalOptions.grouping == LoadOptions::Grouping::MANUAL) {
        LoadOptions globalOptions = generalOptions;
        while(!globalOptions.fileNames.empty()) {
            LoadOptions opt = globalOptions;

            // opt.fileNames and globalOptions.fileNames are equal
            // *optIt == *globaloptIt
            auto optIt = opt.fileNames.begin();
            auto globaloptIt = globalOptions.fileNames.begin();

            // move both iterators by the number of images per bracket
            std::advance(optIt, globalOptions.imagesPerBracket);
            std::advance(globaloptIt, globalOptions.imagesPerBracket);

            // we want to keep only the first imagesPerBracket file names
            // erase the tail
            opt.fileNames.erase(optIt, opt.fileNames.end());

            // the first imagesPerBracket file names found their new home in opt
            // Erase them from globalOptions
            globalOptions.fileNames.erase(globalOptions.fileNames.begin(), globaloptIt);
            result.push_back(opt);
        }
    } else {
        std::list<std::pair<ImageIO::QDateInterval, QString>> dateNames;
        for (const QString & name : generalOptions.fileNames) {
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


int Launcher::automaticMerge() const {
    auto tr = [&] (const char * text) { return QCoreApplication::translate("LoadSave", text); };
    std::list<LoadOptions> optionsSet;
    if (generalOptions.grouping > LoadOptions::Grouping::ALL) {
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
                std::cerr << tr("Error loading %1, it has a different format.").arg(options.fileNames[i]) << std::endl;
            } else {
                std::cerr << tr("Error loading %1, file not found.").arg(options.fileNames[i]) << std::endl;
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
            tr("If neither -o nor --group options are given, the GUI will be presented.\n") +
#endif
            tr("If similar options are specified, only the last one prevails.\n")
    );

    parser.addPositionalArgument("files", "The input raw files", "[raw_files...]");

    const QCommandLineOption outputOption("o",
            tr("Sets <file> as the output file name. "
            "If not set it falls back to '%1'.\n"
            "The following parameters are accepted, most useful in batch mode:").arg("%id[-1]/%iF[0]-%in[-1].dng") +
            "\n- %if[n]: " + tr("Replaced by the base file name of image n. Image file names "
            "are first sorted in lexicographical order. Besides, n = -1 is the "
            "last image, n = -2 is the previous to the last image, and so on.") +
            "\n- %iF[n]: " + tr("Replaced by the base file name of image n without the extension.") +
            "\n- %id[n]: " + tr("Replaced by the directory name of image n.") +
            "\n- %in[n]: " + tr("Replaced by the numerical suffix of image n, if it exists. "
            "For instance, in IMG_1234.CR2, the numerical suffix would be 1234.") +
            "\n- %%: " + tr("Replaced by a single %."),
            "file");
    parser.addOption(outputOption);

    const QCommandLineOption maskFileOption("m",
            tr("Saves the mask to <file> as a PNG image.\n"
            "Besides the parameters accepted by -o, it also accepts:") +
            "\n- %of: " + tr("Replaced by the base file name of the output file.") +
            "\n- %od: " + tr("Replaced by the directory name of the output file."),
            "file");
    parser.addOption(maskFileOption);

    const QCommandLineOption logLevelOption({"l", "loglevel"},
            tr("The level of logging output you want to receive."),
            "verbose|debug");
    parser.addOption(logLevelOption);

    const QCommandLineOption noAlignOption("no-align",
            tr("Do not auto-align source images."));
    parser.addOption(noAlignOption);

    const QCommandLineOption noCropOption("no-crop",
            tr("Do not crop the output image to the optimum size."));
    parser.addOption(noCropOption);

    const QCommandLineOption groupOption({"G", "group"},
            tr("Determines how the input images are grouped.") +
               "\n- all, a: " + tr("Process all input images as a single group. Default.") +
               "\n- auto, t: " + tr("Group images automatically based on their time of creation (Exif DateTimeOriginal).") +
               "\n- manual, m: " + tr("Group images manually based on a number of images per group."),
            "all|auto|manual",
            "all");
    parser.addOption(groupOption);

    const QCommandLineOption bracketSizeOption({"s", "bracket-size"},
            tr("Fixed number of images per bracket set. "
               "Only used when grouping manually. " 
               "Creation time and --single option will be ignored. "
               "The total number of images passed to HDRMerge must be a multiple of this number.") +
               " Default: 3",
            "integer", QString::number(3));
    parser.addOption(bracketSizeOption);

    const QCommandLineOption gapOption({"g", "gap"},
            tr("Maximum gap in seconds between chronologically adjacent images. "
               "If the gap is larger, a new group is created. "
               "Only used when grouping automatically or all.") +
               " Default: 3.0",
            "double", QString::number(3.0));
    parser.addOption(gapOption);

    const QCommandLineOption singleOption("single",
            tr("Include single images when specified. Only used when grouping automatically. "
               "Off by default."));
    parser.addOption(singleOption);

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption bitOption("b",
            tr("Bits per sample."),
            "16|24|32", QString::number(16));
    parser.addOption(bitOption);

    const QCommandLineOption whiteLevelOption("w",
            tr("Use custom white level."), "integer", "16383");
    parser.addOption(whiteLevelOption);

    const QCommandLineOption featherRadiusOption("r",
            tr("Mask blur radius, to soften transitions between images. Default is 3 pixels."),
            "integer", QString::number(3));
    parser.addOption(featherRadiusOption);

    const QCommandLineOption previewSizeOption("p",
            tr("Size of the preview. Default is none."),
            "full|half|none", "none");
    parser.addOption(previewSizeOption);

    parser.process(QCoreApplication::arguments());

    bool ok; // to check conversions from QString

    const char* const errorHint = "There were errors in your commandline options:";

    /**
     * Use this when the user passes an invalid parameter, like "-b=17" (b:16;24;32)
     */
    const auto invalidParameterHandler =
        [&tr, &parser, errorHint] (const QCommandLineOption& opt) {
        std::cerr << parser.helpText() << '\n' <<
                tr(errorHint) << '\n' <<
                tr("Invalid parameter '%1' for option '-%2'. "
                   "Please read the help for valid parameters.")
                  .arg(parser.value(opt))
                  .arg(opt.names().at(0)) << std::endl;
        // exit right away, creating HDR takes a lot of time
        // and just falling back to default is probably not what the user wants...
        exit(EXIT_FAILURE);
    };

    /**
     * Use this when the user passes an invalid type, like "-b=sixteen" (b:integer)
     */
    const auto invalidParameterTypeHandler =
        [&tr, &parser, errorHint] (const QCommandLineOption& opt) {
        std::cerr << parser.helpText() << '\n' <<
                tr(errorHint) << '\n' <<
                tr("Invalid parameter type for option '-%1'. "
                   "Please read the help for the valid type.")
                  .arg(opt.names().at(0)) << std::endl;
        exit(EXIT_FAILURE);
    };

    generalOptions.fileNames = parser.positionalArguments();

    saveOptions.fileName = parser.value(outputOption);
    if (parser.isSet(maskFileOption)) {
        saveOptions.saveMask = true;
        saveOptions.maskFileName = parser.value(outputOption);
    }
    if (parser.isSet(logLevelOption)) {
        const QString value = parser.value(logLevelOption);
        if (value == "verbose" || value == "v") {
            Log::setMinimumPriority(1);
        } else if (value == "debug" || value == "d") {
            Log::setMinimumPriority(0);
        } else {
            invalidParameterHandler(logLevelOption);
        }
    }
    generalOptions.align = !parser.isSet(noAlignOption);
    generalOptions.crop = !parser.isSet(noCropOption);

    if (parser.isSet(groupOption) || parser.isSet(outputOption)) {
        const QString value = parser.value(groupOption);
        if (value == "all" || value == "a") {
            generalOptions.grouping = LoadOptions::Grouping::ALL;
        } else if (value == "auto" || value == "t") {
            generalOptions.grouping = LoadOptions::Grouping::AUTO;
        } else if (value == "manual" || value == "m") {
            generalOptions.grouping = LoadOptions::Grouping::MANUAL;
        } else {
            invalidParameterHandler(groupOption);
        }
    }

    {
        const int value = parser.value(bracketSizeOption).toInt(&ok);
        if (ok) {
            generalOptions.imagesPerBracket = value;
        } else {
            invalidParameterTypeHandler(bracketSizeOption);
        }
    }
    generalOptions.withSingles = parser.isSet(singleOption);
    {
        const double value = parser.value(gapOption).toDouble(&ok);
        if (ok) {
            generalOptions.batchGap = value;
        } else {
            invalidParameterTypeHandler(gapOption);
        }
    }
    {
        const int value = parser.value(featherRadiusOption).toInt(&ok);
        if (ok) {
            saveOptions.featherRadius = value;
        } else {
            invalidParameterTypeHandler(featherRadiusOption);
        }
    }
    {
        generalOptions.useCustomWl = parser.isSet(whiteLevelOption);
        const uint value = parser.value(whiteLevelOption).toUInt(&ok);
        if (ok) {
            generalOptions.customWl = value;
        } else {
            invalidParameterTypeHandler(whiteLevelOption);
        }
    }
    {
        const QString previewSize = parser.value(previewSizeOption);
        if (previewSize == "none" || previewSize == "n") {
            saveOptions.previewSize = 0;
        } else if (previewSize == "half" || previewSize =="h") {
            saveOptions.previewSize = 1;
        } else if (previewSize == "full" || previewSize == "f") {
            saveOptions.previewSize = 2;
        } else {
            invalidParameterHandler(previewSizeOption);
        }
    }
    {
        const int value = parser.value(bitOption).toInt(&ok);
        if (ok) {
            if (value == 32 || value == 24 || value == 16) {
                saveOptions.bps = value;
            } else {
                invalidParameterHandler(bitOption);
            }
        } else {
            invalidParameterTypeHandler(bitOption);
        }
    }

    if (generalOptions.grouping == LoadOptions::Grouping::MANUAL &&
        generalOptions.fileNames.size() % generalOptions.imagesPerBracket != 0) {
        std::cerr << tr("Number of files not a multiple of number per bracketed set (-s). Aborting.");
        exit(EXIT_FAILURE);
    }

#ifdef NO_GUI
    useGui = false;
#else
    if (generalOptions.grouping > LoadOptions::Grouping::UNSET) {
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
