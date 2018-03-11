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
#include <QButtonGroup>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include "LoadOptionsDialog.hpp"

namespace hdrmerge {


class FileItem : public QListWidgetItem {
public:
    FileItem(const QString & filename, QListWidget * parent) : QListWidgetItem(parent, 1000) {
        setText(QFileInfo(filename).fileName());
        setData(Qt::UserRole, QVariant(filename));
        setSizeHint(QSize(0, 24));
    }
};


LoadOptionsDialog::LoadOptionsDialog(QWidget * parent, Qt::WindowFlags f)
        : QDialog(parent, f), LoadOptions() {
    QSettings settings;
    QVBoxLayout * layout = new QVBoxLayout(this);

    QWidget * fileSelector = new QWidget(this);
    QHBoxLayout * fileSelectorLayout = new QHBoxLayout(fileSelector);
    fileSelectorLayout->setMargin(0);
    fileList = new QListWidget(fileSelector);
    fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileSelectorLayout->addWidget(fileList, 1);

    QWidget * addRemoveButtons = new QWidget(this);
    QVBoxLayout * addRemoveButtonsLayout = new QVBoxLayout(addRemoveButtons);
    addRemoveButtonsLayout->setMargin(0);
    QPushButton * addButton = new QPushButton(tr("Add"), addRemoveButtons);
    QPushButton * removeButton = new QPushButton(tr("Remove"), addRemoveButtons);
    addRemoveButtonsLayout->addWidget(addButton, 0, Qt::AlignTop);
    addRemoveButtonsLayout->addWidget(removeButton, 0, Qt::AlignTop);
    addRemoveButtonsLayout->addStretch(1);
    fileSelectorLayout->addWidget(addRemoveButtons, 0);
    layout->addWidget(fileSelector, 1);
    connect(addButton, SIGNAL(clicked(bool)), this, SLOT(addFiles()));
    connect(removeButton, SIGNAL(clicked(bool)), this, SLOT(removeFiles()));

    alignBox = new QCheckBox(tr("Align source images."), this);
    alignBox->setChecked(settings.value("alignOnLoad", true).toBool());
    layout->addWidget(alignBox, 0);

    cropBox = new QCheckBox(tr("Crop result image to optimal size."), this);
    cropBox->setChecked(settings.value("cropOnLoad", true).toBool());
    layout->addWidget(cropBox, 0);

    customWhiteLevelBox = new QCheckBox(tr("Use custom white level."), this);
    customWhiteLevelBox->setChecked(settings.value("useCustomWlOnLoad", false).toBool());
    layout->addWidget(customWhiteLevelBox, 0);

    customWhiteLevelSpinBox = new QSpinBox();
    customWhiteLevelSpinBox->setRange(0, 65535);
    customWhiteLevelSpinBox->setValue(settings.value("customWlOnLoad", 16383).toInt());
    customWhiteLevelSpinBox->setToolTip(tr("Custom white level."));
    layout->addWidget(customWhiteLevelSpinBox, 0);

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
    setWindowTitle(tr("Open raw images"));
}


void LoadOptionsDialog::showEvent(QShowEvent * event) {
    if (fileNames.size() == 0) {
        addFiles();
    } else {
        for (auto & i : fileNames) {
            new FileItem(i, fileList);
        }
        fileNames.clear();
    }
    if (fileList->count() == 0) {
        QMetaObject::invokeMethod(this, "reject", Qt::QueuedConnection);
    }
}


void LoadOptionsDialog::addFiles() {
    QSettings settings;
    QVariant lastDirSetting = settings.value("lastOpenDirectory");
    QString filter(tr("Raw images ("
    "*.3fr "
    "*.ari *.arw "
    "*.bay "
    "*.crw *.cr2 *.cap "
    "*.dcs *.dcr *.dng *.drf "
    "*.eip *.erf "
    "*.fff "
    "*.iiq "
    "*.k25 *.kdc "
    "*.mdc *.mef *.mos *.mrw "
    "*.nef *.nrw "
    "*.obm *.orf "
    "*.pef *.ptx *.pxn "
    "*.r3d *.raf *.raw *.rwl *.rw2 *.rwz "
    "*.sr2 *.srf *.srw "
    "*.x3f"
    ")"));
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open raw images"),
        lastDirSetting.isNull() ? QDir::currentPath() : QDir(lastDirSetting.toString()).absolutePath(),
        filter, NULL, QFileDialog::DontUseNativeDialog);
    if (!files.empty()) {
        QString lastDir = QFileInfo(files.front()).absolutePath();
        settings.setValue("lastOpenDirectory", lastDir);
        for (auto & i : files) {
            new FileItem(i, fileList);
        }
    }
}


void LoadOptionsDialog::removeFiles() {
    QList<QListWidgetItem *> items = fileList->selectedItems();
    for (auto i : items) {
        delete i;
    }
}


void LoadOptionsDialog::accept() {
    QSettings settings;
    align = alignBox->isChecked();
    settings.setValue("alignOnLoad", align);
    crop = cropBox->isChecked();
    settings.setValue("cropOnLoad", crop);
    useCustomWl = customWhiteLevelBox->isChecked();
    settings.setValue("useCustomWlOnLoad", useCustomWl);
    customWl = customWhiteLevelSpinBox->value();
    settings.setValue("customWlOnLoad", customWl);
    for (int i = 0; i < fileList->count(); ++i) {
        fileNames.push_back(fileList->item(i)->data(Qt::UserRole).toString());
    }
    QDialog::accept();
}

} // namespace hdrmerge
