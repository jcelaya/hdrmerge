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

#include <iostream>
#include <sstream>
#include "../MergeMap.hpp"
#include "SampleImage.hpp"
#include "time.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(testMergeMap1) {
    SampleImage image("test/testMap.png");
    size_t size = image.md.width * image.md.height;
    uint16_t * src = image.pixelData;
    image.pixelData = new uint16_t[size];
    MergeMap map(image.md.width, image.md.height);
    for (int radius = 1; radius < 25; radius += 3) {
        for (size_t i = 0; i < size; ++i) {
            map[i] = src[i];
        }
        double blurTime = measureTime([&] () {map.blur(radius);});
        cout << "Image blurred with radius " << radius << " in " << blurTime << " seconds" << endl;
        for (size_t i = 0; i < size; ++i) {
            image.pixelData[i] = std::round(map[i]);
        }
        ostringstream fileName;
        fileName << "test/testMapblur_" << radius << ".png";
        image.save(fileName.str());
    }
    delete[] image.pixelData;
    free(src);
}
