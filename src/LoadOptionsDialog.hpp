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

#ifndef _LOADOPTIONSDIALOG_H_
#define _LOADOPTIONSDIALOG_H_

#include <QDialog>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include "LoadSaveOptions.hpp"

namespace hdrmerge {

class LoadOptionsDialog : public QDialog, public LoadOptions {
public:
    LoadOptionsDialog(QWidget * parent = 0, Qt::WindowFlags f = 0);

    void closeEvent(QCloseEvent * event) { reject(); }

public slots:
    virtual void accept();
    void addFiles();
    void removeFiles();

protected:
    virtual void showEvent(QShowEvent * event);

private:
    Q_OBJECT

    QListWidget * fileList;
    QCheckBox * alignBox;
    QCheckBox * cropBox;
    QCheckBox * customWhiteLevelBox;
    QSpinBox * customWhiteLevelSpinBox;
};

} // namespace hdrmerge

#endif // _LOADOPTIONSDIALOG_H_
