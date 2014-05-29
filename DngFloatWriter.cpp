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

#include <QBuffer>
#include <QDateTime>
#include <QImageWriter>
#include <zlib.h>
#include <exiv2/exiv2.hpp>
#include "config.h"
#include "DngFloatWriter.hpp"
#include "Renderer.hpp"
#include "Log.hpp"
using namespace std;


namespace hdrmerge {

DngFloatWriter::DngFloatWriter(ProgressIndicator & pi)
    : progress(pi), previewWidth(0), bps(16) {}


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
} Tag;


void DngFloatWriter::write(Array2D<float> && rawPixels, const MetaData & md, const string & filename) {
    metaData = &md;
    rawData = std::move(rawPixels);
    width = rawData.getWidth();
    height = rawData.getHeight();

    progress.advance(25, "Rendering preview");
    renderPreviews();

    progress.advance(50, "Initialize metadata");
    file.open(filename, ios_base::binary);
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
    file.seekp(dataOffset);

    progress.advance(75, "Writing output");
    Timer t("Write output");
    writePreviews();
    writeRawData();
    file.seekp(0);
    TiffHeader().write(file);
    mainIFD.write(file, false);
    rawIFD.write(file, false);
    if (previewWidth > 0) {
        previewIFD.write(file, false);
    }
    file.close();

    copyMetadata(metaData->fileName, filename);
    progress.advance(100, "Done writing!");
}


void DngFloatWriter::createMainIFD() {
    uint8_t dngVersion[] = { 1, 4, 0, 0 };
    mainIFD.addEntry(DNGVERSION, IFD::BYTE, 4, dngVersion);
    mainIFD.addEntry(DNGBACKVERSION, IFD::BYTE, 4, dngVersion);
    uint8_t tiffep[] = { 1, 0, 0, 0 };
    mainIFD.addEntry(TIFFEPSTD, IFD::BYTE, 4, tiffep);
    mainIFD.addEntry(MAKE, IFD::ASCII, metaData->maker.length() + 1, metaData->maker.c_str());
    mainIFD.addEntry(MODEL, IFD::ASCII, metaData->model.length() + 1, metaData->model.c_str());
    std::string appVersion("HDRMerge " HDRMERGE_VERSION_STRING);
    mainIFD.addEntry(SOFTWARE, IFD::ASCII, appVersion.length() + 1, appVersion.c_str());
    mainIFD.addEntry(RESOLUTIONUNIT, IFD::SHORT, 3); // Cm
    uint32_t resolution[] = { 100, 1 };
    mainIFD.addEntry(XRESOLUTION, IFD::RATIONAL, 1, resolution);
    mainIFD.addEntry(YRESOLUTION, IFD::RATIONAL, 1, resolution);
    char empty[] = { 0 };
    mainIFD.addEntry(COPYRIGHT, IFD::ASCII, 1, empty);
    mainIFD.addEntry(IMAGEDESCRIPTION, IFD::ASCII, metaData->description.length() + 1, metaData->description.c_str());
    QDateTime currentTime = QDateTime::currentDateTime();
    QString currentTimeText = currentTime.toString("yyyy:MM:dd hh:mm:ss");
    mainIFD.addEntry(DATETIME, IFD::ASCII, 20, currentTimeText.toAscii().constData());
    mainIFD.addEntry(DATETIMEORIGINAL, IFD::ASCII, metaData->dateTime.length() + 1, metaData->dateTime.c_str());

    // Profile
    mainIFD.addEntry(CALIBRATIONILLUMINANT, IFD::SHORT, 21); // D65
    string profName(metaData->maker + " " + metaData->model);
    mainIFD.addEntry(PROFILENAME, IFD::ASCII, profName.length() + 1, profName.c_str());
    int32_t colorMatrix[18];
    for (int row = 0, i = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            colorMatrix[i++] = std::round(metaData->camXyz[row][col] * 10000.0f);
            colorMatrix[i++] = 10000;
        }
    }
    mainIFD.addEntry(COLORMATRIX, IFD::SRATIONAL, 9, colorMatrix);

    // Color
    uint32_t analogBalance[] = { 1, 1, 1, 1, 1, 1 };
    mainIFD.addEntry(ANALOGBALANCE, IFD::RATIONAL, 3, analogBalance);
    double minWb = std::min(metaData->camMul[0], std::min(metaData->camMul[1], metaData->camMul[2]));
    double wb[] = { minWb/metaData->camMul[0], minWb/metaData->camMul[1], minWb/metaData->camMul[2] };
    uint32_t cameraNeutral[] = {
        (uint32_t)std::round(1000000.0 * wb[0]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[1]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[2]), 1000000};
    mainIFD.addEntry(CAMERANEUTRAL, IFD::RATIONAL, 3, cameraNeutral);

    mainIFD.addEntry(ORIENTATION, IFD::SHORT, metaData->tiffOrientation);
    string cameraName(metaData->maker + " " + metaData->model);
    mainIFD.addEntry(UNIQUENAME, IFD::ASCII, cameraName.length() + 1, &cameraName[0]);
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
    mainIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, 2); // RGB
    mainIFD.addEntry(COMPRESSION, IFD::SHORT, 1); // Uncompressed
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
    rawIFD.addEntry(NEWSUBFILETYPE, IFD::LONG, 0);
    rawIFD.addEntry(IMAGEWIDTH, IFD::LONG, width);
    rawIFD.addEntry(IMAGELENGTH, IFD::LONG, height);
    uint32_t aa[] = { 0, 0, height, width };
    rawIFD.addEntry(ACTIVEAREA, IFD::LONG, 4, aa);
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
    uint8_t cfaPattern[] = { metaData->FC(0, 0), metaData->FC(1, 0), metaData->FC(0, 1), metaData->FC(1, 1) };
    for (uint8_t & i : cfaPattern) {
        if (i == 3) i = 1;
    }
    rawIFD.addEntry(CFAPATTERN, IFD::BYTE, 4, cfaPattern);
    uint8_t cfaPlaneColor[] = { 0, 1, 2 };
    rawIFD.addEntry(CFAPLANECOLOR, IFD::BYTE, 3, cfaPlaneColor);
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
    previewIFD.addEntry(PHOTOINTERPRETATION, IFD::SHORT, 6); // YCbCr
    previewIFD.addEntry(COMPRESSION, IFD::SHORT, 7); // JPEG
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
    Renderer r(rawData, *metaData);
    r.process();
    thumbnail = r.getScaledVersion(256).convertToFormat(QImage::Format_RGB888);
    if (previewWidth > 0) {
        preview = r.getScaledVersion(previewWidth);
        QBuffer buffer(&jpegPreviewData);
        buffer.open(QIODevice::WriteOnly);
        QImageWriter writer(&buffer, "JPEG");
        writer.setQuality(90);
        if (!writer.write(preview)) {
            cerr << "Error converting the preview to JPEG: " << writer.errorString();
            previewWidth = 0;
        }
    }
}


void DngFloatWriter::copyMetadata(const std::string & srcFile, const std::string & dstFile) {
    try {
        Exiv2::Image::AutoPtr src = Exiv2::ImageFactory::open(srcFile);
        src->readMetadata();
        Exiv2::Image::AutoPtr result = Exiv2::ImageFactory::open(dstFile);
        result->readMetadata();

        const Exiv2::XmpData & srcXmp = src->xmpData();
        Exiv2::XmpData & dstXmp = result->xmpData();
        Log::msg(Log::DEBUG, "Copying XMP metadata");
        for (const auto & datum : srcXmp) {
            if (datum.groupName() != "tiff" && dstXmp.findKey(Exiv2::XmpKey(datum.key())) == dstXmp.end()) {
                Log::msg(Log::DEBUG, "Add ", datum.key());
                dstXmp.add(datum);
            } else {
                Log::msg(Log::DEBUG, "Ignore ", datum.key());
            }
        }

        const Exiv2::IptcData & srcIptc = src->iptcData();
        Exiv2::IptcData & dstIptc = result->iptcData();
        Log::msg(Log::DEBUG, "Copying IPTC metadata");
        for (const auto & datum : srcIptc) {
            if (dstIptc.findKey(Exiv2::IptcKey(datum.key())) == dstIptc.end()) {
                Log::msg(Log::DEBUG, "Add ", datum.key());
                dstIptc.add(datum);
            } else {
                Log::msg(Log::DEBUG, "Ignore ", datum.key());
            }
        }

        const Exiv2::ExifData & srcExif = src->exifData();
        Exiv2::ExifData & dstExif = result->exifData();
        Log::msg(Log::DEBUG, "Copying EXIF metadata");
        // Correct Make and Model
        auto makeIterator = srcExif.findKey(Exiv2::ExifKey("Exif.Image.Make"));
        if (makeIterator != srcExif.end()) {
            Log::msg(Log::DEBUG, "Reset Exif.Image.Make to ", makeIterator->toString());
            dstExif["Exif.Image.Make"] = makeIterator->toString();
        }
        auto modelIterator = srcExif.findKey(Exiv2::ExifKey("Exif.Image.Model"));
        if (modelIterator != srcExif.end()) {
            Log::msg(Log::DEBUG, "Reset Exif.Image.Model to ", modelIterator->toString());
            dstExif["Exif.Image.Model"] = modelIterator->toString();
        }
        for (const auto & datum : srcExif) {
            if (datum.groupName() != "Thumbnail" &&
                    datum.groupName().substr(0, 5) != "Image" &&
                    datum.groupName().substr(0, 8) != "SubImage" &&
                    dstExif.findKey(Exiv2::ExifKey(datum.key())) == dstExif.end()) {
                Log::msg(Log::DEBUG, "Add ", datum.key());
                dstExif.add(datum);
            } else {
                Log::msg(Log::DEBUG, "Ignore ", datum.key());
            }
        }

        result->writeMetadata();
    }
    catch (Exiv2::Error& e) {
        std::cerr << "Exiv2 error: " << e.what() << std::endl;
    }
}


void DngFloatWriter::writePreviews() {
    size_t thumbsize = thumbnail.width() * thumbnail.height() * 3;
    mainIFD.setValue(STRIPBYTES, thumbsize);
    mainIFD.setValue(STRIPOFFSETS, file.tellp());
    file.write((const char *)thumbnail.bits(), thumbsize);
    if (previewWidth > 0) {
        previewIFD.setValue(STRIPBYTES, jpegPreviewData.size());
        previewIFD.setValue(STRIPOFFSETS, file.tellp());
        file.write(jpegPreviewData.constData(), jpegPreviewData.size());
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


void DngFloatWriter::writeRawData() {
    size_t tileCount = tilesAcross * tilesDown;
    uint32_t tileOffsets[tileCount];
    uint32_t tileBytes[tileCount];
    int bytesps = bps >> 3;
    uLongf dstLen = tileWidth * tileLength * bytesps;

    #pragma omp parallel
    {
        Bytef cBuffer[dstLen];
        Bytef uBuffer[dstLen];

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
