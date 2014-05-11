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

#include <ctime>
#include <iostream>
#include <QImage>
#include <QBuffer>
#include <zlib.h>
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include "config.h"
#include "DngFloatWriter.hpp"
using namespace std;


namespace hdrmerge {

DngFloatWriter::DngFloatWriter(const ImageStack & s, ProgressIndicator & pi)
    : progress(pi), stack(s),
    width(stack.getWidth()), height(stack.getHeight()), previewWidth(width), bps(16) {}


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

    TILEWIDTH = 322,
    TILELENGTH = 323,
    TILEOFFSETS = 324,
    TILEBYTES = 325,

    PLANARCONFIG = 284,
    COMPRESSION = 259,
    PREDICTOR = 317,
    SAMPLEFORMAT = 339,
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
} Tag;


void DngFloatWriter::createMainIFD() {
    const MetaData & md = stack.getImage(0).getMetaData();
    uint8_t dngVersion[] = { 1, 4, 0, 0 };
    mainIFD.addEntry(DNGVERSION, IFD::BYTE, 4, dngVersion);
    mainIFD.addEntry(DNGBACKVERSION, IFD::BYTE, 4, dngVersion);
    // Thumbnail set thumbnail->AddTagSet (mainIFD)
    uint8_t tiffep[] = { 1, 0, 0, 0 };
    mainIFD.addEntry(TIFFEPSTD, IFD::BYTE, 4, tiffep);
    mainIFD.addEntry(MAKE, IFD::ASCII, md.maker.length() + 1, md.maker.c_str());
    mainIFD.addEntry(MODEL, IFD::ASCII, md.model.length() + 1, md.model.c_str());
    std::string appVersion("HDRMerge " HDRMERGE_VERSION_STRING);
    mainIFD.addEntry(SOFTWARE, IFD::ASCII, appVersion.length() + 1, appVersion.c_str());
    mainIFD.addEntry(RESOLUTIONUNIT, IFD::SHORT, 3); // Cm
    uint32_t resolution[] = { 100, 1 };
    mainIFD.addEntry(XRESOLUTION, IFD::RATIONAL, 1, resolution);
    mainIFD.addEntry(YRESOLUTION, IFD::RATIONAL, 1, resolution);
    char empty[] = { 0 };
    mainIFD.addEntry(COPYRIGHT, IFD::ASCII, 1, empty);
    mainIFD.addEntry(IMAGEDESCRIPTION, IFD::ASCII, md.description.length() + 1, md.description.c_str());
    char dateTime[20] = { 0 };
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    int chars = strftime(dateTime, 20, "%Y:%m:%d %T", timeinfo);
    mainIFD.addEntry(DATETIME, IFD::ASCII, chars + 1, dateTime);
    mainIFD.addEntry(DATETIMEORIGINAL, IFD::ASCII, md.dateTime.length() + 1, md.dateTime.c_str());

    // Profile
    mainIFD.addEntry(CALIBRATIONILLUMINANT, IFD::SHORT, 21); // D65
    string profName(md.maker + " " + md.model);
    mainIFD.addEntry(PROFILENAME, IFD::ASCII, profName.length() + 1, profName.c_str());
    int32_t colorMatrix[18];
    for (int im = 0, id = 0; im < 9; ++im, ++id) {
        colorMatrix[id] = std::round(md.camXyz[0][im] * 10000.0f);
        colorMatrix[++id] = 10000;
    }
    mainIFD.addEntry(COLORMATRIX, IFD::SRATIONAL, 9, colorMatrix);

    // Color
    uint32_t analogBalance[] = { 1, 1, 1, 1, 1, 1 };
    mainIFD.addEntry(ANALOGBALANCE, IFD::RATIONAL, 3, analogBalance);
    double minWb = std::min(md.camMul[0], std::min(md.camMul[1], md.camMul[2]));
    double wb[] = { minWb/md.camMul[0], minWb/md.camMul[1], minWb/md.camMul[2] };
    uint32_t cameraNeutral[] = {
        (uint32_t)std::round(1000000.0 * wb[0]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[1]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[2]), 1000000};
    mainIFD.addEntry(CAMERANEUTRAL, IFD::RATIONAL, 3, cameraNeutral);

    mainIFD.addEntry(ORIENTATION, IFD::SHORT, md.flip);
    string cameraName(md.maker + " " + md.model);
    mainIFD.addEntry(UNIQUENAME, IFD::ASCII, cameraName.length() + 1, &cameraName[0]);
    // TODO: Add Digest and Unique ID
    // TODO: Add XMP and Exif and private data
    mainIFD.addEntry(SUBIFDS, IFD::LONG, 1);
}


void DngFloatWriter::calculateTiles() {
    int bytesPerTile = 512 * 1024;
    int cellSize = 16;
    size_t bytesPerSample = bps >> 3;
    size_t samplesPerTile = bytesPerTile / bytesPerSample;
    size_t tileSide = std::round(std::sqrt(samplesPerTile));
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
    rawIFD.addEntry(NEWSUBFILETYPE, IFD::LONG, 0);
    rawIFD.addEntry(IMAGEWIDTH, IFD::LONG, width);
    rawIFD.addEntry(IMAGELENGTH, IFD::LONG, height);
    rawIFD.addEntry(SAMPLESPERPIXEL, IFD::SHORT, 1);
    rawIFD.addEntry(BITSPERSAMPLE, IFD::SHORT, bps);
    if (bps == 24) {
        rawIFD.addEntry(FILLORDER, IFD::SHORT, 1);
    }
    rawIFD.addEntry(PLANARCONFIG, IFD::SHORT, 1);
    rawIFD.addEntry(COMPRESSION, IFD::SHORT, 8); // Deflate
    rawIFD.addEntry(PREDICTOR, IFD::SHORT, 34894); //FP2X
    rawIFD.addEntry(SAMPLEFORMAT, IFD::SHORT, 3); // Floating Point

    calculateTiles();
    uint32_t numTiles = tilesAcross * tilesDown;
    uint32_t buffer[numTiles];
    rawIFD.addEntry(TILEWIDTH, IFD::LONG, tileWidth);
    rawIFD.addEntry(TILELENGTH, IFD::LONG, tileLength);
    rawIFD.addEntry(TILEOFFSETS, IFD::LONG, numTiles, buffer);
    rawIFD.addEntry(TILEBYTES, IFD::LONG, numTiles, buffer);

    rawIFD.addEntry(WHITELEVEL, IFD::SHORT, 1);
    rawIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, 32803); // CFA
    uint16_t cfaPatternDim[] = { 2, 2 };
    rawIFD.addEntry(CFAPATTERNDIM, IFD::SHORT, 2, cfaPatternDim);
    const MetaData & md = stack.getImage(0).getMetaData();
    uint8_t cfaPattern[] = { md.FC(0, 0), md.FC(0, 1), md.FC(1, 0), md.FC(1, 1) };
    for (uint8_t & i : cfaPattern) {
        if (i == 3) i = 1;
    }
    rawIFD.addEntry(CFAPATTERN, IFD::BYTE, 4, cfaPattern);
    uint8_t cfaPlaneColor[] = { 0, 1, 2 };
    rawIFD.addEntry(CFAPLANECOLOR, IFD::BYTE, 3, cfaPlaneColor);
    rawIFD.addEntry(CFALAYOUT, IFD::SHORT, 1);
}


void DngFloatWriter::buildIndexImage() {
    size_t width = stack.getWidth(), height = stack.getHeight();
    QImage indexImage(width, height, QImage::Format_Indexed8);
    int numColors = stack.size() - 1;
    for (int c = 0; c < numColors; ++c) {
        int gray = (256 * c) / numColors;
        indexImage.setColor(c, qRgb(gray, gray, gray));
    }
    indexImage.setColor(numColors, qRgb(255, 255, 255));
    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            indexImage.setPixel(col, row, stack.getImageAt(col, row));
        }
    }
    indexImage.save(indexFile);
}


void DngFloatWriter::write(const string & filename) {
    progress.advance(0, "Rendering image");
    rawData.reset(new float[width * height]);
    stack.compose(rawData.get());

    progress.advance(25, "Rendering preview");
    renderPreviews();

    progress.advance(50, "Initialize metadata");
    file.open(filename, ios_base::binary);
    createMainIFD();
    createRawIFD();
    size_t rawIFDOffset = 8 + mainIFD.length();
    mainIFD.setValue(SUBIFDS, rawIFDOffset);
    size_t dataOffset = rawIFDOffset + rawIFD.length();
    file.seekp(dataOffset);

    progress.advance(75, "Writing output");
    if (!indexFile.isEmpty())
        buildIndexImage();
    // Write previews
    writeRawData();
    file.seekp(0);
    TiffHeader().write(file);
    mainIFD.write(file, false);
    rawIFD.write(file, false);
    file.close();

    copyMetadata(filename);
    progress.advance(100, "Done writing!");
}


void DngFloatWriter::renderPreviews() {
    QImage halfInterpolated(width, height, QImage::Format_RGB32);
    // Make the half size interpolation, and apply tone curve
    
}


void DngFloatWriter::copyMetadata(const string & filename) {
    try {
        Exiv2::Image::AutoPtr src = Exiv2::ImageFactory::open(stack.getImage(stack.size() - 1).getMetaData().fileName);
        src->readMetadata();
        Exiv2::Image::AutoPtr result = Exiv2::ImageFactory::open(filename);
        result->readMetadata();

        const Exiv2::XmpData & srcXmp = src->xmpData();
        Exiv2::XmpData & dstXmp = result->xmpData();
        for (const auto & datum : srcXmp) {
            if (dstXmp.findKey(Exiv2::XmpKey(datum.key())) == dstXmp.end()) {
                dstXmp.add(datum);
            }
        }

        const Exiv2::IptcData & srcIptc = src->iptcData();
        Exiv2::IptcData & dstIptc = result->iptcData();
        for (const auto & datum : srcIptc) {
            if (dstIptc.findKey(Exiv2::IptcKey(datum.key())) == dstIptc.end()) {
                dstIptc.add(datum);
            }
        }

        const Exiv2::ExifData & srcExif = src->exifData();
        Exiv2::ExifData & dstExif = result->exifData();
        for (const auto & datum : srcExif) {
            if (datum.groupName() != "Image" && datum.groupName().substr(0, 8) != "SubImage"
                    && dstExif.findKey(Exiv2::ExifKey(datum.key())) == dstExif.end()) {
                dstExif.add(datum);
            }
        }

        result->writeMetadata();
    }
    catch (Exiv2::Error& e) {
        std::cerr << "Exiv2 error: " << e.what() << std::endl;
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
        if (((union { uint32_t x; uint8_t c; }){1}).c) {
            for (size_t col = 0; col < tileWidth; ++col) {
                for (size_t byte = 0; byte < bytesps; ++byte)
                    dst[col + realTileWidth*(bytesps-byte-1)] = src[col*bytesps + byte];  // Little endian
            }
        } else {
            for (size_t col = 0; col < tileWidth; ++col) {
                for (size_t byte = 0; byte < bytesps; ++byte)
                    dst[col + realTileWidth*byte] = src[col*bytesps + byte];
            }
        }
    }
    // EncodeDeltaBytes
    for (size_t col = realTileWidth*bytesps - 1; col >= factor; --col) {
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
        for (int index = 0; index < tileWidth; ++index) {
            dst16[index] = DNG_FloatToHalf(dst32[index]);
        }
    } else if (bytesps == 3) {
        uint8_t  * dst8  = (uint8_t *)  dst;
        uint32_t * dst32 = (uint32_t *) dst;
        for (int index = 0; index < tileWidth; ++index) {
            DNG_FloatToFP24(dst32[index], dst8);
            dst8 += 3;
        }
    }
}


void DngFloatWriter::writeRawData() {
    size_t tileCount = tilesAcross * tilesDown;
    uint32_t tileOffsets[tileCount];
    uint32_t tileBytes[tileCount];
    uLongf dstLen = tileWidth * tileLength * 4;
    int bytesps = bps >> 3;

//     #pragma omp parallel
    {
        Bytef cBuffer[dstLen];
        Bytef uBuffer[dstLen];

//         #pragma omp for collapse(2)
        for (size_t y = 0; y < height; y += tileLength) {
            for (size_t x = 0; x < width; x += tileWidth) {
                size_t t = (y / tileLength) * tilesAcross + (x / tileWidth);
                fill_n(uBuffer, dstLen, 0);
                size_t thisTileLength = y + tileLength > height ? height - y : tileLength;
                size_t thisTileWidth = x + tileWidth > width ? width - x : tileWidth;
                for (size_t row = 0; row < thisTileLength; ++row) {
                    Bytef * dst = uBuffer + row*tileWidth*bytesps;
                    Bytef * src = (Bytef *)&rawData[(y+row)*width + x];
                    compressFloats(src, thisTileWidth, bytesps);
                    encodeFPDeltaRow(src, dst, thisTileWidth, tileWidth, bytesps, 2);
                }
                uLongf conpressedLength = dstLen;
                int err = compress(cBuffer, &conpressedLength, uBuffer, dstLen);
                tileBytes[t] = conpressedLength;
                if (err != Z_OK) {
                    std::cerr << "DNG Deflate: Failed compressing tile " << t << ", with error " << err << std::endl;
                } else {  // Floating point data
//                     #pragma omp critical
                    {
                        tileOffsets[t] = file.tellp();
                        file.write((const char *)cBuffer, tileBytes[t]);
                    }
                }
            }
        }
    }

    rawIFD.setValue(TILEOFFSETS, tileOffsets);
    rawIFD.setValue(TILEBYTES, tileBytes);
}

} // namespace hdrmerge
