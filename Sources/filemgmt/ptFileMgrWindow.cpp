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

#include "../ptDefines.h"
#include "ptFileMgrWindow.h"

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget *parent): QWidget(parent) {
  setupUi(this);

  // We create our data module
  m_FileMgrDM = ptFileMgrDM::Instance_GoC();

  m_FSModel = new QFileSystemModel();
  m_FSModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
  m_FSModel->setRootPath(m_FSModel->myComputer().toString());

  DirTree->setModel(m_FSModel);
  DirTree->setRootIndex(m_FSModel->index(m_FSModel->rootPath()));
  DirTree->setColumnHidden(1, true);
  DirTree->setColumnHidden(2, true);
  DirTree->setColumnHidden(3, true);
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  DelAndNull(m_FSModel);

  ptFileMgrDM::Instance_Destroy();
}

//==============================================================================
