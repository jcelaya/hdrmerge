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

#include <QImage>
#include <QBuffer>
#include "config.h"
#include "DngWriter.hpp"
#include "dng_image.h"
#include "dng_rect.h"
#include "dng_auto_ptr.h"
#include "dng_xmp_sdk.h"
#include "dng_xmp.h"
#include "dng_exif.h"
#include "dng_tag_values.h"
#include "dng_file_stream.h"
#include "dng_memory_stream.h"
#include "dng_color_space.h"
#include "dng_image_writer.h"
#include "dng_camera_profile.h"
#include "dng_render.h"
#include "dng_simple_image.h"
#include "exiv2meta.h"
using namespace std;


namespace hdrmerge {

class DngFloatImage : public dng_image {
public:
    DngFloatImage(size_t w, size_t h, dng_memory_allocator & a)
    : dng_image(dng_rect(h, w), 1, ttFloat), allocator(a) {
        memory.Reset(allocator.Allocate(w*h*4));
        buffer.fArea = fBounds;
        buffer.fPlane  = 0;
        buffer.fPlanes = 1;
        buffer.fRowStep   = w;
        buffer.fColStep   = 1;
        buffer.fPlaneStep = 1;
        buffer.fPixelType = ttFloat;
        buffer.fPixelSize = 4;
        buffer.fData = memory->Buffer();
    }

    virtual dng_image * Clone () const {
        DngFloatImage * result = new DngFloatImage(fBounds.W(), fBounds.H(), allocator);
        result->buffer.CopyArea(buffer, fBounds, 0, 1);
        return result;
    }

    float * getImageBuffer() {
        return (float *)memory->Buffer();
    }

protected:
    virtual void AcquireTileBuffer(dng_tile_buffer & tBuffer, const dng_rect & area, bool dirty) const {
        (dng_pixel_buffer &)tBuffer = buffer;
        tBuffer.fArea = area;
        tBuffer.fData = (void *) buffer.ConstPixel(area.t, area.l);
        tBuffer.fDirty = dirty;
    }

private:
    dng_pixel_buffer buffer;
    AutoPtr<dng_memory_block> memory;
    dng_memory_allocator & allocator;
};


// This is the default behavior, but just in case it changes...
dng_image * DngWriter::BasicHost::Make_dng_image(const dng_rect &bounds, uint32 planes, uint32 pixelType) {
    dng_image * result = new dng_simple_image(bounds, planes, pixelType, Allocator());
    if (!result) {
        ThrowMemoryFull();
    }
    return result;
}


DngWriter::DngWriter(const ImageStack & s, ProgressIndicator & pi)
: progress(pi), memalloc(gDefaultDNGMemoryAllocator), host(&memalloc), negative(host),
stack(s), appVersion("HDRMerge " HDRMERGE_VERSION_STRING), previewWidth(s.getWidth()), bps(16) {
    host.SetSaveDNGVersion(dngVersion_SaveDefault);
    host.SetSaveLinearDNG(false);
    host.SetKeepOriginalFile(false);
    CurrentDateTimeAndZone(dateTimeNow);
}


void DngWriter::buildExifMetadata() {
    // Exif CFA Pattern
    const dng_mosaic_info * mosaicinfo = negative.GetMosaicInfo();
    if (mosaicinfo != nullptr) {
        dng_exif* exifData = negative.GetExif();
        exifData->fCFARepeatPatternCols = mosaicinfo->fCFAPatternSize.v;
        exifData->fCFARepeatPatternRows = mosaicinfo->fCFAPatternSize.h;
        for (uint16 c = 0; c < exifData->fCFARepeatPatternCols; c++) {
            for (uint16 r = 0; r < exifData->fCFARepeatPatternRows; r++) {
                exifData->fCFAPattern[r][c] = mosaicinfo->fCFAPattern[c][r];
            }
        }
    }

    dng_file_stream stream(stack.getImage(stack.size() - 1).getMetaData().fileName.c_str());
    Exiv2Meta exiv2Meta;
    exiv2Meta.Parse(host, stream);
    exiv2Meta.PostParse(host);

    // Exif Data
    dng_xmp xmpSync(memalloc);
    dng_exif * exifData = exiv2Meta.GetExif();
    exifData->fDateTime = dateTimeNow;
    exifData->fSoftware.Set_ASCII(appVersion.c_str());
    if (exifData != nullptr) {
        xmpSync.SyncExif(*exifData);
        AutoPtr<dng_memory_block> xmpBlock(xmpSync.Serialize());
        negative.SetXMP(host, xmpBlock->Buffer(), xmpBlock->LogicalSize());
        negative.SynchronizeMetadata();
    }

    // XMP Data
    dng_xmp* xmpData = exiv2Meta.GetXMP();
    if (xmpData != nullptr) {
        AutoPtr<dng_memory_block> xmpBlock(xmpData->Serialize());
        negative.SetXMP(host, xmpBlock->Buffer(), xmpBlock->LogicalSize(), false);
        negative.SynchronizeMetadata();
    }

    // Makernote backup.
    if ((exiv2Meta.MakerNoteLength() > 0) && (exiv2Meta.MakerNoteByteOrder().Length() == 2)) {
        dng_memory_stream streamPriv(memalloc);
        streamPriv.SetBigEndian();
        streamPriv.Put("Adobe", 5);
        streamPriv.Put_uint8(0x00);
        streamPriv.Put("MakN", 4);
        streamPriv.Put_uint32(exiv2Meta.MakerNoteLength() + exiv2Meta.MakerNoteByteOrder().Length() + 4);
        streamPriv.Put(exiv2Meta.MakerNoteByteOrder().Get(), exiv2Meta.MakerNoteByteOrder().Length());
        streamPriv.Put_uint32(exiv2Meta.MakerNoteOffset());
        streamPriv.Put(exiv2Meta.MakerNoteData(), exiv2Meta.MakerNoteLength());
        AutoPtr<dng_memory_block> blockPriv(host.Allocate(static_cast<uint32>(streamPriv.Length())));
        streamPriv.SetReadPosition(0);
        streamPriv.Get(blockPriv->Buffer(), static_cast<uint32>(streamPriv.Length()));
        negative.SetPrivateData(blockPriv);
    }

    negative.RebuildIPTC(true);
    negative.SetModelName(negative.GetExif()->fModel.Get());
}


void DngWriter::buildNegative() {
    const MetaData & md = stack.getImage(0).getMetaData();

    negative.SetDefaultScale(dng_urational(1, 1), dng_urational(1, 1));
    negative.SetDefaultCropOrigin(dng_urational(0, 1), dng_urational(0, 1));
    negative.SetDefaultCropSize(dng_urational(stack.getWidth(), 1), dng_urational(stack.getHeight(), 1));
    negative.SetActiveArea(dng_rect(stack.getHeight(), stack.getWidth()));
    negative.SetColorChannels(3); // RGBG
    negative.SetColorKeys(colorKeyRed, colorKeyGreen, colorKeyBlue, colorKeyGreen);

    uint32_t patterns[4] = {0xe1e1e1e1, 0xb4b4b4b4, 0x1e1e1e1e, 0x4b4b4b4b};
    uint32 bayerMosaic = 0;
    while (bayerMosaic < 4 && patterns[bayerMosaic] != md.filters) {
        ++bayerMosaic;
    }
    if (bayerMosaic < 4) {
        negative.SetBayerMosaic(bayerMosaic);
    }

    for (int i = 0; i < 4; ++i) {
        negative.SetWhiteLevel(1, i);
    }
    negative.SetBlackLevel(0, 0);

    negative.SetBaselineExposure(0.0);
    negative.SetBaselineNoise(1.0);
    negative.SetBaselineSharpness(1.0);
    switch (md.flip) {
        case 3: negative.SetBaseOrientation(dng_orientation::Rotate180()); break;
        case 5: negative.SetBaseOrientation(dng_orientation::Rotate90CCW()); break;
        case 6: negative.SetBaseOrientation(dng_orientation::Rotate90CW()); break;
        default: negative.SetBaseOrientation(dng_orientation::Normal()); break;
    }
    negative.SetAntiAliasStrength(dng_urational(100, 100));
    negative.SetLinearResponseLimit(1.0);
    negative.SetShadowScale(dng_urational(1, 1));
    negative.SetAnalogBalance(dng_vector_3(1.0, 1.0, 1.0));

    AutoPtr<dng_camera_profile> prof(new dng_camera_profile);
//     if (profilefilename != nullptr) {
//         dng_file_stream profStream(profilefilename);
//         prof->ParseExtended(profStream);
//     } else
    {
        string profName(md.maker + " " + md.model);
        prof->SetName(profName.c_str());
        dng_matrix_3by3 camXYZ;
        camXYZ[0][0] = md.camXyz[0][0];
        camXYZ[0][1] = md.camXyz[0][1];
        camXYZ[0][2] = md.camXyz[0][2];
        camXYZ[1][0] = md.camXyz[1][0];
        camXYZ[1][1] = md.camXyz[1][1];
        camXYZ[1][2] = md.camXyz[1][2];
        camXYZ[2][0] = md.camXyz[2][0];
        camXYZ[2][1] = md.camXyz[2][1];
        camXYZ[2][2] = md.camXyz[2][2];
        if (camXYZ.MaxEntry() == 0.0) {
            printf("Warning, camera XYZ Matrix is null\n");
            camXYZ = dng_matrix_3by3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
        }
        prof->SetColorMatrix1((dng_matrix) camXYZ);
        prof->SetCalibrationIlluminant1(lsD65);
    }
    negative.AddProfile(prof);
    dng_vector cameraNeutral(3);
    for (int c = 0; c < 3; ++c) {
        cameraNeutral[c] = 1.0 / md.camMul[c];
    }
    negative.SetCameraNeutral(cameraNeutral);

    buildExifMetadata();
}


void DngWriter::buildPreviewList() {
    negative.BuildStage2Image(host);
    negative.BuildStage3Image(host);

    AutoPtr<dng_preview> thumbnail(new dng_image_preview);
    dng_render thumbnail_render(host, negative);
    thumbnail_render.SetFinalSpace(dng_space_sRGB::Get());
    thumbnail_render.SetFinalPixelType(ttByte);
    thumbnail_render.SetMaximumSize(256);
    ((dng_image_preview *)thumbnail.Get())->fImage.Reset(thumbnail_render.Render());
    previewList.Append(thumbnail);

    if (previewWidth) {
        addJpegPreview();
    }
}


void DngWriter::addJpegPreview() {
    AutoPtr<dng_simple_image> renderedPreview;
    dng_render jpeg_render(host, negative);
    jpeg_render.SetFinalSpace(dng_space_sRGB::Get());
    jpeg_render.SetFinalPixelType(ttByte);
    renderedPreview.Reset((dng_simple_image *)jpeg_render.Render());
    dng_pixel_buffer pbuffer;
    renderedPreview->GetPixelBuffer(pbuffer);

    size_t width = pbuffer.fArea.W(), height = pbuffer.fArea.H();
    QImage pre_image(width, height, QImage::Format_RGB888);
    std::copy_n((uchar *)pbuffer.fData, width*height*3, pre_image.bits());
    pre_image = pre_image.scaledToWidth(previewWidth);
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pre_image.save(&buffer, "JPEG", 90);

    // Load JPEG preview file data in DNG preview container.
    AutoPtr<dng_jpeg_preview> jpeg_preview;
    jpeg_preview.Reset(new dng_jpeg_preview);
    jpeg_preview->fPhotometricInterpretation = piYCbCr;
    jpeg_preview->fYCbCrSubSampling          = dng_point(2, 2);
    jpeg_preview->fPreviewSize.v             = pre_image.height();
    jpeg_preview->fPreviewSize.h             = pre_image.width();
    jpeg_preview->fCompressedData.Reset(host.Allocate(ba.size()));
    jpeg_preview->fInfo.fApplicationName.Set_ASCII("HDRMerge");
    jpeg_preview->fInfo.fApplicationVersion.Set_ASCII(appVersion.c_str());
    jpeg_preview->fInfo.fDateTime = dateTimeNow.Encode_ISO_8601();
    jpeg_preview->fInfo.fColorSpace = previewColorSpace_sRGB;
    std::copy_n(ba.data(), ba.size(), jpeg_preview->fCompressedData->Buffer_char());

    AutoPtr<dng_preview> pp((dng_preview *)jpeg_preview.Release());
    previewList.Append(pp);
}


void DngWriter::buildIndexImage() {
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


void DngWriter::write(const string & filename) {
    progress.advance(0, "Initialize negative");
    dng_xmp_sdk::InitializeSDK();

    buildNegative();

    progress.advance(25, "Rendering image");
    AutoPtr<dng_image> imageData(new DngFloatImage(stack.getWidth(), stack.getHeight(), memalloc));
    stack.compose(((DngFloatImage *)imageData.Get())->getImageBuffer());
    negative.SetStage1Image(imageData);
    negative.SetRawFloatBitDepth(bps);

    if (!indexFile.isEmpty())
        buildIndexImage();

    progress.advance(50, "Rendering preview");
    buildPreviewList();

    progress.advance(75, "Writing output");
    dng_image_writer writer;
    dng_file_stream filestream(filename.c_str(), true);
    writer.WriteDNG(host, filestream, negative, &previewList);

    dng_xmp_sdk::TerminateSDK();
    progress.advance(100, "Done writing!");
}


} // namespace hdrmerge
