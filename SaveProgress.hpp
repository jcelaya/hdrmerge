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

#ifndef _SAVEPROGRESS_HPP
#define _SAVEPROGRESS_HPP

#include <iostream>
#include <string>
#include <QObject>
#include <QString>
#include <QProgressDialog>
#include "DngWriter.hpp"

namespace hdrmerge {

class SaveProgress : public QObject {//, public ProgressIndicator {
public:
    SaveProgress(const ImageStack * i, const std::string & f, QProgressDialog * p) :
            QObject(NULL), images(i), fileName(f), state(-1) {
        p->setMaximum(5);
        p->setMinimum(0);
        p->setCancelButtonText(QString());
        connect(this, SIGNAL(setValue(int)), p, SLOT(setValue(int)));
        connect(this, SIGNAL(setLabel(QString)), p, SLOT(setLabelText(QString)));
    }

    void save() {
        DngWriter writer(*images);
        writer.write(fileName);
        advance("Done!");
    }

    virtual void advance(const std::string & message) {
        state++;
        std::cout << message << std::endl;
        emit setValue(state);
        emit setLabel(QString(message.c_str()));
    }
private:
    Q_OBJECT

    const ImageStack * images;
    std::string fileName;
    int state;

signals:
    void setValue(int value);
    void setLabel(QString text);
};

} // namespace hdrmerge

#endif // _SAVEPROGRESS_HPP
