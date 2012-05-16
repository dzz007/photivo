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
#include <QList>
#include <QHash>

#include <wand/magick_wand.h>

#include "ptThumbnailer.h"
#include "ptThumbnailCache.h"
#include "ptGraphicsThumbGroup.h"
#include "ptSingleDirModel.h"
#include "ptTagModel.h"

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
  static void DestroyInstance();

//-------------------------------------

  /*! Clear the data cache of \c ptFileMgrDM */
  void Clear();

  /*! Returns the current folder for thumbnail display. */
  QString currentDir() { return m_CurrentDir; }

  /*! Returns a pointer to the model for the folder ListView. */
  ptSingleDirModel* dirModel() const { return m_DirModel; }

  int focusedThumb() { return m_FocusedThumb; }
  int focusedThumb(QGraphicsItem* group);

  ptGraphicsThumbGroup* MoveFocus(const int index);

  /*! Sets the folder for thumbnail display. Does not trigger the thumbnailer.
      You probably need this only once to init the folder. */
  void setCurrentDir(const QString absolutePath) { m_CurrentDir = absolutePath; }

  /*! Sets the directory for thumbnail generation.
    Returns the total number of applicable entries in that directory.
    Returns \c -1 and does not set the directory if the thumbnailer is
    currently running.
    \param path
      Sets the directory for thumbnail generation. Must be an absolute path.
  */
  int setThumbnailDir(const QString path);

  /*! Starts image thumbnail generation. */
  void StartThumbnailer();

  /*! Aborts a running thumbnailer thread.
    Calling this function when the thumbnailer is not currently running
    does not do any harm. The function will then essentially do nothing.
  */
  void StopThumbnailer();

  /*! Returns a pointer to the tag model. */
  ptTagModel* tagModel() { return m_TagModel; }

  /*! Returns a pointer to the thumbnailer.
      Use this to connect to the thumbnailer’s \c newThumbsNotify signal
  */
  ptThumbnailer* thumbnailer() const { return m_Thumbnailer; }

  /*! Returns a pointer to the list of currently displayed thumbnail images. */
  QList<ptGraphicsThumbGroup*>* thumbList() { return m_ThumbList; }

  /*! Returns a pointer to the thumbnail.*/
  QImage* getThumbnail(const QString FileName,
                       const int     MaxSize);

private:
  static ptFileMgrDM* m_Instance;

  ptFileMgrDM();
  ptFileMgrDM(const ptFileMgrDM&): QObject() {}
  ~ptFileMgrDM();

  // for thumbnails
  QImage* GenerateThumbnail(MagickWand* image, const QSize tSize);
  void ScaleThumbSize(QSize* tSize, const int max);

  int                           m_FocusedThumb;
  ptThumbnailCache*             m_Cache;
  QString                       m_CurrentDir;
  ptSingleDirModel*             m_DirModel;
  ptTagModel*                   m_TagModel;
  ptThumbnailer*                m_Thumbnailer;
  QList<ptGraphicsThumbGroup*>* m_ThumbList;


};

//==============================================================================

#endif // PTFILEMGRDM_h
