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
    CoutProgressIndicator() : currentPercent(0) {}

    virtual void advance(int percent, const std::string & message) {
        std::cout << '[' << std::setw(3) << (currentPercent = percent) << "%] " << message << std::endl;
    }
    virtual int getPercent() const {
        return currentPercent;
    }

private:
    int currentPercent;
};


int automaticMerge(const std::list<string> & inFileNames, const char * outFileName) {
    CoutProgressIndicator pi;
    ImageStack stack;
    if (stack.load(inFileNames, pi)) {
        int i = pi.getPercent() * (inFileNames.size() + 1) / 100;
        for (auto & name : inFileNames) {
            if (!i--) {
                std::cout << "Error loading " << name << std::endl;
                return 1;
            }
        }
    }
    string fileName;
    if (outFileName != NULL) {
        fileName = outFileName;
        size_t extPos = fileName.find_last_of('.');
        if (extPos > fileName.length() || fileName.substr(extPos) != ".dng") {
            fileName += ".dng";
        }
    } else {
        fileName = stack.buildOutputFileName() + ".dng";
    }
    std::cout << "Writing result to " << fileName << std::endl;
    DngWriter writer(stack, pi);
    writer.write(fileName);
    return 0;
}


int main(int argc, char * argv[]) {
    // Parse the list of images in command line
    std::list<string> inFileNames;
    char * outFileName = NULL;
    bool automatic = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string("-o") == argv[i]) {
            if (++i < argc) {
                outFileName = argv[i];
                automatic = true;
            }
        } else if (std::string("-a") == argv[i]) {
            automatic = true;
        } else if (argv[i][0] != '-') {
            inFileNames.push_back(argv[i]);
        }
    }

    if (!automatic || inFileNames.empty()) {
        hdrmerge::GUI app(argc, argv);
        return app.startGUI(inFileNames);
    } else {
        return automaticMerge(inFileNames, outFileName);
    }
}
