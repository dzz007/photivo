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
#include "ptThumbGenHelpers.h"
#include "ptThumbCache.h"
#include "../ptImage8.h"
#include "../ptDcRaw.h"
#include "../ptInfo.h"
#include <QApplication>
#include <QImage>

//------------------------------------------------------------------------------
/*!
  Creates a ptThumbGenWorker object. Note that because the worker is supposed to run in its own
  thread the Qt parent mechanism cannot be used although ptThumbGenWorker is derived from QObject.

  AQueue, ACache and AAbortCtrl must be valid pointers and guaranteed to live at least as long
  as the ptThumbGenWorker object. The object does *not* take ownership of any.
*/
ptThumbGenWorker::ptThumbGenWorker(ptThumbQueue* AQueue,
                                   ptThumbCache* ACache,
                                   ptFlowController* AAbortCtrl):
  QObject(nullptr),
  FIsRunning(false),
  FAbortCtrl(AAbortCtrl),
  FThumbCache(ACache),
  FThumbQueue(AQueue),
  Process_Func("process")
{
  Q_ASSERT_X(AAbortCtrl != nullptr, __PRETTY_FUNCTION__, "Pointer to thumbnail abort ctrler is null.");
  Q_ASSERT_X(ACache != nullptr, __PRETTY_FUNCTION__, "Pointer to thumbnail cache is null.");
  Q_ASSERT_X(AQueue != nullptr, __PRETTY_FUNCTION__, "Pointer to thumbnail queue is null.");
}

//------------------------------------------------------------------------------
/*! Destroys a ptThumbGen object. */
ptThumbGenWorker::~ptThumbGenWorker() {
  // See ctor comment for things you MUST NOT delete here.
}

//------------------------------------------------------------------------------
/*! Returns true if the worker is running, i.e. processing images. */
bool ptThumbGenWorker::isRunning() const {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  return FIsRunning;
}

//------------------------------------------------------------------------------
// Thread-safe setter for the worker’s running state
void ptThumbGenWorker::setIsRunning(bool AValue) {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  FIsRunning = AValue;
}

//------------------------------------------------------------------------------
/*!
  Starts processing images. If the thumbnail queue is empty, the worker goes back
  to idle without emitting anything. If the worker is already busy running this
  function has no effect.
*/
void ptThumbGenWorker::start() {
  this->postProcessEvent();
}

//------------------------------------------------------------------------------
// Processes (= generates and emits) one image. Triggers the next processing or
// sets the worker to idle.
void ptThumbGenWorker::process() {
  if (!FAbortCtrl->isOpen()) {
    return;
  }

  // Generate thumbnail image (if an ID is available) and broadcast to receiver(s)
  auto hThumbAssoc = FThumbQueue->dequeue();

  // Got a thumbnail ID. Continue with generation
  if (hThumbAssoc) {
    auto hThumb = this->generateThumb(hThumbAssoc.ThumbId);
    if (FAbortCtrl->isOpen()) {
      emit broadcast(hThumbAssoc.GroupId, hThumb);
    }
    this->postProcessEvent();

  // Got an invalid thumbnail ID, i.e. queue is empty: put worker into idle
  } else {
    this->setIsRunning(false);
  }
}

//------------------------------------------------------------------------------
// Wrapper for posting a processing request to the workers event loop.
void ptThumbGenWorker::postProcessEvent() {
  QMetaObject::invokeMethod(this, Process_Func, Qt::QueuedConnection);
}

//------------------------------------------------------------------------------
// Reads a thumbnail from disk, resizes to the specified size and returns the final
// thumbnail image as a ptImage8.
TThumbPtr ptThumbGenWorker::generateThumb(const TThumbId& AThumbId) {
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
  auto hThumbnail = FThumbCache->find(AThumbId);

  // cache miss: generate thumbnail
  if (!hThumbnail) {
    const QString hFilePath = AThumbId.FilePath;
    hThumbnail.reset(new ptImage8);

    ptDcRaw     hDcRaw;
    MagickWand* hGMImage = NewMagickWand();
    QSize       hSize;

    if (hDcRaw.Identify(hFilePath) == 0) {
      // we have a raw image
      std::vector<char> hImgData;
      if (hDcRaw.thumbnail(hImgData)) {
        // raw thumbnail read successfully
        hSize = this->scaleSize(hDcRaw.m_ThumbWidth, hDcRaw.m_ThumbHeight, AThumbId.LongEdgeSize);
        MagickSetSize(hGMImage, 2*hSize.width(), 2*hSize.height());
        MagickReadImageBlob(hGMImage, reinterpret_cast<const uchar*>(hImgData.data()), hImgData.size());
      }
    } else {
      // no raw, try for bitmap
      MagickPingImage(hGMImage, hFilePath.toLocal8Bit().data());
      hSize = this->scaleSize(MagickGetImageWidth(hGMImage),
                              MagickGetImageHeight(hGMImage),
                              AThumbId.LongEdgeSize);
      MagickSetSize(hGMImage, 2*hSize.width(), 2*hSize.height());
      MagickReadImage(hGMImage, hFilePath.toLocal8Bit().data());
    }

    ExceptionType hMagickExcept;
    const char* hMagickErrMsg = MagickGetException(hGMImage, &hMagickExcept);
    if (hMagickExcept != UndefinedException) {
      // error occurred: no raw thumbnail, no supported image type, any other GM error
      GInfo->Warning(QString::fromLocal8Bit(hMagickErrMsg).toLocal8Bit().data());
      hThumbnail->FromQImage(QImage(QString::fromUtf8(":/dark/icons/broken-image-48px.png")));
    } else {
      // no error: scale and rotate thumbnail
      this->transformImage(hGMImage, hThumbnail.get(), hSize);
    }

    DestroyMagickWand(hGMImage);
    FThumbCache->insert(AThumbId, hThumbnail);
  }

  return hThumbnail;
}

//------------------------------------------------------------------------------
// Clamp given dimension’s to longer edge to a maximum of AMaxLongEdge.
QSize ptThumbGenWorker::scaleSize(int AWidth, int AHeight, int AMaxLongEdge) {
  // Image is smaller than maximum. Do not change size at all.
  if ((AWidth < AMaxLongEdge) && (AHeight < AMaxLongEdge)) {
    return QSize(AWidth, AHeight);

  // landscape image
  } else if (AWidth > AHeight) {
    return QSize(AMaxLongEdge, (AHeight/(double)AWidth * AMaxLongEdge + 0.5));

  // portrait image
  } else if (AWidth < AHeight) {
    return QSize((AWidth/(double)AHeight * AMaxLongEdge + 0.5), AMaxLongEdge);

  // square image
  } else {
    return QSize(AMaxLongEdge, AMaxLongEdge);
  }
}

//------------------------------------------------------------------------------
// Rotates and scales an image. To avoid scaling set width and height of ASize to <=0.
void ptThumbGenWorker::transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize) {
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  MagickSetImageDepth(AInImage, 8);
  MagickSetImageFormat(AInImage, "RGB");
  MagickSetImageType(AInImage, TrueColorType);

  if (ASize.width() > 0 && ASize.height() > 0)
    MagickScaleImage(AInImage, ASize.width(), ASize.height());

  // read EXIF orientation and rotate image
  int hOrientation = QString::fromLocal8Bit(MagickGetImageAttribute(AInImage, "EXIF:Orientation")).toInt();
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
                       reinterpret_cast<uchar*>(AOutImage->image().data()));
}
