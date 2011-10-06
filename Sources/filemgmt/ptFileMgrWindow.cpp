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

#include <QFileSystemModel>
#include <QFontMetrics>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "ptFileMgrWindow.h"

extern ptSettings* Settings;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget *parent)
: QWidget(parent),
  m_IsFirstShow(true)
{
  setupUi(this);
  setMouseTracking(true);

  // We create our data module
  m_DataModel = ptFileMgrDM::GetInstance();
  QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

  m_DirTree->setModel(fsmodel);
  m_DirTree->setRootIndex(fsmodel->index(fsmodel->rootPath()));
  m_DirTree->setColumnHidden(1, true);
  m_DirTree->setColumnHidden(2, true);
  m_DirTree->setColumnHidden(3, true);
  connect(m_DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  m_DirTree->setFixedWidth(300);  //TODO: temporary
  m_FilesView->setContentsMargins(10, 10, 10, 10);
  m_FilesView->installEventFilter(this);

  // Setup the graphics scene
  m_FilesScene = new QGraphicsScene(0, 0, 0, 0, m_FilesView);
  m_FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbsNotify()),
          this, SLOT(fetchNewThumbs()));

  m_StatusOverlay = new ptReportOverlay(m_FilesView, "Reading thumbnails",
                                        QColor(255,75,75), QColor(255,190,190),
                                        0, Qt::AlignLeft, 20);
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      qobject_cast<QFileSystemModel*>(m_DataModel->treeModel())->filePath(m_DirTree->currentIndex()) );

  ptFileMgrDM::DestroyInstance();
  DelAndNull(m_StatusOverlay);
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  m_DataModel->StopThumbnailer();
  m_FilesScene->clear();
  m_DataModel->thumbQueue()->clear();
  m_StatusOverlay->exec();
  ThumbMetricsReset();
  m_DataModel->StartThumbnailer(m_DirTree->currentIndex());
}

//==============================================================================

void ptFileMgrWindow::fetchNewThumbs() {
  m_StatusOverlay->stop();
  if (m_DataModel->thumbQueue()->isEmpty()) {
    return;
  }

//  for (int i = 0; i < 4; i++) {
//    m_FilesScene->addItem(m_DataModel->thumbQueue()->dequeue());

//    if (m_DataModel->thumbQueue()->isEmpty()) {
//      break;
//    }
//  }

  while (!m_DataModel->thumbQueue()->isEmpty()) {
    ptGraphicsThumbGroup* thumb = m_DataModel->thumbQueue()->dequeue();
    ArrangeThumbnail(thumb);
    m_FilesScene->addItem(thumb);
  }
}

//==============================================================================

void ptFileMgrWindow::ArrangeThumbnail(ptGraphicsThumbGroup* thumb) {
  if (m_ThumbMetrics.Row >= m_ThumbMetrics.PicsInCol) {
    m_ThumbMetrics.Row = 0;
    m_ThumbMetrics.Col++;
  } else {
    m_ThumbMetrics.Row++;
  }

  thumb->setPos(m_ThumbMetrics.Col * m_ThumbMetrics.CellHeight,
                m_ThumbMetrics.Row * m_ThumbMetrics.CellHeight);
}

//==============================================================================

void ptFileMgrWindow::ArrangeThumbnails() {
  QList<QGraphicsItem*> thumbList = m_FilesScene->items(Qt::AscendingOrder);
  QListIterator<QGraphicsItem*> i(thumbList);
  ThumbMetricsReset();

  while (i.hasNext()) {
    if (i.peekNext()->type() == ptGraphicsThumbGroup::Type) {   // check for thumb item group
      ArrangeThumbnail((ptGraphicsThumbGroup*)i.next());
    } else {
      i.next();
    }
  }
}

//==============================================================================

void ptFileMgrWindow::ThumbMetricsReset() {
  // Current row and column
  m_ThumbMetrics.Row = -1;
  m_ThumbMetrics.Col = 0;

  // Padding between thumbnails
  m_ThumbMetrics.Padding = 16;   // TODO: maybe this doesn’t have to be a fixed num of pixels

  // Width of a cell, i.e. thumb width + padding
  m_ThumbMetrics.CellWidth = Settings->GetInt("ThumbnailSize") + m_ThumbMetrics.Padding;

  // Height of a cell, i.e. thumb height + height of text line with the filename + padding
  m_ThumbMetrics.CellHeight =
      m_ThumbMetrics.CellWidth + QFontMetrics(this->font()).lineSpacing();

  // +Padding because we only take care of padding *between* thumbnails here.
  // -1 because we start at row 0
  m_ThumbMetrics.PicsInCol = (m_FilesView->height() + m_ThumbMetrics.Padding)
                             / m_ThumbMetrics.CellHeight - 1;
}

//==============================================================================

void ptFileMgrWindow::showEvent(QShowEvent* event) {
  // When the file manager is opened for the first time set initally selected directory
  if (m_IsFirstShow) {
    QString lastDir = Settings->GetString("LastFileMgrLocation");
    QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

    if (lastDir != "" && QDir(lastDir).exists()) {
      m_DirTree->setCurrentIndex(fsmodel->index(lastDir));
    } else {
      m_DirTree->setCurrentIndex(fsmodel->index(QDir::homePath()));
    }

    m_IsFirstShow = false;
  }

  QWidget::showEvent(event);
}

//==============================================================================

bool ptFileMgrWindow::eventFilter(QObject* obj, QEvent* event) {
  // Rearrange thumbnails when size of viewport changes
  if (obj == m_FilesView && event->type() == QEvent::Resize) {
    ArrangeThumbnails();
    return true;
  } else {
    return QWidget::eventFilter(obj, event);
  }
}

//==============================================================================
