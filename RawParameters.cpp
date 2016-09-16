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
#include <cmath>
#include <algorithm>
#include <functional>
#include <QDateTime>
#include <QFileInfo>
#include <libraw.h>
#include <exiv2/exiv2.hpp>
#include "Log.hpp"
#include "RawParameters.hpp"
using namespace hdrmerge;
using namespace std;
using namespace std::placeholders;


RawParameters::RawParameters() : width(0), height(0), rawWidth(0), rawHeight(0), topMargin(0), leftMargin(0), max(0),
black(0), maxBlack(0), cblack{}, preMul{}, camMul{}, camXyz{}, rgbCam{}, isoSpeed(0.0), shutter(0.0), aperture(0.0), colors(0) {}


void RawParameters::loadCamXyzFromDng() {
    // Try to load it from the DNG metadata
    try {
        const float d65_white[3] = { 0.950456f, 1.0f, 1.088754f };
        double cc[4][4], xyz[] = { 1,1,1 };
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                cc[j][i] = i == j ? 1.0 : 0.0;
            }
        }
        Exiv2::Image::AutoPtr src = Exiv2::ImageFactory::open(fileName.toLocal8Bit().constData());
        src->readMetadata();
        const Exiv2::ExifData & srcExif = src->exifData();

        auto ccData = srcExif.findKey(Exiv2::ExifKey("Exif.Image.CameraCalibration1"));
        if (ccData == srcExif.end()) {
            ccData = srcExif.findKey(Exiv2::ExifKey("Exif.Image.CameraCalibration2"));
        }
        if (ccData != srcExif.end()) {
            for (int i = 0; i < colors; ++i) {
                for (int j = 0; j < colors; ++j) {
                    cc[i][j] = ccData->toFloat(i*colors + j);
                }
            }
        }

        auto xyzData = srcExif.findKey(Exiv2::ExifKey("Exif.Image.AsShotWhiteXY"));
        if (xyzData != srcExif.end()) {
            xyz[0] = xyzData->toFloat(0);
            xyz[1] = xyzData->toFloat(1);
            xyz[2] = 1.0 - xyz[0] - xyz[1];
            for (int i = 0; i < 3; ++i) {
                xyz[i] /= d65_white[i];
            }
        }

        auto cmData = srcExif.findKey(Exiv2::ExifKey("Exif.Image.ColorMatrix1"));
        if (cmData == srcExif.end()) {
            cmData = srcExif.findKey(Exiv2::ExifKey("Exif.Image.ColorMatrix2"));
        }
        if (cmData != srcExif.end() && cmData->count() == 3*colors) {
            for (int c = 0; c < colors; ++c) {
                for (int i = 0; i < 3; ++i) {
                    camXyz[c][i] = 0.0;
                    for (int j = 0; j < colors; ++j) {
                        camXyz[c][i] += cc[c][j] * cmData->toFloat(j*3 + i) * xyz[i];
                    }
                }
            }
        }
    } catch (Exiv2::Error & e) {
        Log::debug("Could not load camXyz values from metadata: ", e.what());
    }
}


static void pseudoinverse(double (*in)[3], double (*out)[3], int size) {
    double work[3][6], num;
    int i, j, k;

    for (i=0; i < 3; i++) {
        for (j=0; j < 6; j++)
            work[i][j] = j == i+3;
        for (j=0; j < 3; j++)
            for (k=0; k < size; k++)
                work[i][j] += in[k][i] * in[k][j];
    }
    for (i=0; i < 3; i++) {
        num = work[i][i];
        for (j=0; j < 6; j++)
            work[i][j] /= num;
        for (k=0; k < 3; k++) {
            if (k==i) continue;
            num = work[k][i];
            for (j=0; j < 6; j++)
                work[k][j] -= work[i][j] * num;
        }
    }
    for (i=0; i < size; i++)
        for (j=0; j < 3; j++)
            for (out[i][j]=k=0; k < 3; k++)
                out[i][j] += work[j][k+3] * in[i][k];
}


void RawParameters::camXyzFromRgbCam() {
    if (rgbCam[0][0]) {
        // Calculate it from rgbCam
        // FIXME: This is experimental, not sure at all if it is correct
        const double rgbXyz[3][3] = { // The inverse of xyz_rgb
            {  3.240481, -1.537152, -0.498536 },
            { -0.969255,  1.875990,  0.041556 },
            {  0.055647, -0.204041,  1.057311 }
        };
        double camRgb[4][3], rgbCamT[4][3];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < colors; ++j) {
                rgbCamT[j][i] = rgbCam[i][j];
            }
        }
        pseudoinverse(rgbCamT, camRgb, colors);
        for (int i = 0; i < colors; ++i) {
            for (int j = 0; j < 3; ++j) {
                camRgb[i][j] /= preMul[i];
            }
        }
        for (int i = 0; i < colors; ++i) {
            for (int j = 0; j < 3; ++j) {
                camXyz[i][j] = 0;
                for (int k = 0; k < 3; ++k) {
                    camXyz[i][j] += camRgb[i][k] * rgbXyz[k][j];
                }
            }
        }
        Log::debug("camXyz values computed from rgbCam.");
        for (int i = 0; i < colors; ++i) {
            for (int j = 0; j < 3; ++j) {
                Log::debugN(" ", camXyz[i][j]);
            }
            Log::debug();
        }
    }
}


void RawParameters::calculateCamXyz() {
    // LibRaw does not create this matrix for DNG files!!!
    loadCamXyzFromDng();
    if (!camXyz[0][0]) {
        camXyzFromRgbCam();
        if (!camXyz[0][0]) {
            // Identity matrix if we know nothing
            for (int i = 0; i < colors; ++i) {
                for (int j = 0; j < 3; ++j) {
                    camXyz[i][j] = i == j ? 1.0 : 0.0;
                }
            }
        }
    }
}


void RawParameters::fromLibRaw(LibRaw & rawData) {
    auto & r = rawData.imgdata;
    width = r.sizes.width;
    height = r.sizes.height;
    rawWidth = r.sizes.raw_width;
    rawHeight = r.sizes.raw_height;
    topMargin = r.sizes.top_margin;
    leftMargin = r.sizes.left_margin;
    auto fcol = std::bind(&LibRaw::fcol, &rawData, _1, _2);
    FC.setPattern(r.idata.filters, fcol);
    colors = r.idata.colors;
    cdesc = r.idata.cdesc;
    max = r.color.maximum;
    black = r.color.black;
    copy_n(r.color.cblack, 4, cblack);
    adjustBlack();
    copy_n(r.color.pre_mul, 4, preMul);
    copy_n(r.color.cam_mul, 4, camMul);
    if (camMul[0] == 0 || camMul[0] == -1) {
        Log::debug("Invalid camera white balance: ", camMul[0], ' ', camMul[1], ' ', camMul[2], ' ', camMul[3]);
        camMul[0] = 0;
    }
    copy_n((float *)r.color.rgb_cam, 4*3, (float *)rgbCam);
    isoSpeed = r.other.iso_speed;
    shutter = r.other.shutter;
    aperture = r.other.aperture;
    if(aperture <= 0.f || isinf(aperture) || isnan(aperture)) {
        Log::debug("Invalid aperture: ", aperture, " replaced by aperture: f8");
        aperture = 8;
    }
    maker = r.idata.make;
    model = r.idata.model;
    description = r.other.desc;
    QDateTime dateTimeTmp = QDateTime::fromTime_t(r.other.timestamp);
    QString dateTimeTmpText = dateTimeTmp.toString("yyyy:MM:dd hh:mm:ss");
    dateTime = dateTimeTmpText.toAscii().constData();
    flip = r.sizes.flip;
    switch ((flip + 3600) % 360) {
        case 270: flip = 5; break;
        case 180: flip = 3; break;
        case  90: flip = 6; break;
    }
    switch (flip) {
        case 0: tiffOrientation = 1; break;
        case 3: tiffOrientation = 3; break;
        case 5: tiffOrientation = 8; break;
        case 6: tiffOrientation = 6; break;
        default: tiffOrientation = 9; break;
    }
    copy_n((float *)r.color.cam_xyz, 3*4, (float *)camXyz);
    if (!camXyz[0][0]) {
        calculateCamXyz();
    }
    dumpInfo();
}


double RawParameters::logExp() const {
    return std::log2(isoSpeed * shutter / (100.0 * aperture * aperture));
}


void RawParameters::adjustBlack() {
    uint16_t minb = cblack[0] + black;
    maxBlack = minb;
    for (int i = 0; i < 4; ++i) {
        cblack[i] += black;
        if (minb > cblack[i]) {
            minb = cblack[i];
        }
        if (maxBlack < cblack[i])
            maxBlack = cblack[i];
    }
    black = minb;
}


void RawParameters::adjustWhite(const Array2D<uint16_t> & image) {
    if (camMul[0] == 0) {
        autoWB(image);
    } else if (camMul[1] == 0) {
        camMul[1] = 1;
    }
    if (colors == 3) {
        camMul[3] = camMul[1];
    } else if (camMul[3] == 0) {
        camMul[3] = 1;
    }
    float min = camMul[0];
    for (int c = 1; c < 4; ++c) {
        min = std::min(min, camMul[c]);
    }
    for (int c = 0; c < 4; ++c) {
        camMul[c] /= min;
    }
    Log::debug("Adjusted white balance: ", camMul[0], ' ', camMul[1], ' ', camMul[2], ' ', camMul[3]);
}


void RawParameters::autoWB(const Array2D<uint16_t> & image) {
    Timer t("AutoWB");
    double dsum[4] = { 0.0, 0.0, 0.0, 0.0 };
    size_t dcount[4] = { 0, 0, 0, 0 };
    for (size_t row = 0; row < image.getHeight(); row += 8) {
        for (size_t col = 0; col < image.getWidth() ; col += 8) {
            double sum[4] = { 0.0, 0.0, 0.0, 0.0 };
            size_t count[4] = { 0, 0, 0, 0 };
            size_t ymax = std::min(row + 8, image.getHeight());
            size_t xmax = std::min(col + 8, image.getWidth());
            bool skipBlock = false;
            for (size_t y = row; y < ymax && !skipBlock; y++) {
                for (size_t x = col; x < xmax; x++) {
                    int c = FC(x, y);
                    uint16_t val = image(x, y);
                    if (val > max - 25) {
                        skipBlock = true;
                        break;
                    }
                    sum[c] += val;
                    count[c]++;
                }
            }
            if (!skipBlock) {
                for (int c = 0; c < 4; ++c) {
                    dsum[c] += sum[c];
                    dcount[c] += count[c];
                }
            }
        }
    }
    for (int c = 0; c < 4; ++c) {
        if (dsum[c] > 0.0) {
            camMul[c] = dcount[c] / dsum[c];
        } else {
            copy_n(preMul, 4, camMul);
            break;
        }
    }
}


void RawParameters::dumpInfo() const {
    Log::debugN(QFileInfo(fileName).fileName(), ": ", width, 'x', height, " (", rawWidth, 'x', rawHeight, '+', leftMargin, '+', topMargin);
    Log::debug(", by ", maker, ' ' , model, ", ", isoSpeed, "ISO 1/", (1.0/shutter), "sec f", aperture, " EV:", logExp());
    Log::debugN(hex, FC.getFilters(), dec, ' ', cdesc, ", sat ", max, ", black ", black, ", flip ", flip);
    Log::debugN(", wb: ", camMul[0], ' ', camMul[1], ' ', camMul[2], ' ', camMul[3]);
    Log::debug(", cblack: ", cblack[0], ' ', cblack[1], ' ', cblack[2], ' ', cblack[3]);
}
