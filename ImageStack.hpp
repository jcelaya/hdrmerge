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

#ifndef _IMAGESTACK_H_
#define _IMAGESTACK_H_

#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include "Image.hpp"
#include "Array2D.hpp"
#include "EditableMask.hpp"
#include "LoadSaveOptions.hpp"

namespace hdrmerge {

class ImageStack {
public:
    ImageStack() : mask(this), width(0), height(0), flip(0) {}
    void clear() {
        images.clear();
        width = height = 0;
        mask.reset();
    }

    int addImage(Image && i);
    void align();
    void crop();
    void computeResponseFunctions();
    void generateMask();
    Array2D<float> compose(const RawParameters & md, int featherRadius) const;

    size_t size() const { return images.size(); }

    size_t getWidth() const {
        return width;
    }
    size_t getHeight() const {
        return height;
    }
    int getFlip() const {
        return flip;
    }
    void setFlip(int f) {
        flip = f;
    }
    double getMaxExposure() const {
        return images.back().getRelativeExposure() / images[0].getRelativeExposure();
    }
    bool isCropped() const {
        return width != images[0].getWidth() || height != images[0].getHeight();
    }

    Image & getImage(unsigned int i) {
        return images[i];
    }
    const Image & getImage(unsigned int i) const {
        return images[i];
    }
    uint8_t getImageAt(size_t x, size_t y) const {
        return mask(x, y);
    }
    EditableMask & getMask() {
        return mask;
    }

    double value(size_t x, size_t y) const;

    bool isLayerValidAt(int layer, size_t x, size_t y) const {
        return images[layer].contains(x, y);
    }
    void calculateSaturationLevel(const RawParameters & params, bool useCustomWl = false);

private:
    class EditableMaskImpl : public EditableMask {
    public:
        EditableMaskImpl(const ImageStack * s) : EditableMask(), stack(s) {}
    private:
        const ImageStack * stack;
        virtual bool isLayerValidAt(int layer, int x, int y) const {
            return stack->isLayerValidAt(layer, x, y);
        }
    };

    std::vector<Image> images;   ///< Images, from most to least exposed
    EditableMaskImpl mask;
    Array2D<uint8_t> origMask;
    size_t width;
    size_t height;
    int flip;
    uint16_t satThreshold;
};

} // namespace hdrmerge

#endif // _IMAGESTACK_H_
