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
#include <QFontMetrics>

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
  m_ImgTypeText = NULL;
  m_InfoText = NULL;

  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);
  setFiltersChildEvents(true);
  setCursor(QCursor(Qt::PointingHandCursor));
  setPen(QPen(Theme->ptBright, 0, Qt::DashLine));
}

//==============================================================================

void ptGraphicsThumbGroup::addInfoItems(const QString fullPath,
                                        const QString description,
                                        const ptFSOType fsoType)
{
// A thumbnail’s geometry:
// Height:                                  Width:
//   InnerPadding                             InnerPadding
//   m_Pixmap->pixmap().height()              m_Pixmap->pixmap().width()
//   InnerPadding                             InnerPadding
//   m_InfoText->boundingRect().height()
//   InnerPadding
//
  m_FSOType = fsoType;
  m_FullPath = fullPath;
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");

  // main description text: currently just the filename
  if (m_InfoText == NULL) {
    m_InfoText = new QGraphicsSimpleTextItem;
    m_InfoText->setParentItem(this);
  }
  m_InfoText->setText(QFontMetrics(m_InfoText->font()).elidedText(description,
                                                                  Qt::ElideRight,
                                                                  (int)ThumbSize));
  m_InfoText->setBrush(QBrush(Theme->ptText));
  m_InfoText->setPos(InnerPadding, ThumbSize + InnerPadding*2);

  // file type display in topleft corner (images only)
  if (fsoType == fsoFile) {
    int SuffixStart = fullPath.lastIndexOf(".");
    QString Suffix = SuffixStart == -1 ? "" : fullPath.mid(SuffixStart + 1);

    if (m_ImgTypeText == NULL) {
      m_ImgTypeText = new QGraphicsSimpleTextItem;
      QFont tempFont = m_ImgTypeText->font();
      tempFont.setBold(true);
      m_ImgTypeText->setFont(tempFont);
      m_ImgTypeText->setBrush(QBrush(Theme->ptText));
      m_ImgTypeText->setPos(InnerPadding, InnerPadding);
      m_ImgTypeText->setParentItem(this);
    }

    m_ImgTypeText->setText(Suffix.toUpper());
  }

  // set rectangle size for the hover border
  this->setRect(0,
                0,
                ThumbSize + InnerPadding*2,
                ThumbSize + InnerPadding*3 + m_InfoText->boundingRect().height());
}

//==============================================================================

void ptGraphicsThumbGroup::addImage(QImage* image) {
  qreal ThumbSize = (qreal)Settings->GetInt("ThumbnailSize");
  if (!m_Pixmap) m_Pixmap = new QGraphicsPixmapItem;
  m_Pixmap->setPixmap(QPixmap::fromImage(*image));

  // center pixmap in the cell if it is not square
  // the +2 offset is for the hover border
  m_Pixmap->setPos(ThumbSize/2 - image->width()/2  + InnerPadding + 0.5,
                   ThumbSize/2 - image->height()/2 + InnerPadding + 0.5);

  m_Pixmap->setParentItem(this);
  DelAndNull(image);
#ifdef DEBUG
  printf("%s: added image to thumb group for %s\n", __FILE__, m_FullPath.toAscii().data());
#endif
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
