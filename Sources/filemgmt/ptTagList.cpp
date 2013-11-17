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

#include <QStandardItemModel>

#include "ptTagList.h"

//==============================================================================

ptTagList::ptTagList(QWidget* parent)
: QListView(parent)
{
  this->setFrameShape(QFrame::NoFrame);
  this->setFrameShadow(QFrame::Plain);
  this->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

//==============================================================================

void ptTagList::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Delete && event->modifiers() == Qt::NoModifier) {
    removeBookmark();
  }

  // call base class to handle default key actions
  else {
    QListView::keyPressEvent(event);
  }
}

//==============================================================================
void ptTagList::removeBookmark() {
  if (currentIndex().isValid()) {
    int row = currentIndex().row();
    dynamic_cast<QStandardItemModel*>(this->model())->removeRow(row);

    // if possible keep the same row selected
    if (this->model()->rowCount() > 0) {
      setCurrentIndex(this->model()->index(qBound(0, row, this->model()->rowCount()), 0));
    }
  }
}
