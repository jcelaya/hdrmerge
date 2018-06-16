/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2018 Jean-Christophe FRISCH
 *  natureh.510@gmail.com
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

#ifndef _FILESYSTEM_HPP_
#define _FILESYSTEM_HPP_

#include <QUrl>
#include <QList>

namespace hdrmerge {

// additionalPath: a non standard path that will be inserted in the url list
QList<QUrl> getStdUrls(const QString additionalPath = "");

}

#endif
