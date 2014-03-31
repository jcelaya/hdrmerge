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

#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include "Postprocess.hpp"
#include <pfs-1.2/pfs.h>
using namespace std;
using namespace hdrmerge;

#define CLASS Postprocess::
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define CLIP(x) x
#define DCRAW_VERBOSE
#define _(x) x
static int verbose = 1;
#include "amaze_demosaic_RT.cc"


static const double xyzd50_srgb[3][3] = {
    { 0.436083, 0.385083, 0.143055 },
    { 0.222507, 0.716888, 0.060608 },
    { 0.013930, 0.097097, 0.714022 } };
static const double rgb_rgb[3][3] = {
    { 1,0,0 }, { 0,1,0 }, { 0,0,1 } };
static const double adobe_rgb[3][3] = {
    { 0.715146, 0.284856, 0.000000 },
    { 0.000000, 1.000000, 0.000000 },
    { 0.000000, 0.041166, 0.958839 } };
static const double wide_rgb[3][3] = {
    { 0.593087, 0.404710, 0.002206 },
    { 0.095413, 0.843149, 0.061439 },
    { 0.011621, 0.069091, 0.919288 } };
static const double prophoto_rgb[3][3] = {
    { 0.529317, 0.330092, 0.140588 },
    { 0.098368, 0.873465, 0.028169 },
    { 0.016879, 0.117663, 0.865457 } };
static const double xyz_rgb[3][3] = {          /* XYZ from RGB */
    { 0.412453, 0.357580, 0.180423 },
    { 0.212671, 0.715160, 0.072169 },
    { 0.019334, 0.119193, 0.950227 } };
static const double (*out_rgb[])[3] = { rgb_rgb, adobe_rgb, wide_rgb, prophoto_rgb, xyz_rgb };
static const char *name[] = { "sRGB", "Adobe RGB (1998)", "WideGamut D65", "ProPhoto D65", "XYZ" };
static const unsigned phead[] = {
    1024, 0, 0x2100000, 0x6d6e7472, 0x52474220, 0x58595a20, 0, 0, 0,
    0x61637370, 0, 0, 0x6e6f6e65, 0, 0, 0, 0, 0xf6d6, 0x10000, 0xd32d };
static unsigned pbody[] = { 10,
    0x63707274, 0, 36,  /* cprt */
    0x64657363, 0, 40,  /* desc */
    0x77747074, 0, 20,  /* wtpt */
    0x626b7074, 0, 20,  /* bkpt */
    0x72545243, 0, 14,  /* rTRC */
    0x67545243, 0, 14,  /* gTRC */
    0x62545243, 0, 14,  /* bTRC */
    0x7258595a, 0, 20,  /* rXYZ */
    0x6758595a, 0, 20,  /* gXYZ */
    0x6258595a, 0, 20 };    /* bXYZ */
static const unsigned pwhite[] = { 0xf351, 0x10000, 0x116cc };
static unsigned pcurve[] = { 0x63757276, 0, 1, 0x1000000 };


Postprocess::Postprocess(const ImageStack & stack, ProgressIndicator & p)
: progress(p), md(stack.getImage(0).getMetaData()), width(stack.getWidth()),
height(stack.getHeight()), pre_mul(md.preMul), outputColor(1) {
    progress.advance("Composing image.");
    image.reset(new float[width*height][4]);
    fill_n((float *)image.get(), width*height*4, 0.0);
    stack.compose(image.get());
    cerr << "Composing done." << endl;
}


void Postprocess::process() {
    // Here, we could do several things:
    //  wavelet denoise
    //  scale_colors: basically WB and fix chromatic aberrations. Can we do this later??
    progress.advance("White balance");
    moveG2toG1();
    //  lots of other things (see LibRaw::dcraw_proc)
    //  noise reduction
    progress.advance("Demosaicing");
    amaze_demosaic_RT();
    //  median filter
    //  recover highlights. Is this needed??
    //  apply color profile
    progress.advance("Converting to sRGB");
    convertToRgb();
}


void Postprocess::convertToRgb() {
    //gamma_curve (gamm[0], gamm[1], 0, 0); sets curve & gamm, but we want linear
    buildOutputProfile();
    buildOutputMatrix();
    convertPixels();
}


void Postprocess::buildOutputMatrix() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < colors; ++j) {
            outCam[i][j] = 0;
            for (int k = 0; k < 3; k++) {
                outCam[i][j] += out_rgb[outputColor-1][i][k] * md.rgbCam[k][j];
            }
        }
    }
}


void Postprocess::convertPixels() {
    float * end = image[0] + 4*width*height;
    float out[3] = {0.0f, 0.0f, 0.0f };
    for (float * img = image[0]; img < end; img += 4) {
        for(int c = 0; c < colors; ++c) {
            out[0] += outCam[0][c] * img[c];
            out[1] += outCam[1][c] * img[c];
            out[2] += outCam[2][c] * img[c];
        }
        for(int c = 0; c < colors; ++c) {
            img[c] = out[c]; //CLIP((int) out[c]);
        }
    }
}


void Postprocess::buildOutputProfile() {
//     double num, inverse[3][3];
//     oprof = (unsigned *) calloc (phead[0], 1);
//     merror (oprof, "convert_to_rgb()");
//     memcpy (oprof, phead, sizeof phead);
//     if (output_color == 5) oprof[4] = oprof[5];
//     oprof[0] = 132 + 12*pbody[0];
//     for (i=0; i < pbody[0]; i++) {
//         oprof[oprof[0]/4] = i ? (i > 1 ? 0x58595a20 : 0x64657363) : 0x74657874;
//         pbody[i*3+2] = oprof[0];
//         oprof[0] += (pbody[i*3+3] + 3) & -4;
//     }
//     memcpy (oprof+32, pbody, sizeof pbody);
//     oprof[pbody[5]/4+2] = strlen(name[output_color-1]) + 1;
//     memcpy ((char *)oprof+pbody[8]+8, pwhite, sizeof pwhite);
//     pcurve[3] = (short)(256/gamm[5]+0.5) << 16;
//     for (i=4; i < 7; i++)
//         memcpy ((char *)oprof+pbody[i*3+2], pcurve, sizeof pcurve);
//     pseudoinverse ((double (*)[3]) out_rgb[output_color-1], inverse, 3);
//     for (i=0; i < 3; i++)
//         for (j=0; j < 3; j++) {
//             for (num = k=0; k < 3; k++)
//                 num += xyzd50_srgb[i][k] * inverse[j][k];
//             oprof[pbody[j*3+23]/4+i+2] = num * 0x10000 + 0.5;
//         }
//         for (i=0; i < phead[0]/4; i++)
//             oprof[i] = htonl(oprof[i]);
//         strcpy ((char *)oprof+pbody[2]+8, "auto-generated by dcraw");
//     strcpy ((char *)oprof+pbody[5]+12, name[output_color-1]);
}


void Postprocess::moveG2toG1() {
    // Move G2 to G1
    for (size_t row = FC(1,0) >> 1; row < height; row += 2) {
        for (size_t col = FC(row,1) & 1; col < width; col += 2) {
            size_t pos = row*width + col;
            image[pos][1] = image[pos][3];
        }
    }
    md.moveG2toG1();
}



void Postprocess::save(const string & fileName) {
    progress.advance("Saving");
    if (fileName.substr(fileName.find_last_of('.')) == ".pfs") {
        savePFS(fileName);
    }
}


void Postprocess::savePFS (const string & fileName) {
    pfs::DOMIO pfsio;
    pfs::Frame * frame = pfsio.createFrame(width, height);

    // create channels for output
    pfs::Channel * r = NULL;
    pfs::Channel * g = NULL;
    pfs::Channel * b = NULL;
    frame->createXYZChannels(r, g, b);

    unsigned int size = width * height;
    for (size_t i = 0; i < size; ++i) {
        (*r)(i) = image[i][0] / 65536.0;
        (*g)(i) = image[i][1] / 65536.0;
        (*b)(i) = image[i][2] / 65536.0;
    }

    // Save output
    pfs::transformColorSpace(pfs::CS_RGB, r, g, b, pfs::CS_XYZ, r, g, b);
    FILE * file = fopen(fileName.c_str(), "w");
    pfsio.writeFrame(frame, file);
    fclose(file);
    pfsio.freeFrame(frame);
}

