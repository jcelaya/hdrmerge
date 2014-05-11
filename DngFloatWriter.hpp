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
#include <vector>
#include <QString>
#include "config.h"
#include "ImageStack.hpp"
#include "ProgressIndicator.hpp"

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
    void setIndexFileName(const QString & name) {
        indexFile = name;
    }
    void write(const std::string & filename);

private:
    struct DirEntry {
        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t offset;
        int bytesPerValue() const {
            static int bytesPerValue[12] =
            { 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
            return bytesPerValue[type - 1];
        }
        uint32_t dataSize() const {
            return count * bytesPerValue();
        }
        bool operator<(const DirEntry & r) const {
            return tag < r.tag;
        }
    };

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

    class IFD {
    public:
        template <typename T> void addEntry(uint16_t tag, uint16_t type, const T & value) {
            DirEntry * entry = &(*entries.insert(entries.end(), DirEntry({tag, type, 1, 0})));
            setValue(entry, value);
        }
        template <typename T> void setValue(uint16_t tag, const T * value) {
            DirEntry * entry = getEntry(tag);
            if (entry) {
                setValue(entry, (const void *)value);
            }
        }
        template <typename T> void setValue(uint16_t tag, const T & value) {
            DirEntry * entry = getEntry(tag);
            if (entry) {
                setValue(entry, value);
            }
        }
        void addEntry(uint16_t tag, uint16_t type, uint32_t count, const void * data);
        void write(std::ofstream & file, bool hasNext);
        size_t length() const;

    private:
        std::vector<DirEntry> entries;
        std::vector<uint8_t> entryData;

        DirEntry * getEntry(uint16_t tag);
        void setValue(DirEntry * entry, const void * data);
        template <typename T> void setValue(DirEntry * entry, const T & value) {
            if (sizeof(T) > 4) {
                setValue(entry, (const void *)&value);
            } else {
                uint32_t leftJustified = 0;
                switch (entry->type) {
                    case BYTE: case ASCII: case SBYTE: case UNDEFINED:
                        *((uint8_t *)&leftJustified) = (uint8_t) value; break;
                    case SHORT: case SSHORT:
                        *((uint16_t *)&leftJustified) = (uint16_t) value; break;
                    case LONG: case SLONG: case FLOAT:
                        leftJustified = *((uint32_t *)&value); break;
                }
                entry->offset = leftJustified;
            }
        }
    };

    ProgressIndicator & progress;
    std::ofstream file;
    const ImageStack & stack;
    const std::string appVersion;
    IFD mainIFD, rawIFD, previewIFD;
    size_t width, height;
    size_t tileWidth, tileLength;
    size_t tilesAcross, tilesDown;

    size_t previewWidth;
    int bps;
    QString indexFile;

    void createMainIFD();
    void createRawIFD();
    void calculateTiles();
    void writeRawData();
};

} // namespace hdrmerge

#endif // _DNGFLOATWRITER_HPP_
