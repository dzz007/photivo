/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILEMGRWINDOW_h
#define PTFILEMGRWINDOW_h

//==============================================================================

#include <QWidget>
#include <QGraphicsScene>

#include "../ptConstants.h"
#include "../ptReportOverlay.h"
#include "../ptConstants.h"
#include "ui_ptFileMgrWindow.h"
#include "ptFileMgrDM.h"
#include "ptGraphicsThumbGroup.h"

//==============================================================================

class ptFileMgrWindow: public QWidget, public Ui::ptFileMgrWindow {
Q_OBJECT

public:
  explicit ptFileMgrWindow(QWidget *parent = 0);
  ~ptFileMgrWindow();


protected:
  bool eventFilter(QObject* obj, QEvent* event);
  void showEvent(QShowEvent* event);


private:
  void ArrangeThumbnail(ptGraphicsThumbGroup* thumb);
  void ArrangeThumbnails();
  void CalcThumbMetrics();

  struct {
    int Col;
    int Row;
    int MaxCol;
    int MaxRow;
    int Padding;
    int CellHeight;
    int CellWidth;
  } m_ThumbMetrics;


  ptThumbnailArrangeMode  m_ArrangeMode;
  ptFileMgrDM*            m_DataModel;
  QGraphicsScene*         m_FilesScene;
  bool                    m_IsFirstShow;
  ptReportOverlay*        m_StatusOverlay;
  int                     m_ThumbCount;
  int                     m_ThumbListIdx;


private slots:
  void changeTreeDir(const QModelIndex& index);
  void execThumbnailAction(const ptThumbnailAction action, const QString location);
  void fetchNewPixmaps();
  void fetchNewThumbs(const bool isLast);

signals:
  void FileMgrWindowClosed();
};
//==============================================================================
#endif // PTFILEMGRWINDOW_h
