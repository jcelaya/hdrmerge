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

#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include "DngPropertiesDialog.hpp"

namespace hdrmerge {

DngPropertiesDialog::DngPropertiesDialog(QWidget * parent, Qt::WindowFlags f)
        : QDialog(parent, f), bps(16), previewSize(0) {
    QVBoxLayout * layout = new QVBoxLayout(this);

    QComboBox * bpsSelector = new QComboBox(this);
    bpsSelector->addItem("16");
    bpsSelector->addItem("24");
    bpsSelector->addItem("32");
    bpsSelector->setEditable(false);
    connect(bpsSelector, SIGNAL(currentIndexChanged( int)), this, SLOT(setBps(int)));

    QComboBox * previewSelector = new QComboBox(this);
    previewSelector->addItem(tr("Full"));
    previewSelector->addItem(tr("Half"));
    previewSelector->addItem(tr("None"));
    previewSelector->setEditable(false);
    connect(previewSelector, SIGNAL(currentIndexChanged( int)), this, SLOT(setBps(int)));

    QWidget * formWidget = new QWidget(this);
    QFormLayout * formLayout = new QFormLayout(formWidget);
    formLayout->addRow(tr("Bits per sample:"), bpsSelector);
    formLayout->addRow(tr("Preview size:"), previewSelector);
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget, 1);

    QPushButton * acceptButton = new QPushButton(tr("Accept"), this);
    acceptButton->setDefault(true);
    connect(acceptButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
    layout->addWidget(acceptButton, 0, Qt::AlignHCenter);

    setLayout(layout);
    setWindowTitle(tr("DNG Properties"));
}


void DngPropertiesDialog::setBps(int index) {
    switch (index) {
        case 0: bps = 16; break;
        case 1: bps = 24; break;
        case 2: bps = 32; break;
    }
}


void DngPropertiesDialog::setPreviewSize(int index) {
    previewSize = index;
}


void DngPropertiesDialog::setIndexFileName() {

}

} // namespace hdrmerge
