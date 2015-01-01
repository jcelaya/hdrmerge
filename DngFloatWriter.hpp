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

#include <QString>
#include <QImage>
#include "config.h"
#include "Array2D.hpp"
#include "TiffDirectory.hpp"

namespace hdrmerge {

class RawParameters;

class DngFloatWriter {
public:
    DngFloatWriter() : previewWidth(0), bps(16) {}

    void setPreviewWidth(size_t w) {
        previewWidth = w;
    }
    void setBitsPerSample(int b) {
        bps = b;
    }
    void setPreview(const QImage & p);
    void write(Array2D<float> && rawPixels, const RawParameters & p, const QString & dstFileName);

private:
    int previewWidth;
    int bps;
    const RawParameters * params;
    Array2D<float> rawData;
    std::unique_ptr<uint8_t[]> fileData;
    size_t pos;
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
    size_t thumbSize();
    size_t previewSize();
    size_t rawSize();
};

} // namespace hdrmerge

#endif // _DNGFLOATWRITER_HPP_
