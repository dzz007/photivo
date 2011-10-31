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
#include "ptSingleDirModel.h"
#include "ptFileMgrConstants.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

//==============================================================================

ptSingleDirModel::ptSingleDirModel(QObject* parent /*= NULL*/)
: QStandardItemModel(parent)
{
  m_CurrentDir = new QDir;
  m_CurrentDir->setFilter(QDir::AllDirs | QDir::NoDot);
  m_CurrentDir->setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
#ifdef Q_OS_WIN
  m_CurrentDirType = fsoUnknown;
#endif
}

//==============================================================================

ptSingleDirModel::~ptSingleDirModel() {
  DelAndNull(m_CurrentDir);
}

//==============================================================================

void ptSingleDirModel::ChangeDir(const QModelIndex& index) {
#ifdef Q_OS_WIN
  QString path = m_EntryList.at(index.row());
  // first determine the type of the new filesystem location ...
  if (path == "..") {
    // change one level up in the hierarchy
    if (m_CurrentDirType == fsoDrive) {
      path.clear();
      m_CurrentDirType = fsoRoot;
    } else if (m_CurrentDirType == fsoDir) {
      path = QDir::cleanPath(m_CurrentDir->absolutePath() + "/..");
      if (path.length() == 3 && path.mid(1,1) == ":") {
        m_CurrentDirType = fsoDrive;
      } else {
        m_CurrentDirType = fsoDir;
      }
    }
  } else {
    // change one level deeper into the hierarchy
    if (m_CurrentDirType == fsoRoot) {
      m_CurrentDirType = fsoDrive;
      path = path.mid(path.length()-3, 2);  // extract drive letter, e.g. C:
    } else {
      m_CurrentDirType = fsoDir;
      path = m_CurrentDir->absolutePath() + "/" + path;
    }
  }

  // ... then prepare for model update accordingly
  if (m_CurrentDirType == fsoRoot) {
    FillListWithDrives();
  } else if (m_CurrentDirType == fsoDrive) {
    m_CurrentDir->setPath(path);
    m_EntryList = m_CurrentDir->entryList();
    m_EntryList.insert(0, "..");
  } else {
    m_CurrentDir->setPath(path);
    m_EntryList = m_CurrentDir->entryList();
  }

#else
  m_CurrentDir->setPath(m_CurrentDir->absolutePath() + "/" + m_EntryList.at(index.row()));
  m_EntryList = m_CurrentDir->entryList();
#endif

  UpdateModel();
}

//==============================================================================

void ptSingleDirModel::ChangeAbsoluteDir(const QString& path) {
#ifdef Q_OS_WIN
  if (path == MyComputerIniString) {
    m_CurrentDirType = fsoRoot;
    FillListWithDrives();
  } else if (path.length() == 2 && path.mid(1,1) == ":") {
    // we have a drive
    m_CurrentDirType = fsoDrive;
    m_CurrentDir->setPath(path);
    m_EntryList = m_CurrentDir->entryList();
  } else {
    m_CurrentDirType = fsoDir;
    m_CurrentDir->setPath(path);
    m_EntryList = m_CurrentDir->entryList();
  }

#else
  m_CurrentDir->setPath(path);
  m_EntryList = m_CurrentDir->entryList();
#endif

  UpdateModel();
}

//==============================================================================

void ptSingleDirModel::UpdateModel() {
  this->removeRows(0, this->rowCount());

#ifdef Q_OS_WIN
  if (m_CurrentDirType == fsoDrive) {
    appendRow(new QStandardItem(QIcon(":/dark/icons/go-up.png"), tr("My Computer")));
  }
#endif

  for (int i = 0; i < m_EntryList.count(); i++) {
    QStandardItem* item = new QStandardItem();

    if (m_EntryList.at(i) == "..") {
      item->setIcon(QIcon(":/dark/icons/go-up.png"));
#ifdef Q_OS_WIN
      if (m_CurrentDirType == fsoDrive) {
        item->setText(tr("My Computer"));
      } else {
        item->setText(QDir(QFileInfo(m_CurrentDir->absolutePath()).path()).dirName());
      }
#else
      item->setText(QDir(QFileInfo(m_CurrentDir->absolutePath()).path()).dirName());
#endif
    } else {
      item->setIcon(QIcon(":/dark/icons/folder.png"));
      item->setText(m_EntryList.at(i));
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    this->appendRow(item);
  }
}

//==============================================================================

QString ptSingleDirModel::absolutePath() {
#ifdef Q_OS_WIN
  if (m_CurrentDirType == fsoRoot) {
    return MyComputerIniString;
  }
#endif

  return m_CurrentDir->absolutePath();
}

//==============================================================================

ptFSOType ptSingleDirModel::pathType() const {
#ifdef Q_OS_WIN
  return m_CurrentDirType;
#else
  return fsoDir;
#endif
}

//==============================================================================

#ifdef Q_OS_WIN
void ptSingleDirModel::FillListWithDrives() {
  QFileInfoList list = QDir::drives();
  m_EntryList.clear();
  m_EntryList.append("..");
  for (int i = 0; i < list.count(); i++) {
    QString drive = list.at(i).canonicalFilePath();
    drive = QString("%1 (%2)").arg(WinApi::VolumeName(drive)).arg(drive);
    m_EntryList.append(drive);
  }
}
#endif

//==============================================================================
