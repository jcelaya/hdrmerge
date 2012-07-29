#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <tiff.h>
#include <tiffio.h>
#include "Exposure.h"
#ifdef HAVE_OPENEXR
    #include <ImfRgbaFile.h>
#endif
#include <pfs-1.2/pfs.h>
using namespace std;


ExposureStack::LoadResult ExposureStack::loadImage(const char * fileName) {
    Exposure e(fileName);
    
    TIFF* file = TIFFOpen(fileName, "r");
    if (file == NULL)
        return LOAD_OPEN_FAIL;
    
    unsigned int w, h;
    if (!TIFFGetField(file, TIFFTAG_IMAGELENGTH, &h))
        return LOAD_PARAM_FAIL;
    if (!TIFFGetField(file, TIFFTAG_IMAGEWIDTH, &w))
        return LOAD_PARAM_FAIL;
    if (width != 0 && (width != w || height != h)) {
        return LOAD_FORMAT_FAIL;
    } else {
        width = w;
        height = h;
    }
    
    unsigned int bits = 0;;
    if (!TIFFGetField(file, TIFFTAG_BITSPERSAMPLE, &bits))
        return LOAD_PARAM_FAIL;
    if (bits != 16)
        return LOAD_FORMAT_FAIL;
    
    unsigned int bytes = TIFFScanlineSize(file);
    
    unsigned int size = height * width;
    e.p.reset(new Pixel[size]);
    e.scaledData.push_back(e.p);
    cerr << "Loaded image " << e.filename << ", with" << (bytes < width * 8 ? "out" : "") << " alpha channel, "
        << width << 'x' << height << ", "
        << (size * sizeof(Pixel)) << " bytes allocated" << endl;
    
    
    tdata_t buffer = _TIFFmalloc(bytes);
    Pixel * pix = e.p.get();
    for (unsigned int row = 0; row < height; ++row) {
        TIFFReadScanline(file, buffer, row);
        uint16_t const * i = static_cast< uint16_t const * >(buffer);
        for (unsigned int column = 0; column < width; column++, pix++) {
            pix->r = *i++;
            pix->g = *i++;
            pix->b = *i++;
            pix->l = pix->r > pix->g ? pix->r : pix->g;
            pix->l = (pix->l > pix->b ? pix->l >> 2 : pix->b >> 2) + Pixel::with_threshold;
            if (bytes == width * 8)
                if (*i++ < (1 << 15))
                    pix->l = Pixel::transparent;
                if (pix->l < Pixel::transparent)
                    e.bn += pix->g;
                if (pix->r > e.maxR) e.maxR = pix->r;
                if (pix->g > e.maxG) e.maxG = pix->g;
                if (pix->b > e.maxB) e.maxB = pix->b;
        }
    }
    e.bn /= size;
    cerr << "  Brightness " << e.bn << endl;
    
    imgs.push_back(e);

    _TIFFfree(buffer);
    TIFFClose(file);
    
    return LOAD_SUCCESS;
}


void ExposureStack::scale(unsigned int i, unsigned int steps) {
    Exposure & e = imgs[i];
    unsigned int swidth = width;
    unsigned int sheight = height;
    for (unsigned int s = 1; s < steps; s++) {
        unsigned int width2 = swidth;
        swidth >>= 1;
        sheight >>= 1;
        if (s < e.scaledData.size()) continue;
        boost::shared_array<Pixel> r(new Pixel[swidth * sheight]), r2 = e.scaledData.back();
        for (unsigned int i = 0, i2 = 0; i < sheight; i++, i2 += 2)
            for (unsigned int j = 0, j2 = 0; j < swidth; j++, j2 += 2) {
                r[i*swidth + j].r = ((uint32_t)r2[i2*width2 + j2].r + r2[i2*width2 + j2 + 1].r
                + r2[(i2 + 1)*width2 + j2].r + r2[(i2 + 1)*width2 + j2 + 1].r) >> 2;
                r[i*swidth + j].g = ((uint32_t)r2[i2*width2 + j2].g + r2[i2*width2 + j2 + 1].g
                + r2[(i2 + 1)*width2 + j2].g + r2[(i2 + 1)*width2 + j2 + 1].g) >> 2;
                r[i*swidth + j].b = ((uint32_t)r2[i2*width2 + j2].b + r2[i2*width2 + j2 + 1].b
                + r2[(i2 + 1)*width2 + j2].b + r2[(i2 + 1)*width2 + j2 + 1].b) >> 2;
                r[i*swidth + j].l = r2[i2*width2 + j2].l;
                if (r[i*swidth + j].l < r2[i2*width2 + j2 + 1].l) r[i*swidth + j].l = r2[i2*width2 + j2 + 1].l;
                if (r[i*swidth + j].l < r2[(i2 + 1)*width2 + j2].l) r[i*swidth + j].l = r2[(i2 + 1)*width2 + j2].l;
                if (r[i*swidth + j].l < r2[(i2 + 1)*width2 + j2 + 1].l) r[i*swidth + j].l = r2[(i2 + 1)*width2 + j2 + 1].l;
            }
            e.scaledData.push_back(r);
    }
}


void ExposureStack::sort() {
    if (!imgs.empty()) {
        std::sort(imgs.begin(), imgs.end());
        for (vector<Exposure>::reverse_iterator cur = imgs.rbegin(), prev = cur++; cur != imgs.rend(); prev = cur++) {
            // Calculate median relative exposure
            const uint16_t min = 3275, max = 52430;
            vector<float> samples;
            const Pixel * rpix = prev->p.get(), * pix = cur->p.get();
            const Pixel * end = pix + width * height;
            for (; pix < end; rpix++, pix++) {
                if (rpix->l == Pixel::transparent)
                    continue;
                // Only sample those pixels that are in the linear zone
                //if (rpix->r < max && rpix->r > min && pix->r < max && pix->r > min)
                //      samples.push_back((float)rpix->r / pix->r);
                if (rpix->g < max && rpix->g > min && pix->g < max && pix->g > min)
                    samples.push_back((float)rpix->g / pix->g);
                //if (rpix->b < max && rpix->b > min && pix->b < max && pix->b > min)
                //      samples.push_back((float)rpix->b / pix->b);
            }
            std::sort(samples.begin(), samples.end());
            cur->immExp = samples[samples.size() / 2];
            cur->relExp = cur->immExp * prev->relExp;
            cerr << "Relative exposure: " << (1.0/cur->relExp) << '(' << (log(1.0/cur->relExp) / log(2.0)) << " EV)" << endl;
        }
        imgs.back().th = 0xffff;
    }
}


void ExposureStack::preScale() {
    for (unsigned int i = 0; i < imgs.size(); ++i)
        scale(i, 5);
}


void ExposureStack::setScale(unsigned int i) {
    currentScale = i;
    for (unsigned int i = 0; i < imgs.size(); ++i) {
        scale(i, currentScale + 1);
        imgs[i].p = imgs[i].scaledData[currentScale];
    }
}


void ExposureStack::setRelativeExposure(unsigned int i, double re) {
    imgs[i].immExp = re;
    // Recalculate relExp
    for (int j = i; j >= 0; --j) {
        imgs[j].relExp = imgs[j + 1].relExp * imgs[j].immExp;
    }
}


void ExposureStack::setThreshold(unsigned int i, uint16_t th) {
    imgs[i].th = (th >> 2) + Pixel::with_threshold;
}


template <typename PixelOp>
void fillCircle(int x0, int y0, int radius, const PixelOp & op) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;
    
    // Line from (x0, y0 + radius) to (x0, y0 - radius)
    for (int i = y0 - radius; i <= y0 + radius; ++i)
        op(x0, i);
    op(x0 + radius, y0);
    op(x0 - radius, y0);
    x++;
    ddF_x += 2;
    
    while(x < y)
    {
        // ddF_x == 2 * x + 1;
        // ddF_y == -2 * y;
        // f == x*x + y*y - radius*radius + 2*x - y + 1;
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
            // Line from (x0 + y, y0 - x) to (x0 + y, y0 + x)
            for (int i = y0 - x; i <= y0 + x; ++i)
                op(x0 + y, i);
            // Line from (x0 - y, y0 - x) to (x0 - y, y0 + x)
                for (int i = y0 - x; i <= y0 + x; ++i)
                    op(x0 - y, i);
        } else {
            // Only points
            op(x0 + y, y0 + x);
            op(x0 - y, y0 + x);
            op(x0 + y, y0 - x);
            op(x0 - y, y0 - x);
        }
        if (y - x > 0) {
            // Line from (x0 + x, y0 - y) to (x0 + x, y0 + y)
            for (int i = y0 - y; i <= y0 + y; ++i)
                op(x0 + x, i);
            // Line from (x0 - x, y0 - y) to (x0 - x, y0 + y)
                for (int i = y0 - y; i <= y0 + y; ++i)
                    op(x0 - x, i);
        }
        f += ddF_x;
        x++;
        ddF_x += 2;
    }
}


void ExposureStack::Exposure::addPixel(int x, int y, int width) {
    unsigned int pos = y * width + x;
    // If the pixel is not visible and not transparent
    uint16_t l = scaledData[0][pos].l;
    if (l != Pixel::transparent && l > th) {
        // If the threshold is enough to make it visible, do it
        uint16_t lth = Pixel::with_threshold + (l & Pixel::mask);
        if (lth <= th) {
            scaledData[0][pos].l = lth;
        } else {
            scaledData[0][pos].l = l & Pixel::mask;
        }
    }
}


void ExposureStack::Exposure::removePixel(int x, int y, int width) {
    unsigned int pos = y * width + x;
    // If the pixel is visible and not transparent
    uint16_t l = scaledData[0][pos].l;
    if (l != Pixel::transparent && l <= th) {
        // If the threshold is enough to make it invisible, do it
        uint16_t lth = Pixel::with_threshold + (l & Pixel::mask);
        if (lth > th) {
            scaledData[0][pos].l = lth;
        } else {
            scaledData[0][pos].l = Pixel::always_off + (l & Pixel::mask);
        }
    }
}


struct addCircleOfPixels {
    ExposureStack::Exposure & e;
    unsigned int w, h;
    addCircleOfPixels(ExposureStack::Exposure & exp, unsigned int width, unsigned int height) : e(exp), w(width), h(height) {}
    void operator()(int x, int y) const {
        if (x >= 0 && x < w && y >= 0 && y < h) e.addPixel(x, y, w);
    }
};


struct removeCircleOfPixels {
    ExposureStack::Exposure & e;
    unsigned int w, h;
    removeCircleOfPixels(ExposureStack::Exposure & exp, unsigned int width, unsigned int height) : e(exp), w(width), h(height) {}
    void operator()(int x, int y) const {
        if (x >= 0 && x < w && y >= 0 && y < h) e.removePixel(x, y, w);
    }
};


void ExposureStack::addPixels(unsigned int i, int x, int y, int radius) {
    x <<= currentScale;
    y <<= currentScale;
    radius <<= currentScale;
    fillCircle(x, y, radius, addCircleOfPixels(imgs[i], width, height));
    updateLValues(i, x - radius, y - radius, 2*radius + 1, 2*radius + 1);
}


void ExposureStack::removePixels(unsigned int i, int x, int y, int radius) {
    x <<= currentScale;
    y <<= currentScale;
    radius <<= currentScale;
    fillCircle(x, y, radius, removeCircleOfPixels(imgs[i], width, height));
    updateLValues(i, x - radius, y - radius, 2*radius + 1, 2*radius + 1);
}


void ExposureStack::updateLValues(unsigned int i, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    Exposure & e = imgs[i];
    unsigned int swidth = width;
    for (unsigned int s = 1; s < e.scaledData.size(); s++) {
        unsigned int width2 = swidth, x2 = x, y2 = y, w2 = w, h2 = h;
        swidth >>= 1;
        x >>= 1;
        y >>= 1;
        w >>= 1;
        h >>= 1;
        boost::shared_array<Pixel> r = e.scaledData[s], r2 = e.scaledData[s - 1];
        for (unsigned int i = y, i2 = y2; i < y + h; i++, i2 += 2)
            for (unsigned int j = x, j2 = x2; j < x + w; j++, j2 += 2) {
                r[i*swidth + j].l = r2[i2*width2 + j2].l;
                if (r[i*swidth + j].l < r2[i2*width2 + j2 + 1].l) r[i*swidth + j].l = r2[i2*width2 + j2 + 1].l;
                if (r[i*swidth + j].l < r2[(i2 + 1)*width2 + j2].l) r[i*swidth + j].l = r2[(i2 + 1)*width2 + j2].l;
                if (r[i*swidth + j].l < r2[(i2 + 1)*width2 + j2 + 1].l) r[i*swidth + j].l = r2[(i2 + 1)*width2 + j2 + 1].l;
            }
    }   
}


#ifdef HAVE_OPENEXR
void ExposureStack::saveEXR(const char * filename) {
/*
    SharedArray< Imf::Rgba > pixels( rows * columns );

    #pragma omp parallel for schedule( static )
    for ( int ii = 0; ii < static_cast< int >( rows * columns ); ++ii ) {

            pixels[ ii ].r = half( image[ ii * 3 + 0 ] );
            pixels[ ii ].g = half( image[ ii * 3 + 1 ] );
            pixels[ ii ].b = half( image[ ii * 3 + 2 ] );
            pixels[ ii ].a = half( 1 );
    }

    Imf::RgbaOutputFile file( filename, Imf::Header( columns, rows ) );
    file.setFrameBuffer( &pixels[ 0 ], 1, columns );
    file.writePixels( rows );
*/
}
#endif


void ExposureStack::savePFS(const char * filename) {
    unsigned int size = width * height;
    unsigned int tmpScale = currentScale;
    setScale(0);

    pfs::DOMIO pfsio;
    pfs::Frame * frame = pfsio.createFrame(width, height);

    // create channels for output
    pfs::Channel * r = NULL;
    pfs::Channel * g = NULL;
    pfs::Channel * b = NULL;
    frame->createXYZChannels(r, g, b);

    // Create merge map
    vector<float> map(size);
    // all pixels
    for (unsigned int j = 0; j < size; j++) {
        map[j] = &getExposureAt(j) - &imgs.front();
    }

    // Progressive merge: gaussian blur
    // TODO: configure radius and sigma
    const int radius = width > height ? height / 200 : width / 200;
    const float sigma = radius / 3.0f;
    gaussianBlur(map, width, radius, sigma);

    // TODO: Save merge map to file, for selection masks


    // Apply map
    for (unsigned int j = 0; j < size; j++) {
        int i = ceil(map[j]);
        Pixel * pix = &imgs[i].p[j];
        double relExp = imgs[i].relExp;
        if (i == 0) {
            (*r)(j) = pix->r * relExp / 65536.0;
            (*g)(j) = pix->g * relExp / 65536.0;
            (*b)(j) = pix->b * relExp / 65536.0;
        } else {
            double p = i - map[j];
            Pixel * ppix = &imgs[i - 1].p[j];
            double prelExp = imgs[i - 1].relExp;
            (*r)(j) = (pix->r * relExp * (1.0 - p) + ppix->r * prelExp * p) / 65536.0;
            (*g)(j) = (pix->g * relExp * (1.0 - p) + ppix->g * prelExp * p) / 65536.0;
            (*b)(j) = (pix->b * relExp * (1.0 - p) + ppix->b * prelExp * p) / 65536.0;
        }
    }

    // Save output
    pfs::transformColorSpace(pfs::CS_RGB, r, g, b, pfs::CS_XYZ, r, g, b);
    FILE * file = fopen(filename, "w");
    pfsio.writeFrame(frame, file);
    fclose(file);
    pfsio.freeFrame(frame);

    setScale(tmpScale);
}


void ExposureStack::gaussianBlur(vector<float> & m, int width, int radius, float sigma) {
    const float pi = 3.14159265358979323846;
    int size = m.size();
    int height = size / width;
    int samples = radius * 2 + 1;
    vector<float> weight(samples);
    float tss = 2.0 * sigma * sigma;
    float div = sqrt(pi * tss);

    // Calculate weights
    weight[radius] = 1.0 / div;
    for (int i = 1; i <= radius; i++)
        weight[radius - i] = weight[radius + i] = exp(-i*i / tss) / div;
    float norm = 0.0;
    for (int i = 0; i < samples; i++)
        norm += weight[i];
    for (int i = 0; i < samples; i++)
        weight[i] /= norm;

    vector<float> m2(size, 0.0);
    // Horizontal blur
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < samples; k++) {
                int kk = j + k - radius;
                if (kk < 0) kk = 0;
                else if (kk >= width) kk = width - 1;
                m2[i * width + j] += m[i * width + kk] * weight[k];
            }
        }
    }
    m.swap(m2);
    m2.assign(size, 0.0);
    // Vertical blur
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < samples; k++) {
                int kk = i + k - radius;
                if (kk < 0) kk = 0;
                else if (kk >= height) kk = height - 1;
                m2[i * width + j] += m[kk * width + j] * weight[k];
            }
        }
    }
    m.swap(m2);
}

