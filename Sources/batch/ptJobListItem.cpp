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
#include <QProcess>
#include <QSettings>
#include "ptJobListItem.h"

//==============================================================================

ptJobListItem::ptJobListItem(const QString &file /* = QString()*/)
{
  if (!file.isEmpty())
    InitFromFile(file);

  m_TempFile = nullptr;
}

//==============================================================================

extern QStringList FileExtsRaw;
void ptJobListItem::InitFromFile(const QString &file)
{
  QSettings jobFile(file, QSettings::IniFormat);
  if (!(jobFile.value("Magic") == "photivoJobFile" ||
        jobFile.value("Magic") == "photivoSettingsFile" ||
        jobFile.value("Magic") == "dlRawJobFile" ||
        jobFile.value("Magic") == "dlRawSettingsFile" ||
        jobFile.value("Magic") == "photivoPresetFile")) {
    throw ptJobExeption();
  }

  m_FileName = file;

  m_OutputPath = jobFile.value("OutputDirectory").toString();
//  try to guess output directory corresponding to settings file
  if (m_OutputPath.isEmpty())
    m_OutputPath = QFileInfo(file).absolutePath();

  m_OutputFileSuffix = jobFile.value("OutputFileNameSuffix").toString();

  m_InputFiles = jobFile.value("InputFileNameList").toStringList();
//  try to guess input file and output file suffix corresponding to settings file
  if (m_InputFiles.isEmpty()) {
    QString basename = QFileInfo(file).absoluteFilePath().section('.', 0, -2);

//    search for the first suitable raw file
    foreach (QString ext, FileExtsRaw.replaceInStrings("*", "")) {
      QString base = basename;
      QString suffix;
      while (!QFileInfo(base + ext).exists()) {
        suffix.prepend(base.right(1));
        base.remove(base.length()-1, 1);
        if (base.right(1) == "/") break;
      }
      if (QFileInfo(base + ext).exists()) {
        m_InputFiles = QStringList(base + ext);
        if (m_OutputFileSuffix.isEmpty())
          m_OutputFileSuffix = suffix;
        break;
      }
    }
  }
}

//==============================================================================

ptJobListItem::~ptJobListItem()
{
  delete m_TempFile;
}

//==============================================================================

void ptJobListItem::SetStatus(ptJobStatus status)
{
  m_Status = status;
//  we save the time when processing started, if it's succeded
  if (status == jsFinished)
    m_lastProcessing = m_ProcessingStarted;
  else
    m_lastProcessing = QDateTime();
  emit itemChanged();
}

//==============================================================================

void ptJobListItem::UpdateTime()
{
  m_ProcessingStarted = QDateTime::currentDateTime();
}

//==============================================================================

void ptJobListItem::UpdateStatusByTime()
{
  if (m_FileName.isEmpty())
    m_Status = jsWaiting;
  else {
//    check if the settings file was modified after last processing
    if (!m_lastProcessing.isValid() || QFileInfo(m_FileName).lastModified() > m_lastProcessing)
      m_Status = jsWaiting;
    else
      m_Status = jsFinished;
  }
}

//==============================================================================

void ptJobListItem::UpdateFromJobItem(ptJobListItem *item)
{
  Q_ASSERT(item->FileName() == m_FileName);

  if (item->InputFiles() != m_InputFiles) {
    m_InputFiles = item->InputFiles();
    SetStatus(jsWaiting);
  }
  if (item->OutputPath() != m_OutputPath) {
    m_OutputPath = item->OutputPath();
    SetStatus(jsWaiting);
  }
  if (item->OutputFileSuffix() != m_OutputFileSuffix) {
    m_OutputFileSuffix = item->OutputFileSuffix();
    SetStatus(jsWaiting);
  }
  if (m_lastProcessing.isValid() && QFileInfo(item->FileName()).lastModified() > m_lastProcessing)
    SetStatus(jsWaiting);
}

//==============================================================================

bool ptJobListItem::IsJobFile() const
{
  if (QFileInfo(m_FileName).suffix() == "ptj")
    return true;
  else
    return false;
}

//==============================================================================

bool ptJobListItem::IsValid() const
{
  if (m_FileName.isEmpty() || m_OutputPath.isEmpty() || m_InputFiles.isEmpty())
    return false;
  else
    return true;
}

//==============================================================================

const QString ptJobListItem::CreateTempJobFile()
{
//  It's impossible to create a valid job file if some fileds are empty.
  if (!IsValid())
    return QString();

//  create a temporary file being a copy of m_FileName
  m_TempFile = new ptTempFile(m_FileName);
  QSettings job(m_TempFile->fileName(), QSettings::IniFormat);

  if (job.value("OutputDirectory").toString().isEmpty())
    job.setValue("OutputDirectory", m_OutputPath);
  if (job.value("OutputFileNameSuffix").toString().isEmpty())
    job.setValue("OutputFileNameSuffix", m_OutputFileSuffix);
  if (job.value("InputFileNameList").toString().isEmpty())
    job.setValue("InputFileNameList", m_InputFiles);

  job.sync();
  if (job.status() != QSettings::NoError)
    return QString();

  return m_TempFile->fileName();
}

//==============================================================================

const QString ptJobListItem::GetCallFileName()
{
//  We should create a copy if is't not a job file but a settings file.
  if (IsJobFile())
    return m_FileName;
  else
    return CreateTempJobFile();
}

//==============================================================================

void ptJobListItem::OnProcessingFinished(int exitCode)
{
  if (m_Status != jsAborted) {
    if (exitCode == 0)
      SetStatus(jsFinished);
    else
      SetStatus(jsError);
  }

//  this slot should be called only once for this job
  disconnect(sender(), 0, this, SLOT(OnProcessingFinished(int)));

//  ensure that temporary file is deleted after processing
  if (m_TempFile != nullptr) {
    delete m_TempFile;
    m_TempFile = nullptr;
  }
}

//==============================================================================

void ptJobListItem::SaveToSettings(QSettings *settings) const
{
  settings->setValue("JobFileName", m_FileName);
  settings->setValue("OutputDirectory", m_OutputPath);
  settings->setValue("OutputFileSuffix", m_OutputFileSuffix);
  settings->setValue("InputFileNameList", m_InputFiles);
  settings->setValue("JobStatus", m_Status);
  settings->setValue("LastProcessing", m_lastProcessing);
}

//==============================================================================

void ptJobListItem::LoadFromSettings(QSettings *settings)
{
  m_FileName = settings->value("JobFileName").toString();
  m_OutputPath = settings->value("OutputDirectory").toString();
  m_OutputFileSuffix = settings->value("OutputFileSuffix").toString();
  m_InputFiles = settings->value("InputFileNameList").toStringList();
  m_Status = (ptJobStatus)settings->value("JobStatus").toInt();
  m_lastProcessing = settings->value("LastProcessing").toDateTime();
}

//==============================================================================

bool ptJobListItem::operator ==(const ptJobListItem &item) const
{
  return (m_FileName == item.FileName());
}

//==============================================================================
