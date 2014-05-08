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

#ifndef _DNGWRITER_HPP_
#define _DNGWRITER_HPP_

#include <string>
#include <QString>
#include "config.h"
#include "dng_memory.h"
#include "dng_host.h"
#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_date_time.h"
#include "ImageStack.hpp"
#include "ProgressIndicator.hpp"

namespace hdrmerge {

class DngWriter {
public:
    DngWriter(const ImageStack & s, ProgressIndicator & pi);

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
    class BasicHost : public dng_host {
    public:
        BasicHost(dng_memory_allocator *allocator) : dng_host(allocator, NULL) {}
        dng_image * Make_dng_image(const dng_rect &bounds, uint32 planes, uint32 pixelType);
    };

    class DummyNegative : public dng_negative {
    public:
        DummyNegative(dng_host & host) : dng_negative(host) {}
    };

    ProgressIndicator & progress;
    dng_memory_allocator memalloc;
    BasicHost host;
    DummyNegative negative;
    dng_preview_list previewList;
    dng_date_time_info dateTimeNow;
    const ImageStack & stack;
    const std::string appVersion;

    size_t previewWidth;
    int bps;
    QString indexFile;

    void buildNegative();
    void buildPreviewList();
    void buildExifMetadata();
    void addJpegPreview();
    void buildIndexImage();
};

} // namespace hdrmerge

#endif // _DNGWRITER_HPP_
