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

#include <cstdlib>
#include <algorithm>
#include <QImage>
#include <QString>
#include <QRegExp>
#include <QFileInfo>
#include <libraw.h>
#include "ImageIO.hpp"
#include "DngFloatWriter.hpp"
#include "Log.hpp"
using namespace std;
using namespace hdrmerge;

Image ImageIO::loadRawImage(RawParameters & rawParameters) {
    LibRaw rawProcessor;
    auto & d = rawProcessor.imgdata;
    if (rawProcessor.open_file(rawParameters.fileName.toLocal8Bit().constData()) == LIBRAW_SUCCESS) {
        Log::msg(Log::DEBUG, "Number of frames : ", d.idata.raw_count);
        libraw_decoder_info_t decoder_info;
        rawProcessor.get_decoder_info(&decoder_info);
        if (d.idata.filters <= 1000 && d.idata.filters != 9) {
            Log::msg(Log::DEBUG, "Unsupported filter array (", d.idata.filters, ").");
#ifdef LIBRAW_DECODER_FLATFIELD
        } else if (!decoder_info.decoder_flags & LIBRAW_DECODER_FLATFIELD) {
            Log::msg(Log::DEBUG, "LibRaw decoder is not flatfield (", ios::hex, decoder_info.decoder_flags, ").");
#endif
        } else if (rawProcessor.unpack() != LIBRAW_SUCCESS) {
            Log::msg(Log::DEBUG, "LibRaw::unpack() failed.");
        } else {
            rawParameters.fromLibRaw(rawProcessor);
        }
    } else {
        Log::msg(Log::DEBUG, "LibRaw::open_file(", rawParameters.fileName, ") failed.");
    }
    return Image(d.rawdata.raw_image, rawParameters);
}



ImageIO::QDateInterval ImageIO::getImageCreationInterval(const QString & fileName) {
    LibRaw rawProcessor;
    QDateInterval result;
    if (rawProcessor.open_file(fileName.toLocal8Bit().constData()) == LIBRAW_SUCCESS) {
        result.end = QDateTime::fromTime_t(rawProcessor.imgdata.other.timestamp);
        result.start = result.end.addMSecs(-rawProcessor.imgdata.other.shutter * 1000.0);
    }
    return result;
}


int ImageIO::load(const LoadOptions & options, ProgressIndicator & progress) {
    int numImages = options.fileNames.size();
    int step = 100 / (numImages + 1);
    int p = -step;
    int error = 0, failedImage = 0;
    stack.clear();
    rawParameters.clear();
    {
        Timer t("Load files");
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < numImages; ++i) {
            if (!error) { // We cannot break from the for loop if we are using OpenMP
                QString name = options.fileNames[i];
                #pragma omp critical
                progress.advance(p += step, "Loading %1", name.toLocal8Bit().constData());
                unique_ptr<RawParameters> params(new RawParameters(name));
                Image image = loadRawImage(*params);
                #pragma omp critical
                if (!error) { // Report on the first image that fails, ignore the rest
                    if (!image.good()) {
                        error = 1;
                        failedImage = i;
                    } else if (stack.size() && !params->isSameFormat(*rawParameters.front())) {
                        error = 2;
                        failedImage = i;
                    } else {
                        int pos = stack.addImage(std::move(image));
                        rawParameters.emplace_back(std::move(params));
                        for (int j = rawParameters.size() - 1; j > pos; --j)
                            rawParameters[j - 1].swap(rawParameters[j]);
                    }
                }
            }
        }
    }
    if (error) {
        stack.clear();
        rawParameters.clear();
        return (failedImage << 1) + error - 1;
    }
    RawParameters & params = *rawParameters.front();
    stack.setFlip(params.flip);
    if(options.useCustomWl)
        // Use custom white level, but only if it's not greater than the value provided by libraw
        params.max = std::min(params.max, options.customWl);
    stack.calculateSaturationLevel(params, options.useCustomWl);
    if (options.align && params.canAlign()) {
        progress.advance(p += step, "Aligning");
        stack.align();
        if (options.crop) {
            stack.crop();
        }
    }
    stack.computeResponseFunctions();
    stack.generateMask();
    progress.advance(100, "Done loading!");
    return numImages << 1;
}


void ImageIO::save(const SaveOptions & options, ProgressIndicator & progress) {
    string cropped = stack.isCropped() ? " cropped" : "";
    Log::msg(2, "Writing ", options.fileName, ", ", options.bps, "-bit, ", stack.getWidth(), 'x', stack.getHeight(), cropped);

    progress.advance(0, "Rendering image");
    RawParameters params = *rawParameters.back();
    params.width = stack.getWidth();
    params.height = stack.getHeight();
    params.adjustWhite(stack.getImage(stack.size() - 1));
    Array2D<float> composedImage = stack.compose(params, options.featherRadius);

    progress.advance(33, "Rendering preview");
    QImage preview = renderPreview(composedImage, params, stack.getMaxExposure(), options.previewSize <= 1);

    progress.advance(66, "Writing output");
    DngFloatWriter writer;
    writer.setBitsPerSample(options.bps);
    writer.setPreviewWidth((options.previewSize * stack.getWidth()) / 2);
    writer.setPreview(preview);
    writer.write(std::move(composedImage), params, options.fileName);
    progress.advance(100, "Done writing!");

    if (options.saveMask) {
        QString name = replaceArguments(options.maskFileName, options.fileName);
        writeMaskImage(name);
    }
}


void ImageIO::writeMaskImage(const QString & maskFile) {
    Log::debug("Saving mask to ", maskFile);
    EditableMask & mask = stack.getMask();
    QImage maskImage(mask.getWidth(), mask.getHeight(), QImage::Format_Indexed8);
    int numColors = stack.size() - 1;
    for (int c = 0; c < numColors; ++c) {
        int gray = (256 * c) / numColors;
        maskImage.setColor(c, qRgb(gray, gray, gray));
    }
    maskImage.setColor(numColors, qRgb(255, 255, 255));
    for (size_t y = 0, pos = 0; y < mask.getHeight(); ++y) {
        for (size_t x = 0; x < mask.getWidth(); ++x, ++pos) {
            maskImage.setPixel(x, y, mask[pos]);
        }
    }
    if (!maskImage.save(maskFile)) {
        Log::progress("Cannot save mask image to ", maskFile);
    }
}


static void prepareRawBuffer(LibRaw & rawProcessor) {
    rawProcessor.imgdata.progress_flags |= LIBRAW_PROGRESS_LOAD_RAW;
    auto & i = rawProcessor.imgdata;
    auto & r = i.rawdata;
    auto & s = i.sizes;
    r.color4_image = nullptr;
    r.color3_image = nullptr;
    size_t numPixels = s.raw_width * (s.raw_height + 7);
    r.raw_alloc = std::malloc(numPixels * sizeof(ushort));
    r.raw_image = (ushort*) r.raw_alloc;
    s.raw_pitch = s.raw_width*2;
    copy_n(&i.color, 1, &r.color);
    copy_n(&i.sizes, 1, &r.sizes);
    copy_n(&i.idata, 1, &r.iparams);
}


QImage ImageIO::renderPreview(const Array2D<float> & rawData, const RawParameters & params, float expShift, bool halfSize) {
    Timer t("Render preview");
    LibRaw rawProcessor;
    auto & d = rawProcessor.imgdata;
    d.params.user_sat = 65535;
    d.params.user_black = 0;
    for (int c = 0; c < 4; ++c) {
        d.params.user_cblack[c] = 0;
    }
    d.params.highlight = 2;
    d.params.user_qual = 3;
    d.params.med_passes = 0;
    copy_n(params.camMul, 4, d.params.user_mul);
    d.params.user_flip = 0;
    d.params.exp_correc = 1;
    d.params.exp_shift = expShift;
    d.params.exp_preser = 1.0;
    d.params.half_size = halfSize ? 1 : 0; // much faster, will be used for preview size 'half' or 'none'
    if (rawProcessor.open_file(params.fileName.toLocal8Bit().constData()) == LIBRAW_SUCCESS) {
//             && rawProcessor.unpack() == LIBRAW_SUCCESS) {
        prepareRawBuffer(rawProcessor);
        // Assume the other sizes are the same as in the raw parameters
        d.sizes.width = params.width;
        d.sizes.height = params.height;
        float scale = d.params.user_sat / (float)(params.max - params.black);
        for (size_t y = 0; y < params.rawHeight; ++y) {
            for (size_t x = 0; x < params.rawWidth; ++x) {
                size_t pos = y*params.rawWidth + x;
                int v = (rawData[pos] - params.blackAt(x - params.leftMargin, y - params.topMargin)) * scale;
                if (v < 0) v = 0;
                else if (v > 65535) v = 65535;
                d.rawdata.raw_image[pos] = v;
            }
        }
        rawProcessor.dcraw_process();
        libraw_processed_image_t * image = rawProcessor.dcraw_make_mem_image();
        if (image == nullptr) {
            Log::msg(2, "dcraw_make_mem_image() returned NULL");
        } else {
            QImage interpolated(image->width, image->height, QImage::Format_RGB32);
            if (interpolated.isNull()) return QImage();
            for (int y = 0; y < image->height; ++y) {
                QRgb* scanline = (QRgb*)interpolated.scanLine(y);
                int pos = (y*image->width)*3;
                for (int x = 0; x < image->width; ++x) {
                    int r = image->data[pos++], g = image->data[pos++], b = image->data[pos++];
                    scanline[x] = qRgb(r, g, b);
                }
            }
            LibRaw::dcraw_clear_mem(image);
            // The result may be some pixels bigger than the original...
            return interpolated.copy(0, 0, params.width/(halfSize ? 2 : 1 ), params.height/(halfSize ? 2 : 1 ));
        }
    }
    return QImage();
}


class FileNameManipulator {
public:
    FileNameManipulator(const vector<unique_ptr<RawParameters>> & paramList) {
        names.reserve(paramList.size());
        for (auto & rp : paramList) {
            names.push_back(rp->fileName);
        }
        sort(names.begin(), names.end());
    }

    QString getInputBaseName(int i) {
        i = adjustIndex(i);
        if (i == -1) return QString();
        else return getBaseName(names[i]);
    }

    QString getInputBaseNameNoExt(int i) {
        QString name = getInputBaseName(i);
        return name.mid(0, name.lastIndexOf('.'));
    }

    QString getInputDirName(int i) {
        i = adjustIndex(i);
        if (i == -1) return QString();
        else return getDirName(names[i]);
    }

    QString getInputNumberSuffix(int i) {
        QString name = getInputBaseNameNoExt(i);
        int pos = name.length() - 1;
        while (pos >= 0 && name[pos] >= '0' && name[pos] <= '9') pos--;
        return name.mid(pos + 1);
    }

    static QString getBaseName(const QString & name) {
        return QFileInfo(name).fileName();
    }

    static QString getDirName(const QString & name) {
        return QFileInfo(name).canonicalPath();
    }

private:
    vector<QString> names;
    int adjustIndex(int i) {
        if (i < 0)
            i = names.size() + i;
        return i < 0 || i >= (int)names.size() ? -1 : i;
    }
};


QString ImageIO::buildOutputFileName() const {
    if (rawParameters.size() > 1)
        return replaceArguments("%id[-1]/%iF[0]-%in[-1].dng", "");
    else
        return replaceArguments("%id[-1]/%iF[0].dng", "");
}


QString ImageIO::getInputPath() const {
    return FileNameManipulator::getDirName(rawParameters[0]->fileName);
}


QString ImageIO::replaceArguments(const QString & pattern, const QString & outFileName) const {
    QString result(pattern);
    QRegExp re;
    if (outFileName == "") {
        re = QRegExp("%(?:i[fFdn]\\[(-?[0-9]+)\\]|%)");
    } else {
        re = QRegExp("%(?:o[fd]|i[fFdn]\\[(-?[0-9]+)\\]|%)");
    }
    int index = 0;
    FileNameManipulator fnm(rawParameters);
    while ((index = result.indexOf(re, index)) != -1) {
        // What was matched?
        QString token = re.cap();
        if (token[1] == '%') {
            result.replace(index, 2, '%');
        } else if (token[1] == 'o') {
            if (token[2] == 'f') {
                result.replace(index, 3, fnm.getBaseName(outFileName));
            } else {
                result.replace(index, 3, fnm.getDirName(outFileName));
            }
        } else { // 'i'
            int imageIndex = re.cap(1).toInt();
            int length = re.cap(1).length() + 5;
            if (token[2] == 'f') {
                result.replace(index, length, fnm.getInputBaseName(imageIndex));
            } else if (token[2] == 'F') {
                result.replace(index, length, fnm.getInputBaseNameNoExt(imageIndex));
            } else if (token[2] == 'd') {
                result.replace(index, length, fnm.getInputDirName(imageIndex));
            } else { // 'n'
                result.replace(index, length, fnm.getInputNumberSuffix(imageIndex));
            }
        }
        index++;
    }
    return result;
}
