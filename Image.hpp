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
 */

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <vector>
#include <string>
#include <memory>
#include <libraw/libraw.h>


namespace hdrmerge {

class Image {
public:
    Image(const char * f);

    bool loadFailed() const {
        return pixel == nullptr;
    }
    size_t getWidth() const {
        return width;
    }
    size_t getHeight() const {
        return height;
    }
    int getDeltaX() const {
        return dx;
    }
    int getDeltaY() const {
        return dy;
    }
    bool isWrongFormat(const Image & ref) const;
    /// Order by exposure
    bool operator<(const Image & r) const { return logExp > r.logExp; }
    void setNextImage(Image * n) {
        nextImage = n;
    }
    void alignWith(const Image & r, float threshold, float tolerance);

    void dumpInfo(const LibRaw & rawData) const;

    static uint16_t getMedian(const uint16_t * values, size_t size, float threshold);

private:
    static const int scaleSteps = 6;

    std::string fileName;
    uint16_t * pixel;
    std::vector<std::unique_ptr<uint16_t[]>> scaledData;
    size_t width, height;
    int dx, dy;
    std::string cdesc;
    uint32_t filter;
    uint16_t max;
    double logExp;          ///< Logarithmic exposure, from metadata
    double relExp;          ///< Relative exposure, from data
    Image * nextImage;
    double immExp;          ///< Exposure relative to the next image

    void subtractBlack(const LibRaw & rawData);
    void computeLogExp(const LibRaw & rawData);
    void computeRelExp();
    void preScale();
    // From LibRaw
    int FC(int row, int col) const {
        return (filter >> (((row << 1 & 14) | (col & 1)) << 1) & 3);
    }
};

} // namespace hdrmerge

#endif // _IMAGE_H_
