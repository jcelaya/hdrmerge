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

#include <exiv2/exiv2.hpp>
#include "ExifTransfer.hpp"
#include "Log.hpp"
using namespace hdrmerge;
using namespace Exiv2;
using namespace std;


void ExifTransfer::copyMetadata() {
    try {
        src = Exiv2::ImageFactory::open(srcFile);
        src->readMetadata();
        dst = Exiv2::ImageFactory::open(dstFile);
        dst->readMetadata();
        copyXMP();
        copyIPTC();
        copyEXIF();
        dst->writeMetadata();
    } catch (Exiv2::Error & e) {
        std::cerr << "Exiv2 error: " << e.what() << std::endl;
    }
}


void ExifTransfer::copyXMP() {
    const Exiv2::XmpData & srcXmp = src->xmpData();
    Exiv2::XmpData & dstXmp = dst->xmpData();
    for (const auto & datum : srcXmp) {
        if (datum.groupName() != "tiff" && dstXmp.findKey(Exiv2::XmpKey(datum.key())) == dstXmp.end()) {
            dstXmp.add(datum);
        } else {
            Log::msg(Log::DEBUG, "Ignore ", datum.key());
        }
    }
}


void ExifTransfer::copyIPTC() {
    const Exiv2::IptcData & srcIptc = src->iptcData();
    Exiv2::IptcData & dstIptc = dst->iptcData();
    for (const auto & datum : srcIptc) {
        if (dstIptc.findKey(Exiv2::IptcKey(datum.key())) == dstIptc.end()) {
            dstIptc.add(datum);
        } else {
            Log::msg(Log::DEBUG, "Ignore ", datum.key());
        }
    }
}


static bool excludeExifDatum(const Exifdatum & datum) {
    const char * previewKeys[] {
        "Exif.OlympusCs.PreviewImageStart",
        "Exif.OlympusCs.PreviewImageLength",
        "Exif.Thumbnail.JPEGInterchangeFormat",
        "Exif.Thumbnail.JPEGInterchangeFormatLength",
        "Exif.NikonPreview.JPEGInterchangeFormat",
        "Exif.NikonPreview.JPEGInterchangeFormatLength",
        "Exif.Pentax.PreviewOffset",
        "Exif.Pentax.PreviewLength",
        "Exif.PentaxDng.PreviewOffset",
        "Exif.PentaxDng.PreviewLength",
        "Exif.Minolta.ThumbnailOffset",
        "Exif.Minolta.ThumbnailLength",
        "Exif.SonyMinolta.ThumbnailOffset",
        "Exif.SonyMinolta.ThumbnailLength",
        "Exif.Olympus.ThumbnailImage",
        "Exif.Olympus2.ThumbnailImage",
        "Exif.Minolta.Thumbnail",
        "Exif.PanasonicRaw.PreviewImage",
        "Exif.SamsungPreview.JPEGInterchangeFormat",
        "Exif.SamsungPreview.JPEGInterchangeFormatLength"
    };
    for (const char * pkey : previewKeys) {
        if (datum.key() == pkey) {
            return true;
        }
    }
    return
        datum.groupName().substr(0, 5) == "Thumb" ||
        datum.groupName().substr(0, 8) == "SubThumb" ||
        datum.groupName().substr(0, 5) == "Image" ||
        datum.groupName().substr(0, 8) == "SubImage";
}


void ExifTransfer::copyEXIF() {
    const Exiv2::ExifData & srcExif = src->exifData();
    Exiv2::ExifData & dstExif = dst->exifData();

    // Correct Make and Model, from the imput files
    // It is needed so that makernote tags are correctly copied
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
        if (!excludeExifDatum(datum) && dstExif.findKey(Exiv2::ExifKey(datum.key())) == dstExif.end()) {
            dstExif.add(datum);
        } else {
            Log::msg(Log::DEBUG, "Ignore ", datum.key());
        }
    }
}
