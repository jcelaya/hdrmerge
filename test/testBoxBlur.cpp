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

#include <string>
#include <cmath>
#include <QDir>
#include "../src/BoxBlur.hpp"
#include "SampleImage.hpp"
#include "../src/Log.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;

BOOST_AUTO_TEST_CASE(testBoxBlur) {
    SampleImage image("test/testMap.png"), result;
    for (int radius = 1; radius < 25; radius += 3) {
        BoxBlur map = image;
        string title = string("Blur with radius ") + to_string(radius);
        measureTime(title.c_str(), [&] () {map.blur(radius);});
        (Array2D<uint16_t> &)result = (Array2D<float> &)map;
        string fileName = QDir::tempPath().toStdString() + "/testMapblur_" + to_string(radius) + ".png";
        result.save(fileName);
    }
}
