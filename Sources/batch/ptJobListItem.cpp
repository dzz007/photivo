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

ptJobListItem::ptJobListItem(const QString &file)
{
  QSettings job(file, QSettings::IniFormat);
  if (!(job.value("Magic") == "photivoJobFile" ||
        job.value("Magic") == "photivoSettingsFile" ||
        job.value("Magic") == "dlRawJobFile" ||
        job.value("Magic") == "dlRawSettingsFile" ||
        job.value("Magic") == "photivoPresetFile")) {
    throw ptJobExeption();
  }

  m_FileName = file;

  m_IsJobFile = (QFileInfo(file).suffix() == "ptj");

  m_OutputPath = job.value("OutputDirectory").toString();
//  try to guess output directory corresponding to settings file
  if (m_OutputPath.isEmpty())
    m_OutputPath = QFileInfo(file).absolutePath();

  m_OutputFileSuffix = job.value("OutputFileNameSuffix").toString();

  m_InputFiles = job.value("InputFileNameList").toStringList();
//  try to guess input file and output file suffix corresponding to settings file
  if (m_InputFiles.isEmpty()) {
    QString basename = QFileInfo(file).absoluteFilePath().section('.', 0, -2);
    QStringList rawExtensions = RawPattern.split(";;").first().section('(', 1).
                          section(')', 0, 0).split(" ", QString::SkipEmptyParts).replaceInStrings("*", "");

//    search for the first suitable raw file
    foreach (QString ext, rawExtensions) {
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

  m_TempFile = nullptr;
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
  emit itemChanged();
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

  return m_TempFile->fileName();
}

//==============================================================================

const QString ptJobListItem::GetCallFileName()
{
//  We should create a copy if is't not a job file but a settings file.
  if (m_IsJobFile)
    return m_FileName;
  else
    return CreateTempJobFile();
}

//==============================================================================

void ptJobListItem::OnProcessingFinished(int status)
{
  if (m_Status != jsAborted) {
    if (status == QProcess::NormalExit)
      SetStatus(jsFinished);
    else
      SetStatus(jsError);
  }

//  this slot should be called only once for this job
  disconnect(sender(), SIGNAL(finished(int)), this, SLOT(OnProcessingFinished(int)));

//  ensure that temporary file is deleted after processing
  if (m_TempFile != nullptr) {
    delete m_TempFile;
    m_TempFile = nullptr;
  }
}

//==============================================================================
