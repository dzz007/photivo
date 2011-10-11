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

#include <QStringList>
#include <QGraphicsTextItem>
#include <QApplication>
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
  m_Cache = NULL;
  m_Queue = NULL;

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

void ptFileMgrThumbnailer::setQueue(QQueue<ptGraphicsThumbGroup*>* queue) {
  if (!this->isRunning() && queue != NULL) {
    m_Queue = queue;
  }
}

//==============================================================================

void ptFileMgrThumbnailer::run() {
  // Check for properly set directory, cache and buffer
  if (!m_Dir->exists() || m_Queue == NULL /*|| m_Cache == NULL*/) {
    return;
  }

  int thumbsSize = Settings->GetInt("ThumbnailSize");
  QFileInfoList files = m_Dir->entryInfoList();
  for (uint i = 0; i < (uint)files.count(); i++) {
    ptGraphicsThumbGroup* thumbGroup = new ptGraphicsThumbGroup;
    QGraphicsPixmapItem* thumbPixmap = new QGraphicsPixmapItem;

//    QTime Timer;
//    Timer.start();
    if (files.at(i).isDir()) {
      // we have a directory
      if (files.at(i).fileName() == "..") {
        thumbPixmap->setPixmap(QPixmap(QString::fromUtf8(":/photivo/FileManager/up.png")));
      } else {
        thumbPixmap->setPixmap(QPixmap(QString::fromUtf8(":/photivo/FileManager/folder.png")));
      }

    } else {
      // we have a file, see if we can get a thumbnail image
      ptDcRaw dcRaw;

      if (dcRaw.Identify(files.at(i).absoluteFilePath()) == 0 ) {
        // we have a raw image ...
        QByteArray* ba = NULL;
        if (dcRaw.thumbnail(ba)) {
          try {
//            printf("DcRaw: %d\n", Timer.elapsed());
            Magick::Blob  blob( ba->data(), ba->length());
            Magick::Image image;
            image.size(Magick::Geometry(2*thumbsSize, 2*thumbsSize));
            image.read(blob);

            GenerateThumbnail(image, thumbPixmap, thumbsSize);
//            printf("Thumbnail Raw: %d\n", Timer.elapsed());
          } catch (Magick::Exception &Error) {
            // ... not supported
            DelAndNull(thumbPixmap);
            DelAndNull(thumbGroup);
          }
        }
        DelAndNull(ba);
      } else {
        // ... or a bitmap ...
        try {
          Magick::Image image;
          image.size(Magick::Geometry(2*thumbsSize, 2*thumbsSize));
          image.read(files.at(i).absoluteFilePath().toAscii().data());

          GenerateThumbnail(image, thumbPixmap, thumbsSize);
//          printf("Thumbnail Bitmap: %d\n", Timer.elapsed());
        } catch (Magick::Exception &Error) {
          // ... or not a supported image file at all
          DelAndNull(thumbPixmap);
          DelAndNull(thumbGroup);
        }
      }
    }

    if (thumbGroup && thumbPixmap) {
      thumbGroup->addItems(thumbPixmap,
                           files.at(i).canonicalFilePath(),
                           files.at(i).fileName(),
                           files.at(i).isDir());
      m_Queue->enqueue(thumbGroup);
    }

    QApplication::processEvents();

//    printf("Done: %d\n\n", Timer.elapsed());

    // Notification signal that new thumbs are in the queue. Emitted every
    // five images. ptFileMgrWindow also reads in five image blocks.
//    if (i % 5 == 0) {
      emit newThumbsNotify();
//    }
  }

  // final notification to make sure the queue gets completely emptied
  emit newThumbsNotify();
}

//==============================================================================

void ptFileMgrThumbnailer::GenerateThumbnail(Magick::Image& image,
                                             QGraphicsPixmapItem*& thumbPixmap,
                                             const int thumbSize) {
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
  uint8_t* ImgBuffer = NULL;
  try {
    ImgBuffer = (uint8_t*)CALLOC(w * h * 4, sizeof(ImgBuffer));
  } catch (std::bad_alloc) {
    // TODO: Cleanup!
    printf("\n********************\n\nMemory error in thumbnail generator\n\n********************\n\n");
    fflush(stdout);
    throw std::bad_alloc();
  }
  ptMemoryError(ImgBuffer,__FILE__,__LINE__);
  image.write(0, 0, w, h, "BGRA", Magick::CharPixel, ImgBuffer);
  QPixmap px;
  // Detour via QImage necessary because QPixmap does not allow direct
  // access to the pixel data.
  px.convertFromImage(QImage(ImgBuffer, w, h, QImage::Format_RGB32));
  FREE(ImgBuffer);
  thumbPixmap->setPixmap(px);
}

//==============================================================================
