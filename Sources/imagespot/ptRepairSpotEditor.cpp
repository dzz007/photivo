/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include <QHBoxLayout>

#include "ptRepairSpotEditor.h"
#include "../ptGuiOptions.h"

extern ptGuiOptions* GuiOptions;


ptRepairSpotEditor::ptRepairSpotEditor(QWidget *parent)
: QWidget(parent)
{
  ModeCombo = new QComboBox(this);
  int i = 0;
  while (GuiOptions->SpotRepair[i].Value != -1) {
    ModeCombo->addItem(GuiOptions->SpotRepair[i].Text);
    i++;
  }

  DelButton = new QToolButton(this);
  DelButton->setIcon(QIcon(QString::fromUtf8(":/photivo/Icons/cancel.png")));

  QHBoxLayout* Layout = new QHBoxLayout;
  Layout->addWidget(ModeCombo);
  Layout->addStretch();
  Layout->addWidget(DelButton);
  this->setLayout(Layout);
}

ptRepairSpotEditor::~ptRepairSpotEditor() {
  delete ModeCombo;
  delete DelButton;
}
