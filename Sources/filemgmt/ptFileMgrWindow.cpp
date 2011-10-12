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

#include <QFileSystemModel>
#include <QFontMetrics>
#include <QList>
#include <QDir>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "ptFileMgrWindow.h"
#include "ptGraphicsSceneEmitter.h"

extern ptSettings* Settings;
extern ptTheme* Theme;
extern void CB_MenuFileOpen(const short HaveFile);
extern QString ImageFileToOpen;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget *parent)
: QWidget(parent),
  m_ArrangeMode(tamVerticalByRow),
  m_IsFirstShow(true),
  m_ThumbCount(-1),
  m_ThumbListIdx(0)
{
  setupUi(this);
  setMouseTracking(true);
  ptGraphicsSceneEmitter::ConnectThumbnailAction(
      this, SLOT(execThumbnailAction(ptThumbnailAction,QString)) );

  // We setup our data module
  m_DataModel = ptFileMgrDM::GetInstance();

  m_DirTree->setModel(m_DataModel->treeModel());
  m_DirTree->setRootIndex(m_DataModel->treeModel()->index(m_DataModel->treeModel()->rootPath()));
  m_DirTree->setColumnHidden(1, true);
  m_DirTree->setColumnHidden(2, true);
  m_DirTree->setColumnHidden(3, true);
  connect(m_DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  // Setup the graphics view/scene
  m_FilesScene = new QGraphicsScene(m_FilesView);
  m_FilesView->installEventFilter(this);
  m_FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbsNotify(const bool)),
          this, SLOT(fetchNewThumbs(const bool)));
  connect(m_DataModel->thumbnailer(), SIGNAL(newPixmapsNotify()),
          this, SLOT(fetchNewPixmaps()));

  m_Progressbar->hide();

//  m_StatusOverlay = new ptReportOverlay(m_FilesView, "Reading thumbnails",
//                                        QColor(255,75,75), QColor(255,190,190),
//                                        0, Qt::AlignLeft, 20);

  QList <int> SizesList;
  SizesList.append(250);
  SizesList.append(1000); // Value obtained to avoid resizing at startup.
  m_MainSplitter->setSizes(SizesList);
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      m_DataModel->treeModel()->filePath(m_DirTree->currentIndex()) );

  DelAndNull(m_StatusOverlay);

  // To avoid dangling pointers destroy singletons last.
  ptFileMgrDM::DestroyInstance();
  ptGraphicsSceneEmitter::DestroyInstance();
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  m_PathInput->setText(
      QDir::toNativeSeparators(m_DataModel->treeModel()->filePath(m_DirTree->currentIndex())));

  m_DataModel->StopThumbnailer();
  m_FilesScene->clear();
  m_DataModel->thumbList()->clear();
  m_ThumbListIdx = 0;
  m_ThumbCount = m_DataModel->setThumbnailDir(index);
  CalcThumbMetrics();

  if (m_ThumbCount > -1) {
    m_Progressbar->setValue(0);
    m_Progressbar->setMaximum(m_ThumbCount);
    m_DataModel->StartThumbnailer();
  }
}

//==============================================================================

void ptFileMgrWindow::fetchNewThumbs(const bool isLast) {
  while (m_ThumbListIdx < m_DataModel->thumbList()->count()) {
    ptGraphicsThumbGroup* thumb = m_DataModel->thumbList()->at(m_ThumbListIdx);
    m_ThumbListIdx++;
    ArrangeThumbnail(thumb);
    m_FilesScene->addItem(thumb);
  }

  if (isLast) {
    m_ThumbListIdx = 0;
    m_Progressbar->show();
    m_PathInput->hide();
  }

  m_FilesScene->setSceneRect(m_FilesScene->itemsBoundingRect());
}

//==============================================================================

void ptFileMgrWindow::fetchNewPixmaps() {
  // We don’t actually need to fetch anything. Adding the QPixmap to the thumb group
  // automatically updates the viewport. So only the progress bar is left.
  m_Progressbar->setValue(m_Progressbar->value() + 1);

  if (m_Progressbar->value() >= m_ThumbCount) {
    m_Progressbar->hide();
    m_PathInput->show();
  }
}

//==============================================================================

void ptFileMgrWindow::ArrangeThumbnail(ptGraphicsThumbGroup* thumb) {
  thumb->setPos(m_ThumbMetrics.Col * m_ThumbMetrics.CellWidth,
                m_ThumbMetrics.Row * m_ThumbMetrics.CellHeight);

  switch (m_ArrangeMode) {
    case tamVerticalByRow:
      if (m_ThumbMetrics.Col >= m_ThumbMetrics.MaxCol) {
        m_ThumbMetrics.Col = 0;
        m_ThumbMetrics.Row++;
      } else {
        m_ThumbMetrics.Col++;
      }
      break;

    case tamHorizontalByColumn:
      if (m_ThumbMetrics.Row >= m_ThumbMetrics.MaxRow) {
        m_ThumbMetrics.Row = 0;
        m_ThumbMetrics.Col++;
      } else {
        m_ThumbMetrics.Row++;
      }
      break;

    default:
      assert(!"Unhandled ptArrangeMode");
      break;
  }
}

//==============================================================================

void ptFileMgrWindow::ArrangeThumbnails() {
  QList<QGraphicsItem*> thumbList = m_FilesScene->items(Qt::AscendingOrder);
  QListIterator<QGraphicsItem*> i(thumbList);
  CalcThumbMetrics();

  while (i.hasNext()) {
    if (i.peekNext()->type() == ptGraphicsThumbGroup::Type) {   // check for thumb item group
      ArrangeThumbnail((ptGraphicsThumbGroup*)i.next());
    } else {
      i.next();
    }
  }
}

//==============================================================================

void ptFileMgrWindow::CalcThumbMetrics() {
  // Current row and column
  m_ThumbMetrics.Row = 0;
  m_ThumbMetrics.Col = 0;

  // Padding between thumbnails
  m_ThumbMetrics.Padding = Settings->GetInt("ThumbnailPadding");

  // Width of a cell, i.e. thumb width + padding
  m_ThumbMetrics.CellWidth = Settings->GetInt("ThumbnailSize") +
                             m_ThumbMetrics.Padding +
                             ptGraphicsThumbGroup::InnerPadding*2;

  // Height of a cell, i.e. thumb height + height of text line with the filename + padding
  m_ThumbMetrics.CellHeight = m_ThumbMetrics.CellWidth +
                              QFontMetrics(this->font()).lineSpacing() +
                              m_ThumbMetrics.Padding*2;


  switch (m_ArrangeMode) {
    case tamVerticalByRow: {
      // +Padding because we only take care of padding *between* thumbnails here.
      // -1 because we start at row 0
      // This calculation does not take scrollbar width/height into account
      m_ThumbMetrics.MaxCol =
          qMin(Settings->GetInt("FileMgrThumbMaxRowCol"),
               (m_FilesView->width() + m_ThumbMetrics.Padding) / m_ThumbMetrics.CellWidth - 1);

      // Test if enough thumbs for scrollbar to appear. If yes check if enough empty space
      // for scrollbar and if not, put one thumb less into row/column.
      if((m_FilesView->height() - m_ThumbCount*m_ThumbMetrics.MaxCol) <
         (m_FilesView->horizontalScrollBar()->height() + m_ThumbMetrics.Padding))
      {
        int cols = (int)((float)m_ThumbCount / (float)m_ThumbMetrics.MaxCol + 0.5);
        if (cols > m_ThumbMetrics.MaxCol) {
          m_ThumbMetrics.MaxCol--;
        }
      }
      break;
    }

    case tamHorizontalByColumn: {
      m_ThumbMetrics.MaxRow =
          qMin(Settings->GetInt("FileMgrThumbMaxRowCol"),
               (m_FilesView->height() + m_ThumbMetrics.Padding) / m_ThumbMetrics.CellHeight - 1);

      if ((m_FilesView->width() - m_ThumbCount*m_ThumbMetrics.MaxRow) <
          (m_FilesView->verticalScrollBar()->width() + m_ThumbMetrics.Padding))
      {
        int rows = (int)((float)m_ThumbCount / (float)m_ThumbMetrics.MaxRow + 0.5);
        if (rows > m_ThumbMetrics.MaxRow) {
          m_ThumbMetrics.MaxRow--;
        }
      }
      break;
    }

    default:
      assert(!"Unhandled ptArrangeMode");
      break;
  }
}

//==============================================================================

void ptFileMgrWindow::showEvent(QShowEvent* event) {
  // When the file manager is opened for the first time set initally selected directory
  if (m_IsFirstShow) {
    setStyle(Theme->ptStyle);
    setStyleSheet(Theme->ptStyleSheet);
    m_FileListLayout->setContentsMargins(10,10,10,10);
    m_FileListLayout->setSpacing(5);

    m_Progressbar->setFixedHeight(m_PathInput->height());

    QString lastDir = Settings->GetString("LastFileMgrLocation");
    QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

    if (lastDir != "" && QDir(lastDir).exists()) {
      m_DirTree->setCurrentIndex(fsmodel->index(lastDir));
    } else {
      m_DirTree->setCurrentIndex(fsmodel->index(QDir::homePath()));
    }

    m_PathInput->setText(
        QDir::toNativeSeparators(m_DataModel->treeModel()->filePath(m_DirTree->currentIndex())));
    m_IsFirstShow = false;
  }

  QWidget::showEvent(event);
}

//==============================================================================

bool ptFileMgrWindow::eventFilter(QObject* obj, QEvent* event) {
  // Resize event
  // Rearrange thumbnails when size of viewport changes
  if (obj == m_FilesView && event->type() == QEvent::Resize) {
    ArrangeThumbnails();
    return false;
  } else {
    return QWidget::eventFilter(obj, event);
  }
}

//==============================================================================

void ptFileMgrWindow::execThumbnailAction(const ptThumbnailAction action, const QString location) {
  if (action == tnaLoadImage) {
    emit FileMgrWindowClosed();
    m_FilesScene->clear();
    ImageFileToOpen = location;
    CB_MenuFileOpen(1);

  } else if (action == tnaChangeDir) {
    m_DirTree->setCurrentIndex(
        qobject_cast<QFileSystemModel*>(m_DataModel->treeModel())->index(location) );
    changeTreeDir(m_DirTree->currentIndex());
  }
}

//==============================================================================
