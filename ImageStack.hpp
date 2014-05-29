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
#include "ProgressIndicator.hpp"
#include "Array2D.hpp"
#include "EditableMask.hpp"
#include "LoadSaveOptions.hpp"

namespace hdrmerge {

class ImageStack {
public:
    ImageStack() : mask(this), width(0), height(0) { setGamma(2.2f); }

    int load(const LoadOptions & options, ProgressIndicator & progress);
    int save(const SaveOptions & options, ProgressIndicator & progress);
    void writeMaskImage(const std::string & maskFile);
    void align();
    void crop();
    void computeRelExposures();
    void generateMask();

    size_t size() const { return images.size(); }

    size_t getWidth() const {
        return width;
    }

    size_t getHeight() const {
        return height;
    }

    Image & getImage(unsigned int i) {
        return *images[i];
    }
    const Image & getImage(unsigned int i) const {
        return *images[i];
    }
    EditableMask & getMask() {
        return mask;
    }

    bool addImage(std::unique_ptr<Image> & i);
    std::string buildOutputFileName() const;
    double value(size_t x, size_t y) const;
    Array2D<float> compose() const;
    void setGamma(float g);
    uint8_t toneMap(double v) {
        return toneCurve[(int)std::floor(v)];
    }
    uint8_t getImageAt(size_t x, size_t y) const {
        return mask(x, y);
    }
    bool isLayerValidAt(int layer, size_t x, size_t y) const {
        return images[layer]->contains(x, y);
    }

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

    std::vector<std::unique_ptr<Image>> images;   ///< Images, from most to least exposed
    EditableMaskImpl mask;
    size_t width;
    size_t height;
    uint8_t toneCurve[65536];

    std::string replaceArguments(const std::string & maskFileName, const std::string & outFileName);
};

} // namespace hdrmerge

#endif // _IMAGESTACK_H_
