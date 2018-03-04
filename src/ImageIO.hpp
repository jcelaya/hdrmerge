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

#ifndef _IMAGEIO_H_
#define _IMAGEIO_H_

#include <vector>
#include <QImage>
#include <QDateTime>
#include <QString>
#include "ImageStack.hpp"
#include "ProgressIndicator.hpp"
#include "LoadSaveOptions.hpp"
#include "RawParameters.hpp"

namespace hdrmerge {

class ImageIO {
public:
    ImageIO() {}

    int load(const LoadOptions & options, ProgressIndicator & progress);
    void save(const SaveOptions & options, ProgressIndicator & progress);

    const ImageStack & getImageStack() const {
        return stack;
    }
    ImageStack & getImageStack() {
        return stack;
    }

    QString buildOutputFileName() const;
    QString getInputPath() const;
    QString replaceArguments(const QString & pattern, const QString & outFileName) const;
    static Image loadRawImage(RawParameters & rawParameters);
    static QImage renderPreview(const Array2D<float> & rawData, const RawParameters & rawParameters, float expShift, bool halfsize = false);

    struct QDateInterval {
        QDateTime start, end;
        bool operator<(const QDateInterval & r) const {
            return start < r.start;
        }
        double difference(const QDateInterval & r) const {
            return end.msecsTo(r.start) / 1000.0;
        }
    };
    static QDateInterval getImageCreationInterval(const QString & fileName);

private:
    ImageStack stack;
    std::vector<std::unique_ptr<RawParameters>> rawParameters;

    void writeMaskImage(const QString & maskFile);
};

} // namespace hdrmerge

#endif // _IMAGEIO_H_
