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

#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <tiff.h>
#include <tiffio.h>
#include <libraw/libraw.h>
#include <pfs-1.2/pfs.h>
#include "RawExposureStack.hpp"
using namespace std;
using namespace hdrmerge;

RawExposureStack::LoadResult RawExposureStack::Exposure::load(const Exposure * ref) {
    // Open the file and read the metadata
    int error = rawData.open_file(fileName.c_str());
    if (error != 0) {
        return LOAD_OPEN_FAIL;
    }
    auto & r = rawData.imgdata;

    unsigned int w = r.sizes.raw_width;
    unsigned int h = r.sizes.raw_height;
    if (ref != nullptr &&
        (ref->rawData.imgdata.sizes.raw_width != w
         || ref->rawData.imgdata.sizes.raw_height != h
         || ref->rawData.imgdata.idata.filters != r.idata.filters
         || strcmp(ref->rawData.imgdata.idata.cdesc, r.idata.cdesc)
        )) {
        return LOAD_FORMAT_FAIL;
    }

    // Let us unpack the image
    rawData.unpack();
    img = r.rawdata.raw_image;
    if (img == nullptr) {
        return LOAD_FORMAT_FAIL;
    }
    max = r.color.maximum;
    subtractBlack();
    computeLogExp();

    cerr << "Loaded image " << fileName << ", " << w << 'x' << h << ", " << (w * h * 2) << " bytes allocated" << endl;

    return LOAD_SUCCESS;
}


void RawExposureStack::Exposure::subtractBlack() {
    unsigned int rowWidth = rawData.imgdata.sizes.raw_width;
    unsigned int rowDisp = 0;
    unsigned int black = rawData.imgdata.color.black;
    unsigned int * cblack = rawData.imgdata.color.cblack;
    for (unsigned int row = 0; row < rawData.imgdata.sizes.raw_height; ++row) {
        for (unsigned int col = 0; col < rowWidth; ++col) {
            img[rowDisp + col] -= black + cblack[rawData.FC(row, col)];
        }
        rowDisp += rowWidth;
    }
    max -= black;
}


void RawExposureStack::Exposure::computeLogExp() {
    auto & o = rawData.imgdata.other;
    logExp = log2(o.iso_speed * o.shutter / (100.0 * o.aperture * o.aperture));
}


RawExposureStack::LoadResult RawExposureStack::loadImage(const char * fileName) {
    Exposure e(fileName);
    LoadResult result = e.load(exps.empty() ? nullptr : &exps.front());
    if (result == LOAD_SUCCESS) {
        width = e.rawData.imgdata.sizes.raw_width;
        height = e.rawData.imgdata.sizes.raw_height;
        exps.push_back(e);
    }
    return result;
}


void RawExposureStack::sort() {
    if (!exps.empty()) {
         std::sort(exps.begin(), exps.end());
//         for (vector<Exposure>::reverse_iterator cur = exps.rbegin(), prev = cur++; cur != exps.rend(); prev = cur++) {
//             // Calculate median relative exposure
//             const uint16_t min = 3275, max = 52430;
//             vector<float> samples;
//             const Pixel * rpix = prev->p.get(), * pix = cur->p.get();
//             const Pixel * end = pix + width * height;
//             for (; pix < end; rpix++, pix++) {
//                 if (rpix->l == Pixel::transparent)
//                     continue;
//                 // Only sample those pixels that are in the linear zone
//                 //if (rpix->r < max && rpix->r > min && pix->r < max && pix->r > min)
//                 //      samples.push_back((float)rpix->r / pix->r);
//                 if (rpix->g < max && rpix->g > min && pix->g < max && pix->g > min)
//                     samples.push_back((float)rpix->g / pix->g);
//                 //if (rpix->b < max && rpix->b > min && pix->b < max && pix->b > min)
//                 //      samples.push_back((float)rpix->b / pix->b);
//             }
//             std::sort(samples.begin(), samples.end());
//             cur->immExp = samples[samples.size() / 2];
//             cur->relExp = cur->immExp * prev->relExp;
//             cerr << "Relative exposure: " << (1.0/cur->relExp) << '(' << (log(1.0/cur->relExp) / log(2.0)) << " EV)" << endl;
//         }
//         exps.back().th = 0xffff;
    }
}


void RawExposureStack::setRelativeExposure(unsigned int i, double re) {
    exps[i].immExp = re;
    for (int j = i; j >= 0; --j) {
        exps[j].relExp = exps[j + 1].relExp * exps[j].immExp;
    }
}


void RawExposureStack::Exposure::dumpInfo() const {
    auto & r = rawData.imgdata;
    // Show idata
    cerr << "Picture by " << r.idata.make << ", model " << r.idata.model << endl;
    cerr << r.idata.colors << " colors with mask " << hex << r.idata.filters << dec << ", " << r.idata.cdesc << endl;
    // Show sizes
    cerr << r.sizes.raw_width << 'x' << r.sizes.raw_height << " (" << r.sizes.width << 'x' << r.sizes.height << '+'
        << r.sizes.left_margin << ',' << r.sizes.top_margin << ") aspect:" << r.sizes.pixel_aspect << ", flip:"
        << r.sizes.flip << ", raw_pitch:" << r.sizes.raw_pitch << endl;
    // Show color
    cerr << r.color.maximum << " max value, black at " << r.color.black << endl;
    // Show other
    cerr << "ISO:" << r.other.iso_speed << " shutter:1/" << (1.0/r.other.shutter) << " aperture:f" << r.other.aperture << endl;
    cerr << "Exposure (log) " << logExp << " steps, pixels saturate at " << max << endl;
    // Show rawdata
}
