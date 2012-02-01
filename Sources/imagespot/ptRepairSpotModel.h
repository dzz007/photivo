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

//==============================================================================

#include <QStandardItemModel>
#include <QList>
#include <QString>
#include <QSettings>

#include "ptRepairSpot.h"
#include "ptImageSpotList.h"

//==============================================================================

class ptRepairSpotModel: public QStandardItemModel {
  Q_OBJECT

public:
  /*! Constructs an empty \c ptRepairSpotModel object.
    \param ASizeHint
      The \c ASizeHint used for each item in the model. Width should be \c 0 and height
      large enough to contain the editor widget. */
  ptRepairSpotModel(const QSize ASizeHint);
  ~ptRepairSpotModel();

  /*! Reimplemented from parent class. */
  Qt::ItemFlags     flags(const QModelIndex &index) const;

  /*! Loads a list of repair spots from a pts file. The existing list is replaced.
      \param APtsFile
        A pointer to the \c QSettings object representing the pts file. */
  void              LoadFromFile(QSettings *APtsFile);

  /*! Update an item and the underlying spot repair data.  */
  bool              setData(const QModelIndex &AIndex,
                            const QVariant    &AValue,
                            int               ARole = Qt::EditRole);

  /*! Assigns a new \c ptSpotData object to a spot.
      \param AIndex
        The list index of the spot.
      \param ASpotData
        A pointer to the new data object. The old data object is deleted and \c ASpotData
        reparented to the model. */
  void              setSpot(const int AIndex, ptRepairSpot *ASpotData);

  /*! Returns a pointer to the spot at list position \c AIndex. */
  ptRepairSpot      *spot(const int AIndex) { return FSpotList->at(AIndex); }

  /*! Reimplemented from parent class. */
  Qt::DropActions   supportedDropActions() const;

  /*! Remove one or more items from the model and delete the underlying repair spots. */
  bool              removeRows(int row, int count, const QModelIndex &parent);

  /*! Saves complete spotlist to pts file.
      \param APtsFile
        A Pointer to a \c QSettings object that represents the pts file. */
  void              WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

private:
  void              ClearList();

  /*! Synchronises the model to the spot list.
      Included data: name of repair algorithm, enabled state. */
  void              RebuildModel();

  const QString         CPtsName;

  QSize                 FSizeHint;
  QList<ptRepairSpot*>  *FSpotList;

};
#endif // PTREPAIRSPOTMODEL_H
