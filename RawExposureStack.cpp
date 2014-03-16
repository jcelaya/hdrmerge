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


// RawExposureStack::LoadResult RawExposureStack::loadImage(const char * fileName) {
//     Exposure e(fileName);
//     LoadResult result = e.load(exps.empty() ? nullptr : &exps.front());
//     if (result == LOAD_SUCCESS) {
//         width = e.rawData.imgdata.sizes.raw_width;
//         height = e.rawData.imgdata.sizes.raw_height;
//         exps.push_back(e);
//     }
//     return result;
// }


bool RawExposureStack::addExposure() {
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
    return true;
}


void RawExposureStack::setRelativeExposure(unsigned int i, double re) {
//     exps[i].immExp = re;
//     for (int j = i; j >= 0; --j) {
//         exps[j].relExp = exps[j + 1].relExp * exps[j].immExp;
//     }
}
