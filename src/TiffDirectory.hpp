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

#include <vector>
#include <string>

#ifndef _TIFFDIRECTORY_HPP_
#define _TIFFDIRECTORY_HPP_

namespace hdrmerge {

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
    void write(uint8_t * buffer, size_t & pos);
};

class IFD {
public:
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
    void addEntry(uint16_t tag, const std::string &str) {
        addEntry(tag, ASCII, str.length() + 1, str.c_str());
    }
    void write(uint8_t * buffer, size_t & pos, bool hasNext);
    size_t length() const;

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

    std::vector<DirEntry> entries;
    std::vector<uint8_t> entryData;

    DirEntry * getEntry(uint16_t tag);
    void setValue(DirEntry * entry, const void * data);
    template <typename T> void setValue(DirEntry * entry, const T & value) {
        if (entry->dataSize() > 4) {
            setValue(entry, (const void *)&value);
        } else {
            union {
                float floatValue;
                uint32_t longValue;
                uint16_t shortValue;
                uint8_t byteValue;
            } u;
            switch (entry->type) {
                case BYTE: case ASCII: case SBYTE: case UNDEFINED:
                    u.byteValue = (uint8_t) value; break;
                case SHORT: case SSHORT:
                    u.shortValue = (uint16_t) value; break;
                case LONG: case SLONG:
                    u.longValue = (uint32_t)value; break;
                case FLOAT:
                    u.floatValue = (float)value; break;
            }
            entry->offset = u.longValue;
        }
    }
};

} // namespace hdrmerge

#endif // _TIFFDIRECTORY_HPP_
