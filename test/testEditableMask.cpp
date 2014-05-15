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
#include "../EditableMask.hpp"
#include "SampleImage.hpp"
#include "../Log.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(testEditableMask) {
    SampleImage image("test/testMap.png");
    size_t size = image.md.width * image.md.height;
    uint16_t * src = image.pixelData;
    image.pixelData = new uint16_t[size];
    EditableMask map(image.md.width, image.md.height);
    for (int radius = 1; radius < 25; radius += 3) {
        for (size_t i = 0; i < size; ++i) {
            map[i] = src[i];
        }
        unique_ptr<float[]> blurred;
        string title = string("Blur with radius ") + to_string(radius);
        measureTime(title.c_str(), [&] () {blurred = map.blur(radius);});
        for (size_t i = 0; i < size; ++i) {
            image.pixelData[i] = std::round(blurred[i]);
        }
        ostringstream fileName;
        fileName << "test/testMapblur_" << radius << ".png";
        image.save(fileName.str());
    }
    delete[] src;
}
