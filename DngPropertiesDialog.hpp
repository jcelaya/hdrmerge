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

#ifndef _DNGPROPERTIESDIALOG_H_
#define _DNGPROPERTIESDIALOG_H_

#include <QDialog>
#include <QLineEdit>

namespace hdrmerge {

class DngPropertiesDialog : public QDialog {
public:
    DngPropertiesDialog(QWidget * parent = 0, Qt::WindowFlags f = 0);

    /// Triggered when the dialog is closed
    void closeEvent(QCloseEvent * event) { accept(); }

    int getBps() const {
        return bps;
    }
    int getPreviewSize() const {
        return previewSize;
    }
    QString getIndexFileName() const {
        return indexFileEditor->text();
    }
    void setIndexFileName(const QString & name) {
        indexFileEditor->setText(name);
    }

private slots:
    void setBps(int index);
    void setPreviewSize(int index);
    void setIndexFileName();

private:
    Q_OBJECT

    int bps;
    int previewSize;
    QLineEdit * indexFileEditor;
};

} // namespace hdrmerge

#endif // _DNGPROPERTIESDIALOG_H_
