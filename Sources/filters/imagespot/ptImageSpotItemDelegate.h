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
/*!
  \class ptImageSpotItemDelegate

  \brief Manager for the editor widget in spot list views.
*/

#ifndef PTIMAGESPOTITEMDELEGATE_H
#define PTIMAGESPOTITEMDELEGATE_H

//==============================================================================

#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QToolButton>

class ptSpotListWidget;

//==============================================================================

class ptImageSpotItemDelegate: public QStyledItemDelegate {
Q_OBJECT

public:
  explicit ptImageSpotItemDelegate(ptSpotListWidget *AParent);

  QWidget* createEditor(QWidget                     *parent,
                        const QStyleOptionViewItem  &option,
                        const QModelIndex           &index) const;

  void setEditorData(QWidget            *editor,
                     const QModelIndex  &index) const;

  void setModelData(QWidget             *editor,
                    QAbstractItemModel  *model,
                    const QModelIndex   &index) const;

//------------------------------------------------------------------------------

private:
  ptSpotListWidget *FSpotList;


};
#endif // PTIMAGESPOTITEMDELEGATE_H
