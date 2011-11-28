#ifndef _EXPOSURE_H_
#define _EXPOSURE_H_

#include <vector>
#include <boost/cstdint.hpp>


class ExposureStack {
	struct Pixel {
		uint16_t r, g, b;
		uint16_t l;

		static const uint16_t transparent = 1 << 15;
	};

	struct Exposure {
		std::vector<Pixel> p;   ///< Image data
		double bn;              ///< Brightness
		double relExp;          ///< Relative exposure
		uint16_t th;            ///< Exposure threshold
	
		/// Create an exposure from a linear 16 bit TIFF file
		Exposure(const char * fileName, unsigned int & width, unsigned int & height);

		/// Calculate relExp relative to a reference image
		void setRelativeExposure(const Exposure * ref);

		/// Order by brightness
		bool operator<(const Exposure & r) const { return bn > r.bn; }
	};

	std::vector<Exposure *> imgs;   ///< Exposures, from top to bottom
	double wbr;             ///< White balance red component
	double wbg;             ///< White balance green component
	double wbb;             ///< White balance blue component
	unsigned int width;     ///< Size of a row
	unsigned int height;    ///< Size of a row

public:
	ExposureStack() : wbr(1.0), wbg(1.0), wbb(1.0), width(0), height(0) {}

	~ExposureStack() {
		for (std::vector<Exposure *>::iterator it = imgs.begin(); it != imgs.end(); it++)
			delete *it;
	}

	/// Load an image into the stack
	void loadImage(const char * fileName) {
		imgs.push_back(new Exposure(fileName, width, height));
	}

	unsigned int size() const { return imgs.size(); }

	unsigned int getWidth() const {
		return width;
	}

	unsigned int getHeight() const {
		return height;
	}

	float getRelativeExposure(int i) const {
		return imgs[i]->relExp;
	}

	void setRelativeExposure(int i, float re) {
		imgs[i]->relExp = re;
	}

	uint16_t getThreshold(int i) const {
		return imgs[i]->th << 1;
	}

	void setThreshold(int i, uint16_t th) {
		imgs[i]->th = th >> 1;
	}

	void setWhiteBalance(double r, double g, double b) {
		wbr = r; wbg = g; wbb = b;
	}

	void calculateWB(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

	double getWBR() const { return wbr; }

	double getWBG() const { return wbg; }

	double getWBB() const { return wbb; }

	void rgb(unsigned int x, unsigned int y, float & r, float & g, float & b) const {
		unsigned int pos = y * width + x;
		Exposure * const * e = &imgs.front();
		while (e != &imgs.back() && (*e)->p[pos].l >= (*e)->th) e++;
		Pixel * pix = &(*e)->p[pos];
		double relExp = (*e)->relExp;
		r = pix->r * relExp * wbr;
		g = pix->g * relExp * wbg;
		b = pix->b * relExp * wbb;
	}


	/// Sort images and calculate relative exposure
	void sort();

	/// Render the image on a vector of double
	void render(std::vector<float> & r);

	/// Merge stack and save as EXR
	void saveEXR(const char * filename);

	/// Merge stack and save as PFS stream
	void savePFS(const char * filename);

	/// Apply a gaussian blur on a mask
	static void gaussianBlur(std::vector<float> & m, int width, int radius, float sigma);

	static bool sortExposurePointer(const Exposure * l, const Exposure * r) { return *l < *r; }
};


#endif // _EXPOSURE_H_
