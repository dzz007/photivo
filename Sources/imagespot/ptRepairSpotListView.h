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
  \class ptRepairSpotListView

  \brief The toolpane ListView for repair spots.

  This class represents the ListView in the toolpane holding the repair spots. It also takes
  care of updating the spot config widgets in the toolpane below the ListView.
*/

#ifndef PTREPAIRSPOTLISTVIEW_H
#define PTREPAIRSPOTLISTVIEW_H

#include <QListView>
#include <QToolButton>

class ptRepairSpotListView : public QListView
{
    Q_OBJECT
public:
    /*! Creates a \c ptRepairSpotListView object. This is done by \c MainWindow in Photivo’s
      startup phase. Do not create additional objects of this type!
    */
    explicit ptRepairSpotListView(QWidget *parent = 0);

public slots:
  /*! This slot gets called when the delete button of a spot is pressed. */
  void deleteSpot();

protected slots:
  /*! [EVENT] Triggered when the focused spot in the list changes. Additional config widgets
    outside the ListView are handled here.
  */
  void currentChanged(const QModelIndex &current, const QModelIndex &previous);

};

#endif // PTREPAIRSPOTLISTVIEW_H
