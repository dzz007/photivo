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

#include "ptThumbGen.h"
#include "../ptImage8.h"
#include "../ptDcRaw.h"
#include "../ptMutexLocker.h"
#include "../ptInfo.h"
#include <QApplication>
#include <QImage>

//------------------------------------------------------------------------------
// Register user-defined types with the Qt meta object system.
// Needed for communication between the thumbnail and GUI thread.
auto TThumbAssoc_MetaId_Dummy = qRegisterMetaType<TThumbAssoc>("photivo_TThumbAssoc");

//------------------------------------------------------------------------------
/*!
  Creates a ptThumbGen object and moves it to its own thread. Note that because of this
  although ptThumbGen is derived from QObject the Qt parent mechanism cannot be used.

  Creation and destruction of heap allocated members needs to be done on the same
  thread where execution happens, otherwise the usual tools for thread-safe access
  need to be used. Therefore the ctor first moves the fresh object to the worker
  thread and then allocates all heap members. Destruction happens in reverse order.
*/
ptThumbGen::ptThumbGen():
  QObject(nullptr),
  FAbortSignaled(false),
  FAbortMutex(),
  FIsRunning(false),
  FIsRunningMutex()
{
  this->moveToThread(&FThread);

  // create heap allocated members; must happen AFTER moveToThread()
}

//------------------------------------------------------------------------------
/*!
  Destroys a ptThumbGen object. The object is moved back to the main GUI thread during destruction.

  See the ctor’s comment for notes about creation/destruction order.
*/
ptThumbGen::~ptThumbGen() {
  // destroy heap allocated members; must happen BEFORE moveToThread()

  auto hGuiThread = QApplication::instance()->thread();
  if (hGuiThread != &FThread) {
    this->moveToThread(hGuiThread);
  }
}

//------------------------------------------------------------------------------
/*! Aborts the currently running thumbnail generation. */
void ptThumbGen::abort() {
  if (this->isRunning()) {
    // Processing stops after the current image is finished ...
    ptMutexLocker hAbortLock(&FAbortMutex);
    FAbortSignaled = true;

    // ... so we need to give generate() some time or we’d return too early.
    QTime hTimer;
    hTimer.start();
    while (this->isRunning() && (hTimer.elapsed() < 5000)) {
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

  } else {
    // when idle stopping can happen immediately
    ptMutexLocker hAbortLock(&FAbortMutex);
    FAbortSignaled = false;
    this->stopProcessing();
  }
}

//------------------------------------------------------------------------------
bool ptThumbGen::isRunning() const {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  return FIsRunning;
}

//------------------------------------------------------------------------------
/*!
  Requests a list of thumbnail images and returns immediately. When the actual image data
  is ready the broadcast() signal is emitted once for each image.
*/
void ptThumbGen::request(QList<TThumbAssoc> AThumbList) {
  /*
    When you write a new public slot it MUST start with an *if* construct like the following.
    It is required to hide the details of thread management. From the caller’s point of view
    calling ptThumbGen methods looks like simple function calls even though they are queued
    slot calls behind the scenes. In effect this approach ensures that any code that accesses
    data on the thumbnail thread is always executed from within that thread, eliminating the
    the need for any explict thread safety measures.
    Alternatively the caller could define and properly connect its own signal and emit it where
    the ptThumbGen method call would occur. However that approach is too error prone and awkward
    to use.
    How it works: When the methid is called from a different thread the only thing that happens
    is the method posting a call to itself to the thumbnail thread’s message queue via Qt’s
    meta object system. Only then execution happens for real.
    Note that parameters of user-defined type must be registered with the meta object system first.
    See the declaration of TThumbId for details.
  */
  if (QThread::currentThread() != &FThread) {
    QMetaObject::invokeMethod(this, __FUNCTION__, Qt::QueuedConnection,
                              Q_ARG(QList<TThumbAssoc>, AThumbList));
    return;
  }

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
void ptThumbGen::processRequests() {
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
void ptThumbGen::stopProcessing() {
  this->setIsRunning(false);
  FThumbQueue.clear();
}

//------------------------------------------------------------------------------
/*!
  Reads a thumbnail from disk, resizes to the specified size and returns the final
  thumbnail image as a ptImage8.
*/
TThumbPtr ptThumbGen::generate(const TThumbId& AThumbId) {
  auto hThumbnail = FThumbCache.find(AThumbId);

  // cache miss: generate thumbnail
  if (!hThumbnail) {
    const QString hFilePath = AThumbId.FilePath;

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
void ptThumbGen::scaleSize(QSize& ASize, int ALongEdge) {
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
void ptThumbGen::setIsRunning(bool AValue) {
  ptMutexLocker hRunLock(&FIsRunningMutex);
  FIsRunning = AValue;
}

//------------------------------------------------------------------------------
/*! Rotates and scales an image. To avoid scaling set width and height of ASize to <=0. */
void ptThumbGen::transformImage(MagickWand* AInImage, ptImage8* AOutImage, const QSize& ASize) {
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
