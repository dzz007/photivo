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

//==============================================================================

#ifndef PTREPAIRINTERACTION_H
#define PTREPAIRINTERACTION_H

#include <QGraphicsItemGroup>

#include "../ptImageInteraction.h"
#include "ptRepairSpotListView.h"
#include "ptRepairSpot.h"
#include "ptRepairSpotShape.h"
#include "ptRepairSpotModel.h"

//==============================================================================

class ptRepairInteraction : public ptImageInteraction {
Q_OBJECT

public:
  /*! Create a repair interaction object.
    \param View
      A pointer to an existing \c QGraphicsView object that will be used for
      drawing the spot. \c View must have a \c QGraphicsScene assigned.
    \param ListView
      A pointer to an existing list view widget containing the spots. \c ListView must
      have a model assigned.
  */
  ptRepairInteraction(QGraphicsView* AView,
                      ptRepairSpotListView* AListView,
                      ptRepairSpot* ASpotData = NULL);

  ~ptRepairInteraction();

  /*! Stop the repair interaction.
    Cleans up the QGraphicsScene and then emits the \c finished() signal.
  */
  void stop();


private:
  void MousePressHandler(QMouseEvent* AEvent);
  void MouseDblClickHandler(QMouseEvent* AEvent);

  ptRepairSpot*           FSpotData;   // pointer to current spot’s config data
  ptRepairSpotListView*   FListView;
  ptRepairSpotModel*      FSpotModel;

  // Components of the spot visual spot shape
  ptRepairSpotShape*      FSpotShape;    // container for the complete spot shape


private slots:
  /*! Slot that is called when the current spot is changed.
    \param index
      The model index of the new spot.
  */
  void changeSpot(const QModelIndex& AIndex);

  /*! Slot for all keyboard events.
    \param event
      A pointer to the QKeyEvent that triggered this slot.
  */
  void keyAction(QKeyEvent* AEvent);

  /*! Slot for all mouse events.
    \param event
      A pointer to the QMouseEvent that triggered this slot.
  */
  void mouseAction(QMouseEvent* AEvent);


};
#endif // PTREPAIRINTERACTION_H
