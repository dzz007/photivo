/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTIMAGESPOTMODEL_H
#define PTIMAGESPOTMODEL_H

#include <QStandardItemModel>

class ptImageSpot;
class ptImageSpotList;

//==============================================================================

class ptImageSpotModel: public QStandardItemModel {
Q_OBJECT

public:
  /*! Constructs an empty \c ptImageSpotModel object.
    \param ASizeHint
      The size hint used for each item in the model. Width should be \c 0 and height
      large enough to contain the editor widget.
    \param AParent
      The models parent widget.
  */
  ptImageSpotModel(const QSize ASizeHint,
                   ptImageSpotList *ASpotList,
                   QObject *AParent = nullptr);
  ~ptImageSpotModel();

  /*! Appends a new spot to the end of the spot list.
   \param ANewSpot
     A pointer to the spot object. The model takes ownership of the spot. */
  void appendSpot(ptImageSpot *ANewSpot);

  /*! Removes all spots from the list. */
  void removeAll();

  /*! Reimplemented from parent class. */
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  /*! Returns the item role affected by the latest data change. This is a bit of a hack to
   *  communicate to the ListView what exactly changed when the dataChanged() signal is emitted. */
  int lastChangedRole() const { return FLastChangedRole; }

  /*! Moves the row given by \c AFromIdx by \c AOffset rows and returns the index
      of the destination row.
      Negative offsets move up, positive offsets move down. If \c AFromIdx is invalid
      or \c AOffset is \c 0 the function does nothing and returns \c -1. */
  int moveRow(const QModelIndex &AFromIdx, const int AOffset);

  /*! Update an item and the underlying spot repair data.  */
  virtual bool setData(const QModelIndex &index,
                       const QVariant    &value,
                       int               role = Qt::EditRole);

  /*! Assigns new \data to a spot.
      \param AIndex
        The list index of the spot.
      \param ASpotData
        A pointer to the new spot object. The old data object is deleted and \c ASpotData
        reparented to the model. */
  virtual void setSpot(const int AIndex, ptImageSpot *ASpotData);

  void setSpotPos(const int AIndex, const int Ax, const int Ay);

  /*! Returns a pointer to the spot at list position \c AIndex. */
//  ptImageSpot *spot(const int AIndex) { return FSpotList->at(AIndex); }

  /*! Reimplemented from parent class. */
  virtual Qt::DropActions supportedDropActions() const;

  /*! Remove one or more items from the model and delete the underlying repair spots. */
  bool removeRows(int row, int count, const QModelIndex &parent);

  /*! Synchronises the model to the spot list.
      Included data: name of repair algorithm, enabled state. */
  void rebuildModel();


private:
  QStandardItem*  createSpotItem(const ptImageSpot &ASpot);
  QString         createToolTip(const ptImageSpot &ASpot);

  int                   FLastChangedRole;
  QSize                 FSizeHint;
  ptImageSpotList      *FSpotList;

};

#endif // PTIMAGESPOTMODEL_H
