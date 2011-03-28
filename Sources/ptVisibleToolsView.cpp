/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2011 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2010-2011 Bernd Schoeler <brjohn@brother-john.net>
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptVisibleToolsView.h"
#include "ptConstants.h"
#include "ptTheme.h"

extern ptTheme* Theme;

ptVisibleToolsModel::ptVisibleToolsModel(QObject *parent) :
  QStandardItemModel(parent) {
}

bool ptVisibleToolsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  bool result = QStandardItemModel::setData(index, value, role);

  // set icon automatically according to state
  if (role == Qt::UserRole+1)
  {
    if (value.toInt() == tsHidden)
      result = (result && setData(index, QIcon(* Theme->ptIconCrossRed), Qt::DecorationRole));
    if (value.toInt() == tsFavourite)
      result = (result && setData(index, QIcon(* Theme->ptIconStar), Qt::DecorationRole));
    if (value.toInt() == tsNormal)
      result = (result && setData(index, QIcon(* Theme->ptIconStarGrey), Qt::DecorationRole));
  }

  return result;
}


ptVisibleToolsItemDelegate::ptVisibleToolsItemDelegate(QObject *parent) :
  QStyledItemDelegate(parent) {
}

QWidget* ptVisibleToolsItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem&, const QModelIndex&) const {
  return new QComboBox(parent);
}

void ptVisibleToolsItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  QComboBox *box = qobject_cast<QComboBox*>(editor);
  box->addItem(index.data().toString(), index.data(Qt::UserRole+1));

  // test if tool isn't always visible
  if (index.data(Qt::UserRole+2).toInt() != 1)
    box->addItem(QIcon(* Theme->ptIconCrossRed), tr("Hidden"));
  box->addItem(QIcon(* Theme->ptIconStarGrey), tr("Normal"));
  box->addItem(QIcon(* Theme->ptIconStar), tr("Favourite"));
}

void ptVisibleToolsItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  QComboBox *box = qobject_cast<QComboBox*>(editor);

  // save state to the model
  if (box->currentText() == tr("Hidden"))
    model->setData(index, tsHidden, Qt::UserRole+1);
  if (box->currentText() == tr("Normal"))
    model->setData(index, tsNormal, Qt::UserRole+1);
  if (box->currentText() == tr("Favourite"))
    model->setData(index, tsFavourite, Qt::UserRole+1);
}
