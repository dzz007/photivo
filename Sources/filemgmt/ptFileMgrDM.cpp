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

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "ptFileMgrDM.h"

extern ptSettings* Settings;

//==============================================================================

//void ClearThumbnailData(ptThumbnailData &Data) {
//  if (Data.Thumbnail) {
//    QGraphicsItem* child;
//    foreach(child, Data.Thumbnail->childItems()) {
//      delete child;
//    }
//    delete Data.Thumbnail;
//    Data.Thumbnail = NULL;
//  }
//  Data.Path = "";
//}

//==============================================================================

ptFileMgrDM* ptFileMgrDM::m_Instance = 0;

//==============================================================================

ptFileMgrDM* ptFileMgrDM::GetInstance() {
  if (m_Instance == 0) {
    m_Instance = new ptFileMgrDM();
  }

  return m_Instance;
}

//==============================================================================

void ptFileMgrDM::Clear() {
  m_Cache->Clear();
}

//==============================================================================

void ptFileMgrDM::DestroyInstance() {
  if (m_Instance != 0) {
    delete m_Instance;
  }

  m_Instance = 0;
}

//==============================================================================

ptFileMgrDM::ptFileMgrDM()
: QObject()
{
  m_TreeModel = new QFileSystemModel;
  m_TreeModel->setFilter(QDir::AllDirs);

  // Set inital directory from settings (if available)
  QString lastDir = Settings->GetString("LastFileMgrLocation");
  if (lastDir == "" || !QDir(lastDir).exists()) {
    #ifdef Q_OS_WIN32
      m_TreeModel->setRootPath(m_TreeModel->myComputer().toString());
    #else
      m_TreeModel->setRootPath(QDir::homePath());
    #endif
  } else {
    m_TreeModel->setRootPath(lastDir);
  }

  // Init stuff for thumbnail generation
  m_ThumbQueue = new QQueue<QGraphicsItemGroup*>;
  m_Thumbnailer = new ptFileMgrThumbnailer;
  m_Thumbnailer->setQueue(m_ThumbQueue);

  // Init thumbnail cache
  m_Cache = new ptThumbnailCache(1000);
  m_Thumbnailer->setCache(m_Cache);
}

//==============================================================================

ptFileMgrDM::~ptFileMgrDM() {
  DelAndNull(m_TreeModel);
  DelAndNull(m_ThumbQueue);
  DelAndNull(m_Thumbnailer);
  DelAndNull(m_Cache);
}

//==============================================================================

void ptFileMgrDM::StartThumbnailer(const QModelIndex index) {
  m_Thumbnailer->setDir(m_TreeModel->filePath(index));
  m_Thumbnailer->run();
}

//==============================================================================

void ptFileMgrDM::StopThumbnailer() {
  if (m_Thumbnailer->isRunning()) {
    m_Thumbnailer->exit();

    // TODO: quick&dirty. Probably more elegantly solved via the thread's finished() signal.
    while (!m_Thumbnailer->isFinished()) {
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
}

//==============================================================================
