#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <tiff.h>
#include <tiffio.h>
#include <ImfRgbaFile.h>
#include <pfs-1.2/pfs.h>
#include "Exposure.h"
using namespace std;


ExposureStack::Exposure::Exposure(const char * fileName, unsigned int & width, unsigned int & height) {
	string name(fileName);
	TIFF* file = TIFFOpen(fileName, "r");
	if (file == NULL)
		throw std::runtime_error("unable to open image file");

	unsigned int w, h;
	if (!TIFFGetField(file, TIFFTAG_IMAGELENGTH, &h))
		throw std::runtime_error( "unable to read rows" );
	if (!TIFFGetField(file, TIFFTAG_IMAGEWIDTH, &w))
		throw std::runtime_error( "unable to read columns" );
	if (width != 0 && (width != w || height != h))
		throw std::runtime_error( "images must be same size" );
	else {
		width = w;
		height = h;
	}

	unsigned int bits = 0;;
	if (!TIFFGetField(file, TIFFTAG_BITSPERSAMPLE, &bits))
		throw std::runtime_error( "unable to read bits" );
	if (bits != 16)
		throw std::runtime_error( "expected 16-bit linear color channels" );

	unsigned int bytes = TIFFScanlineSize(file);

	unsigned int size = height * width;
	p.reset(new Pixel[size]);
	cerr << "Loaded image " << name << ", with" << (bytes < width * 8 ? "out" : "") << " alpha channel, "
		<< width << 'x' << height << ", "
		<< (size * sizeof(Pixel)) << " bytes allocated" << endl;


	tdata_t buffer = _TIFFmalloc(bytes);
	bn = 0.0;
	maxR = maxB = maxG = 0;
	Pixel * pix = p.get();
	for (unsigned int row = 0; row < height; ++row) {
		TIFFReadScanline(file, buffer, row);
		uint16_t const * i = static_cast< uint16_t const * >(buffer);
		for (unsigned int column = 0; column < width; column++, pix++) {
			pix->r = *i++;
			pix->g = *i++;
			pix->b = *i++;
			pix->l = pix->r > pix->g ? pix->r : pix->g;
			pix->l = pix->l > pix->b ? pix->l >> 1 : pix->b >> 1;
			if (bytes == width * 8)
				if (*i++ < Pixel::transparent)
					pix->l += Pixel::transparent;
			if (pix->l < Pixel::transparent)
				bn += pix->g;
			if (pix->r > maxR) maxR = pix->r;
			if (pix->g > maxG) maxG = pix->g;
			if (pix->b > maxB) maxB = pix->b;
		}
	}
	bn /= size;
	th = 25600;
	cerr << "  Brightness " << bn << endl;

	_TIFFfree(buffer);
	TIFFClose(file);
}


void ExposureStack::Exposure::scaled(unsigned int steps, unsigned int width, unsigned int height) {
	scaledData.reserve(steps);
	scaledData.clear();
	scaledData.push_back(p);
	for (unsigned int s = 1; s < steps; s++) {
		unsigned int width2 = width;
		width >>= 1;
		height >>= 1;
		boost::shared_array<Pixel> r(new Pixel[width * height]), r2 = scaledData.back();
		for (unsigned int i = 0, i2 = 0; i < height; i++, i2 += 2)
			for (unsigned int j = 0, j2 = 0; j < width; j++, j2 += 2) {
				r[i*width + j].r = ((uint32_t)r2[i2*width2 + j2].r + r2[i2*width2 + j2 + 1].r
                                        + r2[(i2 + 1)*width2 + j2].r + r2[(i2 + 1)*width2 + j2 + 1].r) >> 2;
				r[i*width + j].g = ((uint32_t)r2[i2*width2 + j2].g + r2[i2*width2 + j2 + 1].g
                                        + r2[(i2 + 1)*width2 + j2].g + r2[(i2 + 1)*width2 + j2 + 1].g) >> 2;
				r[i*width + j].b = ((uint32_t)r2[i2*width2 + j2].b + r2[i2*width2 + j2 + 1].b
                                        + r2[(i2 + 1)*width2 + j2].b + r2[(i2 + 1)*width2 + j2 + 1].b) >> 2;
			}
		scaledData.push_back(r);
	}
}


void ExposureStack::Exposure::setRelativeExposure(const Exposure & ref, unsigned int size) {
	if (&ref == this) {
		relExp = immExp = 1.0;
		return;
	}
	// Calculate median relative exposure
	const uint16_t min = 3275, max = 52430;
	vector<float> samples;
	const Pixel * rpix = ref.p.get(), * pix = p.get();
	const Pixel * end = pix + size;
	for (; pix < end; rpix++, pix++) {
		if (rpix->l >= Pixel::transparent)
			continue;
		// Only sample those pixels that are in the linear zone
		//if (rpix->r < max && rpix->r > min && pix->r < max && pix->r > min)
		//	samples.push_back((float)rpix->r / pix->r);
		if (rpix->g < max && rpix->g > min && pix->g < max && pix->g > min)
			samples.push_back((float)rpix->g / pix->g);
		//if (rpix->b < max && rpix->b > min && pix->b < max && pix->b > min)
		//	samples.push_back((float)rpix->b / pix->b);
	}
	std::sort(samples.begin(), samples.end());
	immExp = samples[samples.size() / 2];
	relExp = immExp * ref.relExp;
	cerr << "Relative exposure: " << (1.0/relExp) << '(' << (log(1.0/relExp) / log(2.0)) << " EV)" << endl;
}


void ExposureStack::sort() {
	if (!imgs.empty()) {
		std::sort(imgs.begin(), imgs.end());
		for (vector<Exposure>::reverse_iterator p = imgs.rbegin(), n = p; n != imgs.rend(); p = n++)
			n->setRelativeExposure(*p, width * height);
		// Calculate fusion map
		map.resize(width * height);
		unsigned int N = imgs.size();
		for (unsigned int j = 0; j < width * height; j++) {
			unsigned int i;
			for (i = 0; i < N - 1 && imgs[i].p[j].l >= imgs[i].th; i++);
			map[j] = i;
		}
	}
}


void ExposureStack::preScale() {
	for (vector<Exposure>::iterator it = imgs.begin(); it != imgs.end(); it++)
		it->scaled(5, width, height);
	// Calculate auto white balance, with gray world
	calculateWB(0, 0, width, height);
}


void ExposureStack::setScale(unsigned int i) {
	scale = i;
	for (vector<Exposure>::iterator it = imgs.begin(); it != imgs.end(); it++)
		it->p = it->scaledData[scale];
}


void ExposureStack::setRelativeExposure(int i, double re) {
	imgs[i].immExp = re;
	// Recalculate relExp
	for (int j = i; j >= 0; j--) {
		imgs[j].relExp = imgs[j + 1].relExp * imgs[j].immExp;
	}
}


void ExposureStack::setThreshold(int i, uint16_t th) {
	imgs[i].th = th >> 1;
}


/*
void ExposureStack::setWhiteBalance(double r, double g, double b) {
	wbr = r;
	wbg = g;
	wbb = b;
}
*/


void ExposureStack::calculateWB(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	x <<= scale;
	y <<= scale;
	w <<= scale;
	h <<= scale;
	cerr << "Calculating white balance in " << x << ',' << y << ':' << w << 'x' << h << endl;
	// Calculate white balance
	wbr = 0.0;
	wbg = 0.0;
	wbb = 0.0;
	for (unsigned int i = x; i < x + w; i++) {
		for (unsigned int j = y; j < y + h; j++) {
			unsigned int pos = j * width + i;
			const Exposure * e = &imgs.front();
			while (e != &imgs.back() && (e->scaledData[0])[pos].l >= e->th) e++;
			Pixel * pix = &(e->scaledData[0])[pos];
			double relExp = e->relExp;
			wbr += pix->r * relExp;
			wbg += pix->g * relExp;
			wbb += pix->b * relExp;
		}
	}
	// TODO: What if min == 0 ??????????
	double min = wbr < wbg ? wbr : wbg;
	min = wbb < min ? wbb : min;
	wbr = min / wbr;
	wbg = min / wbg;
	wbb = min / wbb;
	// Optimize white point
	double wp = 0;
	for (vector<Exposure>::iterator it = imgs.begin(); it != imgs.end(); it++) {
		if (wp < it->maxR * it->relExp * wbr) wp = it->maxR * it->relExp * wbr;
		if (wp < it->maxG * it->relExp * wbg) wp = it->maxG * it->relExp * wbg;
		if (wp < it->maxB * it->relExp * wbb) wp = it->maxB * it->relExp * wbb;
	}
	wp = 65536.0 / wp;
	wbr *= wp;
	wbg *= wp;
	wbb *= wp;
	cerr << "White balance R:" << wbr << " G:" << wbg << " B:" << wbb << endl;
}


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


void ExposureStack::savePFS(const char * filename) {
	unsigned int size = width * height;
	unsigned int N = imgs.size();
	unsigned int tmpScale = scale;
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
		// For each exposure...
		for (unsigned int i = 0; i < N; i++) {
			// If it is under threshold, this is the correctly exposed pixel
			if (i == N - 1 || imgs[i].p[j].l < imgs[i].th) {
				map[j] = i;
				break;
			}
		}
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
			(*r)(j) = pix->r * relExp * wbr / 65536.0;
			(*g)(j) = pix->g * relExp * wbg / 65536.0;
			(*b)(j) = pix->b * relExp * wbb / 65536.0;
		} else {
			double p = i - map[j];
			Pixel * ppix = &imgs[i - 1].p[j];
			double prelExp = imgs[i - 1].relExp;
			(*r)(j) = (pix->r * relExp * (1.0 - p) + ppix->r * prelExp * p) * wbr / 65536.0;
			(*g)(j) = (pix->g * relExp * (1.0 - p) + ppix->g * prelExp * p) * wbg / 65536.0;
			(*b)(j) = (pix->b * relExp * (1.0 - p) + ppix->b * prelExp * p) * wbb / 65536.0;
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

