/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2013 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011-2013 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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

#include "ptFileMgrWindow.h"
#include "ptGraphicsSceneEmitter.h"
#include "ptRowGridThumbnailLayouter.h"
#include "ptColumnGridThumbnailLayouter.h"
#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "../ptImageHelper.h"
#include "../ptMessageBox.h"
#include "../ptImage8.h"
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QList>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QScrollBar>
#include <QFileDialog>
#include <cassert>

extern void CB_MenuFileOpen(const short HaveFile);

extern ptSettings*  Settings;
extern ptTheme*     Theme;
extern QString      ImageFileToOpen;
extern short        InStartup;
extern QString      SaveBitmapPattern;

//------------------------------------------------------------------------------
/*!
  Creates a ptFileMgrWindow instance.
  \param parent
    The file manager’s parent window.
*/
ptFileMgrWindow::ptFileMgrWindow(QWidget* parent)
: QWidget(parent),
  FDataModel(new ptFileMgrDM(this)),  // setup data model first because several UI elements need it
  FIsFirstShow(true),
  FThumbCount(-1),
  FThumbListIdx(0)
{
  // Main UI init
  setupUi(this);
  setMouseTracking(true);

  ptGraphicsSceneEmitter::ConnectThumbnailAction(
      this, SLOT(execThumbnailAction(ptThumbnailAction,QString)));
  ptGraphicsSceneEmitter::ConnectFocusChanged(this, SLOT(thumbFocusChanged()));

  //-------------------------------------

  // sidebar
  FMSidebar->setVisible(Settings->GetInt("FileMgrShowSidebar"));

  // Folder list
#ifdef Q_OS_WIN
  DirListLabel->setText(tr("Folders"));
#else
  DirListLabel->setText(tr("Directories"));
#endif
  m_DirList->setModel(FDataModel->dirModel());
  connect(m_DirList, SIGNAL(activated(QModelIndex)), this, SLOT(changeListDir(QModelIndex)));


#ifdef Q_OS_WIN
  QString BookmarkTooltip = tr("Bookmark current folder (Ctrl+B)");
#else
  QString BookmarkTooltip = tr("Bookmark current directory (Ctrl+B)");
#endif

  // bookmark list in sidebar
  FTagList = new ptTagList(this);
  FTagList->setModel(FDataModel->tagModel());
  connect(FDataModel->tagModel(), SIGNAL(itemChanged(QStandardItem*)),
          this, SLOT(bookmarkDataChanged(QStandardItem*)));
  m_TagPaneLayout->addWidget(FTagList);
  connect(FTagList, SIGNAL(activated(QModelIndex)), this, SLOT(changeToBookmark(QModelIndex)));
  m_AddBookmarkButton->setToolTip(BookmarkTooltip);
  connect(m_AddBookmarkButton, SIGNAL(clicked()), this, SLOT(bookmarkCurrentDir()));

  //-------------------------------------

  //bookmark menu
  FTagMenu = new QMenu(this);
  FTagMenu->hide();
  FTagMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  FTagMenu->setObjectName("FMTagMenu");

  QLabel* label = new QLabel("<b>" + tr("Bookmarks") + "</b>", FTagMenu);
  QToolButton* addButton = new QToolButton(FTagMenu);
  addButton->setIcon(QIcon(Theme->IconAddBookmark));
  addButton->setToolTip(BookmarkTooltip);
  connect(addButton, SIGNAL(clicked()), this, SLOT(bookmarkCurrentDir()));

  QHBoxLayout* headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(0,0,0,0);
  headerLayout->addWidget(addButton);
  headerLayout->addWidget(label);

  FTagMenuList = new ptTagList(FTagMenu);
  FTagMenuList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  FTagMenuList->setModel(FDataModel->tagModel());
  connect(FTagMenuList, SIGNAL(activated(QModelIndex)),
          this, SLOT(changeToBookmarkFromMenu(QModelIndex)));

  QVBoxLayout* layout = new QVBoxLayout(FTagMenu);
  layout->addLayout(headerLayout);
  layout->addWidget(FTagMenuList);
  FTagMenu->setLayout(layout);

  //-------------------------------------

  // Setup the graphics view/scene
  FFilesScene = new QGraphicsScene(m_FilesView);
  FFilesScene->setStickyFocus(true);
  FFilesScene->installEventFilter(this);
  m_FilesView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  m_FilesView->installEventFilter(this);
  m_FilesView->verticalScrollBar()->installEventFilter(this);
  m_FilesView->horizontalScrollBar()->installEventFilter(this);
  m_FilesView->setScene(FFilesScene);
  FDataModel->connectThumbGen(this, SLOT(receiveThumb(uint,TThumbPtr)));
  setLayouter((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType"));

  FPathBar = new ptPathBar(m_PathContainer);
  FPathBar->setObjectName("FMPathBar");
  connect(FPathBar, SIGNAL(changedPath(QString)), this, SLOT(changeDir(QString)));
  m_PathLayout->addWidget(FPathBar);
  m_Progressbar->hide();

  //-------------------------------------

  // construct the image view
  FImageView = new ptImageView(FMImageViewPane);
  FImageView->installEventFilter(this);
  FMImageViewPane->setVisible((bool)Settings->GetInt("FileMgrShowImageView"));

  //-------------------------------------

  // Filemgr main splitter layout
  if (Settings->m_IniSettings->contains("FileMgrMainSplitter")) {
    FMMainSplitter->restoreState(
        Settings->m_IniSettings->value("FileMgrMainSplitter").toByteArray());
  } else {
    FMMainSplitter->setSizes(QList<int>() << 150 << 500 << 500);
  }
  FMMainSplitter->setStretchFactor(1,1);

  // sidebar splitter layout
  if (Settings->m_IniSettings->contains("FileMgrSidebarSplitter")) {
    FMSidebarSplitter->restoreState(
        Settings->m_IniSettings->value("FileMgrSidebarSplitter").toByteArray());
  } else {
    FMSidebarSplitter->setSizes(QList<int>() << 400 << 400);
  }
  FMSidebarSplitter->setStretchFactor(1,1);

  //-------------------------------------

  constructContextMenu();
}

//------------------------------------------------------------------------------
/*! Destroys a \c ptFileMgrWindow instance. */
ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation", FDataModel->currentDir());
  Settings->m_IniSettings->
      setValue("FileMgrMainSplitter", FMMainSplitter->saveState());
  Settings->m_IniSettings->
      setValue("FileMgrSidebarSplitter", FMSidebarSplitter->saveState());

  DelAndNull(FLayouter);
  DelAndNull(FPathBar);

  // Make sure to destroy all thumbnail related things before the singletons!
  ptGraphicsSceneEmitter::DestroyInstance();

  // context menu actions
  DelAndNull(FVerticalThumbsAct);
  DelAndNull(FHorizontalThumbsAct);
  DelAndNull(FDetailedThumbsAct);
  DelAndNull(FDirThumbsAct);
  DelAndNull(FThumbLayoutGroupAct);
  DelAndNull(FToggleSidebarAct);
  DelAndNull(FToggleImageViewAct);
  DelAndNull(FCloseFileMgrAct);
  DelAndNull(FSaveThumbAct);

  DelAndNull(FImageView);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::setLayouter(const ptThumbnailLayout layout) {
  bool RestartThumbnailer = false;
  if (!InStartup) {
    if ((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType") == layout)
      return;
    RestartThumbnailer = FDataModel->thumbGenRunning();
    FDataModel->abortThumbGen();
    DelAndNull(FLayouter);
    Settings->SetValue("FileMgrThumbLayoutType", layout);
  }

  switch (layout) {
    case tlVerticalByRow:
      m_FilesView->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
      FLayouter = new ptRowGridThumbnailLayouter(m_FilesView);
      break;

    case tlHorizontalByColumn:
      m_FilesView->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      FLayouter = new ptColumnGridThumbnailLayouter(m_FilesView);
      break;

    case tlDetailedList:
      m_FilesView->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
//TODO: re-enable      m_Layouter = new ptDetailedThumbnailLayouter(m_FilesView);
      break;

    default:
      assert(!"Unhandled thumbnail layouter type!");
      break;
  }

  if (RestartThumbnailer) {
    this->displayThumbnails();
  } else {
    this->layoutAll();
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeListDir(const QModelIndex& index) {
  FDataModel->dirModel()->ChangeDir(index);
  displayThumbnails(FDataModel->dirModel()->absolutePath(), FDataModel->dirModel()->pathType());
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeToBookmark(const QModelIndex& index) {
  changeDir(FDataModel->tagModel()->path(index));
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeToBookmarkFromMenu(const QModelIndex& index) {
  FTagMenu->hide();
  changeToBookmark(index);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeDir(const QString& path) {
  FDataModel->dirModel()->ChangeAbsoluteDir(path);
  displayThumbnails(path, FDataModel->dirModel()->pathType());
}

//------------------------------------------------------------------------------
/*!
  Creates or refreshes the thumbnail display.
  \param path
    The path to the desired directory. Must be an absolute path. \c path can be
    empty. Then it defaults to the currently set thumbnail directory.
  \param fsoType
    Only relevant on Windows to indicate if the folder is “My Computer”. If that is
    the case, set to \c fsoRoot. Then \c path will be ignored and no thumbnails
    displayed. Do \b not use as a general flag to prevent thumbnail display!
*/
#ifdef Q_OS_UNIX
// Linux does not need parameter fsoType. Disable compiler warning.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif
void ptFileMgrWindow::displayThumbnails(QString path /*= ""*/, ptFSOType fsoType /*= fsoDir*/) {
  if (path.isEmpty()) {
    path = FDataModel->dirModel()->absolutePath();
    fsoType = FDataModel->dirModel()->pathType();
  }

#ifdef Q_OS_WIN
  if (fsoType == fsoRoot) {
    // We are in “My Computer”
    path = MyComputerIdString;
  }
#endif

  m_FilesView->horizontalScrollBar()->setValue(0);
  m_FilesView->verticalScrollBar()->setValue(0);
  FThumbListIdx = 0;
  FThumbCount = FDataModel->setThumDir(path);
  FThumbsReceived = 0;
  FPathBar->setPath(path);

  if (FThumbCount == 0) {
    // setting scene to null dimensions disappears unneeded scrollbars
    FFilesScene->setSceneRect(0,0,0,0);

  } else {
    FLayouter->LazyInit(FThumbCount);
    this->initProgressbar();
    FDataModel->populateThumbs(FFilesScene);  // non-blocking, returns almost immediately
    this->layoutAll();
  }
}
#ifdef Q_OS_UNIX
#pragma GCC diagnostic pop
#endif

//------------------------------------------------------------------------------
// Slot to receive thumbnail images from the generator and dispatch them to their thumb group.
// Also updates the progress bar.
void ptFileMgrWindow::receiveThumb(uint AReceiverId, TThumbPtr AImage) {
  for (ptGraphicsThumbGroup* hThumbGroup: *FDataModel->thumbGroupList()) {
    if (hThumbGroup->id() == AReceiverId) {
      hThumbGroup->addImage(AImage);
      ++FThumbsReceived;
      this->updateProgressbar();
      break;
    }
  }
}

//------------------------------------------------------------------------------
// Sets progress bar range to current FThumbCount, shows the progressbar and hides the path bar.
void ptFileMgrWindow::initProgressbar() {
  m_Progressbar->setValue(0);
  m_Progressbar->setMaximum(FThumbCount);
  m_Progressbar->show();
  m_PathContainer->hide();
}

//------------------------------------------------------------------------------
// Updates the progress bar’s value. Switches display back to path bar when last thumbnail is reached.
void ptFileMgrWindow::updateProgressbar() {
  if (FThumbsReceived < FThumbCount) {
    m_Progressbar->setValue(FThumbsReceived);
  } else {
    m_Progressbar->hide();
    m_PathContainer->show();

    FFilesScene->setFocus();
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::layoutAll() {
  FLayouter->Init(FDataModel->thumbGroupList()->count(), m_FilesView->font());
  QListIterator<ptGraphicsThumbGroup*> i(*FDataModel->thumbGroupList());

  while (i.hasNext()) {
    FLayouter->Layout(i.next());
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::showEvent(QShowEvent* event) {
  if (FIsFirstShow) {
    // Execute once when the file manager is opened for the first time.

    // Theme and layout stuff (wouldn’t work in constructor)
    updateTheme();

    // set initally selected directory
    QString lastDir = Settings->GetString("LastFileMgrLocation");
    ptFSOType lastDirType = fsoDir;
#ifdef Q_OS_WIN
    if (lastDir == MyComputerIdString || lastDir.isEmpty()) {
      lastDir = MyComputerIdString;
      lastDirType = fsoRoot;
    } else {
#endif
      if (lastDir.isEmpty() || !QDir(lastDir).exists()) {
        // dir from ini is broken, default to homePath
        lastDir = QDir::homePath();
      }
#ifdef Q_OS_WIN
    }
#endif
    FDataModel->dirModel()->ChangeAbsoluteDir(lastDir);
    FDataModel->setCurrentDir(lastDir);
    FPathBar->setPath(lastDir);

    // First call base class showEvent, then start thumbnail loading to ensure
    // the file manager is visible before ressource heavy actions begin.
    QWidget::showEvent(event);
    if (!InStartup) {
      this->displayThumbnails(lastDir, lastDirType);
    }

    FIsFirstShow = false;
    return;
  }

  focusThumbnail(FDataModel->focusedThumb());
  if (!event->spontaneous()) {
    QWidget::showEvent(event);
    // Thumbnails are cleared to free memory when the fm window is closed,
    // i.e. we need to refresh the display when opening it again.
// temporarily disabled. should become a user option
//    displayThumbnails();
  }
}

//------------------------------------------------------------------------------
bool ptFileMgrWindow::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_FilesView && event->type() == QEvent::Resize) {
    // Resize event: Rearrange thumbnails when size of viewport changes
    layoutAll();
    return false;   // handle event further
  }


  else if ((obj == qobject_cast<QObject*>(m_FilesView->verticalScrollBar()) ||
            obj == qobject_cast<QObject*>(m_FilesView->horizontalScrollBar())) &&
            event->type() == QEvent::Wheel)
  { // Wheel event
    int dir = ((QWheelEvent*)event)->delta() > 0 ? -1 : 1;
    if (m_FilesView->verticalScrollBar()->isVisible()) {
      m_FilesView->verticalScrollBar()->setValue(
            m_FilesView->verticalScrollBar()->value() + FLayouter->Step()*dir);

    } else if (m_FilesView->horizontalScrollBar()->isVisible()) {
      m_FilesView->horizontalScrollBar()->setValue(
            m_FilesView->horizontalScrollBar()->value() + FLayouter->Step()*dir);
    }
    return true;    // prevent further event handling
  }


  else if (obj == FFilesScene && (event->type() == QEvent::GraphicsSceneDragEnter ||
                                   event->type() == QEvent::GraphicsSceneDrop))
  { // Make sure drag&drop events are passed on to MainWindow
    event->ignore();
    return true;
  }


  else if (obj == FFilesScene && event->type() == QEvent::KeyPress) {
    // Keyboard navigation in thumbnail list
    int newIdx = FLayouter->MoveIndex(FDataModel->focusedThumb(), (QKeyEvent*)event);
    if (newIdx >= 0) {
      focusThumbnail(newIdx);
      return true;
    }
  }


  // unhandled events fall through to here
  // make sure parent event filters are executed
  return QWidget::eventFilter(obj, event);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::focusThumbnail(int index) {
  if (index >= 0) {
    // focus new thumb
    ptGraphicsThumbGroup* thumb = FDataModel->moveFocus(index);
    FFilesScene->setFocusItem(thumb);
    m_FilesView->ensureVisible(thumb, 0, 0);
    m_FilesView->setFocus();

    // if a different thumb is focused update ImageView
    if (thumb->fsoType() == fsoFile) {
      this->loadForImageView(thumb->fullPath());
    }

  } else {
    FFilesScene->clearFocus();
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::loadForImageView(const QString& AFilePath) {
  if (FImageView->isVisible()) {
    FImageView->showImage(AFilePath);
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::thumbFocusChanged() {
  focusThumbnail(FDataModel->focusedThumb(FFilesScene->focusItem()));
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::saveThumbnail() {
  int hThumbIdx = FDataModel->focusedThumb();

  if (hThumbIdx > -1 && (hThumbIdx < FDataModel->thumbGroupList()->count())) {
    QString   hFileName = FDataModel->thumbGroupList()->at(hThumbIdx)->fullPath();
    auto      hImage = FDataModel->getThumb(hFileName, Settings->GetInt("FileMgrThumbSaveSize"));

    if (!hImage) {
      ptMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Thumbnail could not be saved."));
      return;
    }

    QFileInfo hPathInfo(hFileName);
    QString   hSuggestedFileName = hPathInfo.dir().path() + "/" + hPathInfo.completeBaseName() + "-thumb.jpg";

    auto hOutputName = QFileDialog::getSaveFileName(nullptr,
                                                    QObject::tr("Save File"),
                                                    hSuggestedFileName,
                                                    SaveBitmapPattern);

    if (hOutputName.isEmpty()) {
      return; // Operation cancelled.
    }

    if (!(hImage->DumpImage(hOutputName.toLocal8Bit().data(), true))) {
      ptMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Thumbnail could not be saved."));
    }

    if (!ptImageHelper::TransferExif(hFileName, hOutputName)) {
      ptMessageBox::warning(0, QObject::tr("Exif error"), QObject::tr("Exif data could not be written."));
    }
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::execThumbnailAction(const ptThumbnailAction action, const QString location) {
  if (action == tnaLoadImage) {
    closeWindow();
    ImageFileToOpen = location;
    CB_MenuFileOpen(1);

  } else if (action == tnaChangeDir) {
    FDataModel->dirModel()->ChangeAbsoluteDir(location);
    displayThumbnails(location, FDataModel->dirModel()->pathType());

  } else if (action == tnaViewImage) {
    this->loadForImageView(location);
  }
}

//------------------------------------------------------------------------------
/*! Updates the file manager’s visual appearance. Call this once every time Photivo’s theme changes. */
void ptFileMgrWindow::updateTheme() {
  // Update file manager window appearance
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());

  // Thumbgroups don’t need to be updated because Photivo theme can only be changed
  // while fm is closed = thumbnail display cleared
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::closeWindow() {
  // File manager can’t close its window itself. That’s handled by the main window.
  // Signal tells main window to perform necessary closing actions.
  // FM internal cleanup is done in ptFileMgrWindow::hideEvent().
  emit fileMgrWindowClosed();
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::hideEvent(QHideEvent* event) {
  // temporarily disabled. should become a user-option
//  if (!event->spontaneous()) {
//    event->accept();
//    // free memory occupied by thumbnails and thumb cache
//    // clear() includes stopping thumbnail generation
//    FDataModel->clear();
//    FImageView->clear();
//    this->clearScene();
//  } else {
//    event->ignore();
//  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::clearScene() {
  FDataModel->thumbGroupList()->clear();
  FFilesScene->clear();
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::keyPressEvent(QKeyEvent* event) {
  // Esc: close file manager
  if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
    closeWindow();
  }
  // Ctrl+M: close file manager
  else if (event->key() == Qt::Key_M && event->modifiers() == Qt::ControlModifier) {
    closeWindow();
  }
  // F5: refresh thumbnails
  else if (event->key() == Qt::Key_F5 && event->modifiers() == Qt::NoModifier) {
    displayThumbnails();
  }
  // Shift+F5: clear cache and refresh thumbnails
  else if (event->key() == Qt::Key_F5 && event->modifiers() == Qt::ShiftModifier) {
    FDataModel->clear();
    displayThumbnails();
  }
  // Ctrl+B: bookmark current folder
  else if (event->key() == Qt::Key_B && event->modifiers() == Qt::ControlModifier) {
    bookmarkCurrentDir();
  }
  // F11: toggles fullscreen (handled by main window)
  else if (event->key() == Qt::Key_F11 && event->modifiers() == Qt::NoModifier) {
    event->ignore();
  }
  // Alt+1: vertical thumbnail view
  else if (event->key() == Qt::Key_1 && event->modifiers() == Qt::AltModifier) {
    setLayouter(tlVerticalByRow);
  }
  // Alt+2: horizontal thumbnail view
  else if (event->key() == Qt::Key_2 && event->modifiers() == Qt::AltModifier) {
    setLayouter(tlHorizontalByColumn);
  }
  // Alt+3: detailed thumbnail view
//TODO: re-enable  else if (event->key() == Qt::Key_3 && event->modifiers() == Qt::AltModifier) {
//    setLayouter(tlDetailedList);
//  }
  // F3: toggles: ImageView
  else if (event->key() == Qt::Key_F3 && event->modifiers() == Qt::NoModifier) {
    toggleImageView();
  }
  // F4: toggles sidebar
  else if (event->key() == Qt::Key_F4 && event->modifiers() == Qt::NoModifier) {
    toggleSidebar();
  }

  else if (event->modifiers() == Qt::NoModifier) {
    // Keyboard actions for image viewer
    switch (event->key()) {
      case Qt::Key_1: FImageView->zoomIn();  break;
      case Qt::Key_2: FImageView->zoom100(); break;
      case Qt::Key_3: FImageView->zoomOut(); break;
      case Qt::Key_4: FImageView->zoomFit(); break;
      default: break;
    }
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::constructContextMenu() {
  // Actions for thumbnail view submenu
  FVerticalThumbsAct = new QAction(tr("&Vertical") + "\t" + tr("Alt+1"), this);
  FVerticalThumbsAct->setCheckable(true);
  connect(FVerticalThumbsAct, SIGNAL(triggered()), this, SLOT(verticalThumbs()));

  FHorizontalThumbsAct = new QAction(tr("&Horizontal") + "\t" + tr("Alt+2"), this);
  FHorizontalThumbsAct->setCheckable(true);
  connect(FHorizontalThumbsAct, SIGNAL(triggered()), this, SLOT(horizontalThumbs()));

  FDetailedThumbsAct = new QAction(tr("&Details") + "\t" + tr("Alt+3"), this);
  FDetailedThumbsAct->setCheckable(true);
  connect(FDetailedThumbsAct, SIGNAL(triggered()), this, SLOT(detailedThumbs()));

#ifdef Q_OS_WIN
  FDirThumbsAct = new QAction(tr("Show &folder thumbnails"), this);
#else
  FDirThumbsAct = new QAction(tr("Show &directory thumbnails"), this);
#endif
  FDirThumbsAct->setCheckable(true);
  connect(FDirThumbsAct, SIGNAL(triggered()), this, SLOT(toggleDirThumbs()));

  FThumbLayoutGroupAct = new QActionGroup(this);
  FThumbLayoutGroupAct->setExclusive(true);
  FThumbLayoutGroupAct->addAction(FVerticalThumbsAct);
  FThumbLayoutGroupAct->addAction(FHorizontalThumbsAct);
//TODO: re-enable  ac_ThumbLayoutGroup->addAction(ac_DetailedThumbs);

  // actions for main context menu
  FToggleImageViewAct = new QAction(tr("Show &image preview") + "\t" + tr("F3"), this);
  FToggleImageViewAct->setCheckable(true);
  connect(FToggleImageViewAct, SIGNAL(triggered()), this, SLOT(toggleImageView()));

  FToggleSidebarAct = new QAction(tr("Show &sidebar") + "\t" + tr("F4"), this);
  FToggleSidebarAct->setCheckable(true);
  connect(FToggleSidebarAct, SIGNAL(triggered()), this, SLOT(toggleSidebar()));

  FSaveThumbAct = new QAction(tr("&Save thumbnail"), this);
  connect(FSaveThumbAct, SIGNAL(triggered()), this, SLOT(saveThumbnail()));

  FCloseFileMgrAct = new QAction(tr("&Close file manager") + "\t" + tr("Esc"), this);
  connect(FCloseFileMgrAct, SIGNAL(triggered()), this, SLOT(closeWindow()));

  FToggleShowRAWsAct = new QAction(tr("Show RAWs"), this);
  FToggleShowRAWsAct->setCheckable(true);
  connect(FToggleShowRAWsAct, SIGNAL(triggered()), this, SLOT(toggleShowRAWs()));

  FToggleShowBitmapsAct = new QAction(tr("Show bitmaps"), this);
  FToggleShowBitmapsAct->setCheckable(true);
  connect(FToggleShowBitmapsAct, SIGNAL(triggered()), this, SLOT(toggleShowBitmaps()));
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::contextMenuEvent(QContextMenuEvent* event) {
  ptThumbnailLayout currLayout = (ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType");

  // thumbnail view submenu
  QMenu MenuThumbLayout(tr("Thumbnail &view"));
  MenuThumbLayout.setPalette(Theme->menuPalette());
  MenuThumbLayout.setStyle(Theme->style());
  MenuThumbLayout.addActions(FThumbLayoutGroupAct->actions());
  MenuThumbLayout.addSeparator();
  MenuThumbLayout.addAction(FDirThumbsAct);
  FVerticalThumbsAct->setChecked(currLayout == tlVerticalByRow);
  FHorizontalThumbsAct->setChecked(currLayout == tlHorizontalByColumn);
  FDetailedThumbsAct->setChecked(currLayout == tlDetailedList);
  FDirThumbsAct->setChecked(Settings->GetInt("FileMgrShowDirThumbs"));

  // main context menu
  QMenu Menu(NULL);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  Menu.addMenu(&MenuThumbLayout);

  Menu.addSeparator();
  Menu.addAction(FToggleImageViewAct);
  FToggleImageViewAct->setChecked(FMImageViewPane->isVisible());
  Menu.addAction(FToggleSidebarAct);
  FToggleSidebarAct->setChecked(FMSidebar->isVisible());

  Menu.addSeparator();
  Menu.addAction(FToggleShowRAWsAct);
  FToggleShowRAWsAct->setChecked(Settings->GetInt("FileMgrShowRAWs"));
  Menu.addAction(FToggleShowBitmapsAct);
  FToggleShowBitmapsAct->setChecked(Settings->GetInt("FileMgrShowBitmaps"));

  Menu.addSeparator();
  Menu.addAction(FSaveThumbAct);

  Menu.addSeparator();
  Menu.addAction(FCloseFileMgrAct);

  Menu.exec(event->globalPos());
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::verticalThumbs() {
  setLayouter(tlVerticalByRow);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::horizontalThumbs() {
  setLayouter(tlHorizontalByColumn);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::detailedThumbs() {
  setLayouter(tlDetailedList);
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::toggleSidebar() {
  FMSidebar->setVisible(!FMSidebar->isVisible());
  Settings->SetValue("FileMgrShowSidebar", (int)FMSidebar->isVisible());
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::toggleImageView() {
  FMImageViewPane->setVisible(!FMImageViewPane->isVisible());
  Settings->SetValue("FileMgrShowImageView", (int)FMImageViewPane->isVisible());
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::toggleDirThumbs() {
  Settings->SetValue("FileMgrShowDirThumbs", 1 - Settings->GetInt("FileMgrShowDirThumbs"));
  displayThumbnails();
}

void ptFileMgrWindow::toggleShowRAWs() {
  Settings->SetValue("FileMgrShowRAWs", 1 - Settings->GetInt("FileMgrShowRAWs"));
  displayThumbnails();
}

void ptFileMgrWindow::toggleShowBitmaps() {
  Settings->SetValue("FileMgrShowBitmaps", 1 - Settings->GetInt("FileMgrShowBitmaps"));
  displayThumbnails();
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::bookmarkCurrentDir() {
  FDataModel->tagModel()->appendRow(QDir::toNativeSeparators(FDataModel->currentDir()),
                                     FDataModel->currentDir());
  if (FTagMenu->isVisible()) {
    adjustBookmarkMenuSize();
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::on_m_BookmarkButton_clicked() {
  FTagMenu->move(m_BookmarkButton->mapToGlobal(QPoint(0, m_BookmarkButton->height())));
  FTagMenu->setPalette(Theme->menuPalette());
  FTagMenu->setStyle(Theme->style());
  FTagMenu->show();  // must be first or adjust size won’t work correctly
  adjustBookmarkMenuSize();

  FTagMenuList->setFocus();
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::adjustBookmarkMenuSize() {
  QSize MenuSize(0, 0);
  QPoint MenuTopleft = m_BookmarkButton->mapTo(this, QPoint(0, m_BookmarkButton->height()));
  QSize MaxSize((this->width() - MenuTopleft.x()) * 0.85,
                (this->height() - MenuTopleft.y()) * 0.85);

  QFontMetrics metrics(FTagMenuList->font());
  for (int i = 0; i < FDataModel->tagModel()->rowCount(); i++) {
    QModelIndex curIndex = FDataModel->tagModel()->index(i, 0);
    int width = metrics.width(curIndex.data().toString());
    if (width > MenuSize.width()) MenuSize.setWidth(width);
    MenuSize.setHeight(MenuSize.height() + FTagMenuList->visualRect(curIndex).height());
  }

  FTagMenu->setFixedSize(qBound(150,
                                 MenuSize.width() + 20 + FTagMenuList->verticalScrollBar()->width(),
                                 MaxSize.width()),
                          qBound(FTagMenuList->y() + 50,
                                 MenuSize.height() + FTagMenuList->y() + 30,
                                 MaxSize.height()) );
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::bookmarkDataChanged(QStandardItem*) {
  if (FTagMenu->isVisible()) {
    adjustBookmarkMenuSize();
  }
}

