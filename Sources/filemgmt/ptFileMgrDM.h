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

#ifndef PTFILEMGRDM_h
#define PTFILEMGRDM_h

//==============================================================================

#include <QObject>
#include <QPixmap>
#include <QFileSystemModel>
#include <QGraphicsItemGroup>
#include <QQueue>

#include "ptFileMgrThumbnailer.h"

//==============================================================================

/*! This \c struct carries all valuable information for each thumbnail. */
struct ptThumbnailData {
  QPixmap* Thumbnail;
  QString  Location;
};

//==============================================================================

/*! Clear for \c ptThumbnailData. */
void ClearThumbnailData( ptThumbnailData &Data);

//==============================================================================

/*!
  \class ptFileMgrDM

  \brief Data module for file management.

  This data module will handle the thumbnail creation and manage the corresponding
  memory. It's designed as a singleton.
*/

class ptFileMgrDM: public QObject {
Q_OBJECT

public:
  /*! Get or create the singleton instance of \c ptFileMgrDM */
  static ptFileMgrDM* GetInstance();

  /*! Destroy the singleton instance of \c ptFileMgrDM */
  static void         DestroyInstance();

  /*! Clear the data cache of \c ptFileMgrDM */
  void Clear();

  /*! Starts image thumbnail generation.
    \param index
      The QModelIndex corresponding to the directory with the image files.
  */
  void StartThumbnailer(const QModelIndex index);

  /*! Returns a pointer to the thumbnailer.
      Use this to connect to the thumbnailer’s \c newThumbsNotify signal
  */
  ptFileMgrThumbnailer* thumbnailer() const { return m_Thumbnailer; }

  /*! Returns a pointer to the FIFO buffer containing image thumbnails. */
  QQueue<QGraphicsPixmapItem>* thumbQueue() { return m_ThumbQueue; }

  /*! Returns a pointer to the model with the data for the tree view. */
  QAbstractItemModel* treeModel() { return m_TreeModel; }


private:
  static ptFileMgrDM* m_Instance;

  ptFileMgrDM();
  ptFileMgrDM(const ptFileMgrDM&): QObject() {}
  ~ptFileMgrDM();

  ptFileMgrThumbnailer*   m_Thumbnailer;
  QQueue<QGraphicsPixmapItem>*  m_ThumbQueue;
  QFileSystemModel*       m_TreeModel;


};

//==============================================================================

#endif // PTFILEMGRDM_h
