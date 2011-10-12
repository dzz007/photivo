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
  m_FSOType = fsoUnknown;
  m_FullPath = "";
  m_Pixmap = NULL;
  m_InfoText = NULL;

  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);
  setFiltersChildEvents(true);
  setCursor(QCursor(Qt::PointingHandCursor));
  setPen(QPen(Theme->ptBright, 0, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
}

//==============================================================================

void ptGraphicsThumbGroup::addInfoItems(const QString fullPath,
                                        const QString description,
                                        const ptFSOType fsoType)
{
  m_FSOType = fsoType;
  m_FullPath = fullPath;
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");

  if (m_InfoText == NULL) m_InfoText = new QGraphicsTextItem;
  m_InfoText->setPlainText(CutFileName(description));
  m_InfoText->setParentItem(this);
  m_InfoText->setPos(InnerPadding, ThumbSize + InnerPadding*2);

  // set rectangle size for the hover border
  this->setRect(0,
                0,
                ThumbSize + InnerPadding*2,
                ThumbSize + InnerPadding*2 + m_InfoText->boundingRect().height());
}

//==============================================================================

void ptGraphicsThumbGroup::addPixmap(QGraphicsPixmapItem* pixmap) {
  if (m_Pixmap) DelAndNull(m_Pixmap);
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");
  m_Pixmap = pixmap;
  m_Pixmap->setParentItem(this);
  // center pixmap in the cell if it is not square
  // the +2 offset is for the hover border
  m_Pixmap->setPos(ThumbSize/2 - pixmap->pixmap().width()/2  + InnerPadding + 0.5,
                   ThumbSize/2 - pixmap->pixmap().height()/2 + InnerPadding + 0.5);
}

//==============================================================================

bool ptGraphicsThumbGroup::sceneEvent(QEvent* event) {
  switch (event->type()) {
    case QEvent::GraphicsSceneHoverEnter: {
      event->accept();
      this->setPen(QPen(Theme->ptHighLight, 0, Qt::DashLine));
      return true;
    }

    case QEvent::GraphicsSceneHoverLeave: {
      event->accept();
      setPen(QPen(Theme->ptBright, 0, Qt::DashLine));
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
        if (m_FSOType == fsoFile) {
          ptGraphicsSceneEmitter::EmitThumbnailAction(tnaLoadImage, m_FullPath);
        } else {
          ptGraphicsSceneEmitter::EmitThumbnailAction(tnaChangeDir, m_FullPath);
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

void ptGraphicsThumbGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
  painter->setPen(this->pen());
  painter->setBrush(QBrush(Theme->ptDark));
  painter->drawRoundedRect(this->rect(), 5, 5);
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
