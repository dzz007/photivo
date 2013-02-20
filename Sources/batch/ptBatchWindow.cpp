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
#include "ptMessageBox.h"
#include "ptSettings.h"
#include "ptTheme.h"

//==============================================================================

ptBatchWindow::ptBatchWindow(QWidget *parent) :
  QWidget(parent)
{
  setupUi(this);
  UpdateTheme();

  m_WasAborted = false;
  BTAbort->setEnabled(false);

  m_BatchModel = new ptJobListModel(this);
  BTJobList->setModel(m_BatchModel);
  BTJobList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

  connect(m_BatchModel->m_BatchProcess, SIGNAL(finished(int)), SLOT(OnRunNextJob()));
  connect(m_BatchModel->m_BatchProcess, SIGNAL(readyRead()), SLOT(OnLogReady()));
  connect(m_BatchModel, SIGNAL(processingAborted()), SLOT(OnAbortProcessing()));
  connect(BTAbort, SIGNAL(clicked()), m_BatchModel, SLOT(OnAbortProcessing()));

  connect(BTShowLog, SIGNAL(toggled(bool)), SLOT(OnLogVisibilityToggled(bool)));
  BTShowLog->setChecked(Settings->GetInt("BatchLogIsVisible"));
  BTLog->setVisible(Settings->GetInt("BatchLogIsVisible"));

  BTLogSplitter->setStretchFactor(BTLogSplitter->indexOf(BTJobList), 2);
  BTLogSplitter->setStretchFactor(BTLogSplitter->indexOf(BTLog), 1);

  BTJobList->resizeColumnsToContents();
}

//==============================================================================

ptBatchWindow::~ptBatchWindow()
{
  delete m_BatchModel;
}

//==============================================================================

void ptBatchWindow::UpdateTheme()
{
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());
}

void ptBatchWindow::AddJobToList(const QString &settingFile, const QString &rawFile)
{
  m_BatchModel->AddJobToList(settingFile, rawFile);

  BTJobList->resizeColumnsToContents();
  m_BatchModel->AutosaveJobList();
}

//==============================================================================

void ptBatchWindow::AddJobsToList(const QStringList &settingFiles)
{
  foreach (QString fileName, settingFiles)
    m_BatchModel->AddJobToList(fileName);

  BTJobList->resizeColumnsToContents();
  m_BatchModel->AutosaveJobList();
}

//==============================================================================

void ptBatchWindow::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
    OnCloseWindow();
  } else if (event->key() == Qt::Key_B && event->modifiers() == Qt::ControlModifier) {
    // We close also on Ctrl+B to allow toggle.
    OnCloseWindow();
  } else if (event->key() == Qt::Key_S && event->modifiers() == Qt::ControlModifier) {
    OnSaveJobList();
  } else if (event->key() == Qt::Key_O && event->modifiers() == Qt::ControlModifier) {
    OnOpenJobList();
  } else if (event->key() == Qt::Key_R && event->modifiers() == Qt::ControlModifier) {
    OnRunJobs();
  } else if (event->key() == Qt::Key_Delete && event->modifiers() == Qt::NoModifier) {
    OnRemoveJob();
  }
}

//==============================================================================

void ptBatchWindow::showEvent(QShowEvent *event)
{
  QWidget::showEvent(event);
  BTJobList->setFocus();
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
  AddJobsToList(SettingsFileNames);
}

//==============================================================================

void ptBatchWindow::OnRemoveJob()
{
  QList<int> list;
  foreach (QModelIndex idx, BTJobList->selectionModel()->selectedRows())
    list.append(idx.row());
  if (list.isEmpty())
    return;

//  sort in reverse order
  qSort(list.begin(), list.end(), qGreater<int>());
  // we should take into account that indexes will change after some of them are removed
  foreach (int i, list)
    m_BatchModel->RemoveJobFromList(i);

  BTJobList->selectRow(list.last());
  m_BatchModel->AutosaveJobList();
}

//==============================================================================

void ptBatchWindow::OnRunJobs()
{
  for (int i=0; i < m_BatchModel->count(); i++)
    if (m_BatchModel->JobItem(i)->Status() != jsFinished)
      m_BatchModel->JobItem(i)->SetStatus(jsWaiting);

  m_WasAborted = false;
  BTRun->setEnabled(false);
  BTAbort->setEnabled(true);
  OnRunNextJob();
}

//==============================================================================

void ptBatchWindow::OnAbortProcessing()
{
  m_WasAborted = true;
  BTRun->setEnabled(true);
  BTAbort->setEnabled(false);
}

//==============================================================================

void ptBatchWindow::OnResetStatus()
{
  foreach (QModelIndex idx, BTJobList->selectionModel()->selectedRows())
    m_BatchModel->JobItem(idx.row())->SetStatus(jsWaiting);
}

//==============================================================================

void ptBatchWindow::OnSaveJobList()
{
  QString batchPattern = tr("Job list files (*.ptb);;All files (*.*)");
  QString batchFileName =
      QFileDialog::getSaveFileName(nullptr,
                                   tr("Save job list file"),
                                   Settings->GetString("RawsDirectory"),
                                   batchPattern);
  if (batchFileName.isEmpty()) return;

  m_BatchModel->SaveToSettings(batchFileName);
}

//==============================================================================

void ptBatchWindow::OnOpenJobList()
{
  QString batchPattern = tr("Job list files (*.ptb);;All files (*.*)");
  QStringList batchFileNames =
      QFileDialog::getOpenFileNames(nullptr,
                                   tr("Open job list files"),
                                   Settings->GetString("RawsDirectory"),
                                   batchPattern);
  if (batchFileNames.isEmpty()) return;

  foreach (QString file, batchFileNames)
    m_BatchModel->LoadFromSettings(file);
  m_BatchModel->AutosaveJobList();
  BTJobList->resizeColumnsToContents();
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

  bool all = m_BatchModel->RunNextJob();

  if (all) {
//    all jobs have been processed
    BTRun->setEnabled(true);
    BTAbort->setEnabled(false);
  } else
    BTLog->clear();
}

//==============================================================================

void ptBatchWindow::OnLogReady()
{
  BTLog->appendPlainText(m_BatchModel->m_BatchProcess->readAll().data());
}

//==============================================================================

void ptBatchWindow::OnLogVisibilityToggled(bool show)
{
  BTLog->setVisible(show);
  Settings->SetValue("BatchLogIsVisible", int(show));
}

//==============================================================================
