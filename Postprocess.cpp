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
#include <cfloat>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include "Postprocess.hpp"
#include <pfs-1.2/pfs.h>
#include <tiffio.h>
using namespace std;
using namespace hdrmerge;

#define CLASS Postprocess::
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
//#define CLIP(x) MIN(65535, x)
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
height(stack.getHeight()), pre_mul(md.preMul), outputColor(5) {
    progress.advance("Composing image.");
    image.reset(new float[width*height][4]);
    fill_n((float *)image.get(), width*height*4, 0.0);
    stack.compose(image.get());
}


void Postprocess::process() {
    // Here, we could do several things:
    //  wavelet denoise
    progress.advance("White balance");
    whiteBalance(); // And chromatic aberrations.
    moveG2toG1();
    //  lots of other things (see LibRaw::dcraw_proc)
    //  noise reduction
    progress.advance("Demosaicing");
    amaze_demosaic_RT();
    //  median filter
    //  recover highlights. Is this needed??
    //  apply color profile
    progress.advance("Converting to XYZ");
    convertToRgb();
}


void Postprocess::whiteBalance() {
//     if (user_mul[0])
//         memcpy (pre_mul, user_mul, sizeof pre_mul);
    cameraWB();
    if (pre_mul[1] == 0) pre_mul[1] = 1;
    if (pre_mul[3] == 0) pre_mul[3] = colors < 4 ? pre_mul[1] : 1;
    //if (threshold) wavelet_denoise();
    size_t maximum = md.max - md.black;
    double dmin = DBL_MAX, dmax = 0;
    for (int c = 0; c < 4; ++c) {
        if (dmin > pre_mul[c])
            dmin = pre_mul[c];
        if (dmax < pre_mul[c])
            dmax = pre_mul[c];
    }
    //if (!highlight)
        dmax = dmin;
    float scale_mul[4];
    for (int c = 0; c < 4; ++c) {
        scale_mul[c] = (pre_mul[c] /= dmax) * 65535.0 / maximum;
    }
    size_t size = height*width;
    for (size_t i = 0; i < size*4; ++i) {
        double val = image[0][i];
        if (val == 0.0) continue;
        val -= md.cblack[i & 3];
        val *= scale_mul[i & 3];
        image[0][i] = val;
    }
}


void Postprocess::cameraWB() {
    if (md.camMul[0] != -1) {
        uint16_t sum[8] = {};
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                int c = FC(row,col);
                uint16_t val = md.white[row][col] - md.cblack[c];
                if (val > 0)
                    sum[c] += val;
                sum[c + 4]++;
            }
        }
        if (sum[0] && sum[1] && sum[2] && sum[3]) {
            for (int c = 0; c < 4; ++c) {
                pre_mul[c] = (float) sum[c+4] / sum[c];
            }
        } else if (md.camMul[0] && md.camMul[2]) {
            copy_n(md.camMul, 4, pre_mul);
        }
    }
}


void Postprocess::chromaticAberration() {
//     unsigned ur, uc;
//     float fr, fc;
//     ushort *img=0, *pix;
//     if ((aber[0] != 1 || aber[2] != 1) && colors == 3) {
//         #ifdef DCRAW_VERBOSE
//         if (verbose)
//             fprintf (stderr,_("Correcting chromatic aberration...\n"));
//         #endif
//         for (c=0; c < 4; c+=2) {
//             if (aber[c] == 1) continue;
//             img = (ushort *) malloc (size * sizeof *img);
//             merror (img, "scale_colors()");
//             for (i=0; i < size; i++)
//                 img[i] = image[i][c];
//             for (row=0; row < iheight; row++) {
//                 ur = fr = (row - iheight*0.5) * aber[c] + iheight*0.5;
//                 if (ur > iheight-2) continue;
//                 fr -= ur;
//                 for (col=0; col < iwidth; col++) {
//                     uc = fc = (col - iwidth*0.5) * aber[c] + iwidth*0.5;
//                     if (uc > iwidth-2) continue;
//                     fc -= uc;
//                     pix = img + ur*iwidth + uc;
//                     image[row*iwidth+col][c] =
//                     (pix[     0]*(1-fc) + pix[       1]*fc) * (1-fr) +
//                     (pix[iwidth]*(1-fc) + pix[iwidth+1]*fc) * fr;
//                 }
//             }
//             free(img);
//         }
//     }
}


void Postprocess::autoWB() {
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


void Postprocess::convertToRgb() {
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
    for (float * img = image[0]; img < end; img += 4) {
        float out[3] = {0.0f, 0.0f, 0.0f };
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
    string ext = fileName.substr(fileName.find_last_of('.'));
    if (ext == ".pfs") {
        savePFS(fileName);
    } else if (ext == ".tif" || ext == ".tiff") {
        saveTiff(fileName);
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
    FILE * file = fopen(fileName.c_str(), "w");
    pfsio.writeFrame(frame, file);
    fclose(file);
    pfsio.freeFrame(frame);
}


void Postprocess::saveTiff (const string & fileName) {
    TIFF * out = TIFFOpen(fileName.c_str(), "w");

    int sampleperpixel = 3;
    float * logluv = new float[width*height*sampleperpixel];
    unsigned int size = width * height;
    for (size_t i = 0, j = 0; i < size; ++i) {
        logluv[j++] = image[i][0] / 65536.0;
        logluv[j++] = image[i][1] / 65536.0;
        logluv[j++] = image[i][2] / 65536.0;
    }

    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, sizeof(float)*8);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
    //TIFFSetField(out, TIFFTAG_STONITS, stonits);   /* if known */
    TIFFSetField(out, EXIFTAG_FNUMBER, md.aperture);

    tsize_t linesamples = width*sampleperpixel;
    tsize_t linebytes = linesamples*sizeof(float);
    float * buf = nullptr;
    if (TIFFScanlineSize(out) < linebytes) {
        buf = (float *)_TIFFmalloc(linebytes);
    } else {
        buf = (float *)_TIFFmalloc(TIFFScanlineSize(out));
    }
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, linebytes));
    for (uint32_t row = 0; row < height; ++row) {
        copy_n(&logluv[row*linesamples], linesamples, buf);
        if (TIFFWriteScanline(out, buf, row, 0) < 0)
            break;
    }

    _TIFFfree(buf);
    TIFFClose(out);
}
