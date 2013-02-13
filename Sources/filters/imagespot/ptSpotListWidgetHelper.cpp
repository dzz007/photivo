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

#include "ptSpotListWidgetHelper.h"
#include "../ptTheme.h"

extern ptTheme *Theme;

//==============================================================================

ptSpotListView::ptSpotListView(QWidget *AParent)
: QListView(AParent),
  FModel(nullptr)
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());

  setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
  setSelectionMode(QAbstractItemView::SingleSelection);
}

//==============================================================================

ptSpotListView::~ptSpotListView() {}

//==============================================================================

void ptSpotListView::setModel(ptImageSpotModel *AModel) {
  FModel = AModel;
  QListView::setModel(AModel);
}

//==============================================================================

void ptSpotListView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QListView::dataChanged(topLeft, bottomRight);
  if (FModel->lastChangedRole() == Qt::CheckStateRole) {
    emit activeSpotsChanged();
  }
}

//==============================================================================

void ptSpotListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  QListView::currentChanged(current, previous);
  emit rowChanged(current);
}

//==============================================================================

