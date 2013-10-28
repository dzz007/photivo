/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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

#include "ptFileMgrDM.h"
#include "ptFileMgrConstants.h"
#include "ptThumbDefines.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptDcRaw.h"
#include "../ptImage8.h"
#include "../ptWinApi.h"
#include <QGraphicsScene>

extern ptSettings* Settings;
extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//------------------------------------------------------------------------------
ptFileMgrDM::ptFileMgrDM(QObject* AParent)
: QObject(AParent),
  FFocusedThumb(-1),
  FDirModel(new ptSingleDirModel(this)),
  FIsMyComputer(false),
  FTagModel(new ptTagModel(this)),
  // Cache is split 60/40 between thumbnail list and image view
  FThumbGen(Settings->GetInt("FileMgrThumbCacheSize")*1024*1024 * 0.6),
  FThumbGroupList(new QList<ptGraphicsThumbGroup*>)
{}

//------------------------------------------------------------------------------
ptFileMgrDM::~ptFileMgrDM() {
  DelAndNull(FThumbGroupList);
}

//------------------------------------------------------------------------------
/*! Clear the data cache of \c ptFileMgrDM */
void ptFileMgrDM::clear() {
  FThumbGen.clear();
  FThumbGroupList->clear();
}

//------------------------------------------------------------------------------
/*! Returns the current folder for thumbnail display. */
QString ptFileMgrDM::currentDir() const {
  return FCurrentDir.absolutePath();
}

//------------------------------------------------------------------------------
/*! Returns a pointer to the model for the folder ListView. */
ptSingleDirModel*ptFileMgrDM::dirModel() const {
  return FDirModel;
}

//------------------------------------------------------------------------------
/*! Sets the directory for thumbnail generation.
  Returns the total number of applicable entries in that directory.
  Returns \c -1 and does not set the directory if the thumbnailer is
  currently running.
  \param path
    Sets the directory for thumbnail generation. Must be an absolute path.
*/
int ptFileMgrDM::setThumDir(const QString& AAbsolutePath) {
  FIsMyComputer = (AAbsolutePath == MyComputerIdString);

  if (FIsMyComputer) {
    FCurrentDir.setPath("");
    return FCurrentDir.drives().count();

  } else {
    FCurrentDir.setPath(AAbsolutePath);
    QDir::Filters filters = QDir::Files;

    if (Settings->GetInt("FileMgrShowDirThumbs")) {
      filters = filters | QDir::AllDirs | QDir::NoDot;
    }

    FCurrentDir.setFilter(filters);
    FCurrentDir.setSorting(QDir::Name | QDir::DirsFirst);

    QStringList fileExts;
    if (Settings->GetInt("FileMgrShowRAWs")) {
      fileExts << FileExtsRaw;
    }
    if (Settings->GetInt("FileMgrShowBitmaps")) {
      fileExts << FileExtsBitmap;
    }
    QFileInfoList files = FCurrentDir.entryInfoList(fileExts);
    return files.count();
  }
}

//------------------------------------------------------------------------------
/*! Returns the index in thumbGroupList() of the currently focused thumbnail. */
int ptFileMgrDM::focusedThumb() const {
  return FFocusedThumb;
}

//------------------------------------------------------------------------------
/*! Returns a pointer to the tag model. */
ptTagModel*ptFileMgrDM::tagModel() {
  return FTagModel;
}

//------------------------------------------------------------------------------
/*! Returns a pointer to the list of currently displayed thumbnail images. */
QList<ptGraphicsThumbGroup*>*ptFileMgrDM::thumbGroupList() {
  return FThumbGroupList;
}

//------------------------------------------------------------------------------
/*! Connects the signal emitted when a thumbnail image is generated with the given slot. */
void ptFileMgrDM::connectThumbGen(const QObject* AReceiver, const char* ABroadcastSlot) {
  FThumbGen.connectBroadcast(AReceiver, ABroadcastSlot);
}

//------------------------------------------------------------------------------
/*! Returns true while thumbnails are being generated. */
bool ptFileMgrDM::thumbGenRunning() const {
  return FThumbGen.isRunning();
}

//------------------------------------------------------------------------------
/*!
  Stops thumbnail generation. It is guaranteed that generation has actually stopped
  when this method returns.
*/
void ptFileMgrDM::abortThumbGen() {
  FThumbGen.abort();
}

//------------------------------------------------------------------------------
/*! Focusses the thumbgroup with the given index in thumbGroupList(). */
ptGraphicsThumbGroup* ptFileMgrDM::moveFocus(const int index) {
  FFocusedThumb = index;
  return FThumbGroupList->at(index);
}

//------------------------------------------------------------------------------
/*!
  Clears the scene and starts thumbnail generation for the current directory.
  Returns when all ptGraphicsThumbGroup objects are created. The actual thumbnail images
  are generated asynchonously.
  \see connectThumbGen()
*/
void ptFileMgrDM::populateThumbs(QGraphicsScene* AScene) {
  FThumbGen.abort();
  FThumbGroupList->clear();
  AScene->clear();

  QFileInfoList files;

  QStringList fileExts;
  if (Settings->GetInt("FileMgrShowRAWs")) {
    fileExts << FileExtsRaw;
  }
  if (Settings->GetInt("FileMgrShowBitmaps")) {
    fileExts << FileExtsBitmap;
  }

#ifdef Q_OS_WIN
  if (FIsMyComputer) {
    if (Settings->GetInt("FileMgrShowDirThumbs")) {
      files = FCurrentDir.drives();
    }
  } else {
    files = FCurrentDir.entryInfoList(fileExts);
  }
#else
  files = FCurrentDir.entryInfoList(fileExts);
#endif

  auto hLongEdgeMax = Settings->GetInt("FileMgrThumbnailSize");
  QList<TThumbAssoc> hThumbIdList;

  // Create thumbgroup objects and the ID list for thumb image generation.
  // Thumbgroup creation is very fast even for large directories. Can be done in the GUI thread
  // without speed issues.
  uint hGroupId = CFirstThumbReceiverId;
  for (QFileInfo& file: files) {
    this->createThumbGroup(file, hGroupId, AScene);
    hThumbIdList.append({hGroupId,
                         makeThumbId(file, hLongEdgeMax, FThumbGroupList->last()->fsoType())});
    ++hGroupId;
  }

  FThumbGen.request(hThumbIdList);
}

//------------------------------------------------------------------------------
/*! Returns a thumbnail for the given file with high priority. */
TThumbPtr ptFileMgrDM::getThumb(const QString& AFilename, int ALongEdgeSize) {
  // Some work is required to make this function blocking and receive a single image.
  // - Temporarily connect the receiveThumb() slot to FThumbGen.
  // - Post the high prio request for the thumb and spin the event loop until receiveThumb()
  //   was executed and the image put into FSingleThumb.
  // - Clean up and return the image.
  FThumbGen.connectBroadcast(this, SLOT(receiveThumb(uint,TThumbPtr)));
  FThumbGen.request({CSingleThumbReceiverId, makeThumbId(AFilename, ALongEdgeSize, fsoFile)},
                    TThumbQPrio::High);

  // hTime is for deadlock prevention. Abort waiting for thumbnail after 20 seconds.
  QTime hTime;
  hTime.start();
  while (!FSingleThumb) {
    if (hTime.elapsed() > 20000) {
      disconnect(this, SLOT(receiveThumb(uint,TThumbPtr)));
      GInfo->Warning("Thumbnail timeout: " + AFilename, AT);
      break;
    }
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  }

  // Temp variable is needed to not hold a superfluous pointer to the requested image
  // after the function exits.
  auto hTempThumb = FSingleThumb;
  FSingleThumb.reset();
  return hTempThumb;
}

//------------------------------------------------------------------------------
// Helper for getThumb()
void ptFileMgrDM::receiveThumb(uint AId, TThumbPtr AThumb) {
  if (AId == CSingleThumbReceiverId) {
    disconnect(this, SLOT(receiveThumb(uint,TThumbPtr)));
    FSingleThumb = AThumb;
  }
}

//------------------------------------------------------------------------------
/*! Sets the folder for thumbnail display. Does not trigger the thumbnailer.
    You probably need this only once to init the folder. */
void ptFileMgrDM::setCurrentDir(const QString& AAbsolutePath) {
  FCurrentDir.setPath(AAbsolutePath);
}

//------------------------------------------------------------------------------
// Creates a thumbgroup object for the specified file and adds it to the scene
// and to the DM’s FThumbGroupList.
void ptFileMgrDM::createThumbGroup(const QFileInfo& AFileInfo, uint AId, QGraphicsScene* AScene) {
  auto thumbGroup = new ptGraphicsThumbGroup(AId);
  AScene->addItem(thumbGroup);
  ptFSOType type;

  QString descr;
  if (FIsMyComputer) {
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

//------------------------------------------------------------------------------
// TODO BJ: Might change FFocusedThumb. Either bad method naming or bad design. Needs to be changed.
int ptFileMgrDM::focusedThumb(QGraphicsItem* group) {
  for (int i = 0; i < FThumbGroupList->count(); ++i) {
    if (FThumbGroupList->at(i) == group) {
      FFocusedThumb = i;
      return i;
    }
  }
  FFocusedThumb = -1;
  return -1;
}

