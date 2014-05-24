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
#include <QSettings>
#include "DngPropertiesDialog.hpp"

namespace hdrmerge {

DngPropertiesDialog::DngPropertiesDialog(QWidget * parent, Qt::WindowFlags f)
        : QDialog(parent, f), SaveOptions() {
    loadDefaultOptions();

    QVBoxLayout * layout = new QVBoxLayout(this);

    QWidget * bpsSelector = new QWidget(this);
    QHBoxLayout * bpsSelectorLayout = new QHBoxLayout(bpsSelector);
    bpsSelectorLayout->setMargin(0);
    QButtonGroup * bpsGroup = new QButtonGroup(this);
    const char * buttonLabels[] = { "16", "24", "32" };
    int bpsIndex = (bps - 16) / 8;;
    for (int i = 0; i < 3; ++i) {
        QRadioButton * button = new QRadioButton(buttonLabels[i], this);
        button->setChecked(i == bpsIndex);
        bpsGroup->addButton(button, i);
        bpsSelectorLayout->addWidget(button);
    }
    connect(bpsGroup, SIGNAL(buttonClicked(int)), this, SLOT(setBps(int)));

    QWidget * previewSelector = new QWidget(this);
    QHBoxLayout * previewSelectorLayout = new QHBoxLayout(previewSelector);
    previewSelectorLayout->setMargin(0);
    QButtonGroup * previewGroup = new QButtonGroup(this);
    const char * previewLabels[] = { "Full", "Half", "None" };
    for (int i = 0; i < 3; ++i) {
        QRadioButton * button = new QRadioButton(tr(previewLabels[i]), this);
        button->setChecked(i == (2 - previewSize));
        previewGroup->addButton(button, 2 - i);
        previewSelectorLayout->addWidget(button);
    }
    connect(previewGroup, SIGNAL(buttonClicked( int)), this, SLOT(setPreviewSize(int)));

    QCheckBox * saveMaskFile = new QCheckBox(tr("Save"), this);

    maskFileSelector = new QWidget(this);
    QHBoxLayout * maskFileSelectorLayout = new QHBoxLayout(maskFileSelector);
    maskFileSelectorLayout->setMargin(0);
    maskFileEditor = new QLineEdit(maskFileSelector);
    maskFileEditor->setMinimumWidth(200);
    maskFileEditor->setToolTip(tr("You can use the following tokens:") + "\n" +
        "- %if " + tr("for the file name of the least exposed image.") + "\n" +
        "- %id " + tr("for the directory name of the least exposed image.") + "\n" +
        "- %of " + tr("for the file name of the result image.") + "\n" +
        "- %od " + tr("for the directory name of the result image."));
    maskFileEditor->setText(maskFileName.c_str());
    QPushButton * showFileDialog = new QPushButton("...", maskFileSelector);
    connect(showFileDialog, SIGNAL(clicked(bool)), this, SLOT(setMaskFileName()));
    maskFileSelectorLayout->addWidget(maskFileEditor);
    maskFileSelectorLayout->addWidget(showFileDialog);
    connect(saveMaskFile, SIGNAL(stateChanged(int)), this, SLOT(setMaskFileSelectorEnabled(int)));
    saveMaskFile->setChecked(saveMask);
    maskFileSelector->setEnabled(saveMask);

    QWidget * formWidget = new QWidget(this);
    QFormLayout * formLayout = new QFormLayout(formWidget);
    formLayout->addRow(tr("Bits per sample:"), bpsSelector);
    formLayout->addRow(tr("Preview size:"), previewSelector);
    formLayout->addRow(tr("Mask image:"), saveMaskFile);
    formLayout->addRow("", maskFileSelector);
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget, 1);

    saveOptions = new QCheckBox(tr("Save these options as the default values."));
    layout->addWidget(saveOptions, 0, Qt::AlignLeft);

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


void DngPropertiesDialog::accept() {
    saveMask = maskFileSelector->isEnabled();
    maskFileName = QDir::toNativeSeparators(maskFileEditor->text()).toUtf8().constData();
    if (saveOptions->isChecked()) {
        QSettings settings;
        settings.setValue("bps", bps);
        settings.setValue("previewSize", previewSize);
        settings.setValue("saveMask", saveMask);
        settings.setValue("maskFileName", maskFileName.c_str());
    }
    QDialog::accept();
}


void DngPropertiesDialog::loadDefaultOptions() {
    QSettings settings;
    bps = settings.value("bps", 16).toInt();
    previewSize = settings.value("previewSize", 2).toInt();
    saveMask = settings.value("saveMask", false).toBool();
    maskFileName = settings.value("maskFileName", "%od/%of_mask.png").toString().toUtf8().constData();
}


void DngPropertiesDialog::setBps(int i) {
    switch (i) {
        case 0: bps = 16; break;
        case 1: bps = 24; break;
        case 2: bps = 32; break;
    }
}


void DngPropertiesDialog::setPreviewSize(int i) {
    previewSize = i;
}


void DngPropertiesDialog::setMaskFileName() {
    QString file = QFileDialog::getSaveFileName(this, tr("Save DNG file"), "",
        tr("PNG Images (*.png)"), NULL, QFileDialog::DontUseNativeDialog);
    maskFileEditor->setText(file);
}


void DngPropertiesDialog::setMaskFileSelectorEnabled(int state) {
    maskFileSelector->setEnabled(state == 2);
}


} // namespace hdrmerge
