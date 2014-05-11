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

#include <algorithm>
#include "TiffDirectory.hpp"
using namespace hdrmerge;

void IFD::addEntry(uint16_t tag, uint16_t type, uint32_t count, const void * data) {
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


void IFD::setValue(DirEntry * entry, const void * data) {
    const uint8_t * castedData = reinterpret_cast<const uint8_t *>(data);
    size_t dataSize = entry->dataSize();
    if (dataSize > 4) {
        std::copy_n(castedData, dataSize, &entryData[entry->offset]);
    } else {
        std::copy_n(castedData, dataSize, (uint8_t *)&entry->offset);
    }
}


void IFD::write(std::ostream & file, bool hasNext) {
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


size_t IFD::length() const {
    return 6 + 12*entries.size() + entryData.size();
}


IFD::DirEntry * IFD::getEntry(uint16_t tag) {
    auto it = entries.begin();
    while (it != entries.end() && it->tag != tag) it++;
    return it == entries.end() ? nullptr : &(*it);
}
