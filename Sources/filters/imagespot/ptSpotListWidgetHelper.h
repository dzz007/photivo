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
   \brief The \c ptImageSpotListView class is a small helper class for \c ptSpotListWidget
    that adds some small details to \c QListView.
 */

#ifndef PTSPOTLISTWIDGETHELPER_H
#define PTSPOTLISTWIDGETHELPER_H

//==============================================================================

#include <QListView>
#include "ptImageSpotModel.h"

//==============================================================================

class ptSpotListView: public QListView {
Q_OBJECT

public:
  /*!
     Creates a \c ptImageSpotListView object.
     \param AParent
        Parent widget of the ListView.
   */
  explicit ptSpotListView(QWidget *AParent);
  ~ptSpotListView();

  void setModel(ptImageSpotModel *AModel);

//------------------------------------------------------------------------------

private:
  ptImageSpotModel *FModel;

//------------------------------------------------------------------------------

protected:
  virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

//------------------------------------------------------------------------------

signals:
  /*! \c QListView does not emit a signal when the focused row changes. So we use our own signal. */
  void rowChanged(const QModelIndex &ANewIdx);

  /*! Emitted when a spot is enabled or disabled. */
  void activeSpotsChanged();

//------------------------------------------------------------------------------

protected slots:
  /*! [EVENT] Triggered when the focused spot in the list changes. */
  void currentChanged(const QModelIndex &current, const QModelIndex &previous);
};


#endif // PTSPOTLISTWIDGETHELPER_H
