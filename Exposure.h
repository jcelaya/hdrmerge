#ifndef _EXPOSURE_H_
#define _EXPOSURE_H_

#include <vector>
#include <boost/cstdint.hpp>




class ExposureStack {
	struct Exposure {
		std::vector<float> r;   ///< Red channel
		std::vector<float> g;   ///< Green channel
		std::vector<float> b;   ///< Blue channel
		std::vector<bool> a;    ///< Alpha channel, only opaque/transparent
		std::vector<bool> ag;   ///< Antighosting mask
		float bn;               ///< Brightness
		float relExp;           ///< Relative exposure
		float th;               ///< Exposure threshold
	
		/// Create an exposure from a linear 16 bit TIFF file
		Exposure(const char * fileName, unsigned int & width, unsigned int & height);

		/// Calculate relExp relative to a reference image
		void setRelativeExposure(const Exposure * ref);

		/// Order by brightness
		bool operator<(const Exposure & r) const { return bn > r.bn; }
	};

	std::vector<Exposure *> imgs;   ///< Exposures, from top to bottom
	float wbr;              ///< White balance red component
	float wbg;              ///< White balance green component
	float wbb;              ///< White balance blue component
	unsigned int width;     ///< Size of a row
	unsigned int height;    ///< Size of a row

public:
	ExposureStack() : wbr(1.0f), wbg(1.0f), wbb(1.0f), width(0), height(0) {}

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

	int getThreshold(int i) const {
		return (int)(imgs[i]->th / 655.36f);
	}

	void setThreshold(int i, int th) {
		imgs[i]->th = th * 655.36f;
	}

	void rgb(unsigned int x, unsigned int y, float & r, float & g, float & b) const;

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
