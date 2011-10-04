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

#include "ptFileMgrWindow.h"
#include "../ptDefines.h"
#include "../ptSettings.h"

extern ptSettings* Settings;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget *parent)
: QWidget(parent),
  m_IsFirstShow(true)
{
  setupUi(this);

  // We create our data module
  m_DataModel = ptFileMgrDM::GetInstance();
  m_DataModel->setThumbSize(150);   //TODO: only temporarily hardcoded
  QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

  DirTree->setModel(fsmodel);
  DirTree->setRootIndex(fsmodel->index(fsmodel->rootPath()));
  DirTree->setColumnHidden(1, true);
  DirTree->setColumnHidden(2, true);
  DirTree->setColumnHidden(3, true);
  connect(DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  DirTree->setFixedWidth(300);  //TODO: temporary

  // Setup the graphics scene
  m_FilesScene = new QGraphicsScene(0, 0, 0, 0, parent);
  FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbsNotify()),
          this, SLOT(fetchNewThumbs()));

  m_StatusOverlay = new ptReportOverlay(FilesView, "Reading thumbnails",
                                        QColor(255,75,75), QColor(255,190,190),
                                        0, Qt::AlignLeft, 20);

}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      qobject_cast<QFileSystemModel*>(m_DataModel->treeModel())->filePath(DirTree->currentIndex()) );

  ptFileMgrDM::DestroyInstance();
  DelAndNull(m_StatusOverlay);
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  m_DataModel->StopThumbnailer();
  m_FilesScene->clear();
  m_DataModel->thumbQueue()->clear();
  m_StatusOverlay->exec();
  m_DataModel->StartThumbnailer(DirTree->currentIndex());
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

  //m_FilesScene->setSceneRect(0, 0, FilesView->width(), FilesView->height());
  int Row = -1;
  int Col = 0;
  int CellSize = m_DataModel->thumbSize() + 10;
  int PicsInCol = (FilesView->height() / CellSize) - 1;

  while (!m_DataModel->thumbQueue()->isEmpty()) {
    QGraphicsItemGroup* ig = m_DataModel->thumbQueue()->dequeue();

    if (Row >= PicsInCol) {
      Row = 0;
      Col++;
    } else {
      Row++;
    }

    ig->setPos(Col * CellSize, Row * CellSize);
    m_FilesScene->addItem(ig);
  }
}

//==============================================================================

void ptFileMgrWindow::showEvent(QShowEvent* event) {
  // When the file manager is opened for the first time set initally selected directory
  if (m_IsFirstShow) {
    QString lastDir = Settings->GetString("LastFileMgrLocation");
    QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

    if (lastDir != "" && QDir(lastDir).exists()) {
      DirTree->setCurrentIndex(fsmodel->index(lastDir));
    } else {
      DirTree->setCurrentIndex(fsmodel->index(QDir::homePath()));
    }

    m_IsFirstShow = false;
  }

  QWidget::showEvent(event);
}

//==============================================================================
