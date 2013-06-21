/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptThumbGenWorker.h"
#include "../ptImage8.h"
#include "../ptDcRaw.h"
#include "../ptMutexLocker.h"
#include "../ptInfo.h"
#include <QApplication>
#include <QImage>

//------------------------------------------------------------------------------
/*!
  Creates a ptThumbGen object. Note that because the worker is supposed to run in its own thread
  the Qt parent mechanism cannot be used although ptThumbGen is derived from QObject.
*/
ptThumbGenWorker::ptThumbGenWorker():
  QObject(nullptr),
  FAbortSignaled(false),
  FIsRunning(false),
  FThumbCache(ptThumbCache(100*1024*1024)) // TODO BJ: RAM-size dependent/user-definable cache size
{}

//------------------------------------------------------------------------------
/*! Destroys a ptThumbGen object. */
ptThumbGenWorker::~ptThumbGenWorker() {}

//------------------------------------------------------------------------------
/*!
  Aborts the currently running thumbnail generation. When abort() returns it is *not*
  guaranteed that processing has already stopped.
*/
void ptThumbGenWorker::abort() {
  if (this->isRunning()) {
    // Tell the worker to abort as soon as possible.
    // Processing will stop after the current image is finished
    ptMutexLocker hAbortLock(&FAbortMutex);
    FAbortSignaled = true;

  } else {
    // when idle stopping happens immediately
    ptMutexLocker hAbortLock(&FAbortMutex);
    FAbortSignaled = false;
    this->stopProcessing();
  }
}

//------------------------------------------------------------------------------
bool ptThumbGenWorker::isRunning() const {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  return FIsRunning;
}

//------------------------------------------------------------------------------
/*!
  Requests a list of thumbnail images and returns immediately. When the actual image data
  is ready the broadcast() signal is emitted once for each image.
*/
void ptThumbGenWorker::request(QList<TThumbAssoc> AThumbList) {
  for (TThumbAssoc& hThumbAssoc: AThumbList)
    FThumbQueue.enqueue(hThumbAssoc);

    ptMutexLocker hLock(&FIsRunningMutex);
    if (!FIsRunning) {
      FIsRunning = true;
      hLock.unlock();
      this->processRequests();
    }
}

//------------------------------------------------------------------------------
void ptThumbGenWorker::processRequests() {
  while (!FThumbQueue.isEmpty()) {
    // generate thumbnail image and broadcast to receiver(s)
    auto hThumbAssoc = FThumbQueue.dequeue();
    auto hThumb = this->generate(hThumbAssoc.ThumbId);

    QApplication::processEvents();

    {  // abort processing if requested
      ptMutexLocker hAbortLock(&FAbortMutex);
      if (FAbortSignaled) {
        FAbortSignaled = false;
        this->stopProcessing();
        break;
      }
    }

    emit broadcast(hThumbAssoc.GroupId, hThumb);
  }

  this->setIsRunning(false);
}

//------------------------------------------------------------------------------
void ptThumbGenWorker::stopProcessing() {
  this->setIsRunning(false);
  FThumbQueue.clear();
}

//------------------------------------------------------------------------------
/*!
  Reads a thumbnail from disk, resizes to the specified size and returns the final
  thumbnail image as a ptImage8.
*/
TThumbPtr ptThumbGenWorker::generate(const TThumbId& AThumbId) {
  // Directories: not cached and no need to set dcraw or GM on them.
  switch (AThumbId.Type) {
    case fsoParentDir: {
      auto hDirThumbnail = std::make_shared<ptImage8>();
      hDirThumbnail->FromQImage(QImage(QString::fromUtf8(":/dark/icons/go-up-48px.png")));
      return hDirThumbnail;
    }

    case fsoDir:
    case fsoDrive:
    case fsoRoot: {
      auto hPDirThumbnail = std::make_shared<ptImage8>();
      hPDirThumbnail->FromQImage(QImage(QString::fromUtf8(":/dark/icons/folder-48px.png")));
      return hPDirThumbnail;
    }

    default: ; // other cases not handled here intentionally
  }

  // Actual image: First try the cache. On miss generate thumb via dcraw or GM
  auto hThumbnail = FThumbCache.find(AThumbId);

  // cache miss: generate thumbnail
  if (!hThumbnail) {
    const QString hFilePath = AThumbId.FilePath;
    hThumbnail.reset(new ptImage8);

    ptDcRaw     hDcRaw;
    MagickWand* hGMImage = NewMagickWand();
    QSize       hSize = QSize(AThumbId.LongEdgeSize, AThumbId.LongEdgeSize);


    if (hDcRaw.Identify(hFilePath) == 0) {
      // we have a raw image
      std::vector<char> hImgData;
      if (hDcRaw.thumbnail(hImgData)) {
        // raw thumbnail read successfully
        hSize.setWidth(hDcRaw.m_ThumbWidth);
        hSize.setHeight(hDcRaw.m_ThumbHeight);
        this->scaleSize(hSize, AThumbId.LongEdgeSize);
        MagickSetSize(hGMImage, 2*hSize.width(), 2*hSize.height());
        MagickReadImageBlob(hGMImage, reinterpret_cast<const uchar*>(hImgData.data()), hImgData.size());
      }
    } else {
      // no raw, try for bitmap
      MagickPingImage(hGMImage, hFilePath.toAscii().data());
      hSize.setWidth(MagickGetImageWidth(hGMImage));
      hSize.setHeight(MagickGetImageHeight(hGMImage));
      this->scaleSize(hSize, AThumbId.LongEdgeSize);
      MagickSetSize(hGMImage, 2*hSize.width(), 2*hSize.height());
      MagickReadImage(hGMImage, hFilePath.toAscii().data());
    }

    ExceptionType hMagickExcept;
    const char* hMagickErrMsg = MagickGetException(hGMImage, &hMagickExcept);
    if (hMagickExcept != UndefinedException) {
      // error occurred: no raw thumbnail, no supported image type, any other GM error
      GInfo->Warning(QString::fromAscii(hMagickErrMsg).toAscii().data());
      hThumbnail->FromQImage(QImage(QString::fromUtf8(":/dark/icons/broken-image-48px.png")));
    } else {
      // no error: scale and rotate thumbnail
      this->transformImage(hGMImage, hThumbnail.get(), hSize);
    }

    DestroyMagickWand(hGMImage);
    FThumbCache.insert(AThumbId, hThumbnail);
  }

  return hThumbnail;
}

//------------------------------------------------------------------------------
/*! Clamp ASize’s longer edge to a maximum of ALongEdge. */
void ptThumbGenWorker::scaleSize(QSize& ASize, int ALongEdge) {
  if ((ASize.width() < ALongEdge) && (ASize.height() < ALongEdge))
    return;

  if (ASize.width() > ASize.height()) {           // landscape image
    ASize.setHeight(ASize.height()/(double)ASize.width() * ALongEdge + 0.5);
    ASize.setWidth(ALongEdge);

  } else if (ASize.width() < ASize.height()) {    // portrait image
    ASize.setWidth(ASize.width()/(double)ASize.height() * ALongEdge + 0.5);
    ASize.setHeight(ALongEdge);

  } else if (ASize.width() == ASize.height()) {   // square image
    ASize.setWidth(ALongEdge);
    ASize.setHeight(ALongEdge);
  }
}

//------------------------------------------------------------------------------
void ptThumbGenWorker::setIsRunning(bool AValue) {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  FIsRunning = AValue;
}

//------------------------------------------------------------------------------
/*! Rotates and scales an image. To avoid scaling set width and height of ASize to <=0. */
void ptThumbGenWorker::transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize) {
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  MagickSetImageDepth(AInImage, 8);
  MagickSetImageFormat(AInImage, "RGB");
  MagickSetImageType(AInImage, TrueColorType);

  if (ASize.width() > 0 && ASize.height() > 0)
    MagickScaleImage(AInImage, ASize.width(), ASize.height());

  // read EXIF orientation and rotate image
  int hOrientation = QString::fromAscii(MagickGetImageAttribute(AInImage, "EXIF:Orientation")).toInt();
  PixelWand* hPxWand = NewPixelWand();
  switch (hOrientation) {
    case 2: MagickFlopImage(  AInImage); break;
    case 3: MagickRotateImage(AInImage, hPxWand, 180); break;
    case 4: MagickFlipImage(  AInImage); break;
    case 5: MagickFlopImage(  AInImage); MagickRotateImage(AInImage, hPxWand, 270); break;
    case 6: MagickRotateImage(AInImage, hPxWand, 90); break;
    case 7: MagickFlipImage(  AInImage); MagickRotateImage(AInImage, hPxWand, 270); break;
    case 8: MagickRotateImage(AInImage, hPxWand, 270); break;
    default: break;
  }
  DestroyPixelWand(hPxWand);

  // Get the raw image data from GM.
  uint hOutWidth = MagickGetImageWidth(AInImage);
  uint hOutHeight = MagickGetImageHeight(AInImage);

  AOutImage->setSize(hOutWidth, hOutHeight, 3);
  MagickGetImagePixels(AInImage, 0, 0, hOutWidth, hOutHeight, "BGRA", CharPixel,
                       reinterpret_cast<uchar*>(AOutImage->m_Image));
}
