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

#include <QCoreApplication>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QSpinBox>
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

    QSpinBox * radiusSelector = new QSpinBox(this);
    radiusSelector->setRange(0, 1000);
    radiusSelector->setValue(featherRadius);
    connect(radiusSelector, SIGNAL(valueChanged(int)), this, SLOT(setFeatherRadius(int)));

    QCheckBox * saveMaskFile = new QCheckBox(tr("Save"), this);

    maskFileSelector = new QWidget(this);
    QHBoxLayout * maskFileSelectorLayout = new QHBoxLayout(maskFileSelector);
    maskFileSelectorLayout->setMargin(0);
    maskFileEditor = new QLineEdit(maskFileSelector);
    maskFileEditor->setMinimumWidth(200);
    auto trHelp = [&] (const char * text) { return QCoreApplication::translate("Help", text); };
    maskFileEditor->setToolTip(tr("You can use the following tokens:") + "\n" +
        "- %if[n]: " + trHelp("Replaced by the base file name of image n. Image file names") + "\n" +
        "  " + trHelp("are first sorted in lexicographical order. Besides, n = -1 is the") + "\n" +
        "  " + trHelp("last image, n = -2 is the previous to the last image, and so on.") + "\n" +
        "- %iF[n]: " + trHelp("Replaced by the base file name of image n without the extension.") + "\n" +
        "- %id[n]: " + trHelp("Replaced by the directory name of image n.") + "\n" +
        "- %in[n]: " + trHelp("Replaced by the numerical suffix of image n, if it exists.") + "\n" +
        "  " + trHelp("For instance, in IMG_1234.CR2, the numerical suffix would be 1234.") + "\n" +
        "- %of " + trHelp("Replaced by the base file name of the output file.") + "\n" +
        "- %od " + trHelp("Replaced by the directory name of the output file.") + "\n" +
        "- %%: " + trHelp("Replaced by a single %.") + "\n");
    maskFileEditor->setText(maskFileName);
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
    formLayout->addRow(tr("Mask blur radius:"), radiusSelector);
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
    maskFileName = QDir::toNativeSeparators(maskFileEditor->text()).toLocal8Bit().constData();
    if (saveOptions->isChecked()) {
        QSettings settings;
        settings.setValue("bps", bps);
        settings.setValue("previewSize", previewSize);
        settings.setValue("saveMask", saveMask);
        settings.setValue("maskFileName", maskFileName);
        settings.setValue("featherRadius", featherRadius);
    }
    QDialog::accept();
}


void DngPropertiesDialog::loadDefaultOptions() {
    QSettings settings;
    bps = settings.value("bps", 16).toInt();
    previewSize = settings.value("previewSize", 2).toInt();
    saveMask = settings.value("saveMask", false).toBool();
    maskFileName = settings.value("maskFileName", "%od/%of_mask.png").toString().toLocal8Bit().constData();
    featherRadius = settings.value("featherRadius", 3).toInt();
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


void DngPropertiesDialog::setFeatherRadius(int r) {
    featherRadius = r;
}


} // namespace hdrmerge
