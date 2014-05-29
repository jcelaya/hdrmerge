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
#include "ImageStack.hpp"
#include "DngFloatWriter.hpp"
#include "Log.hpp"
#include "BoxBlur.hpp"
using namespace std;
using namespace hdrmerge;


void ImageStack::setGamma(float g) {
    g = 1.0f / g;
    for (int i = 0; i < 65536; i++) {
        toneCurve[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
    }
}


bool ImageStack::addImage(std::unique_ptr<Image> & i) {
    if (images.empty()) {
        width = i->getWidth();
        height = i->getHeight();
        images.push_back(std::move(i));
        return true;
    } else if (images.front()->isSameFormat(*i)) {
        images.push_back(std::move(i));
        std::sort(images.begin(), images.end(), Image::comparePointers);
        return true;
    }
    return false;
}


int ImageStack::load(const LoadOptions & options, ProgressIndicator & progress) {
    int numImages = options.fileNames.size();
    int step = 100 / (numImages + 1);
    int p = -step;
    int error = 0, failedImage = 0;
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
                    } else if (!addImage(image)) {
                        error = 2;
                        failedImage = i;
                    }
                }
            }
        }
    }
    if (error) {
        return (failedImage << 1) + error - 1;
    }
    if (options.align) {
        Timer t("Align");
        progress.advance(p += step, "Aligning");
        align();
        if (options.crop) {
            crop();
        }
    }
    computeRelExposures();
    generateMask();
    progress.advance(100, "Done loading!");
    return numImages << 1;
}


int ImageStack::save(const SaveOptions & options, ProgressIndicator & progress) {
    string cropped("");
    if (width != images[0]->getWidth() || height != images[0]->getHeight()) {
        cropped = " cropped";
    }
    Log::msg(2, "Writing ", options.fileName, ", ", options.bps, "-bit, ", width, 'x', height, cropped);
    DngFloatWriter writer(progress);
    progress.advance(0, "Rendering image");
    writer.setBitsPerSample(options.bps);
    writer.setPreviewWidth((options.previewSize * width) / 2);
    writer.write(compose(), images.back()->getMetaData(), options.fileName);
    if (options.saveMask) {
        string name = replaceArguments(options.maskFileName, options.fileName);
        writeMaskImage(name);
    }
}


void ImageStack::writeMaskImage(const std::string & maskFile) {
    Log::msg(Log::DEBUG, "Saving mask to ", maskFile);
    QImage maskImage(width, height, QImage::Format_Indexed8);
    int numColors = images.size() - 1;
    for (int c = 0; c < numColors; ++c) {
        int gray = (256 * c) / numColors;
        maskImage.setColor(c, qRgb(gray, gray, gray));
    }
    maskImage.setColor(numColors, qRgb(255, 255, 255));
    for (size_t y = 0, pos = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x, ++pos) {
            maskImage.setPixel(x, y, mask[pos]);
        }
    }
    if (!maskImage.save(QString(maskFile.c_str()))) {
        Log::msg(Log::PROGRESS, "Cannot save mask image to ", maskFile);
    }
}


void ImageStack::align() {
    if (images.size() > 1) {
        size_t errors[images.size()];
        for (int i = images.size() - 2; i >= 0; --i) {
            errors[i] = images[i]->alignWith(*images[i + 1]);
        }
        for (int i = images.size() - 2; i >= 0; --i) {
            images[i]->displace(images[i + 1]->getDeltaX(), images[i + 1]->getDeltaY());
            Log::msg(Log::DEBUG, "Image ", i, " displaced to (", images[i]->getDeltaX(), ", ", images[i]->getDeltaY(),
                     ") with error ", errors[i]);
        }
        for (auto & i : images) {
            i->releaseAlignData();
        }
    }
}

void ImageStack::crop() {
    int dx = 0, dy = 0;
    for (auto & i : images) {
        int newDx = max(dx, i->getDeltaX());
        int bound = min(dx + width, i->getDeltaX() + i->getWidth());
        width = bound > newDx ? bound - newDx : 0;
        dx = newDx;
        int newDy = max(dy, i->getDeltaY());
        bound = min(dy + height, i->getDeltaY() + i->getHeight());
        height = bound > newDy ? bound - newDy : 0;
        dy = newDy;
    }
    for (auto & i : images) {
        i->displace(-dx, -dy);
    }
}


void ImageStack::computeRelExposures() {
    for (auto cur = images.rbegin(), next = cur++; cur != images.rend(); next = cur++) {
        (*cur)->relativeExposure(**next);
    }
}


void ImageStack::generateMask() {
    Timer t("Generate mask");
    mask.resize(width, height);
    std::fill_n(&mask[0], width*height, 0);
    for (size_t y = 0, pos = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x, ++pos) {
            int i = mask[pos];
            while (i < images.size() - 1 &&
                (!images[i]->contains(x, y) ||
                images[i]->isSaturated(x, y) ||
                images[i]->isSaturatedAround(x, y))) ++i;
            if (mask[pos] < i) {
                mask[pos] = i;
                mask.traceCircle(x, y, 4, [&] (int col, int row, uint8_t & layer) {
                    if (layer < i && images[i]->contains(col, row)) {
                        layer = i;
                    }
                });
            }
        }
    }
}


double ImageStack::value(size_t x, size_t y) const {
    Image & img = *images[mask(x, y)];
    return img.exposureAt(x, y);
}


Array2D<float> ImageStack::compose() const {
    BoxBlur map(mask);
    measureTime("Blur", [&] () { map.blur(3); });
    Timer t("Compose");
    Array2D<float> dst(width, height);
    const MetaData & md = images.front()->getMetaData();
    int imageMax = images.size() - 1;
    float max = 0.0;
    #pragma omp parallel for schedule(dynamic)
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0, pos = y*width; x < width; ++x, ++pos) {
            int j = map[pos] > imageMax ? imageMax : ceil(map[pos]);
            double v = 0.0, vv = 0.0, p;
            if (images[j]->contains(x, y)) {
                v = images[j]->exposureAt(x, y);
                // Adjust false highlights
                if (j < imageMax && images[j]->isSaturatedAround(x, y)) {
                    v /= md.whiteMultAt(x, y);
                }
                p = j - map[pos];
            } else {
                p = 1.0;
            }
            if (j > 0 && images[j - 1]->contains(x, y)) {
                vv = images[j - 1]->exposureAt(x, y);
                if (images[j - 1]->isSaturatedAround(x, y)) {
                    vv /= md.whiteMultAt(x, y);
                }
            } else {
                p = 0.0;
            }
            v = v*(1.0 - p) + vv*p;
            dst[pos] = v;
            if (v > max) {
                max = v;
            }
        }
    }

    for (size_t pos = 0; pos < width * height; ++pos) {
        dst[pos] /= max;
    }
    return dst;
}


string ImageStack::buildOutputFileName() const {
    string name;
    std::vector<string> names;
    for (auto & image : images) {
        const string & f = image->getMetaData().fileName;
        names.push_back(f.substr(0, f.find_last_of('.')));
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


string ImageStack::replaceArguments(const string & maskFileName, const string & outFileName) {
    string result = maskFileName;
    const char * specs[4] = {
        "%if",
        "%id",
        "%of",
        "%od"
    };
    string names[4];
    string inFileName = images.back()->getMetaData().fileName;
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
