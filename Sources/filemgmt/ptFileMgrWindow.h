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
  /*! Creates a \c ptFileMgrWindow instance.
    \param parent
      The file manager’s parent window.
  */
  explicit ptFileMgrWindow(QWidget* parent = 0);

  /*! Destroys a \c ptFileMgrWindow instance. */
  ~ptFileMgrWindow();

  /*! Creates or refreshes the thumbnail display.
    \param index
      The model index corresponding to the desired directory.
    \param clearCache
      Set this flag to \c true if you want to clear the thumbnail cache.
      Default is \c false.
  */
  void DisplayThumbnails(const QModelIndex& index, bool clearCache = false);

  /*! Updates the file manager’s visual appearance.
      Call this once every time Photivo’s theme changes.
  */
  void UpdateTheme();


protected:
  /*! Reimplemented from QObject::eventFilter() */
  bool eventFilter(QObject* obj, QEvent* event);

  /*! Reimplemented from QWidget::keyPressEvent().
    The file manager handles its keys completely on its own. The main window
    is only responsible for the Ctrl+M shortcut to open the manager window.
  */
  void keyPressEvent(QKeyEvent* event);

  /*! Reimplemented from QWidget::showEvent() */
  void showEvent(QShowEvent* event);


private:
  void ArrangeThumbnail(ptGraphicsThumbGroup* thumb);
  void ArrangeThumbnails();
  void CalcThumbMetrics();
  void CloseWindow();

  struct {
    int Col;      // zero based index
    int Row;      // zero based index
    int MaxCol;   // max column index
    int MaxRow;   // max row index
    int Padding;
    int CellHeight;   // cell dimensions include padding
    int CellWidth;    //
  } m_ThumbMetrics;

  ptThumbnailArrangeMode  m_ArrangeMode;
  ptFileMgrDM*            m_DataModel;
  QGraphicsScene*         m_FilesScene;
  bool                    m_IsFirstShow;
  int                     m_ThumbCount;
  int                     m_ThumbListIdx;


public slots:

private slots:
  void changeTreeDir(const QModelIndex& index);
  void execThumbnailAction(const ptThumbnailAction action, const QString location);
  void fetchNewImages(ptGraphicsThumbGroup* group, QImage* pix);
  void fetchNewThumbs(const bool isLast);
  void on_m_PathInput_returnPressed();

signals:
  void FileMgrWindowClosed();
};
//==============================================================================
#endif // PTFILEMGRWINDOW_h
