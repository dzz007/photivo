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
  \class ptAbstractInteraction
  \brief Abstract base class for image interactions (crop, spot repair etc.).
         Do not instantiate directly!
*/

#ifndef PTIMAGEINTERACTION_H
#define PTIMAGEINTERACTION_H

//==============================================================================

#include "ptConstants.h"

#include <QObject>
#include <QGraphicsView>

//==============================================================================

enum ptMouseAction {
  maNone          = 0,
  maClick         = 1,
  maDoubleClick   = 2,
  maDragDrop      = 3
};
Q_DECLARE_FLAGS(ptMouseActions, ptMouseAction)
Q_DECLARE_OPERATORS_FOR_FLAGS(ptMouseActions)

//==============================================================================

class ptAbstractInteraction: public QObject {
Q_OBJECT

public:
  explicit ptAbstractInteraction(QGraphicsView *AView);

  /*! Use this method to signal to the Interaction that that the current mouse action should
   *  be cancelled. E.g. when a button press has happend the subsequent release should be
   *  ignored and the action initiated by the press not finished/rolled back.
   *
   *  The default implementation does nothing.
   */
  virtual void abortMouseAction(const ptMouseAction /*AAction*/) {}

  /*! Returns the keyboard modifiers this interaction uses. Derived classes must implement
   *  this method to return the appropriate values. */
  virtual Qt::KeyboardModifiers modifiers() const = 0;

  /*! Returns the mouse actions this interaction uses. Derived classes must implement
   *  this method to return the appropriate values. */
  virtual ptMouseActions        mouseActions() const = 0;

  /*! Returns the mouse buttons this interaction uses. Derived classes must implement
   *  this method to return the appropriate values. */
  virtual Qt::MouseButtons      mouseButtons() const = 0;


//------------------------------------------------------------------------------

protected:
  QGraphicsView *FView;

//------------------------------------------------------------------------------

signals:
  void finished(ptStatus AExitStatus);
};

//==============================================================================

// TODO: maybe later during the *real* pt*Interaction redesign
///*!
// * \brief The \c ptDefaultInteraction class is a dummy for the ViewWindow.
// */
//class ptDefaultInteraction: public ptAbstractInteraction {
//Q_OBJECT

//public:
//  explicit ptDefaultInteraction(QGraphicsView *AView): ptAbstractInteraction(AView) {}
//  virtual Qt::KeyboardModifiers modifiers() const { return Qt::NoModifier; }
//  virtual ptMouseActions        mouseActions() const { maNone; }
//  virtual Qt::MouseButtons      mouseButtons() const { Qt::NoButton; }
//};


#endif // PTIMAGEINTERACTION_H
