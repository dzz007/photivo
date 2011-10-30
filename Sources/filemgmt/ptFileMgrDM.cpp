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

ptFileMgrDM* ptFileMgrDM::m_Instance = NULL;

//==============================================================================

/*static*/
ptFileMgrDM* ptFileMgrDM::GetInstance() {
  if (m_Instance == NULL) {
    m_Instance = new ptFileMgrDM();
  }

  return m_Instance;
}

//==============================================================================

/*static*/
void ptFileMgrDM::DestroyInstance() {
  delete m_Instance;
  m_Instance = NULL;
}

//==============================================================================

ptFileMgrDM::ptFileMgrDM()
: QObject()
{
  m_TreeModel = new QFileSystemModel;
  m_TreeModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);

  // Set root dir for the file system watcher
  #ifdef Q_OS_WIN32
    m_TreeModel->setRootPath(m_TreeModel->myComputer().toString());
  #else
    m_TreeModel->setRootPath("/");
  #endif

  m_DirModel = new ptSingleDirModel;

  // Init stuff for thumbnail generation
  m_ThumbList = new QList<ptGraphicsThumbGroup*>;
  m_Thumbnailer = new ptThumbnailer;
  m_Thumbnailer->setThumbList(m_ThumbList);

  // Init thumbnail cache
  m_Cache = new ptThumbnailCache(1000);
  m_Thumbnailer->setCache(m_Cache);
}

//==============================================================================

ptFileMgrDM::~ptFileMgrDM() {
  StopThumbnailer();
  DelAndNull(m_TreeModel);
  DelAndNull(m_ThumbList);
  DelAndNull(m_Thumbnailer);
  DelAndNull(m_Cache);
}

//==============================================================================

void ptFileMgrDM::Clear() {
  StopThumbnailer();
  m_Cache->Clear();
}

//==============================================================================

int ptFileMgrDM::setThumbnailDir(const QModelIndex index) {
  int result = m_Thumbnailer->setDir(m_TreeModel->filePath(index));
  if (result > -1) {
    m_CurrentImgDir = m_TreeModel->filePath(index);
  } else {
    m_CurrentImgDir.clear();
  }
  return result;
}

//==============================================================================

void ptFileMgrDM::StartThumbnailer() {
  m_Thumbnailer->start();
}

//==============================================================================

void ptFileMgrDM::StopThumbnailer() {
  m_Thumbnailer->blockSignals(true);
  m_Thumbnailer->Abort();
  m_Thumbnailer->blockSignals(false);
}

//==============================================================================
