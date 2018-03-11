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
#include "../src/ImageIO.hpp"
#include "../src/Log.hpp"
#include "../src/DngFloatWriter.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;


BOOST_AUTO_TEST_CASE(testDngFloatWriter) {
    RawParameters params("test/sample1.dng");
    Image image = ImageIO::loadRawImage(params);
    int imageWidth = image.getWidth();
    float max = 0;
    for (auto i : image) {
        if (max < i) max = i;
    }
    int bps = 16, width = imageWidth;
//     for (int bps : {16, 24, 32}) {
//         for (int width : {0, imageWidth / 2, imageWidth}) {
            Array2D<float> result(image.getWidth(), image.getHeight());
            for (size_t i = 0; i < image.getWidth()*image.getHeight(); ++i)
                result[i] = image[i] / max;
            QImage preview = ImageIO::renderPreview(result, params, 1.0);
            DngFloatWriter writer;
            writer.setBitsPerSample(bps);
            writer.setPreviewWidth(width);
            writer.setPreview(preview);
            QString fileName = QDir::tempPath() + QString("/testDngFloat_%1_%2.dng").arg(bps).arg(width);
            string title = string("Save Dng Float with ") + to_string(bps) + " bps and preview width " + to_string(width);
            measureTime(title.c_str(), [&] () {
                writer.write(std::move(result), params, fileName);
            });
//         }
//     }
}
