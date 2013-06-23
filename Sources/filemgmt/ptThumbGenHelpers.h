/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2013 Michael Munzert <mail@mm-log.com>
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

#ifndef PTTHUMBGENHELPERS_H
#define PTTHUMBGENHELPERS_H

#include "ptThumbDefines.h"
#include "ptMutexLocker.h"
#include "../ptInfo.h"
#include <QObject>
#include <QMutex>
#include <QList>
#include <QLinkedList>

/*!
  The ptFlowController class is a small helper to control a flow. It offers thread-safe access
  to a boolean flags representing “open” and “closed” states.
*/
class ptFlowController {
public:
  explicit ptFlowController(bool AInitiallyOpen);
  ~ptFlowController();

  bool isOpen() const;
  void setOpen(bool AIsOpen);
  bool toggleOpen();

private:
  bool FFlowing;
  mutable QMutex FFlowMutex;
};

//------------------------------------------------------------------------------

enum class TThumbQPrio { Normal, High };

/*!
  The *ptThumbQueue* class provides a thread-safe FIFO container for *TThumbAssoc*s.
*/
class ptThumbQueue {
public:
  explicit ptThumbQueue();   //!< Creates a ptQueue object.
  ~ptThumbQueue();           //!< Destroys a ptQueue object.

  void clear();
  TThumbAssoc dequeue();
  void enqueue(const TThumbAssoc& AItem, TThumbQPrio APriority = TThumbQPrio::Normal);
  void enqueue(const QList<TThumbAssoc>& AItems, TThumbQPrio APriority = TThumbQPrio::Normal);
  bool isEmpty() const;

private:
  QLinkedList<TThumbAssoc> FItems;
  mutable QMutex FItemsMutex;
};


#endif // PTTHUMBGENHELPERS_H
