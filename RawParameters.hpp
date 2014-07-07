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

#ifndef _RAWPARAMETERS_H_
#define _RAWPARAMETERS_H_

#include <string>
#include "Array2D.hpp"

class LibRaw;

namespace hdrmerge {

class RawParameters {
public:
    RawParameters();
    RawParameters(const std::string & f) : RawParameters() {
        fileName = f;
    }
    virtual ~RawParameters() {}

    void fromLibRaw(const LibRaw & rawData);

    bool isSameFormat(const RawParameters & r) const {
        return width == r.width && height == r.height && filters == r.filters && cdesc == r.cdesc;
    }
    uint8_t FC(int x, int y) const {
        // (x, y) is relative to the ACTIVE AREA
        if (filters == 9) {
            return xtrans[(y + topMargin + 6) % 6][(x + leftMargin + 6) % 6];
        } else {
            return (filters >> (((y << 1 & 14) | (x & 1)) << 1) & 3);
        }
    }
    double logExp() const;
    void dumpInfo() const;
    uint16_t blackAt(int x, int y) const {
        return cblack[FC(x, y)];
    }
    bool hasBlack() const {
        return black || cblack[0] || cblack[1] || cblack[2] || cblack[3];
    }
    float whiteMultAt(int x, int y) const {
        return camMul[FC(x, y)];
    }
    void adjustWhite(const Array2D<uint16_t> & image);
    void autoWB(const Array2D<uint16_t> & image);
    bool canAlign() const;

    std::string fileName;
    size_t width, height;
    size_t rawWidth, rawHeight, topMargin, leftMargin;
    std::string cdesc;
    uint32_t filters;
    uint8_t xtrans[6][6];
    uint16_t max;
    uint16_t black;
    uint16_t maxBlack;
    uint16_t cblack[4];
    float preMul[4];
    float camMul[4];
    float camXyz[4][3];
    float rgbCam[3][4];
    float isoSpeed;
    float shutter;
    float aperture;
    std::string maker, model, description, dateTime;
    int colors;
    int flip;
    int tiffOrientation;

private:
    void adjustBlack();
    void calculateCamXyz();
    void loadCamXyzFromDng();
    void camXyzFromRgbCam();
};

} // namespace hdrmerge

#endif // _RAWPARAMETERS_H_
