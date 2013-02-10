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

extern ptSettings* Settings;
extern ptTheme* Theme;

//==============================================================================

ptGraphicsThumbGroup::ptGraphicsThumbGroup(QGraphicsItem      *AParent       /*= nullptr*/,
                                           ptThumbGroupEvents *AEventHandler /*= nullptr*/)
: QGraphicsRectItem(AParent),
  FEventHandler(AEventHandler),
  m_Brush(Qt::SolidPattern),
  m_hasHover(false),
  m_Pen(Qt::DashLine),
  FImage(),
  FHaveImage(false),
  FIndex(-1)
{
  m_FSOType = fsoUnknown;
  m_FullPath = "";
  m_ImgTypeText = NULL;
  m_InfoText = NULL;
  m_Pixmap = NULL;

  setFlags(QGraphicsItem::ItemIsFocusable);
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);
  setFiltersChildEvents(true);
  setCursor(QCursor(Qt::PointingHandCursor));

  m_Pen.setCosmetic(true);
  SetupPenAndBrush();
  m_ThumbSize = (qreal)Settings->GetInt("FileMgrThumbnailSize");
}

//==============================================================================

ptGraphicsThumbGroup::~ptGraphicsThumbGroup() {
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
  qreal ThumbSize = (qreal)Settings->GetInt("FileMgrThumbnailSize");

  // main description text: currently just the filename
  if (m_InfoText == NULL) {
    m_InfoText = new QGraphicsSimpleTextItem;
    m_InfoText->setParentItem(this);
  }
  m_InfoText->setText(QFontMetrics(m_InfoText->font()).elidedText(description,
                                                                  Qt::ElideRight,
                                                                  (int)ThumbSize));
  m_InfoText->setBrush(QBrush(Theme->textColor()));
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
      m_ImgTypeText->setBrush(QBrush(Theme->textColor()));
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

  if (m_FSOType == fsoParentDir || m_FSOType == fsoDir) {
    std::shared_ptr<ptImage8> thumbImage = std::make_shared<ptImage8>();
    if (m_FSOType == fsoParentDir) {
      // we have a parent directory (dirs are not cached!)
      thumbImage->FromQImage(QImage(QString::fromUtf8(":/dark/icons/go-up-48px.png")));
    } else if (m_FSOType == fsoDir) {
      // we have a subdirectory (dirs are not cached!)
      thumbImage->FromQImage(QImage(QString::fromUtf8(":/dark/icons/folder-48px.png")));
    }
    addImage(thumbImage);
  }
}

//==============================================================================

void ptGraphicsThumbGroup::addImage(ptThumbPtr AImage) {
  if (!AImage) {
    return;
  }

  FImageData.Set(AImage.get());
  FImage     = QImage((const uchar*) FImageData.m_Image,
                      FImageData.m_Width,
                      FImageData.m_Height,
                      QImage::Format_ARGB32);

  FHaveImage = !FImage.isNull();

  if (!FHaveImage) return;

  // center pixmap in the cell if it is not square
  // the +2 offset is for the hover border
  m_ThumbPos.setX(m_ThumbSize/2 - FImage.width()/2  + InnerPadding + 0.5);
  m_ThumbPos.setY(m_ThumbSize/2 - FImage.height()/2 + InnerPadding + 0.5);

  this->update();
}

//==============================================================================

bool ptGraphicsThumbGroup::sceneEvent(QEvent* event) {
  switch (event->type()) {
    case QEvent::GraphicsSceneHoverEnter: {
      m_hasHover = true;
      this->update();
      return true;
    }

    case QEvent::GraphicsSceneHoverLeave: {
      m_hasHover = false;
      this->update();
      return true;
    }

    case QEvent::GraphicsSceneMouseDoubleClick: {
      exec();
      return true;
    }

    case QEvent::GraphicsSceneMousePress: {
      // Must accept mouse press to get mouse release as well.
      event->accept();
      return true;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      // set focus, FM window takes care of showing image in the viewer if necessary
      setFocus(Qt::MouseFocusReason);
      if (FEventHandler) {
        if (m_FSOType == fsoFile) {
          FEventHandler->thumbnailAction(tnaViewImage, m_FullPath);
          FEventHandler->currentThumbnail(m_FullPath);
        }
        FEventHandler->focusChanged();
      }
      return true;
    }

    case QEvent::KeyPress: {
      QKeyEvent* e = (QKeyEvent*)event;
      if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        exec();
        return true;
      }
      break;
    }

    case QEvent::GraphicsSceneContextMenu: {
      // We set the focus but we don't accept the event
      setFocus(Qt::MouseFocusReason);
      if (FEventHandler) {
        if (m_FSOType == fsoFile) {
          FEventHandler->currentThumbnail(m_FullPath);
        }
        FEventHandler->focusChanged();
      }
    }

    default:
      break;
  }

  return QGraphicsRectItem::sceneEvent(event);
}

//==============================================================================

void ptGraphicsThumbGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
  SetupPenAndBrush();
  painter->setPen(m_Pen);
  painter->setBrush(m_Brush);
  painter->drawRoundedRect(this->rect(), 5, 5);
  if (FHaveImage) {
    painter->drawImage(m_ThumbPos.x(), m_ThumbPos.y(), FImage);
  }
}

//==============================================================================

QFont ptGraphicsThumbGroup::font() const {
  if (m_InfoText == NULL) {
    return QApplication::font();
  } else {
    return m_InfoText->font();
  }
}

//==============================================================================

void ptGraphicsThumbGroup::exec() {
  if (m_InfoText && FEventHandler) {
    if (m_FSOType == fsoFile) {
      FEventHandler->thumbnailAction(tnaLoadImage, m_FullPath);
    } else {
      FEventHandler->thumbnailAction(tnaChangeDir, m_FullPath);
    }
  }
}

//==============================================================================

void ptGraphicsThumbGroup::SetupPenAndBrush() {
  int PenWidth = m_hasHover ? 2 : 1;

  if (this->hasFocus()) {
    m_Pen.setColor(Theme->highlightColor());
    m_Pen.setWidth(PenWidth);
    m_Brush.setColor(Theme->gradientColor());

  } else {
    m_Pen.setColor(Theme->emphasizedColor());
    m_Pen.setWidth(PenWidth);
    m_Brush.setColor(Theme->altBaseColor());
  }
}

//==============================================================================
