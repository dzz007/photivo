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

#include "../ptError.h"
#include "../ptCalloc.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptImage8.h"
#include "ptFileMgrConstants.h"
#include "ptThumbnailer.h"
#include "ptFileMgrDM.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

extern ptSettings* Settings;
extern QStringList FileExtsRaw;
extern QStringList FileExtsBitmap;

//==============================================================================

ptThumbnailer::ptThumbnailer()
: QThread()
{
  m_AbortRequested = false;
  m_Cache          = NULL;
  m_ThumbList      = NULL;


}

//==============================================================================

ptThumbnailer::~ptThumbnailer() {
}

//==============================================================================

void ptThumbnailer::setCache(ptThumbnailCache* cache) {
  if (!this->isRunning() && cache != NULL) {
    m_Cache = cache;
  }
}

//==============================================================================

void ptThumbnailer::setThumbList(QList<ptGraphicsThumbGroup*>* ThumbList) {
  if (!this->isRunning() && ThumbList != NULL) {
    m_ThumbList = ThumbList;
  }
}

//==============================================================================


void ptThumbnailer::run() {

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
