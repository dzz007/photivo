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

#include <Magick++.h>

#include "ptThumbnailCache.h"
#include "ptGraphicsThumbGroup.h"

//==============================================================================

/*!
  \class ptFileMgrThumbnailer

  \brief Generates image thumbnails in a separate thread.

  This class is used by \c ptFileMgrDM to generate image thumbnails in a separate
  thread. It fills a FIFO buffer with \c QGraphicsItem objects containing the
  thumbnails.
*/
class ptFileMgrThumbnailer: public QThread {
Q_OBJECT

public:
  /*! Creates an empty ptFileMgrThumbnailer object. */
  explicit ptFileMgrThumbnailer();

  /*! Destroys a \c ptFileMgrThumbnailer object. */
  ~ptFileMgrThumbnailer();

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


protected:
  /*! This function does the actual thumbnail generating. */
  void run();


private:
  // Resizes the image to the specified size and writes it to the pixmap
  void GenerateThumbnail(Magick::Image& image,
                         QPixmap* thumbPixmap,
                         const int thumbSize);

  bool                            m_AbortRequested;
  ptThumbnailCache*               m_Cache;
  QDir*                           m_Dir;
  QList<ptGraphicsThumbGroup*>*   m_ThumbList;


signals:
  void newThumbsNotify(const bool isLast);
  void newPixmapNotify(ptGraphicsThumbGroup* group, QPixmap* pix);


public slots:



};
#endif // PTFILEMGRTHUMBNAILER_H
