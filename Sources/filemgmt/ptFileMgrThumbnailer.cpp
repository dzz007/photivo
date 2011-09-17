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

void ptFileMgrThumbnailer::setQueue(QQueue<QGraphicsItem>* queue) {
  if (!this->isRunning() && queue != NULL) {
    m_Queue = queue;
  }
}

//==============================================================================

ptFileMgrThumbnailer::run() {
  QDir thumbsDir = QDir(m_Dir);

  // Check for properly set directory and buffer
  if (!thumbsDir.exists() || m_Queue == NULL) {
    return;
  }

  thumbsDir.setSorting(QDir::DirsFirst | QDir::Name);
  thumbsDir.setFilter(QDir::AllDirs | QDir::NoDot | QDir::Files);
  thumbsDir.setNameFilters(FileExtsRaw + FileExtsBitmap);
  QFileInfoList files = QDir::entryInfoList();

  ptDcRaw dcRaw;
  for (uint i = 0; i < files.count(); i++) {
    QGraphicsPixmapItem* thumbItem = new QGraphicsPixmapItem;
    QPixmap thumb = NULL;

    if(dcRaw.Identify(files.at(i).absoluteFilePath()) == 0 ) {    // we have a raw image
      thumb = *dcRaw.thumbnail();

    } else {    // or we might have a bitmap
      try {
        Magick::Image image;
        image.ping(files.at(i).absoluteFilePath().toAscii().data());
        // TODO: load bitmap

      } catch (Magick::Exception &Error) {}   // no supported image file
    }

    // scale to thumbnail size and update graphicsitem
    if (thumb) {
      thumb = thumb.scaled(150, 150);
      thumbItem->setPixmap(thumb);
    }

    // notification signal that new thumbs are in the queue
    if (i % 5 == 0) {
      emit newThumbsNotify(false);
    }
  }

  emit newThumbsNotify(true);
}
