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

#include "../ptAbstractInteraction.h"

//==============================================================================

class ptSpotInteraction: public ptAbstractInteraction {
Q_OBJECT

public:
  explicit ptSpotInteraction(QGraphicsView *AView);
  ~ptSpotInteraction();

  /*! Reimplemented from base class. */
  virtual void abortMouseAction(const ptMouseAction AAction);

  /*! Reimplemented from base class. */
  virtual Qt::KeyboardModifiers modifiers()    const { return Qt::NoModifier; }

  /*! Reimplemented from base class. */
  virtual ptMouseActions        mouseActions() const { return maClick; }

  /*! Reimplemented from base class. */
  virtual Qt::MouseButtons      mouseButtons() const { return Qt::LeftButton; }

  /*! Stops the spotinteraction. Makes sure any necessary cleanup happens properly
      and emits finished() afterwards.  */
  void stop();

//------------------------------------------------------------------------------

private:
  bool          FAbortNextMouseAction;
  
//------------------------------------------------------------------------------

signals:
  /*!
   *  Signal emitted when the user left-clicks a spot on the image.
   *  \param APos
   *    Coordinates of the clicked image position in current pipe size scale.
   */
  void clicked(const QPoint &APos);

//------------------------------------------------------------------------------

private slots:
  void mouseAction(QMouseEvent *AEvent);


};
#endif // PTSPOTINTERACTION_H
