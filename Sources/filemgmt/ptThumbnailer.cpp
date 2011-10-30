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

#include "../ptDcRaw.h"
#include "../ptError.h"
#include "../ptCalloc.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "ptThumbnailer.h"

extern ptSettings* Settings;
extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//==============================================================================

ptThumbnailer::ptThumbnailer()
: QThread()
{
  m_AbortRequested = false;
  m_Cache     = NULL;
  m_ThumbList = NULL;

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
  if (!this->isRunning() && cache != NULL)
  {
    m_Cache = cache;
  }
}

//==============================================================================

int ptThumbnailer::setDir(const QString dir) {
  if (this->isRunning()) {
    return -1;
  }

  m_Dir->setPath(dir);

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
//  QTime timer;
//  timer.start();
  // Check for properly set directory, cache and buffer
  if (!m_Dir->exists() || m_ThumbList == NULL || m_Cache == NULL) {
    return;
  }

  QFileInfoList files = m_Dir->entryInfoList();
  int thumbMaxSize = Settings->GetInt("FileMgrThumbnailSize");
  QSize thumbSize = QSize(thumbMaxSize, thumbMaxSize);

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
        ptDcRaw dcRaw;
        bool isRaw = false;
        MagickWand* image = NewMagickWand();

        if (dcRaw.Identify(currentGroup->fullPath()) == 0 ) {
          // we have a raw image
          isRaw = true;
          QByteArray* ImgData = NULL;
          if (dcRaw.thumbnail(ImgData)) {
            // raw thumbnail read successfully
            thumbSize.setWidth(dcRaw.m_Width);
            thumbSize.setHeight(dcRaw.m_Height);
            ScaleThumbSize(&thumbSize, thumbMaxSize);
            MagickSetSize(image, 2*thumbSize.width(), 2*thumbSize.height());
            MagickReadImageBlob(image, (const uchar*)ImgData->data(), (const size_t)ImgData->length());
          }
          DelAndNull(ImgData);
        }

        if (!isRaw) {
          // no raw, try for bitmap
          MagickPingImage(image, currentGroup->fullPath().toAscii().data());
          thumbSize.setWidth(MagickGetImageWidth(image));
          thumbSize.setHeight(MagickGetImageHeight(image));
          ScaleThumbSize(&thumbSize, thumbMaxSize);
          MagickSetSize(image, 2*thumbSize.width(), 2*thumbSize.height());
          MagickReadImage(image, currentGroup->fullPath().toAscii().data());
        }

        ExceptionType MagickExcept;
        char* MagickErrMsg = MagickGetException(image, &MagickExcept);
        if (MagickExcept != UndefinedException) {
          // error occurred: no raw thumbnail, no supported image type, any other GM error
          printf("%s\n", QString::fromAscii(MagickErrMsg).toAscii().data());
          DelAndNull(thumbImage);
          thumbImage = new QImage(QString::fromUtf8(":/dark/icons/broken-image-48px.png"));

        } else {
          // no error: scale and rotate thumbnail
          thumbImage = GenerateThumbnail(image, thumbSize);
        }

        DestroyMagickWand(image);
      }
    }

  // Notification signal for each finished thumb image.
    emit newImageNotify(m_ThumbList->at(i), thumbImage);
  } // main FOR loop step 2

//  printf("elapsed %d, cache size %d\n", timer.elapsed(), m_Cache->Count());
}

//==============================================================================

QImage* ptThumbnailer::GenerateThumbnail(MagickWand* image, const QSize tSize)
{
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  MagickSetImageDepth(image, 8);
  MagickSetImageFormat(image, "RGB");
  MagickSetImageType(image, TrueColorType);
  MagickScaleImage(image, tSize.width(), tSize.height());

  // read EXIF orientation and correct image
  int orientation = QString::fromAscii(MagickGetImageAttribute(image, "EXIF:Orientation")).toInt();
  PixelWand* pxWand = NewPixelWand();
  switch (orientation) {
    case 2: MagickFlopImage(image); break;
    case 3: MagickRotateImage(image, pxWand, 180); break;
    case 4: MagickFlipImage(image); break;
    case 5: MagickFlopImage(image); MagickRotateImage(image, pxWand, 270); break;
    case 6: MagickRotateImage(image, pxWand, 90); break;
    case 7: MagickFlipImage(image); MagickRotateImage(image, pxWand, 270); break;
    case 8: MagickRotateImage(image, pxWand, 270); break;
    default: break;
  }
  DestroyPixelWand(pxWand);

  // Get the raw image data from GM.
  uint w = MagickGetImageWidth(image);
  uint h = MagickGetImageHeight(image);

  QImage* thumbImage = new QImage(w, h, QImage::Format_RGB32);
  MagickGetImagePixels(image, 0, 0, w, h, "BGRA", CharPixel, (uchar*)thumbImage->scanLine(0));
  return thumbImage;
}

//==============================================================================

void ptThumbnailer::ScaleThumbSize(QSize* tSize, const int max) {
  if (tSize->width() == tSize->height()) {    // square image
    tSize->setWidth(max);
    tSize->setHeight(max);
  } else if (tSize->width() > tSize->height()) {    // landscape image
    tSize->setHeight(tSize->height()/(double)tSize->width() * max + 0.5);
    tSize->setWidth(max);
  } else if (tSize->width() < tSize->height()) {    // portrait image
    tSize->setWidth(tSize->width()/(double)tSize->height() * max + 0.5);
    tSize->setHeight(max);
  }
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
