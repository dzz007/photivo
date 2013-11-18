/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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

#include "ptFSMProxy.h"
#include "../ptDefines.h"
#include "../ptTheme.h"
#include <QFileSystemModel>
#include <QDirIterator>
#include <QUrl>
#include "../ptMessageBox.h"

// Proxy class between QTreeView and QFileSystemModel.
// We need this class to perform the following actions:
// - filter out unnecessary columns (leave just first one)
// - display dirs only, no files
// - sort drives by drive letter, not by label
// - remove unnecessary "+" sign when a dir has no nested dirs
// - provide own icon for folders
// - provide drop functionality (drag&drop from thumb list)

ptFSMProxy::ptFSMProxy(QObject *parent)
    : QSortFilterProxyModel(parent) {
  m_model = new QFileSystemModel(this);
  m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
  m_model->setRootPath(QDir::currentPath());
  m_model->setIconProvider(new ptFileIconProviderPrivate);
  setSourceModel(m_model);
}

QModelIndex ptFSMProxy::getIndexForPath(const QString &path) const {
  return mapFromSource(m_model->index(path));
}

QString ptFSMProxy::getPathForIndex(const QModelIndex &index) const {
  return m_model->filePath(mapToSource(index));
}

// filter out unnecessary columns (leave just first one)
bool ptFSMProxy::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const {
  Q_UNUSED(source_parent)

  if(source_column > 0)
      return false;
  return true;
}

// remove unnecessary "+" sign when a dir has no nested dirs
bool ptFSMProxy::hasChildren(const QModelIndex &parent) const {
  QModelIndex source_parent = mapToSource(parent);
  if (m_model->isDir(source_parent)) {
   // return if at least one child exists
   return QDirIterator(
               m_model->filePath(source_parent),
               m_model->filter(),
               QDirIterator::NoIteratorFlags
           ).hasNext();
  }

  return true;
}

// sort drives by drive letter, not by label
bool ptFSMProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  QFileSystemModel* model = dynamic_cast<QFileSystemModel*>(this->sourceModel());
  QVariant leftData = model->data(left);
  QVariant rightData = model->data(right);
  QString leftString = leftData.toString();
  QString rightString = rightData.toString();

  if (leftString.endsWith(":)"))
    leftString = leftString.mid(leftString.length() - 3, 2);
  if (rightString.endsWith(":)"))
    rightString = rightString.mid(rightString.length() - 3, 2);
  return QString::localeAwareCompare(leftString, rightString) > 0;
}

// provide drop functionality (drag&drop from thumb list)
Qt::DropActions ptFSMProxy::supportedDropActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}

QStringList ptFSMProxy::mimeTypes() const {
  QStringList l;
  l << "text/plain";
  return l;
}

Qt::ItemFlags ptFSMProxy::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);
  return Qt::ItemIsDropEnabled | defaultFlags;
}

bool ptFSMProxy::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) {
  Q_UNUSED(action)
  Q_UNUSED(row)
  Q_UNUSED(column)

  if (!(data->hasText() && data->text().startsWith("photivo:")))
    return false;

  // normalize path
  QString newPath = getPathForIndex(parent);
  if (!(newPath.endsWith("/") || newPath.endsWith("\\"))) {
    newPath += QDir::separator();
  }
  if (newPath == m_CurrentDir)
    return false;

  QStringList files = data->text().split("\n", QString::SkipEmptyParts);
  bool yesToAll = false;
  // start from 1, skip "photivo:" first element
  for (int i = 1; i < files.count(); i++) {
    QString imageFileName = files.at(i);
    QFileInfo imagePathInfo(imageFileName);
    QString newImageFileName = newPath + imagePathInfo.fileName();

    // check if destination folder has a file with the same name
    bool renameFile = true;
    if (QFile::exists(newImageFileName) && !yesToAll) {
      QMessageBox::StandardButton result = ptMessageBox::warning(0, QObject::tr("Warning"),
        QObject::tr("This folder already contains a file called '%1'.\n\nWould you like to replace the existing file with this one?").arg(imagePathInfo.fileName()), 
        QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel, 
        QMessageBox::No);
      if (result == QMessageBox::YesToAll) {
        yesToAll = true;
      } else if (result == QMessageBox::No) {
        renameFile = false;
      } else if (result == QMessageBox::Cancel) {
        break;
      }
    }

    if (renameFile) {
      // delete the destination file before renaming
      if (QFile::exists(newImageFileName))
        QFile::remove(newImageFileName);
      // rename the image file
      QFile::rename(imageFileName, newPath + imagePathInfo.fileName());
      // process associated settings file
      QString ptsFileName = imagePathInfo.dir().absolutePath() + QDir::separator() + imagePathInfo.completeBaseName() + ".pts";
      QString newPtsFileName = newPath + imagePathInfo.completeBaseName() + ".pts";
      if (QFile::exists(ptsFileName)) {
        // delete the destination file before renaming
        if (QFile::exists(newPtsFileName))
          QFile::remove(newPtsFileName);
        QFile::rename(ptsFileName, newPtsFileName);
      }
    }
  }

  return true;
}


// - provide own icon for folders
ptFileIconProviderPrivate::ptFileIconProviderPrivate()
 : QFileIconProvider() {
  folderIcon = QIcon(QString::fromUtf8(":/dark/icons/folder.png"));
}

QIcon ptFileIconProviderPrivate::icon(const QFileInfo &info) const {
  if (!info.isRoot())
    return folderIcon;
  return QFileIconProvider::icon(info);
}
