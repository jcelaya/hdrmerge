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

#include <fstream>
#include <string>
#include <QImage>
#include "config.h"
#include "Array2D.hpp"
#include "ProgressIndicator.hpp"
#include "TiffDirectory.hpp"

namespace hdrmerge {

class MetaData;

class DngFloatWriter {
public:
    DngFloatWriter(ProgressIndicator & pi);

    void setPreviewWidth(size_t w) {
        previewWidth = w;
    }
    void setBitsPerSample(int b) {
        bps = b;
    }
    void write(Array2D<float> && rawPixels, const MetaData & md, const std::string & filename);

private:
    ProgressIndicator & progress;
    uint32_t previewWidth;
    int bps;
    const MetaData * metaData;
    Array2D<float> rawData;
    std::ofstream file;
    IFD mainIFD, rawIFD, previewIFD;
    uint32_t width, height;
    uint32_t tileWidth, tileLength;
    uint32_t tilesAcross, tilesDown;
    QImage thumbnail;
    QImage preview;
    QByteArray jpegPreviewData;
    uint32_t subIFDoffsets[2];

    void createMainIFD();
    void createRawIFD();
    void calculateTiles();
    void writeRawData();
    void renderPreviews();
    void writePreviews();
    void createPreviewIFD();
};

} // namespace hdrmerge

#endif // _DNGFLOATWRITER_HPP_
