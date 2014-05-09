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
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QFileDialog>
#include <QCheckBox>
#include "DngPropertiesDialog.hpp"

namespace hdrmerge {

DngPropertiesDialog::DngPropertiesDialog(QWidget * parent, Qt::WindowFlags f)
        : QDialog(parent, f), bps(16), previewSize(0) {
    QVBoxLayout * layout = new QVBoxLayout(this);

    QWidget * bpsSelector = new QWidget(this);
    QHBoxLayout * bpsSelectorLayout = new QHBoxLayout(bpsSelector);
    bpsSelectorLayout->setMargin(0);
    QButtonGroup * bpsGroup = new QButtonGroup(this);
    const char * buttonLabels[] = { "16", "24", "32" };
    for (int i = 0; i < 3; ++i) {
        QRadioButton * button = new QRadioButton(buttonLabels[i], this);
        button->setChecked(i == 0);
        bpsGroup->addButton(button, i);
        bpsSelectorLayout->addWidget(button);
    }
    connect(bpsGroup, SIGNAL(buttonClicked( int)), this, SLOT(setBps(int)));

    QWidget * previewSelector = new QWidget(this);
    QHBoxLayout * previewSelectorLayout = new QHBoxLayout(previewSelector);
    previewSelectorLayout->setMargin(0);
    QButtonGroup * previewGroup = new QButtonGroup(this);
    const char * previewLabels[] = { "Full", "Half", "None" };
    for (int i = 0; i < 3; ++i) {
        QRadioButton * button = new QRadioButton(tr(previewLabels[i]), this);
        button->setChecked(i == 0);
        previewGroup->addButton(button, i);
        previewSelectorLayout->addWidget(button);
    }
    connect(previewGroup, SIGNAL(buttonClicked( int)), this, SLOT(setPreviewSize(int)));

    QCheckBox * saveIndexFile = new QCheckBox(tr("Save"), this);

    indexFileSelector = new QWidget(this);
    QHBoxLayout * indexFileSelectorLayout = new QHBoxLayout(indexFileSelector);
    indexFileSelectorLayout->setMargin(0);
    indexFileEditor = new QLineEdit(indexFileSelector);
    indexFileEditor->setMinimumWidth(200);
    QPushButton * showFileDialog = new QPushButton("...", indexFileSelector);
    connect(showFileDialog, SIGNAL(clicked(bool)), this, SLOT(setIndexFileName()));
    indexFileSelectorLayout->addWidget(indexFileEditor);
    indexFileSelectorLayout->addWidget(showFileDialog);
    connect(saveIndexFile, SIGNAL(stateChanged(int)), this, SLOT(setIndexFileSelectorEnabled(int)));
    indexFileSelector->setEnabled(false);

    QWidget * formWidget = new QWidget(this);
    QFormLayout * formLayout = new QFormLayout(formWidget);
    formLayout->addRow(tr("Bits per sample:"), bpsSelector);
    formLayout->addRow(tr("Preview size:"), previewSelector);
    formLayout->addRow(tr("Mask image:"), saveIndexFile);
    formLayout->addRow("", indexFileSelector);
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget, 1);

    QWidget * buttons = new QWidget(this);
    QHBoxLayout * buttonsLayout = new QHBoxLayout(buttons);
    QPushButton * acceptButton = new QPushButton(tr("Accept"), this);
    acceptButton->setDefault(true);
    connect(acceptButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
    QPushButton * cancelButton = new QPushButton(tr("Cancel"), this);
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(reject()));
    buttonsLayout->addWidget(acceptButton);
    buttonsLayout->addWidget(cancelButton);
    layout->addWidget(buttons, 0, Qt::AlignHCenter);

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
    QString file = QFileDialog::getSaveFileName(this, tr("Save DNG file"), "",
        tr("PNG Images (*.png)"), NULL, QFileDialog::DontUseNativeDialog);
    indexFileEditor->setText(file);
}


void DngPropertiesDialog::setIndexFileSelectorEnabled(int state) {
    indexFileSelector->setEnabled(state == 2);
}


} // namespace hdrmerge
