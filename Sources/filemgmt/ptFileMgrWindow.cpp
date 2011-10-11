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
  m_IsFirstShow(true),
  m_ThumbnailCount(-1)
{
  setupUi(this);
  setMouseTracking(true);
  ptGraphicsSceneEmitter::ConnectThumbnailAction(
      this, SLOT(execThumbnailAction(ptThumbnailAction,QString)) );

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
  m_FilesView->installEventFilter(this);

  // Setup the graphics scene
  m_FilesScene = new QGraphicsScene(m_FilesView);
  m_FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbsNotify()),
          this, SLOT(fetchNewThumbs()));

  m_Progressbar->hide();

//  m_StatusOverlay = new ptReportOverlay(m_FilesView, "Reading thumbnails",
//                                        QColor(255,75,75), QColor(255,190,190),
//                                        0, Qt::AlignLeft, 20);

//  QList <int> SizesList;
//  SizesList.append(250);
//  SizesList.append(1000); // Value obtained to avoid resizing at startup.
//  m_MainSplitter->setSizes(SizesList);
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      qobject_cast<QFileSystemModel*>(m_DataModel->treeModel())->filePath(m_DirTree->currentIndex()) );

  ptFileMgrDM::DestroyInstance();
  ptGraphicsSceneEmitter::DestroyInstance();
  DelAndNull(m_StatusOverlay);
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  m_PathInput->setText(
      QDir::toNativeSeparators(m_DataModel->treeModel()->filePath(m_DirTree->currentIndex())));

  m_DataModel->StopThumbnailer();
  m_FilesScene->clear();
  m_DataModel->thumbQueue()->clear();
  ThumbMetricsReset();
  m_ThumbnailCount = m_DataModel->setThumbnailDir(index);

  if (m_ThumbnailCount > -1) {
    m_Progressbar->setValue(0);
    m_Progressbar->setMaximum(m_ThumbnailCount);
    m_Progressbar->show();
    m_PathInput->hide();

    m_DataModel->StartThumbnailer();
  }
}

//==============================================================================

void ptFileMgrWindow::fetchNewThumbs() {
  while (!m_DataModel->thumbQueue()->isEmpty()) {
    m_Progressbar->setValue(m_Progressbar->value() + 1);
    ptGraphicsThumbGroup* thumb = m_DataModel->thumbQueue()->dequeue();
    ArrangeThumbnail(thumb);
    m_FilesScene->addItem(thumb);
  }

  if (m_Progressbar->value() >= m_ThumbnailCount) {
    m_Progressbar->hide();
    m_PathInput->show();
  }

  m_FilesScene->setSceneRect(m_FilesScene->itemsBoundingRect());
}

//==============================================================================

void ptFileMgrWindow::ArrangeThumbnail(ptGraphicsThumbGroup* thumb) {
  thumb->setPos(m_ThumbMetrics.Col * m_ThumbMetrics.CellWidth,
                m_ThumbMetrics.Row * m_ThumbMetrics.CellHeight);

  // arrange thumbs in rows
  if (m_ThumbMetrics.Col >= m_ThumbMetrics.MaxCol) {
    m_ThumbMetrics.Col = 0;
    m_ThumbMetrics.Row++;
  } else {
    m_ThumbMetrics.Col++;
  }
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
  m_ThumbMetrics.Row = 0;
  m_ThumbMetrics.Col = 0;

  // Padding between thumbnails
  m_ThumbMetrics.Padding = Settings->GetInt("ThumbnailPadding");

  // Width of a cell, i.e. thumb width + padding
  m_ThumbMetrics.CellWidth = Settings->GetInt("ThumbnailSize") + m_ThumbMetrics.Padding;

  // Height of a cell, i.e. thumb height + height of text line with the filename + padding
  m_ThumbMetrics.CellHeight =
      m_ThumbMetrics.CellWidth + QFontMetrics(this->font()).lineSpacing() + m_ThumbMetrics.Padding;

  // +Padding because we only take care of padding *between* thumbnails here.
  // -1 because we start at row 0
  m_ThumbMetrics.MaxRow = (m_FilesView->height() + m_ThumbMetrics.Padding)
                             / m_ThumbMetrics.CellHeight - 1;

  m_ThumbMetrics.MaxCol = (m_FilesView->width() + m_ThumbMetrics.Padding)
                             / m_ThumbMetrics.CellWidth - 1;
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
