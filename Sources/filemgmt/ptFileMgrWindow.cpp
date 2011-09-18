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

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "ptFileMgrWindow.h"

extern ptSettings* Settings;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget *parent): QWidget(parent) {
  setupUi(this);

  // We create our data module
  m_DataModel = ptFileMgrDM::GetInstance();

  DirTree->setModel(m_DataModel->treeModel());
  DirTree->setRootIndex(m_DataModel->treeModel()->index(m_DataModel->treeModel()->rootPath()));
  DirTree->setColumnHidden(1, true);
  DirTree->setColumnHidden(2, true);
  DirTree->setColumnHidden(3, true);
  connect(DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  // Setup the graphics scene
  m_FilesScene = new QGraphicsScene;
  FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbsNotify(bool)),
          this, SLOT(fetchNewThumbs(bool)));
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      qobject_cast<QFileSystemModel*>(m_DataModel->treeModel())->filePath(index) );

  ptFileMgrDM::DestroyInstance();
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  m_FilesScene->clear();
  m_DataModel->thumbQueue()->clear();
  m_DataModel->StartThumbnailer(DirTree->currentIndex());
}

//==============================================================================

void ptFileMgrWindow::fetchNewThumbs(const bool isCompleted) {
  if (m_DataModel->thumbQueue()->isEmpty()) {
    return;
  }

  for (int i = 0; i < 4; i++) {
    m_FilesScene->addItem(m_DataModel->thumbQueue()->dequeue());

    if (m_DataModel->thumbQueue()->isEmpty()) {
      break;
    }
  }
}

//==============================================================================
