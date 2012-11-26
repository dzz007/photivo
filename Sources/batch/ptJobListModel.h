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

#ifndef PTJOBLISTMODEL_H
#define PTJOBLISTMODEL_H

//==============================================================================

#include <QAbstractTableModel>
#include <QProcess>
#include "ptJobListItem.h"

//==============================================================================

/*!
 * \brief The model for the job list.
 *
 * Handles all operations with the job list such as saving, loading, modifying, etc.
 */
class ptJobListModel : public QAbstractTableModel
{
  friend class ptBatchWindow;
  Q_OBJECT
public:
  /*!
   * Default constructor for ptJobListModel.
   */
  explicit ptJobListModel(QObject *parent = 0);
  ~ptJobListModel();

  /*!
   * Reimplements \c QAbstractTableModel's columnCount().
   * \return the number of coloumns in the job list.
   */
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  /*!
   * Reimplements \c QAbstractTableModel's rowCount().
   * \return the number of rows in the job list.
   */
  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  /*!
   * Reimplements \c QAbstractTableModel's data().
   * \return the content of the job list.
   */
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  /*!
   * Reimplements \c QAbstractTableModel's headerData().
   * \return the titles of the coloumns.
   */
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  /*!
   * A replacement fot rowCount() method. Added for convenience.
   * \return the number of jobs in the list.
   */
  int count() const { return rowCount(); }

  /*!
   * Returns a pointer to the job number \c i.
   */
  ptJobListItem* JobItem(int i) const;

  /*!
   * Creates a job from \c file and adds it to the list.
   */
  void AddJobToList(const QString &file);

  /*!
   * Adds the job \c item to the list.
   */
  void AddJobToList(ptJobListItem *item);

  /*!
   * Removes the job number \c row from the list.
   */
  void RemoveJobFromList(int row);

  /*!
   * Saves current job list to \c file.
   * \return 0 if there were no errors and non-zero values otherwise.
   */
  short SaveToSettings(const QString &file);

  /*!
   * Loads jobs from \c file. Jobs are added to current list. Repeated jobs are skipped.
   * \return 0 if there were no errors and non-zero values otherwise.
   */
  short LoadFromSettings(const QString &file);

  /*!
   * Starts processing of the first job with \c Waiting status.
   * \retval false there are more jobs for processing
   * \retval true all jobs were processed
   */
  bool RunNextJob();
  
public slots:
  /*!
   * Called after a job has changed.
   */
  void OnJobItemChanged();

  /*!
   * Saves current list to the standard path or to the file it was previously saved to.
   * Autosaving is performed every time the list changes.
   */
  void AutosaveJobList();

  /*!
   * Aborts processing of current job.
   */
  void OnAbortProcessing();

signals:
  /*!
   * This signal is emitted when current job processing was aborted.
   */
  void processingAborted();

private:
  QList<ptJobListItem*> *m_JobList;
  ptJobListItem *m_CurrentJob;
  QProcess *m_BatchProcess;
  QString m_DefaultAutosaveFileName;
  QString m_AutosaveFileName;
};

//==============================================================================

#endif // PTJOBLISTMODEL_H
