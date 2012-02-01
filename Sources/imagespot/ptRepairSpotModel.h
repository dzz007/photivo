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
  /*! Constructs a \c ptRepairSpotModel object and creates the list of items
    from the actual repair spot data.
    \param AIniPrefix
      IniPrefix is a unique string that identifies this image spot list. The ini
      file needs this so we can store multiple lists in it.
    \param ASizeHint
      The \c ASizeHint used for each item in the model. Width should be \c 0 and height
      large enough to contain the editor widget. */
  ptRepairSpotModel(const QSize ASizeHint);
  ~ptRepairSpotModel();

  QString           iniName() const { return CIniName; }
  Qt::ItemFlags     flags(const QModelIndex &index) const;

  /*! Update an item and the underlying spot repair data.  */
  bool              setData(const QModelIndex &AIndex,
                            const QVariant    &AValue,
                            int               ARole = Qt::EditRole);

  ptRepairSpot      *spot(const int AIndex)
    { return static_cast<ptRepairSpot*>(FSpotList->at(AIndex)); }

  /*! Returns a pointer to the \c ptImageSpotList associated with the model. */
  ptImageSpotList   *spotList() { return FSpotList; }

  Qt::DropActions   supportedDropActions() const;

  /*! Remove one or more items from the model and delete the underlying repair spots. */
  bool              removeRows(int row, int count, const QModelIndex &parent);

  /*! Saves complete spotlist to pts file.
      \param AIni
      A Pointer to a \c QSettings object that represents the pts file. */
  void              WriteToIni(QSettings *AIni);

//------------------------------------------------------------------------------

private:
  const QString     CIniName;

  QSize             FSizeHint;
  ptImageSpotList   *FSpotList;

};
#endif // PTREPAIRSPOTMODEL_H
