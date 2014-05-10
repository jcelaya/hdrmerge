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
#include "config.h"
#include "DngFloatWriter.hpp"
using namespace std;


namespace hdrmerge {

DngFloatWriter::DngFloatWriter(const ImageStack & s, ProgressIndicator & pi)
    : progress(pi), stack(s), appVersion("HDRMerge " HDRMERGE_VERSION_STRING),
    previewWidth(s.getWidth()), bps(16) {}


enum {
    DNGVERSION = 50706,
    DNGBACKVERSION = 50707,
    CALIBRATIONILLUMINANT = 50778,
    COLORMATRIX = 50721,
    PROFILENAME = 50936,
} Tag;


enum {
    BYTE = 1,
    ASCII,
    SHORT,
    LONG,
    RATIONAL,
    SBYTE,
    UNDEFINED,
    SSHORT,
    SLONG,
    SRATIONAL,
    FLOAT,
    DOUBLE
} Type;


void DngFloatWriter::IFD::addEntry(uint16_t tag, uint16_t type, uint32_t count, const void * data) {
    const uint8_t * castedData = reinterpret_cast<const uint8_t *>(data);
    entries.push_back(DirEntry({tag, type, count, 0}));
    DirEntry & entry = entries.back();
    size_t dataSize = entry.dataSize();
    if (dataSize > 4) {
        entry.offset = entryData.size();
        for (size_t i = 0; i < dataSize; ++i)
            entryData.push_back(castedData[i]);
        if (entryData.size() & 1) {
            entryData.push_back(0);
        }
    } else {
        std::copy_n(castedData, dataSize, (uint8_t *)&entry.offset);
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



void DngFloatWriter::createIFD() {
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

    // Create IFD, maybe subIFDs too
    createIFD();
    // Measure IFDs size plus header
    size_t dataOffset = 8 + mainIFD.length() + rawIFD.length();
    // Seek file to that offset
    // Write previews
    // Write raw data
    // Seek file to origin, write IFDs
    TiffHeader().write(file);
    mainIFD.write(file, false);
    file.close();
}

} // namespace hdrmerge
