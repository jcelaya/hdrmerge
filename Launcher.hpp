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

#ifndef _LAUNCHER_HPP_
#define _LAUNCHER_HPP_

#include <list>
#include <string>

namespace hdrmerge {

class Launcher {
public:
    Launcher();

    void parseCommandLine(int argc, char * argv[]);

    int run();

private:
    int startGUI();
    int automaticMerge();
    void showHelp();

    int argcGUI;
    char ** argvGUI;
    std::list<std::string> inFileNames;
    char * outFileName;
    bool automatic;
    bool help;
};

} // namespace hdrmerge

#endif // _LAUNCHER_HPP_
