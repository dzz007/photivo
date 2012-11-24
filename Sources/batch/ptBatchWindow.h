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

#include <QAbstractTableModel>
#include <QProcess>
#include "ui_ptBatchWindow.h"
#include "ptJobListModel.h"

//==============================================================================

/*! This is the window corresponding to the batch manager */
class ptBatchWindow : public QWidget, private Ui::ptBatchWindow
{
  Q_OBJECT
  
public:
  explicit ptBatchWindow(QWidget *parent = 0);
  ~ptBatchWindow();

  void UpdateTheme();

protected:
  void keyPressEvent(QKeyEvent *event);

private slots:
  void OnAddJob();
  void OnRemoveJob();
  void OnCloseWindow();
  void OnRunNextJob();
  void OnRunJobs();
  void OnAbortProcessing();
  void OnLogReady();
  void OnLogVisibilityToggled(bool show);

signals:
  void BatchWindowClosed();

private:
  ptJobListModel *m_BatchModel;
  ptJobListItem *m_CurrentJob;
  QProcess *m_BatchProcess;
  bool m_WasAborted;
};

//==============================================================================

#endif // PTBATCHWINDOW_H
