#ifndef _EXPOSURE_H_
#define _EXPOSURE_H_

#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_array.hpp>
#include "config.h"


class ExposureStack {
public:
    struct Pixel {
        uint16_t r, g, b;
        uint16_t l;

        static const uint16_t always_on = 0;
        static const uint16_t with_threshold = 1 << 14;
        static const uint16_t always_off = 1 << 15;
        static const uint16_t transparent = 3 << 14;
        static const uint16_t mask = (1 << 14) - 1;
    };

    struct Exposure {
        std::string filename;
        boost::shared_array<Pixel> p;   ///< Image data
        std::vector<boost::shared_array<Pixel> > scaledData;
        uint16_t maxR, maxG, maxB;
        double bn;              ///< Brightness
        double relExp;          ///< Relative exposure
        double immExp;          ///< Exposure relative to the next image
        uint16_t th;            ///< Exposure threshold

        Exposure(const char * f) : filename(f), maxR(0), maxG(0), maxB(0), bn(0.0), relExp(1.0), immExp(1.0), th((254 << 6) + Pixel::with_threshold) {}

        /// Order by brightness
        bool operator<(const Exposure & r) const { return bn > r.bn; }
        
        void addPixel(int x, int y, int width);
        void removePixel(int x, int y, int width);
    };

    ExposureStack() : width(0), height(0), currentScale(0) {}
    
    enum LoadResult {
        LOAD_SUCCESS = 0,
        LOAD_OPEN_FAIL = 1,
        LOAD_PARAM_FAIL = 2,
        LOAD_FORMAT_FAIL = 3,
    };
    
    /**
     * Load an image into the stack
     * Create an exposure from a linear 16 bit TIFF file
     * On error, no image is loaded.
     * @return Error code, or 0 if success.
     */
    LoadResult loadImage(const char * fileName);
    
    /// Sort images and calculate relative exposure
    void sort();
    
    void preScale();
    
    void setScale(unsigned int i);
    
    unsigned int size() const { return imgs.size(); }
    
    unsigned int getWidth() const {
        return width >> currentScale;
    }
    
    unsigned int getHeight() const {
        return height >> currentScale;
    }
    
    float getRelativeExposure(unsigned int i) const {
        return imgs[i].immExp;
    }
    
    uint16_t getThreshold(unsigned int i) const {
        return (imgs[i].th - Pixel::with_threshold) << 2;
    }
    
    const std::string & getFileName(unsigned int i) const {
        return imgs[i].filename;
    }
    
    void setRelativeExposure(unsigned int i, double re);
    
    void setThreshold(unsigned int i, uint16_t th);
    
    void addPixels(unsigned int i, int x, int y, int radius);
    
    void removePixels(unsigned int i, int x, int y, int radius);
    
    void rgb(unsigned int x, unsigned int y, double & r, double & g, double & b) const {
        unsigned int pos = y * (width >> currentScale) + x;
        const Exposure & e = getExposureAt(pos);
        Pixel * pix = &e.p[pos];
        double relExp = e.relExp;
        r = pix->r * relExp;
        g = pix->g * relExp;
        b = pix->b * relExp;
    }
    
    #ifdef HAVE_OPENEXR
    /// Merge stack and save as EXR
    void saveEXR(const char * filename);
    #endif
    
    /// Merge stack and save as PFS stream
    void savePFS(const char * filename);
    
    /// Apply a gaussian blur on a mask
    static void gaussianBlur(std::vector<float> & m, int width, int radius, float sigma);
    
private:
    std::vector<Exposure> imgs;   ///< Exposures, from top to bottom
    std::vector<unsigned char> map;   ///< Fusion map
    unsigned int width;     ///< Size of a row
    unsigned int height;    ///< Size of a column
    unsigned int currentScale;     ///< Current scale factor

    const Exposure & getExposureAt(unsigned int pos) const {
        const Exposure * e = &imgs.front();
        while (e->p[pos].l > e->th) e++;
        // If the last image pixel is transparent, look for another one that is not
        while (e->p[pos].l == Pixel::transparent && e != &imgs.front()) e--;
        return *e;
    }

    /// Create the scaled copies of an image, scale is 1/2 each step
    void scale(unsigned int i, unsigned int steps);

    void updateLValues(unsigned int i, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
};


#endif // _EXPOSURE_H_
