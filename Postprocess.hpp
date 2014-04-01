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

#ifndef _POSTPROESS_HPP_
#define _POSTPROESS_HPP_

#include <memory>
#include "ImageStack.hpp"

namespace hdrmerge {

class ProgressIndicator {
public:
    virtual void advance(const std::string & message) = 0;
};


class Postprocess {
public:
    Postprocess(const ImageStack & stack, ProgressIndicator & p);

    void process();
    void save(const std::string & fileName);

private:
    void amaze_demosaic_RT();
    int FC(int row, int col) const {
        return md.FC(row, col);
    }
    void moveG2toG1();
    void convertToRgb();
    void buildOutputMatrix();
    void convertPixels();
    void buildOutputProfile();
    void savePFS(const std::string & fileName);
    void whiteBalance();
    void autoWB();
    void cameraWB();
    void chromaticAberration();

    ProgressIndicator & progress;
    const int colors = 3;
    MetaData md;
    int width, height;
    float * pre_mul;
    std::unique_ptr<float[][4]> image;
    float outCam[3][4];
    int outputColor;
};

}

#endif // _POSTPROESS_HPP_
