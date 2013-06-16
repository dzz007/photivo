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

#ifndef ptThumbGen_H
#define ptThumbGen_H

#include "ptThumbDefines.h"
#include "ptThumbCache.h"
#include <QObject>
#include <QThread>
#include <QDateTime>
#include <QSize>
#include <QQueue>
#include <QMutex>
#include <wand/magick_wand.h>
#include <memory>

//------------------------------------------------------------------------------
/*! Struct to associate a specific thumbgroup object with a specific image file on disk. */
struct TThumbAssoc {
  TThumbId ThumbId;
  uint     GroupId;
};
Q_DECLARE_METATYPE(TThumbAssoc)
Q_DECLARE_METATYPE(QList<TThumbAssoc>)

//------------------------------------------------------------------------------
class ptThumbGen: public QObject {
Q_OBJECT

public:
  explicit ptThumbGen();
  ~ptThumbGen();

  // these are the exceptions to the "no plain public methods" rule
  void abort();
  bool isRunning() const;

  // No plain public methods!
  // Because a ptThumbGen object lives in its own separate thread all public access except
  // ction and dtion MUST occur EXCLUSIVELY via queued signals/slots.
  // See the comment at the top of requestThumb() for the correct way to write new public slots.

public slots:
  void request(QList<TThumbAssoc> AThumbList);

signals:
  void broadcast(uint AReceiverId, TThumbPtr AImage);

private:
  TThumbPtr generate(const TThumbId& AThumbId);
  void processRequests();
  void stopProcessing();
  void scaleSize(QSize& ASize, int ALongEdge);
  void setIsRunning(bool AValue);
  void transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize);

  // Access to following bool variables MUST ALWAYS be protected via their respective mutexes.
  // Use ptMutexLocker for easy locking/unlocking. Do NOT use QMutexLocker.
  bool   FAbortSignaled;
  QMutex FAbortMutex;
  bool   FIsRunning;
  mutable QMutex FIsRunningMutex;

  QThread FThread;
  ptThumbCache FThumbCache;
  QQueue<TThumbAssoc> FThumbQueue;
};

#endif // ptThumbGen_H
