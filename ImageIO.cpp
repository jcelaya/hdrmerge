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
#include <QImage>
#include "ImageIO.hpp"
#include "DngFloatWriter.hpp"
#include "Log.hpp"
using namespace std;
using namespace hdrmerge;


int ImageIO::load(const LoadOptions & options, ProgressIndicator & progress) {
    int numImages = options.fileNames.size();
    int step = 100 / (numImages + 1);
    int p = -step;
    int error = 0, failedImage = 0;
    stack.clear();
    fileNames.clear();
    {
        Timer t("Load files");
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < numImages; ++i) {
            if (!error) { // We cannot break from the for loop if we are using OpenMP
                string name = options.fileNames[i];
                #pragma omp critical
                progress.advance(p += step, "Loading %1", name.c_str());
                std::unique_ptr<Image> image(new Image(name.c_str()));
                #pragma omp critical
                if (!error) { // Report on the first image that fails, ignore the rest
                    if (image.get() == nullptr || !image->good()) {
                        error = 1;
                        failedImage = i;
                    } else {
                        int pos = stack.addImage(image);
                        if (pos == -1) {
                            error = 2;
                            failedImage = i;
                        } else {
                            fileNames.insert(fileNames.begin() + pos, name);
                        }
                    }
                }
            }
        }
    }
    if (error) {
        stack.clear();
        fileNames.clear();
        return (failedImage << 1) + error - 1;
    }
    if (options.align) {
        Timer t("Align");
        progress.advance(p += step, "Aligning");
        stack.align();
        if (options.crop) {
            stack.crop();
        }
    }
    stack.computeRelExposures();
    stack.generateMask();
    progress.advance(100, "Done loading!");
    return numImages << 1;
}


int ImageIO::save(const SaveOptions & options, ProgressIndicator & progress) {
    string cropped = stack.isCropped() ? " cropped" : "";
    Log::msg(2, "Writing ", options.fileName, ", ", options.bps, "-bit, ", stack.getWidth(), 'x', stack.getHeight(), cropped);
    DngFloatWriter writer(progress);
    writer.setBitsPerSample(options.bps);
    writer.setPreviewWidth((options.previewSize * stack.getWidth()) / 2);
    writer.write(stack.compose(), stack.getImage(stack.size() - 1).getMetaData(), options.fileName);
    if (options.saveMask) {
        string name = replaceArguments(options.maskFileName, options.fileName);
        writeMaskImage(name);
    }
}


void ImageIO::writeMaskImage(const std::string & maskFile) {
    Log::msg(Log::DEBUG, "Saving mask to ", maskFile);
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
    if (!maskImage.save(QString(maskFile.c_str()))) {
        Log::msg(Log::PROGRESS, "Cannot save mask image to ", maskFile);
    }
}


string ImageIO::buildOutputFileName() const {
    string name;
    vector<string> names;
    for (auto & f : fileNames) {
        name = f.substr(0, f.find_last_of('.'));
        names.push_back(name);
    }
    sort(names.begin(), names.end());
    if (names.size() > 1) {
        string & last = names.back();
        int pos = last.length() - 1;
        while (last[pos] >= '0' && last[pos] <= '9') pos--;
        name = names.front() + '-' + names.back().substr(pos + 1);
    } else {
        name = names.front();
    }
    return name;
}


string ImageIO::replaceArguments(const string & maskFileName, const string & outFileName) {
    string result = maskFileName;
    const char * specs[4] = {
        "%if",
        "%id",
        "%of",
        "%od"
    };
    string names[4];
    string inFileName = fileNames.back();
    size_t index = inFileName.find_last_of('/');
    if (index != string::npos) {
        names[1] = inFileName.substr(0, index);
        names[0] = inFileName.substr(index + 1);
    } else {
        names[0] = inFileName;
    }
    index = outFileName.find_last_of('/');
    if (index != string::npos) {
        names[3] = outFileName.substr(0, index);
        names[2] = outFileName.substr(index + 1);
    } else {
        names[2] = outFileName;
    }
    // Replace specifiers
    for (int i = 0; i < 4; ++i) {
        while ((index = result.find(specs[i])) != string::npos) {
            result.replace(index, 3, names[i]);
        }
    }
    return result;
}
