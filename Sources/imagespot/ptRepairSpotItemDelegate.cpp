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

#include "ptRepairSpotItemDelegate.h"
#include "../ptConstants.h"
#include "../ptTheme.h"
#include "ptRepairSpotEditor.h"
#include "ptImageSpotList.h"
#include "ptRepairSpot.h"

extern ptTheme* Theme;
extern ptImageSpotList* RepairSpotList;

ptRepairSpotItemDelegate::ptRepairSpotItemDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}

QWidget* ptRepairSpotItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem&, const QModelIndex&) const {
  return new ptRepairSpotEditor(parent);
}

void ptRepairSpotItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  ptRepairSpotEditor *ed = qobject_cast<ptRepairSpotEditor*>(editor);
  ed->ModeCombo->setCurrentIndex(
        static_cast<ptRepairSpot*>(RepairSpotList->at(index.row()))->algorithm() );
}

void ptRepairSpotItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  ptRepairSpotEditor *ed = qobject_cast<ptRepairSpotEditor*>(editor);

  model->setData(index,
                 GuiOptions->SpotRepair[ed->ModeCombo->currentIndex()].Text,
                 Qt::DisplayRole);
}
