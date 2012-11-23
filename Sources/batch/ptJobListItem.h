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

#ifndef PTJOBLISTITEM_H
#define PTJOBLISTITEM_H

//==============================================================================

#include <QStringList>
#include "ptTempFile.h"

//==============================================================================

/*! This enum corresponds to coloumns of the table in the batch window */
enum ptJobData {
  jdFileName         = 0,
  jdStatus           = 1,
  jdOutputPath       = 2,
  jdOutputFileSuffix = 3,
  jdInputFiles       = 4,
  jdMaxNumber        = 5    // This constant gives us the number of coloumns.
                            // It should be the last in this enum.
};

/*! This enum corresponds to the current status of the job */
enum ptJobStatus {
  jsWaiting    = 0,
  jsProcessing = 1,
  jsFinished   = 2,
  jsSkipped    = 3,
  jsError      = 4,
  jsAborted    = 5
};

//==============================================================================

extern QString RawPattern;

//==============================================================================

/*! This class describes an element of the job list in batch window */
class ptJobListItem : public QObject
{
  Q_OBJECT
public:
  explicit ptJobListItem(const QString &file);
  ~ptJobListItem();

  const QString& FileName() const { return m_FileName; }
  const QString& OutputPath() const { return m_OutputPath; }
  const QString& OutputFileSuffix() const { return m_OutputFileSuffix; }
  const QStringList& InputFiles() const { return m_InputFiles; }
  ptJobStatus Status() const {return m_Status; }

  void SetStatus(ptJobStatus status);

  /*! Returns true if file name, output path and input files list
      aren't empty. Otherwise ruterns false.                      */
  bool IsValid() const;

  /*! Create a temporary job file from a settings file by adding
      corresponding OutputDirectory and InputFileNameList values.
      Returns empty string if file can't be created.              */
  const QString CreateTempJobFile();

  /*! Returns file name to be provided to "photivo -j".
      Creates temporary file if necessary.
      Returns empty string if something goes wrong.     */
  const QString GetCallFileName();

  /*! Exeption class is used to check if settings file is correct. */
  class ptJobExeption {};

public slots:
  void OnProcessingFinished(int status);

signals:
  void itemChanged();

private:
  QString m_FileName;
  QString m_OutputPath;
  QString m_OutputFileSuffix;
  QStringList m_InputFiles;
  ptJobStatus m_Status;
  bool m_IsJobFile;         // in other case it's a settings file
  ptTempFile *m_TempFile;
};

//==============================================================================

#endif // PTJOBLISTITEM_H
