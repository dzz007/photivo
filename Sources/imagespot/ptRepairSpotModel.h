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
#ifndef PTREPAIRSPOTMODEL_H
#define PTREPAIRSPOTMODEL_H

#include <QStandardItemModel>

// model for UI settings tool
class ptRepairSpotModel : public QStandardItemModel {
  Q_OBJECT
public:
  explicit ptRepairSpotModel(QObject *parent = 0);
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};


#endif // PTREPAIRSPOTMODEL_H
