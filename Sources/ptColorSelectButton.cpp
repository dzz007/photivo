/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2015 Bernd Schoeler <brjohn@brother-john.net>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptColorSelectButton.h"
#include "filters/ptCfgItem.h"
#include "ptInfo.h"
#include "ptTheme.h"
#include <QPixmap>
#include <QColorDialog>
#include <QVBoxLayout>

// -----------------------------------------------------------------------------

ptColorSelectButton::ptColorSelectButton(QWidget* AParent):
  ptWidget(AParent),
  FButton(new QToolButton(this))
{
  this->setBaseSize(85, 25);
  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(2, 2, 2, 2);
  layout->setSpacing(0);
  FButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(FButton);

  connect(FButton, SIGNAL(clicked()), SLOT(onButtonClicked()));
}

// -----------------------------------------------------------------------------

ptColorSelectButton::ptColorSelectButton(const ptCfgItem& ACfgItem, QWidget* AParent):
  ptColorSelectButton(AParent)
{
  this->init(ACfgItem);
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::init(const ptCfgItem& ACfgItem) {
  GInfo->Assert(
      this->objectName().isEmpty(),
      "Trying to initialize an already initalized color select button. Object name: " + this->objectName(),
      AT);
  GInfo->Assert(
      ACfgItem.Type == ptCfgItem::ColorSelectButton,
      QString("Wrong GUI type (%1). Must be ptCfgItem::ColorSelectButton.").arg(ACfgItem.Type), AT);

  this->setObjectName(ACfgItem.Id);
  this->updateColor(ACfgItem.Default.value<QColor>());
  FButton->setToolTip(ACfgItem.ToolTip);
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::setValue(const QVariant& AValue) {
  Q_ASSERT(AValue.canConvert(QMetaType::QColor));
  this->updateColor(AValue.value<QColor>());
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::updateColor(const QColor& AColor) {
  FCurrColor = AColor;
  QPixmap pix(FButton->width(), FButton->height());
  pix.fill(FCurrColor);
  FButton->setIcon(pix);
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::onButtonClicked() {
  QColorDialog colorDialog(FCurrColor);
  colorDialog.setStyle(Theme->systemStyle());
  colorDialog.setPalette(Theme->systemPalette());

  if (colorDialog.exec() == QDialog::Accepted) {
    this->updateColor(colorDialog.selectedColor());
    emit valueChanged(this->objectName(), FCurrColor);
  }
}

// -----------------------------------------------------------------------------
