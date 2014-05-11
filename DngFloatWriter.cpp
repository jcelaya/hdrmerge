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
#include <QImage>
#include <QBuffer>
#include <zlib.h>
#include "config.h"
#include "DngFloatWriter.hpp"
using namespace std;


namespace hdrmerge {

DngFloatWriter::DngFloatWriter(const ImageStack & s, ProgressIndicator & pi)
    : progress(pi), stack(s), appVersion("HDRMerge " HDRMERGE_VERSION_STRING),
    width(stack.getWidth()), height(stack.getHeight()), previewWidth(width), bps(16) {}


void DngFloatWriter::IFD::addEntry(uint16_t tag, uint16_t type, uint32_t count, const void * data) {
    uint32_t newSize = entryData.size();
    DirEntry * entry = &(*entries.insert(entries.end(), DirEntry({tag, type, count, newSize})));
    uint32_t dataSize = entry->dataSize();
    if (dataSize > 4) {
        newSize += dataSize;
        if (newSize & 1)
            ++newSize;
        entryData.resize(newSize);
    }
    setValue(entry, data);
}


void DngFloatWriter::IFD::setValue(DirEntry * entry, const void * data) {
    const uint8_t * castedData = reinterpret_cast<const uint8_t *>(data);
    size_t dataSize = entry->dataSize();
    if (dataSize > 4) {
        std::copy_n(castedData, dataSize, &entryData[entry->offset]);
    } else {
        std::copy_n(castedData, dataSize, (uint8_t *)&entry->offset);
    }
}


void DngFloatWriter::IFD::write(ofstream & file, bool hasNext) {
    uint16_t numEntries = entries.size();
    uint32_t offsetData = (uint32_t)file.tellp() + 12*numEntries + 6;
    sort(entries.begin(), entries.end());
    uint32_t offsetNext = hasNext ? offsetData + entryData.size() : 0;
    for (auto & entry : entries) {
        if (entry.dataSize() > 4) {
            entry.offset += offsetData;
        }
    }
    file.write((const char *)&numEntries, 2);
    file.write((const char *)&entries[0], 12*numEntries);
    file.write((const char *)&offsetNext, 4);
    file.write((const char *)&entryData[0], entryData.size());
}


size_t DngFloatWriter::IFD::length() const {
    return 6 + 12*entries.size() + entryData.size();
}


DngFloatWriter::DirEntry * DngFloatWriter::IFD::getEntry(uint16_t tag) {
    auto it = entries.begin();
    while (it != entries.end() && it->tag != tag) it++;
    return it == entries.end() ? nullptr : &(*it);
}


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

} Tag;


void DngFloatWriter::createMainIFD() {
    uint8_t dngVersion[] = { 1, 4, 0, 0 };
    mainIFD.addEntry(DNGVERSION, BYTE, 4, dngVersion);
    mainIFD.addEntry(DNGBACKVERSION, BYTE, 4, dngVersion);
    // Thumbnail set thumbnail->AddTagSet (mainIFD)

    // Profile
    const MetaData & md = stack.getImage(0).getMetaData();
    mainIFD.addEntry(CALIBRATIONILLUMINANT, SHORT, 21); // D65
    string profName(md.maker + " " + md.model);
    mainIFD.addEntry(PROFILENAME, ASCII, profName.length() + 1, profName.c_str());
    int32_t colorMatrix[18];
    for (int im = 0, id = 0; im < 9; ++im, ++id) {
        colorMatrix[id] = std::round(md.camXyz[0][im] * 10000.0f);
        colorMatrix[++id] = 10000;
    }
    mainIFD.addEntry(COLORMATRIX, SRATIONAL, 9, colorMatrix);

    // Color
    uint32_t analogBalance[] = { 1, 1, 1, 1, 1, 1 };
    mainIFD.addEntry(ANALOGBALANCE, RATIONAL, 3, analogBalance);
    double minWb = std::min(md.camMul[0], std::min(md.camMul[1], md.camMul[2]));
    double wb[] = { minWb/md.camMul[0], minWb/md.camMul[1], minWb/md.camMul[2] };
    uint32_t cameraNeutral[] = {
        (uint32_t)std::round(1000000.0 * wb[0]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[1]), 1000000,
        (uint32_t)std::round(1000000.0 * wb[2]), 1000000};
    mainIFD.addEntry(CAMERANEUTRAL, RATIONAL, 3, cameraNeutral);

    mainIFD.addEntry(ORIENTATION, SHORT, md.flip);
    string cameraName(md.maker + " " + md.model);
    mainIFD.addEntry(UNIQUENAME, ASCII, cameraName.length() + 1, &cameraName[0]);
    // TODO: Add Digest and Unique ID
    // TODO: Add XMP and Exif and private data
    mainIFD.addEntry(SUBIFDS, LONG, 1);
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
    rawIFD.addEntry(NEWSUBFILETYPE, LONG, 0);
    rawIFD.addEntry(IMAGEWIDTH, LONG, width);
    rawIFD.addEntry(IMAGELENGTH, LONG, height);
    rawIFD.addEntry(SAMPLESPERPIXEL, SHORT, 1);
    rawIFD.addEntry(BITSPERSAMPLE, SHORT, bps);
    if (bps == 24) {
        rawIFD.addEntry(FILLORDER, SHORT, 1);
    }
    rawIFD.addEntry(PLANARCONFIG, SHORT, 1);
    rawIFD.addEntry(COMPRESSION, SHORT, 8); // Deflate
    rawIFD.addEntry(PREDICTOR, SHORT, 34894); //FP2X
    rawIFD.addEntry(SAMPLEFORMAT, SHORT, 3); // Floating Point

    calculateTiles();
    uint32_t numTiles = tilesAcross * tilesDown;
    uint32_t buffer[numTiles];
    rawIFD.addEntry(TILEWIDTH, LONG, tileWidth);
    rawIFD.addEntry(TILELENGTH, LONG, tileLength);
    rawIFD.addEntry(TILEOFFSETS, LONG, numTiles, buffer);
    rawIFD.addEntry(TILEBYTES, LONG, numTiles, buffer);

    rawIFD.addEntry(WHITELEVEL, SHORT, 1);
    rawIFD.addEntry(PHOTOINTERPRETATION, SHORT, 32803); // CFA
    uint16_t cfaPatternDim[] = { 2, 2 };
    rawIFD.addEntry(CFAPATTERNDIM, SHORT, 2, cfaPatternDim);
    const MetaData & md = stack.getImage(0).getMetaData();
    uint8_t cfaPattern[] = { md.FC(0, 0), md.FC(0, 1), md.FC(1, 0), md.FC(1, 1) };
    for (uint8_t & i : cfaPattern) {
        if (i == 3) i = 1;
    }
    rawIFD.addEntry(CFAPATTERN, BYTE, 4, cfaPattern);
    uint8_t cfaPlaneColor[] = { 0, 1, 2 };
    rawIFD.addEntry(CFAPLANECOLOR, BYTE, 3, cfaPlaneColor);
    rawIFD.addEntry(CFALAYOUT, SHORT, 1);
}


struct TiffHeader {
    union {
        uint32_t endian;
        struct {
            uint16_t endian;
            uint16_t magic;
        } sep;
    };
    uint32_t offset;
    // It sets the first two bytes to their correct value, given the architecture
    TiffHeader() : endian(0x4D4D4949), offset(8) { sep.magic = 42; }
    void write(ofstream & file) const { file.write((const char *)this, 8); }
};


void DngFloatWriter::write(const string & filename) {
    file.open(filename, ios_base::binary);

    createMainIFD();
    createRawIFD();
    size_t rawIFDOffset = 8 + mainIFD.length();
    mainIFD.setValue(SUBIFDS, rawIFDOffset);
    size_t dataOffset = rawIFDOffset + rawIFD.length();
    file.seekp(dataOffset);
    // Write previews
    writeRawData();
    file.seekp(0);
    TiffHeader().write(file);
    mainIFD.write(file, false);
    rawIFD.write(file, false);
    file.close();
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
    unique_ptr<float[]> rawData(new float[width * height]);
    stack.compose(rawData.get());
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
