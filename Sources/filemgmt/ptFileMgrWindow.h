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
#include "ptAbstractThumbnailLayouter.h"

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
  void contextMenuEvent(QContextMenuEvent* event);
  bool eventFilter(QObject* obj, QEvent* event);
  void hideEvent(QHideEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void showEvent(QShowEvent* event);


private:
  void ClearScene();
  void LayoutAll();
  void setLayouter(const ptThumbnailLayout layout);
  void ConstructContextMenu();

  ptFileMgrDM*            m_DataModel;
  QGraphicsScene*         m_FilesScene;
  bool                    m_IsFirstShow;
  ptAbstractThumbnailLayouter* m_Layouter;
  int                     m_ThumbCount;
  int                     m_ThumbListIdx;

  // context menu actions
  QAction* ac_VerticalThumbs;
  QAction* ac_HorizontalThumbs;
  QAction* ac_DetailedThumbs;
  QActionGroup* ac_ThumbLayoutGroup;
  QAction* ac_ToggleNaviPane;
  QAction* ac_CloseFileMgr;


public slots:

private slots:
  void changeTreeDir(const QModelIndex& index);
  void closeWindow();
  void execThumbnailAction(const ptThumbnailAction action, const QString location);
  void fetchNewImages(ptGraphicsThumbGroup* group, QImage* pix);
  void fetchNewThumbs(const bool isLast);
  void on_m_PathInput_returnPressed();

  // context menu slots
  void verticalThumbs();
  void horizontalThumbs();
  void detailedThumbs();
  void toggleNaviPane();

signals:
  void FileMgrWindowClosed();
};
//==============================================================================
#endif // PTFILEMGRWINDOW_h
