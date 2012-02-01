/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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
#include "../ptMainWindow.h"
#include "../ptTheme.h"

extern ptGuiOptions* GuiOptions;
extern ptMainWindow* MainWindow;
extern ptTheme* Theme;

//==============================================================================

ptRepairSpotEditor::ptRepairSpotEditor(QWidget *AParent,
                                       const int AInitialAlgoIndex)
: QWidget(AParent)
{
  setContentsMargins(0,0,0,0);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Theme->baseColor());
  setPalette(pal);

  // init combobox with repair alorithms
  AlgoCombo = new QComboBox(this);
  int i = 0;
  while (GuiOptions->SpotRepair[i].Value != -1) {
    AlgoCombo->addItem(GuiOptions->SpotRepair[i].Text);
    i++;
  }
  AlgoCombo->setCurrentIndex(AInitialAlgoIndex);

  DelButton = new QToolButton(this);
  DelButton->setIcon(QIcon(QString::fromUtf8(":/photivo/Icons/cancel.png")));
  DelButton->installEventFilter(this);

  QHBoxLayout* Layout = new QHBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(AlgoCombo);
  Layout->addWidget(DelButton);
  this->setLayout(Layout);

  connect(this, SIGNAL(deleteButtonClicked()),
          MainWindow->RepairSpotListView, SLOT(deleteSpot()));
}

//==============================================================================

ptRepairSpotEditor::~ptRepairSpotEditor() {
  delete AlgoCombo;
  delete DelButton;
}

//==============================================================================

bool ptRepairSpotEditor::eventFilter(QObject *obj, QEvent *event) {
  // detect delete button click
  if (event->type() == QEvent::MouseButtonPress) {
    if (obj == DelButton) {
      emit deleteButtonClicked();
      return true;
    }
  }

  // else: pass the event on to the parent class
  return QObject::eventFilter(obj, event);
}

//==============================================================================

