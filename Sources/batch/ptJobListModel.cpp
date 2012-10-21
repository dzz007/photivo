/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Sergey Salnikov <salsergey@gmail.com>
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

#include <QFileInfo>
#include "ptJobListModel.h"
#include "ptMessageBox.h"

//==============================================================================

ptJobListModel::ptJobListModel(QObject *parent) :
  QAbstractTableModel(parent)
{
  m_JobList = new QList<ptJobListItem*>;
}

//==============================================================================

ptJobListModel::~ptJobListModel()
{
  delete m_JobList;
}

//==============================================================================

int ptJobListModel::columnCount(const QModelIndex & /*parent  = QModelIndex()*/) const
{
  return jdMaxNumber;
}

//==============================================================================

int ptJobListModel::rowCount(const QModelIndex & /*parent  = QModelIndex()*/) const
{
  return m_JobList->count();
}

//==============================================================================

QVariant ptJobListModel::data(const QModelIndex &index, int role) const
{
  int r = index.row();
  int c = index.column();

  if (role == Qt::DisplayRole) {
    switch (c) {
      case jdFileName:         return m_JobList->at(r)->FileName();
      case jdOutputPath:       return m_JobList->at(r)->OutputPath();
      case jdOutputFileSuffix: return m_JobList->at(r)->OutputFileSuffix();
      case jdInputFiles: {
          QStringList list = m_JobList->at(r)->InputFiles();
//          path to input files is shown without directory
          return list.replaceInStrings(QFileInfo(m_JobList->at(r)->FileName()).absolutePath() + "/", "").join(", ");
        }
      case jdStatus:
        switch (m_JobList->at(r)->Status()) {
          case jsWaiting:    return tr("Waiting");
          case jsProcessing: return tr("Processing");
          case jsFinished:   return tr("Finished");
          case jsSkipped:    return tr("Skipped");
          case jsError:      return tr("Error");
          case jsAborted:    return tr("Aborted");
          default:           return QVariant();
        }
      default:                 return QVariant();
    }
  }

  if (role == Qt::ForegroundRole && c == jdStatus) {
    if (m_JobList->at(r)->Status() == jsFinished)
      return Qt::darkGreen;
    if (m_JobList->at(r)->Status() == jsError || m_JobList->at(r)->Status() == jsAborted)
      return Qt::darkRed;
    if (m_JobList->at(r)->Status() == jsSkipped)
      return Qt::darkYellow;
  }

  return QVariant();
}

//==============================================================================

QVariant ptJobListModel::headerData(int section, Qt::Orientation orientation, int role /*  = Qt::DisplayRole*/) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
      case jdFileName:         return tr("File name");
      case jdOutputPath:       return tr("Output path");
      case jdOutputFileSuffix: return tr("Output suffix");
      case jdInputFiles:       return tr("Input files");
      case jdStatus:           return tr("Status");
      default:                 return QAbstractTableModel::headerData(section, orientation, role);
    }
  }

  return QAbstractTableModel::headerData(section, orientation, role);
}

//==============================================================================

ptJobListItem* ptJobListModel::JobItem(int i) const
{
  return m_JobList->at(i);
}

//==============================================================================

void ptJobListModel::AddJob(const QString &file)
{
  ptJobListItem *item = nullptr;
  try {
    item = new ptJobListItem(file);
  }
  catch (ptJobListItem::ptJobExeption) {
//    if file is not a Photivo settings file
//    item == nullptr in this case
    ptMessageBox::warning(nullptr, tr("Invalid settings file"),
                          file + "\n" + tr("is not a Photivo settings file."));
  }

//  a valid settings file
  if (item != nullptr) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_JobList->append(item);
    endInsertRows();
    connect(item, SIGNAL(itemChanged()), SLOT(OnJobItemChanged()));
  }
}

//==============================================================================

void ptJobListModel::RemoveJob(int row)
{
  Q_ASSERT(row >= 0 && row < rowCount());

  beginRemoveRows(QModelIndex(), row, row);
  m_JobList->removeAt(row);
  endRemoveRows();
}

//==============================================================================

void ptJobListModel::OnJobItemChanged()
{
  int i = m_JobList->indexOf(qobject_cast<ptJobListItem*>((sender())));
  Q_ASSERT(i >= 0);
  emit dataChanged(index(i, 0), index(i, columnCount()));
}

//==============================================================================
