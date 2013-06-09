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

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include "ptTempFile.h"

//==============================================================================

/*!
 * \brief The ptJobData enum numerates the coloumns of the job list view.
 *
 * \c jdMaxNumber is the number of coloumns. It should be the last in this enum.
 */
enum ptJobData {
  jdFileName         = 0,
  jdStatus           = 1,
  jdOutputPath       = 2,
  jdOutputFileSuffix = 3,
  jdInputFiles       = 4,
  jdMaxNumber        = 5
};

/*!
 * \brief The ptJobStatus enum describes the current status of the job.
 */
enum ptJobStatus {
  jsWaiting    = 0,
  jsProcessing = 1,
  jsFinished   = 2,
  jsSkipped    = 3,
  jsError      = 4,
  jsAborted    = 5
};

//==============================================================================

/*!
 * \class ptJobListItem
 *
 * \brief Element of the job list in the batch window.
 */
class ptJobListItem : public QObject
{
  Q_OBJECT
public:
  /*!
   * Creates a new \c ptJobListItem object.
   *
   * \param file is the path to the settings file
   *        containing the processing instructions.
   */
  explicit ptJobListItem(const QString &file = QString());
  ~ptJobListItem();

  /*!
   * Initializes the ptJobListItem object.
   * \param file is the path to the settings file
   *        containing the processing instructions.
   */
  void InitFromFile(const QString &file);

  /*!
   * Returns the path to the settings file.
   */
  const QString& FileName() const { return m_FileName; }

  /*!
   * Returns the path processed photoes will be saved to.
   */
  const QString& OutputPath() const { return m_OutputPath; }

  /*!
   * Returns the suffix to be appended to the end of the saved files.
   */
  const QString& OutputFileSuffix() const { return m_OutputFileSuffix; }

  /*!
   * Returns the list of files to be processed.
   */
  const QStringList& InputFiles() const { return m_InputFiles; }

  /*!
   * Returns the current status of the job.
   */
  ptJobStatus Status() const {return m_Status; }

  /*!
   * Sets m_InputFiles to \c inputFiles.
   */
  void SetInputFiles(const QStringList &inputFiles);

  /*!
   * Sets the current status of the job to \c status.
   */
  void SetStatus(ptJobStatus status);

  /*!
   * Sets last processing  time to the current time.
   */
  void UpdateTime();

  /*!
   * Updates the current status of the job that was previously processed.
   * Sets the status to \c Waiting if the job file was modified after last processing
   * and to \c Finished otherwise.
   */
  void UpdateStatusByTime();

  /*!
   * Updates current job to \c item, which corresponds to the same settings file
   * but is more recent.
   */
  void UpdateFromJobItem(ptJobListItem *item);

  /*!
   * Tests if the job corresponds to a settings file or a job file.
   * \return \c true if it's a job file and \c false otherwise.
   */
  bool IsJobFile() const;

  /*!
   * Tests if the job is valid
   * \return \c true if file name, output path and input files list
     aren't empty and \c false otherwise.
   */
  bool IsValid() const;

  /*! Creates a temporary job file from a settings file by adding
      corresponding OutputDirectory and InputFileNameList values.
      \return the temporary file name or an empty string if file can't be created.
   */
  const QString CreateTempJobFile();

  /*! Returns file name to be provided to "photivo -j".
      Creates temporary file if necessary.
      \return the file name or an empty string the job is invalid.
   */
  const QString GetCallFileName();

  /*!
   * Saves all job parameters to \c setting.
   */
  void SaveToSettings(QSettings *settings) const;

  /*!
   * Reads job parameters from \c setting.
   */
  void LoadFromSettings(QSettings *settings);

  /*!
   * Checks if two ptJobListItems are equal. They are considered equal
   * if their settings file's names are equal.
   */
  bool operator ==(const ptJobListItem &item) const;

  /*!
   * Exeption class used to check if the loaded settings file is correct.
   */
  class ptJobExeption {};

public slots:
  /*!
   * OnProcessingFinished is called when the processing of this job is finished.
   * \param exitCode is the exit code of the external Photivo process.
   */
  void OnProcessingFinished(int exitCode);

signals:
  /*!
   * itemChanged is emitted when some job parameters are changed.
   */
  void itemChanged();

private:
  QString m_FileName;
  QString m_OutputPath;
  QString m_OutputFileSuffix;
  QStringList m_InputFiles;
  ptJobStatus m_Status;
  ptTempFile *m_TempFile;
  QDateTime m_ProcessingStarted;
  QDateTime m_lastProcessing;
};

//==============================================================================

/*!
 * Functions for sorting job list by corresponding coloumns.
 */
bool SortByFileName(ptJobListItem *i1, ptJobListItem *i2);
bool ReverseSortByFileName(ptJobListItem *i1, ptJobListItem *i2);
bool SortByStatus(ptJobListItem *i1, ptJobListItem *i2);
bool ReverseSortByStatus(ptJobListItem *i1, ptJobListItem *i2);
bool SortByOutputPath(ptJobListItem *i1, ptJobListItem *i2);
bool ReverseSortByOutputPath(ptJobListItem *i1, ptJobListItem *i2);
bool SortByOutputFileSuffix(ptJobListItem *i1, ptJobListItem *i2);
bool ReverseSortByOutputFileSuffix(ptJobListItem *i1, ptJobListItem *i2);
bool SortByInputFiles(ptJobListItem *i1, ptJobListItem *i2);
bool ReverseSortByInputFiles(ptJobListItem *i1, ptJobListItem *i2);

//==============================================================================

#endif // PTJOBLISTITEM_H
