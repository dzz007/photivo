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
  \class ptRepairInteraction

  \brief On-image user interaction for the spot repair tool

  This class provides all the functionality for the interactive spot repair
  mode, i.e. all manipulation of repair spots that happen directly on the image.
*/

//==============================================================================

#ifndef PTREPAIRINTERACTION_H
#define PTREPAIRINTERACTION_H

#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsDropShadowEffect>

#include "../ptAbstractInteraction.h"
#include "ptImageSpotListView.h"
#include "ptRepairSpot.h"
#include "ptImageSpotModel.h"

//==============================================================================

class ptRepairInteraction : public ptAbstractInteraction {
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
  ptRepairInteraction(QGraphicsView *AView,
                      ptImageSpotListView *AListView);

  ~ptRepairInteraction();

  // TODO: key/mouse flags are very preliminary!
  /*! Reimplemented from base class. */
  virtual Qt::KeyboardModifiers modifiers()    const { return Qt::NoModifier; }

  /*! Reimplemented from base class. */
  virtual ptMouseActions        mouseActions() const { return maClick; }

  /*! Reimplemented from base class. */
  virtual Qt::MouseButtons      mouseButtons() const { return Qt::LeftButton; }

  /*! Stop the repair interaction.
    Cleans up the QGraphicsScene and then emits the \c finished() signal. */
  void stop();

//------------------------------------------------------------------------------

private:
  struct TSpotShape {
    QGraphicsItemGroup   *Group;            // container for the complete spot-shape
    QGraphicsItemGroup   *SpotGroup;        // container for the spot
    QGraphicsItemGroup   *RepairerGroup;    // container for repairer and connector line

    QGraphicsEllipseItem *Spot;             // outer border of the area to be repaired
    QGraphicsEllipseItem *SpotBorder;       // inner border, for edge blur
    QGraphicsRectItem    *RadiusHandle;     // mouse handle to change spot radius
    QGraphicsEllipseItem *RotationHandle;   // mouse handle to rotate the spot

    QGraphicsEllipseItem *Repairer;         // area with the repair reference data
    QGraphicsLineItem    *Connector;        // connector line between spot and repairer
  };

  /*! Creates and initialises a TSpotShape. The shape is hidden. Component positions are not set. */
  TSpotShape  *CreateShape();
  /*! Destroys all objects of a TSpotShape as well as the shape struct itself. */
  void        DestroyShape(TSpotShape *AShape);
  /*! Draws a spotshape.
      \param AShape
        A pointer to the TSpotShape that should be drawn.
      \param ASpotData
        A pointer to the spot data to use. */
  void        Draw(TSpotShape *AShape, ptRepairSpot *ASpotData);

  void        MousePressHandler(QMouseEvent* AEvent);
  void        MouseDblClickHandler(QMouseEvent* AEvent);

  ptImageSpotListView      *FListView;

  // Components of the spot visual spot shape
  QGraphicsDropShadowEffect *FShadow;      // for the b/w line effect
  QList<TSpotShape*>        *FShapes;      // container for the complete spot shape

//------------------------------------------------------------------------------

private slots:
  /*! Slot that is called when the current spot is changed.
    \param index
      The model index of the new spot.
  */
  void changeSpot(const QModelIndex &AIndex);

  /*! Slot for all keyboard events.
    \param event
      A pointer to the QKeyEvent that triggered this slot.
  */
  void keyAction(QKeyEvent *AEvent);

  /*! Slot for all mouse events.
    \param event
      A pointer to the QMouseEvent that triggered this slot.
  */
  void mouseAction(QMouseEvent *AEvent);


};
#endif // PTREPAIRINTERACTION_H
