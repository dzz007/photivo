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
/*!
  \class ptRepairSpotModel

  \brief The model belonging to \c ptRepairSpotListView.
*/

#ifndef PTREPAIRSPOTMODEL_H
#define PTREPAIRSPOTMODEL_H

#include <QStandardItemModel>
#include "ptImageSpotList.h"

class ptRepairSpotModel : public QStandardItemModel {
  Q_OBJECT

public:
  /*! Constructs a \c ptRepairSpotModel object and creates the list of items from the
    actual repair spot data.
    \param SpotList
      A pointer to the list where the actual spot data is stored.
    \param SizeHint
      The \c SizeHint used for each item in the model. Width should be \c 0 and height
      large enough to contain the editor widget.
  */
  explicit ptRepairSpotModel(ptImageSpotList* SpotList, const QSize SizeHint);

  /*! Update an item and the underlying spot repair data.  */
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

  /*! Remove one or more items from the model and delete the underlying repair spots. */
  bool removeRows(int row, int count, const QModelIndex &parent);


private:
  QSize m_SizeHint;
  ptImageSpotList* m_SpotList;

};
#endif // PTREPAIRSPOTMODEL_H
