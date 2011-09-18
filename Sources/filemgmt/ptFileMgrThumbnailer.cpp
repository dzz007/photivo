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

#include <QDir>
#include <QStringList>

#include <Magick++.h>

#include "../ptDcRaw.h"
#include "../ptError.h"
#include "ptFileMgrThumbnailer.h"

extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//==============================================================================

ptFileMgrThumbnailer::ptFileMgrThumbnailer()
: QThread(),
  m_Dir(""),
  m_Queue(NULL)
{}

//==============================================================================

void ptFileMgrThumbnailer::setDir(const QString dir) {
  if (!this->isRunning()) {
    m_Dir = dir;
  }
}

//==============================================================================

void ptFileMgrThumbnailer::setQueue(QQueue<QGraphicsPixmapItem>* queue) {
  if (!this->isRunning() && queue != NULL) {
    m_Queue = queue;
  }
}

//==============================================================================

void ptFileMgrThumbnailer::run() {
  QDir thumbsDir = QDir(m_Dir);

  // Check for properly set directory and buffer
  if (!thumbsDir.exists() || m_Queue == NULL) {
    return;
  }

  thumbsDir.setSorting(QDir::DirsFirst | QDir::Name);
  thumbsDir.setFilter(QDir::AllDirs | QDir::NoDot | QDir::Files);
  thumbsDir.setNameFilters(FileExtsRaw + FileExtsBitmap);
  QFileInfoList files = thumbsDir.entryInfoList();

  ptDcRaw dcRaw;
  for (uint i = 0; i < files.count(); i++) {
    QGraphicsPixmapItem* thumbItem = new QGraphicsPixmapItem;

    // we have a raw image ...
    if(dcRaw.Identify(files.at(i).absoluteFilePath()) == 0 ) {
      QPixmap* thumb = dcRaw.thumbnail();
      thumbItem->setPixmap(thumb->scaled(150,150));
      DelAndNull(thumb);

    // ... or a bitmap ...
    } else {
      try {
        Magick::Image image(files.at(i).absoluteFilePath().toAscii().data());

        // We want 8bit RGB data without alpha channel, scaled to thumbnail size
        image.type(Magick::TrueColorType);
        image.magick("RGB");
        image.depth(8);
        image.zoom(Magick::Geometry(150,150));

        // Get the raw image data from GM.
        uint w = image.columns();
        uint h = image.rows();
        uchar (*ImgBuffer)[3] =
            (uchar (*)[3]) CALLOC(w*h, sizeof(*ImgBuffer));
        ptMemoryError(ImgBuffer,__FILE__,__LINE__);
        image.write(0, 0, w, h, "RGB", Magick::CharPixel, ImgBuffer);
        QPixmap thumb;
        // Detour via QImage necessary because QPixmap does not allow direct
        // access to the pixel data.
        thumb.convertFromImage(QImage(ImgBuffer, w, h, QImage::Format_RGB888));
        FREE(ImgBuffer);
        thumbItem->setPixmap(thumb);

      // ... or not a supported image file at all
      } catch (Magick::Exception &Error) {}
    }

    m_Queue->enqueue(thumbItem);

    // notification signal that new thumbs are in the queue
    if (i % 5 == 0) {
      emit newThumbsNotify(false);
    }
  }

  emit newThumbsNotify(true);
}
