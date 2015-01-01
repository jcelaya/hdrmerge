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
#include <cmath>
#include <QBuffer>
#include <QDateTime>
#include <QImageWriter>
#include <zlib.h>
#include "config.h"
#include "DngFloatWriter.hpp"
#include "RawParameters.hpp"
#include "Log.hpp"
#include "ExifTransfer.hpp"
using namespace std;


namespace hdrmerge {

enum {
    DNGVERSION = 50706,
    DNGBACKVERSION = 50707,

    CALIBRATIONILLUMINANT = 50778,
    COLORMATRIX = 50721,
    PROFILENAME = 50936,

    NEWSUBFILETYPE = 254,
    IMAGEWIDTH = 256,
    IMAGELENGTH = 257,
    PHOTOINTERPRETATION = 262,
    SAMPLESPERPIXEL = 277,
    BITSPERSAMPLE = 258,
    FILLORDER = 266,
    ACTIVEAREA = 50829,
    MASKEDAREAS = 50830,
    CROPORIGIN = 50719,
    CROPSIZE = 50720,

    TILEWIDTH = 322,
    TILELENGTH = 323,
    TILEOFFSETS = 324,
    TILEBYTES = 325,
    ROWSPERSTRIP = 278,
    STRIPOFFSETS = 273,
    STRIPBYTES = 279,

    PLANARCONFIG = 284,
    COMPRESSION = 259,
    PREDICTOR = 317,
    SAMPLEFORMAT = 339,
    BLACKLEVELREP = 50713,
    BLACKLEVEL = 50714,
    WHITELEVEL = 50717,
    CFAPATTERNDIM = 33421,
    CFAPATTERN = 33422,
    CFAPLANECOLOR = 50710,
    CFALAYOUT = 50711,

    ANALOGBALANCE = 50727,
    CAMERANEUTRAL = 50728,
    ORIENTATION = 274,
    UNIQUENAME = 50708,
    SUBIFDS = 330,

    TIFFEPSTD = 37398,
    MAKE = 271,
    MODEL = 272,
    SOFTWARE = 305,
    DATETIMEORIGINAL = 36867,
    DATETIME = 306,
    IMAGEDESCRIPTION = 270,
    RESOLUTIONUNIT = 269,
    XRESOLUTION = 282,
    YRESOLUTION = 283,
    COPYRIGHT = 33432,

    YCBCRCOEFFS = 529,
    YCBCRSUBSAMPLING = 530,
    YCBCRPOSITIONING = 531,
    REFBLACKWHITE = 532,
};


enum {
    TIFF_CM = 3,
    TIFF_D65 = 21,
    TIFF_RGB = 2,
    TIFF_UNCOMPRESSED = 1,
    TIFF_DEFLATE = 8,
    TIFF_JPEG = 7,
    TIFF_FP2XPREDICTOR = 34894,
    TIFF_FPFORMAT = 3,
    TIFF_CFA = 32803,
    TIFF_YCBCR = 6,
};


void DngFloatWriter::write(Array2D<float> && rawPixels, const RawParameters & p, const QString & dstFileName) {
    params = &p;
    rawData = std::move(rawPixels);
    width = rawData.getWidth();
    height = rawData.getHeight();

    renderPreviews();

    createMainIFD();
    subIFDoffsets[0] = 8 + mainIFD.length();
    createRawIFD();
    size_t dataOffset = subIFDoffsets[0] + rawIFD.length();
    if (previewWidth > 0) {
        createPreviewIFD();
        subIFDoffsets[1] = subIFDoffsets[0] + rawIFD.length();
        dataOffset += previewIFD.length();
    }
    mainIFD.setValue(SUBIFDS, (const void *)subIFDoffsets);
    pos = dataOffset;
    size_t dataSize = dataOffset + thumbSize() + previewSize() + rawSize();
    fileData.reset(new uint8_t[dataSize]);

    Timer t("Write output");
    writePreviews();
    writeRawData();
    dataSize = pos;
    pos = 0;
    TiffHeader().write(fileData.get(), pos);
    mainIFD.write(fileData.get(), pos, false);
    rawIFD.write(fileData.get(), pos, false);
    if (previewWidth > 0) {
        previewIFD.write(fileData.get(), pos, false);
    }

    Exif::transfer(p.fileName, dstFileName, fileData.get(), dataSize);
}


void DngFloatWriter::createMainIFD() {
    uint8_t dngVersion[] = { 1, 4, 0, 0 };
    mainIFD.addEntry(DNGVERSION, IFD::BYTE, 4, dngVersion);
    mainIFD.addEntry(DNGBACKVERSION, IFD::BYTE, 4, dngVersion);
    uint8_t tiffep[] = { 1, 0, 0, 0 };
    mainIFD.addEntry(TIFFEPSTD, IFD::BYTE, 4, tiffep);
    mainIFD.addEntry(MAKE, params->maker);
    mainIFD.addEntry(MODEL, params->model);
    mainIFD.addEntry(SOFTWARE, "HDRMerge " HDRMERGE_VERSION_STRING);
    mainIFD.addEntry(RESOLUTIONUNIT, IFD::SHORT, TIFF_CM);
    uint32_t resolution[] = { 100, 1 };
    mainIFD.addEntry(XRESOLUTION, IFD::RATIONAL, 1, resolution);
    mainIFD.addEntry(YRESOLUTION, IFD::RATIONAL, 1, resolution);
    mainIFD.addEntry(COPYRIGHT, "");
    mainIFD.addEntry(IMAGEDESCRIPTION, params->description);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString currentTimeText = currentTime.toString("yyyy:MM:dd hh:mm:ss");
    mainIFD.addEntry(DATETIME, currentTimeText.toAscii().constData());
    mainIFD.addEntry(DATETIMEORIGINAL, params->dateTime);

    // Profile
    mainIFD.addEntry(CALIBRATIONILLUMINANT, IFD::SHORT, TIFF_D65);
    string profName(params->maker + " " + params->model);
    mainIFD.addEntry(PROFILENAME, profName);
    int32_t colorMatrix[24];
    for (int row = 0, i = 0; row < params->colors; ++row) {
        for (int col = 0; col < 3; ++col) {
            colorMatrix[i++] = std::round(params->camXyz[row][col] * 10000.0f);
            colorMatrix[i++] = 10000;
        }
    }
    mainIFD.addEntry(COLORMATRIX, IFD::SRATIONAL, params->colors * 3, colorMatrix);

    // Color
    uint32_t analogBalance[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    mainIFD.addEntry(ANALOGBALANCE, IFD::RATIONAL, params->colors, analogBalance);
    double wb[] = { 1.0/params->camMul[0], 1.0/params->camMul[1], 1.0/params->camMul[2], 1.0/params->camMul[3] };
    uint32_t cameraNeutral[] = {
        (uint32_t)std::round(1000000.0 * wb[0]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[1]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[2]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[3]), 1000000};
    mainIFD.addEntry(CAMERANEUTRAL, IFD::RATIONAL, params->colors, cameraNeutral);
    mainIFD.addEntry(ORIENTATION, IFD::SHORT, params->tiffOrientation);
    mainIFD.addEntry(UNIQUENAME, params->maker + " " + params->model);
    // TODO: Add Digest and Unique ID
    mainIFD.addEntry(SUBIFDS, IFD::LONG, previewWidth > 0 ? 2 : 1, subIFDoffsets);

    // Thumbnail
    mainIFD.addEntry(NEWSUBFILETYPE, IFD::LONG, 1);
    mainIFD.addEntry(IMAGEWIDTH, IFD::LONG, thumbnail.width());
    mainIFD.addEntry(IMAGELENGTH, IFD::LONG, thumbnail.height());
    mainIFD.addEntry(SAMPLESPERPIXEL, IFD::SHORT, 3);
    uint16_t bpsthumb[] = {8, 8, 8};
    mainIFD.addEntry(BITSPERSAMPLE, IFD::SHORT, 3, bpsthumb);
    mainIFD.addEntry(PLANARCONFIG, IFD::SHORT, 1);
    mainIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, TIFF_RGB);
    mainIFD.addEntry(COMPRESSION, IFD::SHORT, TIFF_UNCOMPRESSED);
    mainIFD.addEntry(ROWSPERSTRIP, IFD::LONG, thumbnail.height());
    mainIFD.addEntry(STRIPBYTES, IFD::LONG, 0);
    mainIFD.addEntry(STRIPOFFSETS, IFD::LONG, 0);
}


void DngFloatWriter::calculateTiles() {
    int bytesPerTile = 512 * 1024;
    int cellSize = 16;
    uint32_t bytesPerSample = bps >> 3;
    uint32_t samplesPerTile = bytesPerTile / bytesPerSample;
    uint32_t tileSide = std::round(std::sqrt(samplesPerTile));
    tileWidth = std::min(width, tileSide);
    tilesAcross = (width + tileWidth - 1) / tileWidth;;
    tileWidth = (width + tilesAcross - 1) / tilesAcross;
    tileWidth = ((tileWidth + cellSize - 1) / cellSize) * cellSize;
    tileLength = std::min(samplesPerTile / tileWidth, height);
    tilesDown = (height + tileLength - 1) / tileLength;
    tileLength = (height + tilesDown - 1) / tilesDown;
    tileLength = ((tileLength + cellSize - 1) / cellSize) * cellSize;
}


void DngFloatWriter::createRawIFD() {
    uint16_t cfaRows = params->FC.getRows(), cfaCols = params->FC.getColumns();
    uint16_t cfaPatternDim[] = { cfaRows, cfaCols };

    rawIFD.addEntry(NEWSUBFILETYPE, IFD::LONG, 0x10001); // Fix it later in ExifTransfer.cpp
    rawIFD.addEntry(IMAGEWIDTH, IFD::LONG, width);
    rawIFD.addEntry(IMAGELENGTH, IFD::LONG, height);

    // Areas
    uint32_t crop[2];
    crop[0] = crop[1] = 0;
    rawIFD.addEntry(CROPORIGIN, IFD::LONG, 2, crop);
    crop[0] = params->width;
    crop[1] = params->height;
    rawIFD.addEntry(CROPSIZE, IFD::LONG, 2, crop);
    uint32_t aa[4];
    aa[0] = params->topMargin;
    aa[1] = params->leftMargin;
    aa[2] = aa[0] + params->height;
    aa[3] = aa[1] + params->width;
    rawIFD.addEntry(ACTIVEAREA, IFD::LONG, 4, aa);
    rawIFD.addEntry(BLACKLEVELREP, IFD::SHORT, 2, cfaPatternDim);
    uint16_t cblack[cfaRows * cfaCols];
    for (int row = 0; row < cfaRows; ++row) {
        for (int col = 0; col < cfaCols; ++col) {
            cblack[row*cfaCols + col] = params->blackAt(col, row);
        }
    }
    rawIFD.addEntry(BLACKLEVEL, IFD::SHORT, cfaRows * cfaCols, cblack);
    rawIFD.addEntry(WHITELEVEL, IFD::SHORT, params->max);
    rawIFD.addEntry(SAMPLESPERPIXEL, IFD::SHORT, 1);
    rawIFD.addEntry(BITSPERSAMPLE, IFD::SHORT, bps);
    if (bps == 24) {
        rawIFD.addEntry(FILLORDER, IFD::SHORT, 1);
    }
    rawIFD.addEntry(PLANARCONFIG, IFD::SHORT, 1);
    rawIFD.addEntry(COMPRESSION, IFD::SHORT, TIFF_DEFLATE);
    rawIFD.addEntry(PREDICTOR, IFD::SHORT, TIFF_FP2XPREDICTOR);
    rawIFD.addEntry(SAMPLEFORMAT, IFD::SHORT, TIFF_FPFORMAT);

    calculateTiles();
    uint32_t numTiles = tilesAcross * tilesDown;
    uint32_t buffer[numTiles];
    rawIFD.addEntry(TILEWIDTH, IFD::LONG, tileWidth);
    rawIFD.addEntry(TILELENGTH, IFD::LONG, tileLength);
    rawIFD.addEntry(TILEOFFSETS, IFD::LONG, numTiles, buffer);
    rawIFD.addEntry(TILEBYTES, IFD::LONG, numTiles, buffer);

    rawIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, TIFF_CFA);
    rawIFD.addEntry(CFAPATTERNDIM, IFD::SHORT, 2, cfaPatternDim);
    uint8_t cfaPattern[cfaRows * cfaCols];
    for (int row = 0; row < cfaRows; ++row) {
        for (int col = 0; col < cfaCols; ++col) {
            cfaPattern[row*cfaCols + col] = params->FC(col, row);
        }
    }
    if (params->colors == 3) {
        for (uint8_t & i : cfaPattern) {
            if (i == 3) i = 1;
        }
    }
    rawIFD.addEntry(CFAPATTERN, IFD::BYTE, cfaRows * cfaCols, cfaPattern);
    uint8_t cfaPlaneColor[] = { 0, 1, 2, 3 };
    rawIFD.addEntry(CFAPLANECOLOR, IFD::BYTE, params->colors, cfaPlaneColor);
    rawIFD.addEntry(CFALAYOUT, IFD::SHORT, 1);
}


void DngFloatWriter::createPreviewIFD() {
    previewIFD.addEntry(NEWSUBFILETYPE, IFD::LONG, 1);
    previewIFD.addEntry(IMAGEWIDTH, IFD::LONG, preview.width());
    previewIFD.addEntry(IMAGELENGTH, IFD::LONG, preview.height());
    previewIFD.addEntry(SAMPLESPERPIXEL, IFD::SHORT, 3);
    uint16_t bpspre[] = {8, 8, 8};
    previewIFD.addEntry(BITSPERSAMPLE, IFD::SHORT, 3, bpspre);
    previewIFD.addEntry(PLANARCONFIG, IFD::SHORT, 1);
    previewIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, TIFF_YCBCR);
    previewIFD.addEntry(COMPRESSION, IFD::SHORT, TIFF_JPEG);
    previewIFD.addEntry(ROWSPERSTRIP, IFD::LONG, preview.height());
    previewIFD.addEntry(STRIPBYTES, IFD::LONG, 0);
    previewIFD.addEntry(STRIPOFFSETS, IFD::LONG, 0);
    uint16_t subsampling[] = { 2, 2 };
    previewIFD.addEntry(YCBCRSUBSAMPLING, IFD::SHORT, 2, subsampling);
    previewIFD.addEntry(YCBCRPOSITIONING, IFD::SHORT, 2);
    uint32_t coefficients[] = { 299, 1000, 587, 1000, 114, 1000 };
    previewIFD.addEntry(YCBCRCOEFFS, IFD::RATIONAL, 3, coefficients);
    uint32_t refs[] = { 0, 1, 255, 1, 128, 1, 255, 1, 128, 1, 255, 1};
    previewIFD.addEntry(REFBLACKWHITE, IFD::RATIONAL, 6, refs);
}


void DngFloatWriter::renderPreviews() {
    if (previewWidth > 0) {
        QBuffer buffer(&jpegPreviewData);
        buffer.open(QIODevice::WriteOnly);
        QImageWriter writer(&buffer, "JPEG");
        writer.setQuality(85);
        if (!writer.write(preview)) {
            cerr << "Error converting the preview to JPEG: " << writer.errorString() << endl;
            previewWidth = 0;
        }
    }
}


void DngFloatWriter::setPreview(const QImage & p) {
    thumbnail = p.scaledToWidth(256, Qt::SmoothTransformation).convertToFormat(QImage::Format_RGB888);
    if (previewWidth != p.width()) {
        preview = p.scaledToWidth(previewWidth, Qt::SmoothTransformation);
    } else {
        preview = p;
    }
}


size_t DngFloatWriter::thumbSize() {
    return thumbnail.width() * thumbnail.height() * 3;
}


size_t DngFloatWriter::previewSize() {
    return previewWidth > 0 ? jpegPreviewData.size() : 0;
}


void DngFloatWriter::writePreviews() {
    size_t ts = thumbSize();
    mainIFD.setValue(STRIPBYTES, ts);
    mainIFD.setValue(STRIPOFFSETS, pos);
    pos = std::copy_n((const uint8_t *)thumbnail.bits(), ts, &fileData[pos]) - fileData.get();
    if (previewWidth > 0) {
        ts = previewSize();
        previewIFD.setValue(STRIPBYTES, ts);
        previewIFD.setValue(STRIPOFFSETS, pos);
        pos = std::copy_n((const uint8_t *)jpegPreviewData.constData(), ts, &fileData[pos]) - fileData.get();
    }
}


static void encodeFPDeltaRow(Bytef * src, Bytef * dst, size_t tileWidth, size_t realTileWidth, int bytesps, int factor) {
    // Reorder bytes into the image
    // 16 and 32-bit versions depend on local architecture, 24-bit does not
    if (bytesps == 3) {
        for (size_t col = 0; col < tileWidth; ++col) {
            dst[col] = src[col*3];
            dst[col + realTileWidth] = src[col*3 + 1];
            dst[col + realTileWidth*2] = src[col*3 + 2];
        }
    } else {
        for (size_t col = 0; col < tileWidth; ++col) {
            for (int byte = 0; byte < bytesps; ++byte)
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
                dst[col + realTileWidth*(bytesps-byte-1)] = src[col*bytesps + byte];
#else
                dst[col + realTileWidth*byte] = src[col*bytesps + byte];
#endif
        }
    }
    // EncodeDeltaBytes
    for (int col = realTileWidth*bytesps - 1; col >= factor; --col) {
        dst[col] -= dst[col - factor];
    }
}


// From DNG SDK dng_utils.h
inline uint16_t DNG_FloatToHalf(uint32_t i) {
    int32_t sign     =  (i >> 16) & 0x00008000;
    int32_t exponent = ((i >> 23) & 0x000000ff) - (127 - 15);
    int32_t mantissa =   i            & 0x007fffff;
    if (exponent <= 0) {
        if (exponent < -10) {
            return (uint16_t)sign;
        }
        mantissa = (mantissa | 0x00800000) >> (1 - exponent);
        if (mantissa &  0x00001000)
            mantissa += 0x00002000;
        return (uint16_t)(sign | (mantissa >> 13));
    } else if (exponent == 0xff - (127 - 15)) {
        if (mantissa == 0) {
            return (uint16_t)(sign | 0x7c00);
        } else {
            return (uint16_t)(sign | 0x7c00 | (mantissa >> 13));
        }
    }
    if (mantissa & 0x00001000) {
        mantissa += 0x00002000;
        if (mantissa & 0x00800000) {
            mantissa =  0;          // overflow in significand,
            exponent += 1;          // adjust exponent
        }
    }
    if (exponent > 30) {
        return (uint16_t)(sign | 0x7c00); // infinity with the same sign as f.
    }
    return (uint16_t)(sign | (exponent << 10) | (mantissa >> 13));
}


inline void DNG_FloatToFP24(uint32_t input, uint8_t *output) {
    int32_t exponent = (int32_t) ((input >> 23) & 0xFF) - 128;
    int32_t mantissa = input & 0x007FFFFF;
    if (exponent == 127) {
        if (mantissa != 0x007FFFFF && ((mantissa >> 7) == 0xFFFF)) {
            mantissa &= 0x003FFFFF;         // knock out msb to make it a NaN
        }
    } else if (exponent > 63) {
        exponent = 63;
        mantissa = 0x007FFFFF;
    } else if (exponent <= -64) {
        if (exponent >= -79) {
            mantissa = (mantissa | 0x00800000) >> (-63 - exponent);
        } else {
            mantissa = 0;
        }
        exponent = -64;
    }
    output [0] = (uint8_t)(((input >> 24) & 0x80) | (uint32_t) (exponent + 64));
    output [1] = (mantissa >> 15) & 0x00FF;
    output [2] = (mantissa >>  7) & 0x00FF;
}


static void compressFloats(Bytef * dst, int tileWidth, int bytesps) {
    if (bytesps == 2) {
        uint16_t * dst16 = (uint16_t *) dst;
        uint32_t * dst32 = (uint32_t *) dst;
        for (int i = 0; i < tileWidth; ++i) {
            dst16[i] = DNG_FloatToHalf(dst32[i]);
        }
    } else if (bytesps == 3) {
        uint8_t  * dst8  = (uint8_t *)  dst;
        uint32_t * dst32 = (uint32_t *) dst;
        for (int i = 0; i < tileWidth; ++i) {
            DNG_FloatToFP24(dst32[i], dst8);
            dst8 += 3;
        }
    }
}


size_t DngFloatWriter::rawSize() {
    // Worst case size
    return tilesAcross * tilesDown * tileWidth * tileLength * (bps >> 3);
}


void DngFloatWriter::writeRawData() {
    size_t tileCount = tilesAcross * tilesDown;
    uint32_t tileOffsets[tileCount];
    uint32_t tileBytes[tileCount];
    int bytesps = bps >> 3;
    uLongf dstLen = tileWidth * tileLength * bytesps;

    #pragma omp parallel
    {
        Bytef * cBuffer = new Bytef[dstLen];
        Bytef * uBuffer = new Bytef[dstLen];

        #pragma omp for collapse(2) schedule(dynamic)
        for (size_t y = 0; y < height; y += tileLength) {
            for (size_t x = 0; x < width; x += tileWidth) {
                size_t t = (y / tileLength) * tilesAcross + (x / tileWidth);
                size_t thisTileLength = y + tileLength > height ? height - y : tileLength;
                size_t thisTileWidth = x + tileWidth > width ? width - x : tileWidth;
                if (thisTileLength != tileLength || thisTileWidth != tileWidth) {
                    fill_n(uBuffer, dstLen, 0);
                }
                for (size_t row = 0; row < thisTileLength; ++row) {
                    Bytef * dst = uBuffer + row*tileWidth*bytesps;
                    Bytef * src = (Bytef *)&rawData(x, y+row);
                    compressFloats(src, thisTileWidth, bytesps);
                    encodeFPDeltaRow(src, dst, thisTileWidth, tileWidth, bytesps, 2);
                }
                uLongf conpressedLength = dstLen;
                int err = compress(cBuffer, &conpressedLength, uBuffer, dstLen);
                tileBytes[t] = conpressedLength;
                if (err != Z_OK) {
                    std::cerr << "DNG Deflate: Failed compressing tile " << t << ", with error " << err << std::endl;
                } else {
                    #pragma omp critical
                    {
                        tileOffsets[t] = pos;
                        std::copy_n((const uint8_t *)cBuffer, tileBytes[t], &fileData[pos]);
                        pos += tileBytes[t];
                    }
                }
            }
        }

        delete [] cBuffer;
        delete [] uBuffer;
    }

    rawIFD.setValue(TILEOFFSETS, tileOffsets);
    rawIFD.setValue(TILEBYTES, tileBytes);
}

} // namespace hdrmerge
