#ifndef _EXPOSURE_H_
#define _EXPOSURE_H_

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/shared_array.hpp>


class ExposureStack {
	struct Pixel {
		uint16_t r, g, b;
		uint16_t l;
		uint16_t max() const {
			if (l >= transparent) return transparent;
			uint16_t c = r > g ? r : g;
			return c > b ? c >> 1 : b >> 1;
		}

		static const uint16_t transparent = 1 << 15;
	};

	struct Exposure {
		boost::shared_array<Pixel> p;   ///< Image data
		std::vector<boost::shared_array<Pixel> > scaledData;
		double bn;              ///< Brightness
		double relExp;          ///< Relative exposure
		double immExp;          ///< Exposure relative to the next image
		uint16_t th;            ///< Exposure threshold
	
		/// Create an exposure from a linear 16 bit TIFF file
		Exposure(const char * fileName, unsigned int & width, unsigned int & height);

		/// Create the scaled copies of an image, scale is 1/2 each step
		void scaled(unsigned int steps, unsigned int width, unsigned int height);

		/// Calculate relExp relative to a reference image
		void setRelativeExposure(const Exposure & ref, unsigned int size);

		/// Order by brightness
		bool operator<(const Exposure & r) const { return bn > r.bn; }
	};

	std::vector<Exposure> imgs;   ///< Exposures, from top to bottom
	std::vector<unsigned char> map;   ///< Fusion map
	double wbr;             ///< White balance red component
	double wbg;             ///< White balance green component
	double wbb;             ///< White balance blue component
	unsigned int width;     ///< Size of a row
	unsigned int height;    ///< Size of a column
	unsigned int scale;     ///< Current scale factor

public:
	ExposureStack() : wbr(1.0), wbg(1.0), wbb(1.0), width(0), height(0), scale(0) {}

	/// Load an image into the stack
	void loadImage(const char * fileName) {
		imgs.push_back(Exposure(fileName, width, height));
	}

	/// Sort images and calculate relative exposure
	void sort();

	void preScale();

	void setScale(unsigned int i);

	unsigned int size() const { return imgs.size(); }

	unsigned int getWidth() const {
		return width >> scale;
	}

	unsigned int getHeight() const {
		return height >> scale;
	}

	float getRelativeExposure(int i) const {
		return imgs[i].immExp;
	}

	uint16_t getThreshold(int i) const {
		return imgs[i].th << 1;
	}

	double getWBR() const { return wbr; }

	double getWBG() const { return wbg; }

	double getWBB() const { return wbb; }

	void setRelativeExposure(int i, double re);

	void setThreshold(int i, uint16_t th);

	void setWhiteBalance(double r, double g, double b);

	void calculateWB(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

	void rgb(unsigned int x, unsigned int y, float & r, float & g, float & b) const {
		unsigned int pos = y * (width >> scale) + x;
		const Exposure * e = &imgs.front();
		//while (e != &imgs.back() && e->p[pos].l > e->th) e++;
		while (e != &imgs.back() && e->p[pos].max() > e->th) e++;
		Pixel * pix = &e->p[pos];
		double relExp = e->relExp;
		r = pix->r * relExp * wbr;
		g = pix->g * relExp * wbg;
		b = pix->b * relExp * wbb;
	}

	/// Merge stack and save as EXR
	void saveEXR(const char * filename);

	/// Merge stack and save as PFS stream
	void savePFS(const char * filename);

	/// Apply a gaussian blur on a mask
	static void gaussianBlur(std::vector<float> & m, int width, int radius, float sigma);
};


#endif // _EXPOSURE_H_
