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

#include "config.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPushButton>
#include "AboutDialog.hpp"

namespace hdrmerge {

AboutDialog::AboutDialog(QWidget * parent, Qt::WindowFlags f) : QDialog(parent, f) {
    QVBoxLayout * buttonLayout = new QVBoxLayout(this);
    QWidget * logoText = new QWidget(this);
    QHBoxLayout * layout = new QHBoxLayout(logoText);
    QLabel * logoLabel = new QLabel(logoText);
    logoLabel->setPixmap(QPixmap(":/images/logo.png").scaledToWidth(400, Qt::SmoothTransformation));
    layout->addWidget(logoLabel);
    layout->addSpacing(12);
    QLabel * text = new QLabel("<h1>HDRMerge " HDRMERGE_VERSION_STRING "</h1>"
    "<p><a href=\"http://jcelaya.github.io/hdrmerge/\">http://jcelaya.github.io/hdrmerge/</a></p>"
    "<p>" + tr("A software for the fusion of multiple raw images into a single high dynamic range image.") + "</p>"
        "<p>Copyright &copy; 2012 Javier Celaya (jcelaya@gmail.com)</p>"
        "<p>This is free software: you can redistribute it and/or modify it under the terms of the GNU "
        "General Public License as published by the Free Software Foundation, either version 3 of the License, "
        "or (at your option) any later version.</p>", logoText);
    text->setWordWrap(true);
    layout->addWidget(text);
    layout->setAlignment(text, Qt::AlignTop);
    QPushButton * acceptButton = new QPushButton(tr("Accept"), this);
    acceptButton->setDefault(true);
    connect(acceptButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
    buttonLayout->addWidget(logoText);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->setAlignment(acceptButton, Qt::AlignCenter);
    setWindowTitle(tr("About HDRMerge..."));
    buttonLayout->setSizeConstraint(QLayout::SetFixedSize);
}

} // namespace hdrmerge
