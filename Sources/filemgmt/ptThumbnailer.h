/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILEMGRTHUMBNAILER_H
#define PTFILEMGRTHUMBNAILER_H

//==============================================================================

#include <QThread>
#include <QList>
#include <QGraphicsItemGroup>
#include <QDir>

#include "ptThumbnailCache.h"
#include "ptGraphicsThumbGroup.h"

//==============================================================================

typedef QImage* (*getThumbnail_ptr)(const QString, const int);

//==============================================================================

/*!
  \class ptThumbnailer

  \brief Generates image thumbnails in a separate thread.

  This class is used by \c ptFileMgrDM to generate image thumbnails in a separate
  thread. It fills a FIFO buffer with \c QGraphicsItem objects containing the
  thumbnails.
*/
class ptThumbnailer: public QThread {
Q_OBJECT

public:
  /*! Creates an empty ptThumbnailer object. */
  explicit ptThumbnailer();

  /*! Destroys a \c ptThumbnailer object. */
  ~ptThumbnailer();

  /*! Sends an abort request to the thread and waits until the thread has stopped
    before returning. Calling \c Abort() on a not running thread does nothing.
  */
  void Abort();

  /*! Sets the cache for thumbnail objects. */
  void setCache(ptThumbnailCache* cache);

  /*! Sets the directory for thumbnail generation.
    Returns the total number of applicable entries in that directory.
    Returns \c -1 and does not set the directory if the thumbnailer is
    currently running.
  */
  int setDir(const QString dir);

  /*! Sets the FIFO buffer where the thumbnails are written to.
      Note that the buffer is taken as is, i.e. it is not cleared by the
      thumbnailer.
  */
  void setThumbList(QList<ptGraphicsThumbGroup*>* ThumbList);

  /*! Sets the working function */
  void setWorker(getThumbnail_ptr Worker);


protected:
  /*! This function performs the actual thumbnail generation. */
  void run();


private:

  bool                            m_AbortRequested;
  ptThumbnailCache*               m_Cache;
  QDir*                           m_Dir;
  QList<ptGraphicsThumbGroup*>*   m_ThumbList;
  getThumbnail_ptr                m_getThumbnail;


signals:
  void newThumbNotify(const bool isLast);
  void newImageNotify(ptGraphicsThumbGroup* group, QImage* pix);


public slots:



};
#endif // PTFILEMGRTHUMBNAILER_H
