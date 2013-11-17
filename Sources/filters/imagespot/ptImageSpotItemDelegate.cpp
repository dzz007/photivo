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

#include "../../ptConstants.h"
#include "../../ptTheme.h"
#include "ptSpotListWidget.h"
#include "ptImageSpotItemDelegate.h"
#include "ptImageSpotEditor.h"

//==============================================================================

ptImageSpotItemDelegate::ptImageSpotItemDelegate(ptSpotListWidget *AParent)
: QStyledItemDelegate(AParent),
  FSpotList(AParent)
{}

//==============================================================================

QWidget* ptImageSpotItemDelegate::createEditor(QWidget *parent,
                                                const QStyleOptionViewItem&,
                                                const QModelIndex& /*index*/) const
{
  auto hEd = new ptImageSpotEditor(parent);
  connect(hEd, SIGNAL(deleteButtonClicked()), FSpotList, SLOT(deleteSpot()));
  return hEd;
}

//==============================================================================

void ptImageSpotItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto hNameEd = qobject_cast<ptImageSpotEditor*>(editor)->NameEditor;
  hNameEd->setText(index.data().toString());
  hNameEd->selectAll();
  hNameEd->setFocus();
}

//==============================================================================

void ptImageSpotItemDelegate::setModelData(QWidget *editor,
                                           QAbstractItemModel *model,
                                           const QModelIndex &index) const
{
  model->setData(index,
                 qobject_cast<ptImageSpotEditor*>(editor)->NameEditor->text(),
                 Qt::DisplayRole);
}

//==============================================================================

