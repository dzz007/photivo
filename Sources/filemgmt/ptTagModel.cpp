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

#include <QSettings>
#include <QDir>

#include "../ptDefines.h"
#include "../ptConstants.h"
#include "ptTagModel.h"

extern QString UserDirectory;

//==============================================================================

ptTagModel::ptTagModel(QObject* parent)
: QStandardItemModel(parent)
{
  m_TagList = new QList<TagItem>;
  QSettings IniFile(UserDirectory + PhotivoFile.TagsIni, QSettings::IniFormat);

  int BookmarkCount = IniFile.beginReadArray("Bookmarks");
  for (int i = 0; i < BookmarkCount; i++) {
    // read bookmark from ini
    IniFile.setArrayIndex(i);
    this->appendRow(IniFile.value("Caption").toString(),
                    IniFile.value("Directory").toString());
  }
}

//==============================================================================

ptTagModel::~ptTagModel() {
  // save bookmarks list to disk
  QSettings IniFile(UserDirectory + PhotivoFile.TagsIni, QSettings::IniFormat);
  IniFile.beginGroup("Bookmarks");
  IniFile.remove("");
  IniFile.endGroup();
  IniFile.beginWriteArray("Bookmarks");
  for (int i = 0; i < m_TagList->count(); i++) {
    IniFile.setArrayIndex(i);
    IniFile.setValue("Caption", m_TagList->at(i).Caption);
    IniFile.setValue("Directory", m_TagList->at(i).Directory);
  }
  IniFile.endArray();

  DelAndNull(m_TagList);
}

//==============================================================================

bool ptTagModel::setData(const QModelIndex& index,
                         const QVariant& value,
                         int role /*= Qt::EditRole*/)
{
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    TagItem item = m_TagList->at(index.row());
    item.Caption = value.toString();
    m_TagList->replace(index.row(), item);
  }

  return QStandardItemModel::setData(index, value, role);
}

//==============================================================================

void ptTagModel::appendRow(QString caption, QString dirPath) {
  // create new tag data entity
  TagItem item = { caption, dirPath };
  m_TagList->append(item);

  // create new model entry
  QStandardItem* ModelItem = new QStandardItem(item.Caption);
  ModelItem->setToolTip(QDir::toNativeSeparators(item.Directory));
  ModelItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  QStandardItemModel::appendRow(ModelItem);
}

//==============================================================================

bool ptTagModel::removeRows(int row, int count, const QModelIndex& parent) {
  beginRemoveRows(parent, row, row+count-1);
  m_TagList->removeAt(row);
  bool success = QStandardItemModel::removeRows(row, count, parent);
  endRemoveRows();
  return success;
}

//==============================================================================

Qt::ItemFlags ptTagModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

  if (index.isValid()) {
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  } else {
    return Qt::ItemIsDropEnabled | defaultFlags;
  }
}

//==============================================================================

Qt::DropActions ptTagModel::supportedDropActions() const {
  return Qt::MoveAction;
}

//==============================================================================

QString ptTagModel::path(const QModelIndex& index) {
  QString result;
  if (index.model() == this && index.isValid()) {
    result = m_TagList->at(index.row()).Directory;
  }
  return result;
}

//==============================================================================
