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
#include "ptConstants.h"
#include "ptJobListModel.h"
#include "ptMessageBox.h"
#include "ptSettings.h"

//==============================================================================

ptJobListModel::ptJobListModel(QObject *parent) :
  QAbstractTableModel(parent)
{
  m_JobList = new QList<ptJobListItem*>;
  m_CurrentJob = nullptr;
  m_BatchProcess = new QProcess(this);
  connect(this, SIGNAL(processingAborted()), SLOT(OnAbortProcessing()));

  m_DefaultAutosaveFileName = Settings->GetString("UserDirectory")+"currentBatch.ptb";
  if (Settings->GetValue("BatchMgrAutoload").toBool() && QFileInfo(m_DefaultAutosaveFileName).exists())
    LoadFromSettings(m_DefaultAutosaveFileName);
}

//==============================================================================

ptJobListModel::~ptJobListModel()
{
  foreach (ptJobListItem *item, *m_JobList)
    delete item;
  delete m_JobList;
  delete m_BatchProcess;
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
      return QColor(Qt::green);
    if (m_JobList->at(r)->Status() == jsError || m_JobList->at(r)->Status() == jsAborted)
      return QColor(Qt::darkRed);
    if (m_JobList->at(r)->Status() == jsSkipped)
      return QColor(Qt::yellow);
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

void ptJobListModel::AddJobToList(const QString &file, const QString &inputFile)
{
  ptJobListItem *item = nullptr;
  try {
    item = new ptJobListItem(file);
  }
  catch (ptJobListItem::ptJobExeption) {
//    if file is not a Photivo settings file
//    item == nullptr in this case
    ptMessageBox::critical(nullptr, tr("Invalid settings file"),
                           file + tr("\nis not a Photivo settings file."));
  }

//  a valid settings file
  if (item != nullptr) {
    if (!inputFile.isEmpty())
      item->SetInputFiles(QStringList(inputFile));
    AddJobToList(item);
  }
}

//==============================================================================

void ptJobListModel::AddJobToList(ptJobListItem *item)
{
  foreach (ptJobListItem *i, *m_JobList) {
    if (*i == *item) {
      i->UpdateFromJobItem(item);
      return;
    }
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_JobList->append(item);
  endInsertRows();
  item->UpdateStatusByTime();
  connect(item, SIGNAL(itemChanged()), SLOT(OnJobItemChanged()));
}

//==============================================================================

void ptJobListModel::RemoveJobFromList(int row)
{
  Q_ASSERT(row >= 0 && row < rowCount());

  if (JobItem(row)->Status() == jsProcessing) {
    int result = ptMessageBox::question(nullptr, tr("Remove this job?"),
                   JobItem(row)->FileName() + tr("\njob is being processed."
                   " Do you want to abort processing and remove it from the list?"),
                                        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes)
      emit processingAborted();
    else
      return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  m_JobList->removeAt(row);
  endRemoveRows();
}

//==============================================================================

short ptJobListModel::SaveToSettings(const QString &file)
{
  QSettings settings(file, QSettings::IniFormat);
  settings.clear();
  settings.setValue("Magic", "photivoBatchFile");

  for (int i=0; i < m_JobList->count(); i++) {
    settings.beginGroup(QString::number(i));
    m_JobList->at(i)->SaveToSettings(&settings);
    settings.endGroup();
  }

  settings.sync();
  if (settings.status() != QSettings::NoError) {
    ptMessageBox::critical(nullptr, tr("Error"), tr("Error writing job list file\n") + file);
    return ptError_FileOpen;
  }

  if (Settings->GetInt("BatchMgrAutosaveFile") == bsfLocal)
    m_AutosaveFileName = file;
  return 0;
}

//==============================================================================

short ptJobListModel::LoadFromSettings(const QString &file)
{
  QSettings settings(file, QSettings::IniFormat);
  settings.sync();
  if (settings.status() != QSettings::NoError) {
    ptMessageBox::critical(nullptr, tr("Error"), tr("Error reading job list file\n") + file);
    return ptError_FileOpen;
  }
  if (settings.value("Magic") != "photivoBatchFile") {
    ptMessageBox::critical(nullptr, tr("Error"), file + tr("\nis not a job list file."));
    return ptError_FileFormat;
  }

  QStringList groups = settings.childGroups();
  foreach (QString g, groups) {
    ptJobListItem *item = new ptJobListItem;
    settings.beginGroup(g);
    item->LoadFromSettings(&settings);
    settings.endGroup();
    AddJobToList(item);
  }
  return 0;
}

//==============================================================================

bool ptJobListModel::RunNextJob()
{
  int i;
  for (i = 0; i < count(); i++) {
    m_CurrentJob = JobItem(i);
    if (m_CurrentJob->Status() == jsWaiting) {
      QString fileName = m_CurrentJob->GetCallFileName();
      if (fileName.isEmpty()) {
//        invalid job
        m_CurrentJob->SetStatus(jsSkipped);
        continue;
      }
      m_CurrentJob->SetStatus(jsProcessing);

      connect(m_BatchProcess, SIGNAL(finished(int)), m_CurrentJob, SLOT(OnProcessingFinished(int)));
      QString photivoApp = QCoreApplication::applicationFilePath();
      QStringList arguments = QStringList() << "-j" << fileName;
      m_BatchProcess->start(photivoApp, arguments, QIODevice::ReadOnly);
      m_CurrentJob->UpdateTime();

      break;
    }
  }

  return (i == count());
}

//==============================================================================

void ptJobListModel::OnJobItemChanged()
{
  int i = m_JobList->indexOf(qobject_cast<ptJobListItem*>((sender())));
  Q_ASSERT(i >= 0);
  emit dataChanged(index(i, 0), index(i, columnCount()));
  AutosaveJobList();
}

//==============================================================================

void ptJobListModel::AutosaveJobList()
{
  if (Settings->GetValue("BatchMgrAutosave").toBool()) {
    if (Settings->GetInt("BatchMgrAutosaveFile") == bsfStandard)
      SaveToSettings(m_DefaultAutosaveFileName);
    if (Settings->GetInt("BatchMgrAutosaveFile") == bsfLocal && !m_AutosaveFileName.isEmpty())
      SaveToSettings(m_AutosaveFileName);
  }
}

//==============================================================================

void ptJobListModel::OnAbortProcessing()
{
  m_BatchProcess->terminate();
  m_CurrentJob->SetStatus(jsAborted);
}

//==============================================================================
