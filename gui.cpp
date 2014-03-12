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

#include <string>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include "gui.hpp"
#include "MainWindow.hpp"

namespace hdrmerge {

int GUI::startGUI(const std::list<char *> & fileNames) {
    // Settings
    QCoreApplication::setOrganizationName("JaviSoft");
    QCoreApplication::setOrganizationDomain("javisoft.com");
    QCoreApplication::setApplicationName("HdrMerge");

    // Translation
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load("hdrmerge_" + QLocale::system().name());
    installTranslator(&appTranslator);

    // Create main window
    MainWindow mw;
    mw.preload(fileNames);
    mw.show();

    return exec();
}

} // namespace hdrmerge
