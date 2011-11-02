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

#include <cassert>

#include <QStringList>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QFileInfoList>

#include "../ptError.h"
#include "../ptCalloc.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "ptThumbnailer.h"
#include "ptFileMgrDM.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

extern ptSettings* Settings;
extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//==============================================================================

ptThumbnailer::ptThumbnailer()
: QThread()
{
  m_AbortRequested = false;
  m_Cache          = NULL;
  m_ThumbList      = NULL;

  m_Dir = new QDir("");
  m_Dir->setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
  m_Dir->setNameFilters(FileExtsRaw + FileExtsBitmap);
}

//==============================================================================

ptThumbnailer::~ptThumbnailer() {
  DelAndNull(m_Dir);
}

//==============================================================================

void ptThumbnailer::setCache(ptThumbnailCache* cache) {
  if (!this->isRunning() && cache != NULL) {
    m_Cache = cache;
  }
}

//==============================================================================

int ptThumbnailer::setDir(const QString dir) {
  if (this->isRunning()) {
    return -1;
  }

  m_Dir->setPath(dir);

  if (dir.isEmpty()) {
    // Empty paths are valid. They tell the thumbnailer to do nothing.
    return -1;
  }

  QDir::Filters filters = QDir::Files;
  if (Settings->GetInt("FileMgrShowDirThumbs")) {
    filters = filters | QDir::AllDirs | QDir::NoDot;
  }
  m_Dir->setFilter(filters);
  return m_Dir->count();
}

//==============================================================================

void ptThumbnailer::setThumbList(QList<ptGraphicsThumbGroup*>* ThumbList) {
  if (!this->isRunning() && ThumbList != NULL) {
    m_ThumbList = ThumbList;
  }
}

//==============================================================================


void ptThumbnailer::run() {
  // Check for properly set directory, cache and buffer
  if (m_Dir->path().isEmpty() || !m_Dir->exists() || m_ThumbList == NULL || m_Cache == NULL) {
    return;
  }

  ptFileMgrDM* DataModule = ptFileMgrDM::GetInstance();

  QFileInfoList files = m_Dir->entryInfoList();
  int thumbMaxSize = Settings->GetInt("FileMgrThumbnailSize");

  /***
    Step 1: Generate thumb groups without the thumbnail images
  ***/

  for (uint i = 0; i < (uint)files.count(); i++) {
    if (m_AbortRequested) {
      m_AbortRequested = false;
      return;
    }

    // query cache (returns NULL on miss, valid pointer on hit)
    ptGraphicsThumbGroup* thumbGroup =
        m_Cache->RequestThumbnail(ptThumbnailCache::GetKey(files.at(i).filePath()));

    if (thumbGroup) {
      //cache hit
      thumbGroup = ptGraphicsThumbGroup::AddRef(thumbGroup);

    } else {
      // cache miss: first create and setup thumb group object ...
      thumbGroup = ptGraphicsThumbGroup::AddRef();
      ptFSOType type;

      QString descr = files.at(i).fileName();
      if (files.at(i).isDir()) {
        if (descr == "..") {
          type = fsoParentDir;
          descr = QDir(files.at(i).canonicalFilePath()).dirName();
#ifdef Q_OS_WIN
          if (descr.isEmpty()) {
            // parent folder is a drive
            QString drive = files.at(i).canonicalFilePath().left(2);
            descr = QString("%1 (%2)").arg(WinApi::VolumeName(drive)).arg(drive).trimmed();
          }
#endif
        } else {
          type = fsoDir;
        }
      } else {
        type = fsoFile;
      }

      thumbGroup->addInfoItems(files.at(i).canonicalFilePath(), descr, type);

      // ... then add the group to the cache. Directories are not cached because
      // they load very quickly anyway.
      if (thumbGroup->fsoType() == fsoFile) {
        m_Cache->CacheThumbnail(thumbGroup);
      }
    }

    m_ThumbList->append(thumbGroup);
    emit newThumbNotify(i == (uint)files.count()-1);
  } // main FOR loop step 1


  /***
    Step 2: Add the images to the thumbnail groups
  ***/

  for (uint i = 0; i < (uint)m_ThumbList->count(); i++) {
    if (m_AbortRequested) {
      m_AbortRequested = false;
      return;
    }

    QImage* thumbImage = NULL;
    ptGraphicsThumbGroup* currentGroup = m_ThumbList->at(i);

    if (currentGroup->fsoType() == fsoParentDir) {
      // we have a parent directory (dirs are not cached!)
      thumbImage = new QImage(QString::fromUtf8(":/dark/icons/go-up-48px.png"));

    } else if (currentGroup->fsoType() == fsoDir) {
      // we have a subdirectory (dirs are not cached!)
      thumbImage = new QImage(QString::fromUtf8(":/dark/icons/folder-48px.png"));

    } else {
      if (!currentGroup->hasImage()) {
        // we have a file and no image in the thumb group == cache miss.
        // See if we can get a thumbnail image

        thumbImage = DataModule->getThumbnail(currentGroup->fullPath(), thumbMaxSize);
      }
    }

  // Notification signal for each finished thumb image.
    emit newImageNotify(m_ThumbList->at(i), thumbImage);
  } // main FOR loop step 2
}

//==============================================================================

void ptThumbnailer::Abort() {
  if (isRunning()) {
    m_AbortRequested = true;

    QTime timer;
    timer.start();
    while (isRunning() && timer.elapsed() < 500) {
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
}

//==============================================================================
