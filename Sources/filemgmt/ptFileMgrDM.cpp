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
#include "../ptDcRaw.h"
#include "../ptImage8.h"
#include "../ptWinApi.h"
#include "ptFileMgrDM.h"
#include "ptFileMgrConstants.h"
#include "ptThumbDefines.h"

extern ptSettings* Settings;

//==============================================================================

ptFileMgrDM* ptFileMgrDM::m_Instance = nullptr;

//==============================================================================

/*static*/
ptFileMgrDM* ptFileMgrDM::GetInstance() {
  if (m_Instance == nullptr) {
    m_Instance = new ptFileMgrDM();
  }

  return m_Instance;
}

//==============================================================================

/*static*/
void ptFileMgrDM::DestroyInstance() {
  delete m_Instance;
  m_Instance = nullptr;
}

//==============================================================================

ptFileMgrDM::ptFileMgrDM()
: QObject(),
  m_IsMyComputer(false),
  FThumbGen(make_unique<ptThumbGen>())
{
  m_DirModel    = new ptSingleDirModel;
  m_TagModel    = new ptTagModel;

  // Init stuff for thumbnail generation
  FThumbGroupList = new QList<ptGraphicsThumbGroup*>;

  m_FocusedThumb = -1;
}

//==============================================================================

ptFileMgrDM::~ptFileMgrDM() {
  DelAndNull(FThumbGroupList);
  DelAndNull(m_DirModel);
  DelAndNull(m_TagModel);
}

//==============================================================================

void ptFileMgrDM::Clear() {
  FThumbGroupList->clear();
}

//==============================================================================

int ptFileMgrDM::setThumDir(const QString& hAbsolutePath) {
  m_IsMyComputer = (hAbsolutePath == MyComputerIdString);

  if (m_IsMyComputer) {
    m_CurrentDir.setPath("");
    return m_CurrentDir.drives().count();

  } else {
    m_CurrentDir.setPath(hAbsolutePath);
    QDir::Filters filters = QDir::Files;

    if (Settings->GetInt("FileMgrShowDirThumbs")) {
      filters = filters | QDir::AllDirs | QDir::NoDot;
    }

    m_CurrentDir.setFilter(filters);
    return m_CurrentDir.count();
  }
}

//------------------------------------------------------------------------------
void ptFileMgrDM::connectThumbGen(const QObject* AReceiver, const char* ABroadcastSlot) {
  connect(FThumbGen.get(), SIGNAL(broadcast(uint, TThumbPtr)), AReceiver, ABroadcastSlot);
}

//------------------------------------------------------------------------------
bool ptFileMgrDM::thumbGenRunning() const {
  return FThumbGen->isRunning();
}

//------------------------------------------------------------------------------
void ptFileMgrDM::abortThumbGen() {
  FThumbGen->abort();
}

//==============================================================================

ptGraphicsThumbGroup* ptFileMgrDM::MoveFocus(const int index) {
  m_FocusedThumb = index;
  return FThumbGroupList->at(index);
}

//------------------------------------------------------------------------------
void ptFileMgrDM::populateThumbs(QGraphicsScene* AScene) {
  FThumbGen->abort();
  AScene->clear();

  QFileInfoList files;
#ifdef Q_OS_WIN
  if (m_IsMyComputer) {
    if (Settings->GetInt("FileMgrShowDirThumbs")) {
      files = m_CurrentDir.drives();
    }
  } else {
    files = m_CurrentDir.entryInfoList();
  }
#else
  files = m_CurrentDir.entryInfoList();
#endif

  auto hLongEdgeMax = Settings->GetInt("FileMgrThumbnailSize");
  QList<TThumbAssoc> hThumbIdList;

  // Create thumbgroup objects and the ID list for thumb image generation.
  // Thumbgroup creation is very fast even for large directories. Can be done in the GUI thread
  // without speed issues.
  uint hGroupId = 0;
  for (QFileInfo& file: files) {
    this->createThumbGroup(file, hGroupId, AScene);
    hThumbIdList.append({{file.canonicalFilePath(), file.lastModified(), hLongEdgeMax}, hGroupId});
    ++hGroupId;
  }

  FThumbGen->request(hThumbIdList);
}

//------------------------------------------------------------------------------
/*!
  Creates a thumbgroup object for the specified file and adds it to the scene
  and to the DM’s FThumbGroupList.
*/
void ptFileMgrDM::createThumbGroup(const QFileInfo& AFileInfo, uint AId, QGraphicsScene* AScene) {
  auto thumbGroup = new ptGraphicsThumbGroup(AId);
  AScene->addItem(thumbGroup);
  ptFSOType type;

  QString descr;
  if (m_IsMyComputer) {
#ifdef Q_OS_WIN
    type = fsoDir;
    descr = WinApi::VolumeNamePretty(AFileInfo.absoluteFilePath());
    thumbGroup->addInfoItems(AFileInfo.absoluteFilePath(), descr, type);
#else
    assert(!"Folder MyComputer must not happen on non-Windows systems!");
#endif

  } else {
    descr = AFileInfo.fileName();
    if (AFileInfo.isDir()) {
      if (descr == "..") {
        type = fsoParentDir;
        descr = QDir(AFileInfo.canonicalFilePath()).dirName();
#ifdef Q_OS_WIN
        if (descr.isEmpty()) {
          // parent folder is a drive
          QString drive = AFileInfo.canonicalFilePath().left(2);
          descr = QString("%1 (%2)").arg(WinApi::VolumeName(drive)).arg(drive).trimmed();
        }
#endif
      } else {
        type = fsoDir;
      }
    } else {
      type = fsoFile;
    }
    thumbGroup->addInfoItems(AFileInfo.canonicalFilePath(), descr, type);
  }

  FThumbGroupList->append(thumbGroup);
}

//==============================================================================

int ptFileMgrDM::focusedThumb(QGraphicsItem* group) {
  for (int i = 0; i < FThumbGroupList->count(); ++i) {
    if (FThumbGroupList->at(i) == group) {
      m_FocusedThumb = i;
      return i;
    }
  }
  m_FocusedThumb = -1;
  return -1;
}

//==============================================================================
