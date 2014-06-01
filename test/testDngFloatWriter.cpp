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
#include "../ImageIO.hpp"
#include "../Log.hpp"
#include "../DngFloatWriter.hpp"
#include "../ExifTransfer.hpp"
#include <boost/test/unit_test.hpp>
using namespace hdrmerge;
using namespace std;


struct NullProgressIndicator : public ProgressIndicator {
    virtual void advance(int percent, const char * message, const char * arg) {}
};


BOOST_AUTO_TEST_CASE(testDngFloatWriter) {
    MetaData metaData("test/sample1.dng");
    Image image = ImageIO::loadRawImage(metaData);
    int imageWidth = image.getWidth();
    NullProgressIndicator npi;
    for (int bps : {16, 24, 32}) {
        for (int width : {0, imageWidth / 2, imageWidth}) {
            Array2D<float> result(image);
            DngFloatWriter writer(npi);
            writer.setBitsPerSample(bps);
            writer.setPreviewWidth(width);
            string fileName = QDir::tempPath().toStdString() + "/testDngFloat_" + to_string(bps) + "_" + to_string(width) + ".dng";
            string title = string("Save Dng Float with ") + to_string(bps) + " bps and preview width " + to_string(width);
            measureTime(title.c_str(), [&] () {
                writer.write(std::move(result), metaData, fileName);
                ExifTransfer exif("test/sample1.dng", fileName);
                exif.copyMetadata();
            });
        }
    }
}
