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

#ifndef PTGRAPHICSSCENEEMITTER_H
#define PTGRAPHICSSCENEEMITTER_H

//==============================================================================

#include <QObject>

#include "ptFileMgrConstants.h"

//==============================================================================

/*!
  \class ptGraphicsSceneEmitter

  \brief Emits signals for non-QObject QGraphicsScene elements.

  This singleton class QGraphicsScene elements to use the signals/slots
  mechanism even if they are not derived from \c QObject.
*/
class ptGraphicsSceneEmitter: public QObject {
Q_OBJECT

public:
  /*! Connects the signal that is emitted when a new thumbnail gets focused.
    Works similar to \c QObject::connect().
    \param receiver
      A pointer to the receiver object for the signal.
    \param method
      The slot the signal should be connected to. Use the SLOT() macro in the
      same way as with QObject::connect().
  */
  static bool ConnectFocusChanged(const QObject* receiver, const char* method);
  /*! Emits the signal indicating that a new thumbnail has the focus. */
  static void EmitFocusChanged();

  /*! Connects the signal that triggers an action when a thumbnail is clicked.
    Works similar to \c QObject::connect().
    \param receiver
      A pointer to the receiver object for the signal.
    \param method
      The slot the signal should be connected to. Use the SLOT() macro in the
      same way as with QObject::connect().
  */
  static bool ConnectThumbnailAction(const QObject* receiver, const char* method);
  /*! Emits the signal that triggers an action when a thumbnail is clicked.
    \param action
      The type of action to be performed.
    \param location
      The file name or directory name for the action.
  */
  static void EmitThumbnailAction(const ptThumbnailAction action, const QString location);

  /*! Destroys the ptGraphicsSceneEmitter singleton object. */
  static void DestroyInstance();


private:
  explicit ptGraphicsSceneEmitter();
  ~ptGraphicsSceneEmitter();
  static ptGraphicsSceneEmitter* GetInstance();
  static ptGraphicsSceneEmitter* m_Instance;


signals:
  void focusChanged();
  void thumbnailAction(const ptThumbnailAction action, const QString location);
};

//==============================================================================

#endif // PTGRAPHICSSCENEEMITTER_H
