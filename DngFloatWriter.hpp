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

#ifndef _DNGFLOATWRITER_HPP_
#define _DNGFLOATWRITER_HPP_

#include <memory>
#include <fstream>
#include <string>
#include <QString>
#include <QImage>
#include "config.h"
#include "ImageStack.hpp"
#include "ProgressIndicator.hpp"
#include "TiffDirectory.hpp"

namespace hdrmerge {

class DngFloatWriter {
public:
    DngFloatWriter(const ImageStack & s, ProgressIndicator & pi);

    void setPreviewWidth(size_t w) {
        previewWidth = w;
    }
    void setBitsPerSample(int b) {
        bps = b;
    }
    void write(const std::string & filename);

private:
    ProgressIndicator & progress;
    std::ofstream file;
    const ImageStack & stack;
    IFD mainIFD, rawIFD, previewIFD;
    uint32_t width, height;
    uint32_t tileWidth, tileLength;
    uint32_t tilesAcross, tilesDown;
    std::unique_ptr<float[]> rawData;
    uint32_t previewWidth;
    int bps;
    QImage thumbnail;
    QImage preview;
    uint32_t subIFDoffsets[2];

    void createMainIFD();
    void createRawIFD();
    void calculateTiles();
    void writeRawData();
    void copyMetadata(const std::string & filename);
    void renderPreviews();
    void writePreviews();
    void createPreviewIFD();
};

} // namespace hdrmerge

#endif // _DNGFLOATWRITER_HPP_
