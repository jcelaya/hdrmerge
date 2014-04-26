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
#include <list>
#include <string>
#include "gui.hpp"
#include "ImageStack.hpp"
#include "DngWriter.hpp"
using namespace hdrmerge;
using namespace std;


class CoutProgressIndicator : public ProgressIndicator {
public:
    virtual void advance(int percent, const std::string & message) {
        std::cout << '[' << std::setw(3) << percent << "%] " << message << std::endl;
    }
};


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
        CoutProgressIndicator pi;
        ImageStack stack;
        int step = 100 / (inFileNames.size() + 2);
        int p = -step;
        for (auto name : inFileNames) {
            pi.advance(p += step, std::string("Loading ") + name);
            std::unique_ptr<Image> image(new Image(name));
            if (!image->good() || !stack.addImage(image)) {
                std::cout << "Error loading " << name << std::endl;
                return 1;
            }
        }
        pi.advance(p += step, "Aligning...");
        stack.align();
        pi.advance(p += step, "Referencing...");
        stack.computeRelExposures();
        pi.advance(p += step, "Done loading!");
        string fileName(outFileName);
        if (fileName.substr(fileName.find_last_of('.')) != ".dng") {
            fileName += ".dng";
        }
        std::cout << "Writing result to " << fileName << std::endl;
        DngWriter writer(stack, pi);
        writer.write(fileName);
        return 0;
    }
}

