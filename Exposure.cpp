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


static float max3(float a, float b, float c) {
	float tmp = a > b ? a : b;
	return tmp > c ? tmp : c;
}


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
	r.resize(size);
	g.resize(size);
	b.resize(size);
	a.resize(size, true);   // Initially opaque
	ag.resize(size, true);
	cerr << "Loaded image " << name << ", with" << (bytes < width * 8 ? "out" : "") << " alpha channel, "
		<< width << 'x' << height << ", "
		<< (r.capacity() * sizeof(float) * 3 + a.capacity() / 4) << " bytes allocated" << endl;


	tdata_t buffer = _TIFFmalloc(bytes);
	bn = 0.0;
	for (unsigned int row = 0; row < height; ++row) {
		TIFFReadScanline(file, buffer, row);
		uint16_t const * i = static_cast< uint16_t const * >(buffer);
		for (unsigned int column = 0; column < width; column++) {
			unsigned int pos = row * width + column;
			r[pos] = *i++;
			g[pos] = *i++;
			b[pos] = *i++;
			if (bytes == width * 8)
				a[pos] = *i++ >= 1 << 15;
			if (a[pos])
				bn += r[pos] + g[pos] + b[pos];
		}
	}
	bn /= 3 * size;
	th = 0.8 * 65536.0f;
	cerr << "  Brightness " << bn << endl;

	_TIFFfree(buffer);
	TIFFClose(file);
}


void ExposureStack::Exposure::setRelativeExposure(const Exposure * ref) {
	if (ref == this) {
		relExp = 1.0;
		return;
	}
	// Calculate median relative exposure
	const float min = 0.05f * 65536.0f, max = 0.8 * 65536.0f;
	vector<float> samples;
	unsigned int size = r.size();
	for (unsigned int i = 0; i < size; i++) {
		if (!ref->a[i] || !a[i])
			continue;
		// Only sample those pixels that are in the linear zone
		if (ref->r[i] < max && ref->r[i] > min && r[i] < max && r[i] > min)
			samples.push_back(ref->r[i] / r[i]);
		if (ref->g[i] < max && ref->g[i] > min && g[i] < max && g[i] > min)
			samples.push_back(ref->g[i] / g[i]);
		if (ref->b[i] < max && ref->b[i] > min && b[i] < max && b[i] > min)
			samples.push_back(ref->b[i] / b[i]);
	}
	std::sort(samples.begin(), samples.end());
	relExp = samples[samples.size() / 2] * ref->relExp;
	cerr << "Relative exposure: " << (1.0/relExp) << '(' << (log(1.0/relExp) / log(2.0)) << " EV)" << endl;
}


/*
void Exposure::setThreshold(float th) {
	for (unsigned int i = 0; i < size; i++) {
		// Calculate maximum value among three channels
		float lum = max3(r[i], g[i], b[i]);
		// If it is under threshold, this is the correctly exposed pixel
		if (lum < th)
			m[i] = 1.0;
		else
			m[i] = 0.0;
	}
}
*/


void ExposureStack::sort() {
	if (!imgs.empty()) {
		std::sort(imgs.begin(), imgs.end(), sortExposurePointer);
		for (vector<Exposure *>::reverse_iterator p = imgs.rbegin(), n = p; n != imgs.rend(); p = n++)
			(*n)->setRelativeExposure(*p);
	}
}


void ExposureStack::rgb(unsigned int x, unsigned int y, float & r, float & g, float & b) const {
	unsigned int pos = y * width + x;
	for (unsigned int i = 0; i < imgs.size() - 1; i++) {
		Exposure * e = imgs[i];
		if (imgs[i]->a[pos] && imgs[i]->ag[pos]) {
			// Calculate maximum value among three channels
			float lum = max3(e->r[pos], e->g[pos], e->b[pos]);
			// If it is under threshold, this is the correctly exposed pixel
			if (lum < e->th) {
				r = e->r[pos] * e->relExp * wbr;
				g = e->g[pos] * e->relExp * wbg;
				b = e->b[pos] * e->relExp * wbb;
				return;
			}
		}
	}
	Exposure * e = imgs.back();
	r = e->r[pos] * e->relExp * wbr;
	g = e->g[pos] * e->relExp * wbg;
	b = e->b[pos] * e->relExp * wbb;
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
			// Calculate maximum value among three channels
			float lum = max3(imgs[i]->r[j], imgs[i]->g[j], imgs[i]->b[j]);
			// If it is under threshold, this is the correctly exposed pixel
			if (i == N - 1 || lum < imgs[i]->th) {
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
		if (i == 0) {
			(*r)(j) = imgs[i]->r[j] * imgs[i]->relExp;
			(*g)(j) = imgs[i]->g[j] * imgs[i]->relExp;
			(*b)(j) = imgs[i]->b[j] * imgs[i]->relExp;
		} else {
			float p = i - map[j];
			(*r)(j) = imgs[i]->r[j] * imgs[i]->relExp * (1.0f - p) + imgs[i - 1]->r[j] * imgs[i - 1]->relExp * p;
			(*g)(j) = imgs[i]->g[j] * imgs[i]->relExp * (1.0f - p) + imgs[i - 1]->g[j] * imgs[i - 1]->relExp * p;
			(*b)(j) = imgs[i]->b[j] * imgs[i]->relExp * (1.0f - p) + imgs[i - 1]->b[j] * imgs[i - 1]->relExp * p;
		}
	}

	// Save output
	pfs::transformColorSpace(pfs::CS_RGB, r, g, b, pfs::CS_XYZ, r, g, b);
	FILE * file = fopen(filename, "w");
	pfsio.writeFrame(frame, file);
	fclose(file);
	pfsio.freeFrame(frame);
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

