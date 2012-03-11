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

#include "ptImageSpotEditor.h"
#include "../ptMainWindow.h"
#include "../ptTheme.h"

extern ptMainWindow* MainWindow;
extern ptTheme* Theme;

//==============================================================================

ptImageSpotEditor::ptImageSpotEditor(QWidget        *AParent,
                                     const QString  &AInitialName)
: QWidget(AParent)
{
  // ensure proper widget styling
  setContentsMargins(0,0,0,0);
  auto hPal = palette();
  hPal.setColor(QPalette::Window, Theme->baseColor());
  setPalette(hPal);

  // init delete button
  DelButton = new QToolButton(this);
  DelButton->setIcon(QIcon(QString::fromUtf8(":/photivo/Icons/cancel.png")));
  DelButton->installEventFilter(this);
  connect(this, SIGNAL(deleteButtonClicked()),
          MainWindow->RepairSpotListView, SLOT(deleteSpot()));

  // init name editor
  NameEditor = new QLineEdit(this);
  NameEditor->setText(AInitialName);
  NameEditor->selectAll();
  NameEditor->setFocus();

  // setup layout: DelButton right-justified, NameEditor gets remaining width
  QHBoxLayout* Layout = new QHBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(NameEditor);
  Layout->addWidget(DelButton);
  this->setLayout(Layout);
}

//==============================================================================

ptImageSpotEditor::~ptImageSpotEditor()
{}
/* Auto resource management: do not delete these members explicitely.
  DelButton
  NameEditor
*/

//==============================================================================

bool ptImageSpotEditor::eventFilter(QObject *obj, QEvent *event) {
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

