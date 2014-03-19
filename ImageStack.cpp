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

#include <tiff.h>
#include <tiffio.h>
#include <libraw/libraw.h>
#include <pfs-1.2/pfs.h>
#include "ImageStack.hpp"
using namespace std;
using namespace hdrmerge;


bool ImageStack::addImage(std::unique_ptr<Image> & i) {
    if (images.empty() || !images.front()->isWrongFormat(*i)) {
        width = i->getWidth();
        height = i->getHeight();
        images.push_back(std::move(i));
        return true;
    }
    return false;
}

// ImageStack::LoadResult ImageStack::loadImage(const char * fileName) {
//     Exposure e(fileName);
//     LoadResult result = e.load(images.empty() ? nullptr : &images.front());
//     if (result == LOAD_SUCCESS) {
//         width = e.rawData.imgdata.sizes.raw_width;
//         height = e.rawData.imgdata.sizes.raw_height;
//         images.push_back(e);
//     }
//     return result;
// }


//void ImageStack::setRelativeExposure(unsigned int i, double re) {
//     images[i].immExp = re;
//     for (int j = i; j >= 0; --j) {
//         images[j].relExp = images[j + 1].relExp * images[j].immExp;
//     }
//}
