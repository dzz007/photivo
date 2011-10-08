/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include <QCursor>

#include "../ptDefines.h"
#include "../ptTheme.h"
#include "../ptSettings.h"
#include "ptGraphicsThumbGroup.h"
#include "ptGraphicsSceneEmitter.h"

extern ptSettings* Settings;
extern ptTheme* Theme;

//==============================================================================

ptGraphicsThumbGroup::ptGraphicsThumbGroup(QGraphicsItem* parent /*= 0*/)
: QGraphicsRectItem(parent)
{
  m_isDir = false;
  m_FullPath = "";
  m_Pixmap = NULL;
  m_InfoText = NULL;

  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);
  setFiltersChildEvents(true);
  setCursor(QCursor(Qt::PointingHandCursor));
  setPen(QPen(Qt::NoPen));
  setBrush(QBrush(Qt::NoBrush));
}

//==============================================================================

void ptGraphicsThumbGroup::addItems(QGraphicsPixmapItem* pixmap,
                                    const QString fullPath,
                                    const QString description,
                                    const bool isDir)
{
  if (m_Pixmap) DelAndNull(m_Pixmap);
  if (m_InfoText == NULL) m_InfoText = new QGraphicsTextItem;
  m_isDir = isDir;
  m_FullPath = fullPath;
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");

  m_Pixmap = pixmap;
  m_Pixmap->setParentItem(this);
  // center pixmap in the cell if it is not square
  // the +2 offset is for the hover border
  m_Pixmap->setPos(ThumbSize/2 - pixmap->pixmap().width()/2  + 2 + 0.5,
                   ThumbSize/2 - pixmap->pixmap().height()/2 + 2 + 0.5);

  m_InfoText->setPlainText(CutFileName(description));
  m_InfoText->setParentItem(this);
  m_InfoText->setPos(2, ThumbSize + 2);

  // set rectangle size for the hover border
  this->setRect(0,
                0,
                ThumbSize + 5,
                ThumbSize + m_InfoText->boundingRect().height() + 5);
}

//==============================================================================

bool ptGraphicsThumbGroup::sceneEvent(QEvent* event) {
  switch (event->type()) {
    case QEvent::GraphicsSceneHoverEnter: {
      event->accept();
      this->setPen(QPen(Theme->ptHighLight));
      return true;
    }

    case QEvent::GraphicsSceneHoverLeave: {
      event->accept();
      this->setPen(QPen(Qt::NoPen));
      return true;
    }

    case QEvent::GraphicsSceneMousePress: {
      // Must accept mouse press to get mouse release as well.
      event->accept();
      return true;
    }

  case QEvent::GraphicsSceneMouseRelease: {
      event->accept();
      if (m_InfoText) {
        if (m_isDir) {
          ptGraphicsSceneEmitter::EmitThumbnailAction(tnaChangeDir, m_FullPath);
        } else {
          ptGraphicsSceneEmitter::EmitThumbnailAction(tnaLoadImage, m_FullPath);
        }
      }
      return true;
    }

    default: {
      return QGraphicsRectItem::sceneEvent(event);
    }
  }
}

//==============================================================================

QString ptGraphicsThumbGroup::CutFileName(const QString FileName) {
  int SuffixStart = FileName.lastIndexOf(".");
  QString BaseName = FileName.left(SuffixStart);
  QString Suffix = SuffixStart == -1 ? "" : FileName.mid(SuffixStart);

  if (BaseName.size() > 16) {
    BaseName = QString("%1...").arg(BaseName.left(13));
  }

  return BaseName + Suffix;
}

//==============================================================================
