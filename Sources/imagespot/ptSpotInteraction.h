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
/*!
  \class ptSpotInteraction

  \brief Class for a simple interaction where the user selects a coordinate on
  the image by clicking on it.
*/

#ifndef PTSPOTINTERACTION_H
#define PTSPOTINTERACTION_H

//==============================================================================

#include <QPoint>
#include <QMouseEvent>
#include <QKeyEvent>

#include "../ptImageInteraction.h"

//==============================================================================

class ptSpotInteraction: public ptImageInteraction {
Q_OBJECT

public:
  explicit ptSpotInteraction(QGraphicsView *AView);

  /*! Stops the spotinteraction. Makes sure any necessary cleanup happens properly
      and emits finished() afterwards.  */
  void stop();

  
//------------------------------------------------------------------------------

signals:
  /*! Signal emitted when the user left-clicks a spot on the image.
      \param APos
        Coordinates of the clicked image position in current pipe size scale.
      \param AMoveCurrent
        Indicates if an existing spot should be moved (true) or a new spot should
        be created (false). Set to true when the user holds down Ctrl while clicking.
        Connected slots are responsible to interpret what “an existing spot”
        actually means – usually this is the currently selected spot in a list of spots.
  */
  void clicked(const QPoint &APos, const bool AMoveCurrent);

//------------------------------------------------------------------------------

private slots:
  void mouseAction(QMouseEvent *AEvent);


};
#endif // PTSPOTINTERACTION_H
