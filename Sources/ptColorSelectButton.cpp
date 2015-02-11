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
  this->setBaseSize(40, 20);
  this->setMinimumSize(40, 20);
  this->setMaximumSize(40, 20);
  this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  FButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);
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
      ACfgItem.Type == ptCfgItem::ColorSelectButton,
      QString("Wrong GUI type (%1). Must be ptCfgItem::ColorSelectButton.").arg(ACfgItem.Type), AT);

  this->setObjectName(ACfgItem.Id);
  this->updateColor(ACfgItem.Default.value<QColor>());
  FTooltipText = ACfgItem.ToolTip;
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::setValue(const QVariant& AValue) {
  Q_ASSERT(AValue.canConvert(QMetaType::QColor));
  this->updateColor(AValue.value<QColor>());
}

// -----------------------------------------------------------------------------
// Button needs to be updated on becoming visible so the color panel will become
// the correct size.
void ptColorSelectButton::showEvent(QShowEvent*) {
  this->updateColor(FCurrColor);
}

// -----------------------------------------------------------------------------

void ptColorSelectButton::updateColor(const QColor& AColor) {
  FCurrColor = AColor;
  QPixmap colorPanel(FButton->width()-CMarginSize, FButton->height()-CMarginSize);
  colorPanel.fill(FCurrColor);
  FButton->setIconSize(colorPanel.size());
  FButton->setIcon(colorPanel);
  FButton->setToolTip(FTooltipText + tr("\ncurrently: red %1, green %2, blue %3")
                      .arg(FCurrColor.red())
                      .arg(FCurrColor.green())
                      .arg(FCurrColor.blue()));
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
