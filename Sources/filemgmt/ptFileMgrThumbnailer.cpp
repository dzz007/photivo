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
#include "ptFileMgrThumbnailer.h"

extern ptSettings* Settings;
extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//==============================================================================

ptFileMgrThumbnailer::ptFileMgrThumbnailer()
: QThread()
{
  m_AbortRequested = false;
  m_Cache     = NULL;
  m_ThumbList = NULL;

  m_Dir = new QDir("");
  m_Dir->setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
  m_Dir->setFilter(QDir::AllDirs | QDir::NoDot | QDir::Files);
  m_Dir->setNameFilters(FileExtsRaw + FileExtsBitmap);
}

//==============================================================================

ptFileMgrThumbnailer::~ptFileMgrThumbnailer() {
  DelAndNull(m_Dir);
}

//==============================================================================

void ptFileMgrThumbnailer::setCache(ptThumbnailCache* cache) {
  if (!this->isRunning() && cache != NULL)
  {
    m_Cache = cache;
  }
}

//==============================================================================

int ptFileMgrThumbnailer::setDir(const QString dir) {
  if (this->isRunning()) {
    return -1;
  }

  m_Dir->setPath(dir);
  return m_Dir->count();
}

//==============================================================================

void ptFileMgrThumbnailer::setThumbList(QList<ptGraphicsThumbGroup*>* ThumbList) {
  if (!this->isRunning() && ThumbList != NULL) {
    m_ThumbList = ThumbList;
  }
}

//==============================================================================


void ptFileMgrThumbnailer::run() {
  // Check for properly set directory, cache and buffer
  if (!m_Dir->exists() || m_ThumbList == NULL /*|| m_Cache == NULL*/) {
    return;
  }

  QFileInfoList files = m_Dir->entryInfoList();
  int thumbsSize = Settings->GetInt("ThumbnailSize");

  /***
    Step 1: Generate thumb groups without the thumbnail images
  ***/

  for (uint i = 0; i < (uint)files.count(); i++) {
    if (m_AbortRequested) {
      m_AbortRequested = false;
      return;
    }

    ptGraphicsThumbGroup* thumbGroup = new ptGraphicsThumbGroup;
    ptFSOType type;

    if (files.at(i).isDir()) {
      if (files.at(i).fileName() == "..")
        type = fsoParentDir;
      else
        type = fsoDir;
    } else {
      type = fsoFile;
    }

    thumbGroup->addInfoItems(files.at(i).canonicalFilePath(),
                             files.at(i).fileName(),
                             type);
    m_ThumbList->append(thumbGroup);

    emit newThumbNotify(false);
  }

  // final notification: signals end of step 1 to GUI thread
  emit newThumbNotify(true);


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
      // we have a parent directory
      thumbImage = new QImage(QString::fromUtf8(":/photivo/FileManager/up.png"));

    } else if (currentGroup->fsoType() == fsoDir) {
      // we have a subdirectory
      thumbImage = new QImage(QString::fromUtf8(":/photivo/FileManager/folder.png"));

    } else {
      // we have a file, see if we can get a thumbnail image
      ptDcRaw dcRaw;

      if (dcRaw.Identify(currentGroup->fullPath()) == 0 ) {
        // we have a raw image ...
        QByteArray* ba = NULL;
        if (dcRaw.thumbnail(ba)) {
          try {
            Magick::Blob  blob( ba->data(), ba->length());
            Magick::Image image;
            image.size(Magick::Geometry(2*thumbsSize, 2*thumbsSize));
            image.read(blob);

            thumbImage = GenerateThumbnail(image, thumbsSize);
          } catch (Magick::Exception &Error) {
            // ... not supported
            DelAndNull(thumbImage);
          }
        }
        DelAndNull(ba);

      } else {
        // ... or a bitmap ...
        try {
          Magick::Image image;
          image.size(Magick::Geometry(2*thumbsSize, 2*thumbsSize));
          image.read(currentGroup->fullPath().toAscii().data());

          thumbImage = GenerateThumbnail(image, thumbsSize);
        } catch (Magick::Exception &Error) {
          // ... or not a supported image file at all
          DelAndNull(thumbImage);
        }
      }
    }

    // Notification signal for each finished thumb image.
#ifdef DEBUG
  printf("%s: generated thumb image for %s\n", __FILE__, m_ThumbList->at(i)->fullPath().toAscii().data());
#endif
    emit newImageNotify(m_ThumbList->at(i), thumbImage);
  }
}

//==============================================================================

QImage* ptFileMgrThumbnailer::GenerateThumbnail(Magick::Image& image, const int thumbSize)
{
//  if (thumbImage != NULL) DelAndNull(thumbImage);
  // We want 8bit RGB data without alpha channel, scaled to thumbnail size
  image.depth(8);
  image.magick("RGB");
  image.type(Magick::TrueColorType);
  image.scale(Magick::Geometry(thumbSize, thumbSize));

  // read EXIF orientation and correct image
  QString orientation = QString::fromStdString(image.attribute("EXIF:Orientation"));
  if (orientation == "2") image.flop();
  else if (orientation == "3") image.rotate(180);
  else if (orientation == "4") image.flip();
  else if (orientation == "5") {image.flop(); image.rotate(270);}
  else if (orientation == "6") image.rotate(90);
  else if (orientation == "7") {image.flip(); image.rotate(270);}
  else if (orientation == "8") image.rotate(270);

  // Get the raw image data from GM.
  uint w = image.columns();
  uint h = image.rows();

  QImage* thumbImage = new QImage(w, h, QImage::Format_RGB32);
  image.write(0, 0, w, h, "BGRA", Magick::CharPixel, (uchar*)thumbImage->scanLine(0));
  return thumbImage;

//  uint8_t* ImgBuffer = NULL;
//  try {
//    ImgBuffer = (uint8_t*)CALLOC(w * h * 4, sizeof(ImgBuffer));
//  } catch (std::bad_alloc) {
//    // TODO: Cleanup!
//    printf("\n********************\n\nMemory error in thumbnail generator\n\n********************\n\n");
//    fflush(stdout);
//    throw std::bad_alloc();
//  }
//  ptMemoryError(ImgBuffer,__FILE__,__LINE__);
//  image.write(0, 0, w, h, "BGRA", Magick::CharPixel, ImgBuffer);

//  // Detour via QImage necessary because QPixmap does not allow direct
//  // access to the pixel data.
//  thumbImage->convertFromImage(QImage(ImgBuffer, w, h, QImage::Format_RGB32));
//  FREE(ImgBuffer);
}

//==============================================================================

void ptFileMgrThumbnailer::Abort() {
  if (isRunning()) {
    m_AbortRequested = true;

    while (isRunning()) {
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
}

//==============================================================================
