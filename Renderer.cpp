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
 *  Part of this file is generated from Dave Coffin's dcraw.c
 *  dcraw.c -- Dave Coffin's raw photo decoder
 *  Copyright 1997-2010 by Dave Coffin, dcoffin a cybercom o net
 *
 *  Look into dcraw homepage (probably http://cybercom.net/~dcoffin/dcraw/)
 *  for more information
 *
 */

#include "Renderer.hpp"
using namespace hdrmerge;

Renderer::Renderer(const Array2D<float> & rawData, const MetaData & md)
: md(md), width(rawData.getWidth()/2), height(rawData.getHeight()/2) {
    image.reset(new float[width*height][4]);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t posDst = y*width + x;
            size_t y2 = y << 1, x2 = x << 1;
            image[posDst][md.FC(x2, y2)] = rawData(x2, y2) * 65535.0;
            image[posDst][md.FC(x2 + 1, y2)] = rawData(x2 + 1, y2) * 65535.0;
            image[posDst][md.FC(x2, y2 + 1)] = rawData(x2, y2 + 1) * 65535.0;
            image[posDst][md.FC(x2 + 1, y2 + 1)] = rawData(x2 + 1, y2 + 1) * 65535.0;
        }
    }
}


QImage Renderer::getScaledVersion(size_t width) {
    return halfInterpolated.scaledToWidth(width);
}


void Renderer::process() {
    whiteBalance();
    for (size_t i = 0; i < width*height; ++i) {
        image[i][1] = (image[i][1] + image[i][3]) / 2.0;
    }
    convertToRgb();
    buildResult();
}


void Renderer::whiteBalance() {
    double dmin = 10000000.0;
    for (int c = 0; c < 4; ++c) {
        if (dmin > md.camMul[c])
            dmin = md.camMul[c];
    }
    float scale_mul[4];
    for (int c = 0; c < 4; ++c) {
        scale_mul[c] = (md.camMul[c] /= dmin);
    }
    size_t size = height*width;
    float * dst = image[0];
    for (size_t i = 0; i < size*4; ++i) {
        dst[i] *= scale_mul[i & 3];
    }
}


void Renderer::autoWB() {
//     unsigned bottom, right, sum[8];
//     double dsum[8];
//     if (use_auto_wb || (use_camera_wb && md.camMul[0] == -1)) {
//         memset (dsum, 0, sizeof dsum);
//         bottom = MIN (greybox[1]+greybox[3], height);
//         right  = MIN (greybox[0]+greybox[2], width);
//         for (row=greybox[1]; row < bottom; row += 8)
//             for (col=greybox[0]; col < right; col += 8) {
//                 memset (sum, 0, sizeof sum);
//                 for (y=row; y < row+8 && y < bottom; y++)
//                     for (x=col; x < col+8 && x < right; x++)
//                         FORC4 {
//                             if (filters) {
//                                 c = fcol(y,x);
//                                 val = BAYER2(y,x);
//                             } else
//                                 val = image[y*width+x][c];
//                             if (val > maximum-25) goto skip_block;
//                             if ((val -= cblack[c]) < 0) val = 0;
//                             sum[c] += val;
//                             sum[c+4]++;
//                             if (filters) break;
//                         }
//                         FORC(8) dsum[c] += sum[c];
//                         skip_block: ;
//             }
//             FORC4 if (dsum[c]) pre_mul[c] = dsum[c+4] / dsum[c];
//     }
}


void Renderer::convertToRgb() {
    float * end = image[0] + 4*width*height;
    for (float * img = image[0]; img < end; img += 4) {
        float out[3] = {0.0f, 0.0f, 0.0f };
        for(int c = 0; c < 3; ++c) {
            out[0] += md.rgbCam[0][c] * img[c];
            out[1] += md.rgbCam[1][c] * img[c];
            out[2] += md.rgbCam[2][c] * img[c];
        }
        for(int c = 0; c < 3; ++c) {
            float v = out[c];
            if (v > 65535.0f) v = 65535.0f;
            else if (v < 0.0f) v = 0.0f;
            img[c] = v;
        }
    }
}


void Renderer::buildResult() {
    uint8_t gamma[65536];
    float g = 1.0f / 2.2f;
    for (int i = 0; i < 65536; i++) {
        gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
    }

    halfInterpolated = QImage(width, height, QImage::Format_RGB32);
    QRgb * dst = (QRgb *)halfInterpolated.bits();
    for (size_t i = 0; i < width*height; ++i) {
        dst[i] = qRgb(gamma[(int)image[i][0]], gamma[(int)image[i][1]], gamma[(int)image[i][2]]);
    }
}
