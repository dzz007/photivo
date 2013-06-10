/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
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

#include "ui_ptFileMgrWindow.h"
#include "ptFileMgrDM.h"
#include "ptGraphicsThumbGroup.h"
#include "ptAbstractThumbnailLayouter.h"
#include "ptFileMgrConstants.h"
#include "ptImageView.h"
#include "ptPathBar.h"
#include "ptTagList.h"
#include "../ptConstants.h"
#include "../ptReportOverlay.h"
#include "../ptConstants.h"
#include <QWidget>
#include <QGraphicsScene>
#include <memory>

class ptImage8;

//------------------------------------------------------------------------------

class ptFileMgrWindow: public QWidget, private Ui::ptFileMgrWindow {
Q_OBJECT

public:
  explicit ptFileMgrWindow(QWidget* parent = nullptr);
  ~ptFileMgrWindow();

  void displayThumbnails(QString path = "", ptFSOType fsoType = fsoDir);
  void updateTheme();

signals:
  void fileMgrWindowClosed();

protected:
  void contextMenuEvent(QContextMenuEvent* event);
  bool eventFilter(QObject* obj, QEvent* event);
  void hideEvent(QHideEvent* event);
  void keyPressEvent(QKeyEvent* event);
  void showEvent(QShowEvent* event);

private:
  void adjustBookmarkMenuSize();
  void clearScene();
  void focusThumbnail(int index);
  void layoutAll();
  void setLayouter(const ptThumbnailLayout layout);
  void constructContextMenu();

  ptFileMgrDM*            FDataModel;
  QGraphicsScene*         FFilesScene;
  bool                    FIsFirstShow;
  ptAbstractThumbnailLayouter* FLayouter;
  ptPathBar*              FPathBar;
  ptTagList*              FTagList;      // bookmarks in sidebar
  ptTagList*              FTagMenuList;  // bookmarks in popup menu
  QMenu*                  FTagMenu;
  int                     FThumbCount;
  int                     FThumbListIdx;
  ptImageView*            FImageView;

  // context menu actions
  QAction*      FVerticalThumbsAct;
  QAction*      FHorizontalThumbsAct;
  QAction*      FDetailedThumbsAct;
  QAction*      FDirThumbsAct;
  QActionGroup* FThumbLayoutGroupAct;
  QAction*      FToggleSidebarAct;
  QAction*      FToggleImageViewAct;
  QAction*      FCloseFileMgrAct;
  QAction*      FSaveThumbAct;

private slots:
  void bookmarkCurrentDir();
  void bookmarkDataChanged(QStandardItem*);
  void changeListDir(const QModelIndex& index);
  void changeToBookmark(const QModelIndex& index);
  void changeToBookmarkFromMenu(const QModelIndex& index);
  void changeDir(const QString& path);
  void closeWindow();
  void execThumbnailAction(const ptThumbnailAction action, const QString location);
  void receiveThumb(uint AReceiverId, TThumbPtr AImage);
  void on_m_BookmarkButton_clicked();
  void thumbFocusChanged();
  void saveThumbnail();

  // context menu slots
  void verticalThumbs();
  void horizontalThumbs();
  void detailedThumbs();
  void toggleDirThumbs();
  void toggleSidebar();
  void toggleImageView();

};

#endif // PTFILEMGRWINDOW_h
