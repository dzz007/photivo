/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
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
#include <QDir>
#include <QFileInfoList>

#include <wand/magick_wand.h>

#include "ptThumbDM.h"
#include "ptGraphicsThumbGroup.h"
#include "ptSingleDirModel.h"
#include "ptTagModel.h"

//==============================================================================

class ptImage8;

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

  void MoveFocus(const int index);

  /*! Sets the folder for thumbnail display. Does not trigger the thumbnailer.
      You probably need this only once to init the folder. */
  void setCurrentDir(const QString absolutePath) { m_CurrentDir = absolutePath; }

  /*! Returns a pointer to the tag model. */
  ptTagModel* tagModel() { return m_TagModel; }

  /*! Returns a pointer to the thumbnail.*/
  bool getThumbnail(ptImage8     *&AImage,
                    const QString &AFileName,
                    const int      AMaxSize);

  ptThumbDM* getThumbDM();

  /*! Get list of all thumbs. */
  QFileInfoList getThumbsFileList(const QString &APath);
private:
  static ptFileMgrDM* m_Instance;

  ptFileMgrDM();
  ptFileMgrDM(const ptFileMgrDM&): QObject() {}
  ~ptFileMgrDM();

  // for thumbnails
  void GenerateThumbnail(MagickWand *AInImage, ptImage8 *AOutImage, const QSize tSize);
  void ScaleThumbSize(QSize* tSize, const int max);

  int                           m_FocusedThumb;
  QString                       m_CurrentDir;
  ptSingleDirModel*             m_DirModel;
  ptTagModel*                   m_TagModel;
  std::unique_ptr<ptThumbDM>    FThumbDM;
  QDir                          FDir;
  bool                          FIsMyComputer;
};

//==============================================================================

#endif // PTFILEMGRDM_h
