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

#include "ptBatchWindow.h"
#include "ptSettings.h"
#include "ptTheme.h"

//==============================================================================

ptBatchWindow::ptBatchWindow(QWidget *parent) :
  QWidget(parent)
{
  setupUi(this);
  UpdateTheme();

  m_CurrentJob = nullptr;
  m_WasAborted = false;
  BTAbort->setEnabled(false);

  m_BatchModel = new ptJobListModel(this);
  BTJobList->setModel(m_BatchModel);
  BTJobList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

  m_BatchProcess = new QProcess(this);
  connect(m_BatchProcess, SIGNAL(finished(int)), SLOT(OnRunNextJob()));
  connect(m_BatchProcess, SIGNAL(readyRead()), SLOT(OnLogReady()));

  connect(BTShowLog, SIGNAL(toggled(bool)), SLOT(OnLogVisibilityToggled(bool)));
  BTShowLog->setChecked(Settings->GetInt("BatchLogIsVisible"));
  BTLog->setShown(Settings->GetInt("BatchLogIsVisible"));

  BTLogSplitter->setStretchFactor(BTLogSplitter->indexOf(BTJobList), 2);
  BTLogSplitter->setStretchFactor(BTLogSplitter->indexOf(BTLog), 1);
}

//==============================================================================

ptBatchWindow::~ptBatchWindow()
{
  delete m_BatchModel;
  delete m_BatchProcess;
  delete m_CurrentJob;
}

//==============================================================================

void ptBatchWindow::UpdateTheme()
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());
}

//==============================================================================

void ptBatchWindow::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier)
    OnCloseWindow();
}

//==============================================================================

void ptBatchWindow::OnAddJob()
{
  QString SettingsFilePattern =
    QObject::tr("Settings files (*.pts *.ptj);;All files (*.*)");
  QStringList SettingsFileNames =
    QFileDialog::getOpenFileNames(nullptr,
                                 QObject::tr("Open setting files"),
                                 Settings->GetString("RawsDirectory"),
                                 SettingsFilePattern);
  if (SettingsFileNames.isEmpty()) return;

  Settings->SetValue("RawsDirectory", QFileInfo(SettingsFileNames.first()).absolutePath());
  foreach (QString fileName, SettingsFileNames)
    m_BatchModel->AddJob(fileName);

  BTJobList->resizeColumnsToContents();
}

//==============================================================================

void ptBatchWindow::OnRemoveJob()
{
  QList<int> list;
  foreach (QModelIndex idx, BTJobList->selectionModel()->selectedRows())
    list.append(idx.row());

//  sort in reverse order
  qSort(list.begin(), list.end(), qGreater<int>());
  // we should take into account that indexes will change after some of them are removed
  foreach (int i, list)
    m_BatchModel->RemoveJob(i);
}

//==============================================================================

void ptBatchWindow::OnCloseWindow()
{
  emit BatchWindowClosed();
}

//==============================================================================

void ptBatchWindow::OnRunNextJob()
{
  if (m_WasAborted)
    return;

  int i;
  for (i = 0; i < BTJobList->model()->rowCount(); i++) {
    m_CurrentJob = qobject_cast<ptJobListModel*>(BTJobList->model())->JobItem(i);
    if (m_CurrentJob->Status() == jsWaiting) {
      QString fileName = m_CurrentJob->GetCallFileName();
      if (fileName.isEmpty()) {
//        invalid job
        m_CurrentJob->SetStatus(jsSkipped);
        continue;
      }

      QString photivoApp = QCoreApplication::applicationFilePath();
      QStringList arguments = QStringList() << "-j" << fileName;
      m_BatchProcess->start(photivoApp, arguments, QIODevice::ReadOnly);
      connect(m_BatchProcess, SIGNAL(finished(int)), m_CurrentJob, SLOT(OnProcessingFinished(int)));

      m_CurrentJob->SetStatus(jsProcessing);
      break;
    }
  }

  if (i == BTJobList->model()->rowCount()) {
//    all jobs have been processed
    BTRun->setEnabled(true);
    BTAbort->setEnabled(false);
  }
  else
    BTLog->clear();
}

//==============================================================================

void ptBatchWindow::OnRunJobs()
{
  for (int i=0; i < BTJobList->model()->rowCount(); i++)
    if (qobject_cast<ptJobListModel*>(BTJobList->model())->JobItem(i)->Status() != jsFinished)
      qobject_cast<ptJobListModel*>(BTJobList->model())->JobItem(i)->SetStatus(jsWaiting);

  m_WasAborted = false;
  BTRun->setEnabled(false);
  BTAbort->setEnabled(true);
  OnRunNextJob();
}

//==============================================================================

void ptBatchWindow::OnAbortProcessing()
{
  m_BatchProcess->terminate();
  m_CurrentJob->SetStatus(jsAborted);

  m_WasAborted = true;
  BTRun->setEnabled(true);
  BTAbort->setEnabled(false);
}

//==============================================================================

void ptBatchWindow::OnLogReady()
{
  BTLog->appendPlainText(m_BatchProcess->readAll().data());
}

//==============================================================================

void ptBatchWindow::OnLogVisibilityToggled(bool show)
{
  BTLog->setShown(show);
  Settings->SetValue("BatchLogIsVisible", int(show));
}

//==============================================================================
