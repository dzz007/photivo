/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef ptThumbGenMgr_H
#define ptThumbGenMgr_H

#include "ptThumbDefines.h"
#include <QThread>

class ptThumbGenWorker;

//------------------------------------------------------------------------------
/*!
   The ptThumbGenMgr class controls thumbnail generation, i.e. mainly concurrency management.
   It does not do the real work. That is delegated to ptThumbGenWorker.
*/
class ptThumbGenMgr {
public:
  explicit ptThumbGenMgr();
  ~ptThumbGenMgr();

  /*! \name Methods for controlling thumbnail generation *//*! @{*/
  // When you add a method that calls into the worker you MUST route the call through
  // the worker thread’s event loop to ensure thread-safety. See the request() implementation
  // for an example. abort() and isRunning() are the only exceptions from that rule.
  void abort();
  void connectBroadcast(const QObject* AReceiver, const char* ABroadcastSlot);
  bool isRunning() const;
  void request(QList<TThumbAssoc> AThumbList);
  /*! @}*/

private:
  // FWorker is a raw pointer because I’m not certain how well behaved smart pointers would be
  // accross thread boundaries in this scenario.
  ptThumbGenWorker* FWorker;
  QThread FThread;
};

#endif // ptThumbGenMgr_H
