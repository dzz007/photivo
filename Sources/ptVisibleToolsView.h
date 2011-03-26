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

#ifndef PTVISIBLETOOLSVIEW_H
#define PTVISIBLETOOLSVIEW_H

#include <QStandardItemModel>
#include <QStyledItemDelegate>

// model for UI settings tool
class ptVisibleToolsModel : public QStandardItemModel {
//  Q_OBJECT
public:
  explicit ptVisibleToolsModel(QObject *parent = 0);
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};


// item delegate for UI settings tool
class ptVisibleToolsItemDelegate : public QStyledItemDelegate {
//  Q_OBJECT
public:
  explicit ptVisibleToolsItemDelegate(QObject *parent = 0);
  QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

#endif // PTVISIBLETOOLSVIEW_H
