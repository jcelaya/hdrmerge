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

#ifndef _ABOUTDIALOG_H_
#define _ABOUTDIALOG_H_

#include <QDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>

namespace hdrmerge {

class AboutDialog : public QDialog {
public:
    explicit AboutDialog(QWidget * parent = 0, Qt::WindowFlags f = 0) : QDialog(parent, f) {
        QHBoxLayout * layout = new QHBoxLayout(this);
        logoLabel = new QLabel(this);
        logoLabel->setPixmap(QPixmap(":/images/logo.png"));
        layout->addWidget(logoLabel);
        layout->addSpacing(12);
        text = new QLabel("<h1>HDRMerge</h1>"
        "<p>" + tr("A software for the fusion of multiple raw images into a single high dynamic range image.") + "</p>"
            "<p>Copyright &copy; 2012 Javier Celaya (jcelaya@gmail.com)</p>"
            "<p>This is free software: you can redistribute it and/or modify it under the terms of the GNU "
            "General Public License as published by the Free Software Foundation, either version 3 of the License, "
            "or (at your option) any later version.</p>", this);
        text->setWordWrap(true);
        layout->addWidget(text);
        layout->setAlignment(text, Qt::AlignTop);
        setWindowTitle(tr("About HDRMerge..."));
    }

    /// Triggered when the dialog is closed
    void closeEvent(QCloseEvent * event) { accept(); }

private:
    Q_OBJECT

    QLabel * logoLabel;
    QLabel * text;
};

} // namespace hdrmerge

#endif // _ABOUTDIALOG_H_
