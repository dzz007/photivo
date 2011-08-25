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
  \class ptRepairInteraction

  \brief On-image user interaction for the spot repair tool

  This class provides all the functionality for the interactive spot repair
  mode, i.e. all manipulation of repair spots that happen directly on the image.
*/

#ifndef PTREPAIRINTERACTION_H
#define PTREPAIRINTERACTION_H

#include <QGraphicsItemGroup>

#include "../ptAbstractInteraction.h"
#include "ptRepairSpotListView.h"
#include "ptRepairSpot.h"
#include "ptRepairSpotShape.h"

///////////////////////////////////////////////////////////////////////////
//
// class ptRepairInteraction
//
///////////////////////////////////////////////////////////////////////////
class ptRepairInteraction : public ptAbstractInteraction {
Q_OBJECT


///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  /*! Create a repair interaction object.
    \param View
      A pointer to an existing \c QGraphicsView object that will be used for
      drawing the spot. \c View must have a \c QGraphicsScene assigned.
    \param ListView
      A pointer to an existing list view widget containing the spots. \c ListView must
      have a model assigned.
  */
  explicit ptRepairInteraction(QGraphicsView* View, ptRepairSpotListView* ListView);

  ~ptRepairInteraction();

  /*! Stop the repair interaction.
    Cleans up the QGraphicsScene and then emits the \c finished() signal.
  */
  void stop();


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  void MousePressHandler(QMouseEvent* event);

  ptRepairSpot* m_SpotData;   // pointer to current spot’s config data
  ptRepairSpotListView* m_ListView;

  // Components of the spot visual spot shape
  ptRepairSpotShape*    m_SpotShape;    // container for the complete spot shape


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  /*! Slot that is called when the current spot is changed.
    \param index
      The model index of the new spot.
  */
  void changeSpot(const QModelIndex &index);

  /*! Slot for all keyboard events.
    \param event
      A pointer to the QKeyEvent that triggered this slot.
  */
  void keyAction(QKeyEvent* event);

  /*! Slot for all mouse events.
    \param event
      A pointer to the QMouseEvent that triggered this slot.
  */
  void mouseAction(QMouseEvent* event);


};
#endif // PTREPAIRINTERACTION_H
