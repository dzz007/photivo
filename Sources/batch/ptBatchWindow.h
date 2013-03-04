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

#ifndef PTBATCHWINDOW_H
#define PTBATCHWINDOW_H

//==============================================================================

#include "ui_ptBatchWindow.h"
#include "ptJobListModel.h"

//==============================================================================

/*!
 * \brief The batch manager window.
 */
class ptBatchWindow : public QWidget, private Ui::ptBatchWindow
{
  Q_OBJECT
  
public:
  explicit ptBatchWindow(QWidget *parent = 0);
  ~ptBatchWindow();

  void UpdateTheme();

  /*!
   * Adds job from \c settingFile to the list. Allows one to specify a \c rawFile.
   */
  void AddJobToList(const QString &settingFile, const QString &rawFile = QString());

  /*!
   * Adds jobs from \c settingFiles to the list.
   */
  void AddJobsToList(const QStringList &settingFiles);

protected:
  void keyPressEvent(QKeyEvent *event);
  void showEvent(QShowEvent *event);

private slots:
  void OnAddJob();
  void OnRemoveJob();
  void OnRunJobs();
  void OnAbortProcessing();
  void OnResetStatus();
  void OnSaveJobList();
  void OnOpenJobList();
  void OnCloseWindow();

  /*!
   * Starts processing of the first job with \c Waiting status.
   */
  void OnRunNextJob();

  /*!
   * OnLogReady is called when external Photivo process prints something
   * to \c stdout.
   */
  void OnLogReady();

  /*!
   * Handles Log button pressings.
   */
  void OnLogVisibilityToggled(bool show);

signals:
  void BatchWindowClosed();

private:
  ptJobListModel *m_BatchModel;
  bool m_WasAborted;
};

//==============================================================================

#endif // PTBATCHWINDOW_H
