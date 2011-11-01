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
#include "ptFileMgrConstants.h"
#include "ptImageView.h"

//==============================================================================

class ptFileMgrWindow: public QWidget, private Ui::ptFileMgrWindow {
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
    \param path
      The path to the desired directory. Must be an absolute path. \c path can be
      empty. Then it defaults to the currently set thumbnail directory.
    \param fsoType
      Only relevant on Windows to indicate if the folder is “My Computer”. If that is
      the case, set to \c fsoRoot. Then \c path will be ignored and no thumbnails
      displayed. Do \b not use as a general flag to prevent thumbnail display!
  */
  void DisplayThumbnails(QString path = "", ptFSOType fsoType = fsoDir);

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
  ptImageView*            m_ImageView;

  // context menu actions
  QAction* ac_VerticalThumbs;
  QAction* ac_HorizontalThumbs;
  QAction* ac_DetailedThumbs;
  QAction* ac_DirThumbs;
  QActionGroup* ac_ThumbLayoutGroup;
  QAction* ac_ToggleNaviPane;
  QAction* ac_CloseFileMgr;


public slots:

private slots:
  void changeListDir(const QModelIndex& index);
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
  void toggleDirThumbs();
  void toggleNaviPane();

signals:
  void FileMgrWindowClosed();
};
//==============================================================================
#endif // PTFILEMGRWINDOW_h
