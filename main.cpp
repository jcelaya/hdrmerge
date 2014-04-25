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
#include <list>
#include <string>
#include "gui.hpp"
#include "ImageStack.hpp"
#include "DngWriter.hpp"
using namespace hdrmerge;
using namespace std;


int main(int argc, char * argv[]) {
    // Parse the list of images in command line
    std::list<char *> inFileNames;
    char * outFileName = NULL;
    for (int i = 1; i < argc; ++i) {
        if (std::string("-o") == argv[i]) {
            if (++i < argc)
                outFileName = argv[i];
        } else if (argv[i][0] != '-') {
            inFileNames.push_back(argv[i]);
        }
    }

    if (outFileName == NULL || inFileNames.empty()) {
        hdrmerge::GUI app(argc, argv);
        return app.startGUI(inFileNames);
    } else {
        ImageStack stack;
        for (auto name : inFileNames) {
            std::unique_ptr<Image> image(new Image(name));
            if (!image->good() || !stack.addImage(image)) {
                return 1;
            }
        }
        stack.align();
        stack.computeRelExposures();
        string fileName(outFileName);
        if (fileName.substr(fileName.find_last_of('.')) != ".dng") {
            fileName += ".dng";
        }
        DngWriter writer(stack);
        writer.write(fileName);
        return 0;
    }
}

