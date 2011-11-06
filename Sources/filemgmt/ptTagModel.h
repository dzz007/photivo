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

#ifndef PTTAGMODEL_H
#define PTTAGMODEL_H

//==============================================================================

#include <QStandardItemModel>
#include <QList>

//==============================================================================

class ptTagModel: public QStandardItemModel {
Q_OBJECT
public:
  explicit ptTagModel(QObject* parent = NULL);
  ~ptTagModel();

  void appendRow(QString caption, QString dirPath);
  Qt::ItemFlags flags(const QModelIndex& index) const;
  QString path(const QModelIndex& index);
  bool removeRows(int row, int count, const QModelIndex& parent);
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
  Qt::DropActions supportedDropActions() const;


protected:


private:
  struct TagItem {
    QString Caption;
    QString Directory;
  };

  QList<TagItem>*   m_TagList;


public slots:


signals:


//==============================================================================
};
#endif // PTTAGMODEL_H
