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

#ifndef ptThumbGenMgr_H
#define ptThumbGenMgr_H

#include "ptThumbDefines.h"
#include "ptThumbGenHelpers.h"
#include "ptThumbGenWorker.h"
#include "ptThumbCache.h"
#include "../ptConstants.h"
#include <QList>
#include <QThread>
#include <QFileInfo>
#include <limits>

//------------------------------------------------------------------------------
/*!
   The ptThumbGenMgr class controls thumbnail generation, i.e. mainly concurrency management.
   It does not do the real work. That is delegated to ptThumbGenWorker.
*/
class ptThumbGenMgr {
public:
  explicit ptThumbGenMgr(uint AMaxCacheSize, int AMaxThreads = std::numeric_limits<int>::max());
  ~ptThumbGenMgr();

  /*! \name Methods for controlling thumbnail generation *//*! @{*/
  void abort();
  void clear();
  void connectBroadcast(const QObject* AReceiver, const char* ABroadcastSlot);
  bool isRunning() const;
  void request(const TThumbAssoc& AThumb, TThumbQPrio APriority = TThumbQPrio::Normal);
  void request(const QList<TThumbAssoc>& AThumbList, TThumbQPrio APriority = TThumbQPrio::Normal);
  /*! @}*/

private:
  void start();

  const int CMinWorkerThreads = 1;

  ptFlowController          FAbortCtrl;
  ptThumbCache              FThumbCache;
  ptThumbQueue              FThumbQueue;
  QList<QThread*>           FThreadpool;
  QList<ptThumbGenWorker*>  FWorkers;
};


#endif // ptThumbGenMgr_H
