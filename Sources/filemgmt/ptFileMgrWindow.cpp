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
#include "../batch/ptBatchWindow.h"
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
extern void CB_BatchButton();
extern void CB_FullScreenButton(const int State);

extern ptSettings*  Settings;
extern ptTheme*     Theme;
extern QString      ImageFileToOpen;
extern short        InStartup;
extern QString      SaveBitmapPattern;
extern ptBatchWindow*      BatchWindow;

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
  ptGraphicsSceneEmitter::ConnectItemsChanged(this, SLOT(updateThumbList()));

  //-------------------------------------

  // sidebar
  FMSidebar->setVisible(Settings->GetInt("FileMgrShowSidebar"));

  // Folder list
#ifdef Q_OS_WIN
  DirListLabel->setText(tr("Folders"));
#else
  DirListLabel->setText(tr("Directories"));
#endif

// ATZ
  m_DirTree->setModel(FDataModel->fileSystemModel());
  m_DirTree->setHeaderHidden(true);
  m_DirTree->setSortingEnabled(true);
  m_DirTree->setAcceptDrops(true);
  connect(m_DirTree, SIGNAL(activated(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));
  connect(m_DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  ColorLabel1 = new ptColorLabel(ColorLabelWidget1);
  ColorLabel1->setToolTip(tr("Color label"));
  ColorLabel1->setMultiSelect(true);
  StarRating1 = new ptStarRating(StarRatingWidget1);
  StarRating1->setToolTip(tr("Rating"));
  StarRating2 = new ptStarRating(StarRatingWidget2);
  StarRating2->setToolTip(tr("Rating"));
  FileTypeComboBox->addItem(tr("All files"));
  FileTypeComboBox->addItem(tr("RAW files"));
  FileTypeComboBox->addItem(tr("Bitmap files"));
  FileTypeComboBox->addItem(tr("RAW+Bitmap files"));
  FileTypeComboBox->addItem(tr("Deleted files (.trash)"));
  int index = Settings->GetInt("FileMgrShowRAWs") + Settings->GetInt("FileMgrShowBitmaps") * 2;
  FileTypeComboBox->setCurrentIndex(index);
  ColorLabel2 = new ptColorLabel(ColorLabelWidget2);
  ColorLabel2->setToolTip(tr("Color label"));
  StarRating3 = new ptStarRating(StarRatingWidget3);
  StarRating3->setToolTip(tr("Rating"));
  RestoreImageButton->hide();

  connect(ColorLabel1,SIGNAL(valueChanged()),this,SLOT(filterChanged()));
  connect(ColorLabel2,SIGNAL(valueChanged()),this,SLOT(colorLabelChanged()));
  connect(StarRating1,SIGNAL(valueChanged()),this,SLOT(filterChanged()));
  connect(StarRating2,SIGNAL(valueChanged()),this,SLOT(filterChanged()));
  connect(StarRating3,SIGNAL(valueChanged()),this,SLOT(starRatingChanged()));
  connect(FileTypeComboBox,SIGNAL(activated(int)),this,SLOT(fileTypeFilterChanged(int)));

  connect(RestoreImageButton,SIGNAL(clicked()),this,SLOT(OnRestoreImageButtonClicked()));
  connect(DeleteImageButton,SIGNAL(clicked()),this,SLOT(OnDeleteImageButtonClicked()));
  connect(SendToBatchButton,SIGNAL(clicked()),this,SLOT(OnSendToBatchButtonClicked()));
  connect(BatchButton,SIGNAL(clicked()),this,SLOT(OnBatchButtonClicked()));
  connect(ProcessingButton,SIGNAL(clicked()),this,SLOT(OnProcessingButtonClicked()));
  connect(FullScreenButton,SIGNAL(clicked()),this,SLOT(OnFullScreenButtonClicked()));


  QMargins margins = m_ThumbPaneLayout->contentsMargins();
  margins.setRight(0);
  margins.setBottom(0);
  m_ThumbPaneLayout->setContentsMargins(margins);
// end ATZ

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
  m_RemoveBookmarkButton->setToolTip(tr("Remove bookmark"));
  connect(m_RemoveBookmarkButton, SIGNAL(clicked()), this, SLOT(removeBookmark()));

  //-------------------------------------

  // Setup the graphics view/scene
  FFilesScene = new QGraphicsScene(m_FilesView);
  FFilesScene->setStickyFocus(true);
  FFilesScene->installEventFilter(this);
// ATZ
  connect(FFilesScene, SIGNAL(selectionChanged()),
          this, SLOT(thumbSelectionChanged()));
  m_FilesView->setDragMode(QGraphicsView::RubberBandDrag);
// end ATZ
  m_FilesView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  m_FilesView->installEventFilter(this);
  m_FilesView->verticalScrollBar()->installEventFilter(this);
  m_FilesView->horizontalScrollBar()->installEventFilter(this);
  m_FilesView->setScene(FFilesScene);
  FDataModel->connectThumbGen(this, SLOT(receiveThumb(uint,TThumbPtr)));
  setLayouter((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType"));

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
//      m_FilesView->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
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
// ATZ
void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  changeDir(FDataModel->fileSystemModel()->getPathForIndex(index));
}
// end ATZ

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeToBookmark(const QModelIndex& index) {
  changeDir(FDataModel->tagModel()->path(index));
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::changeDir(const QString& path) {
// ATZ
  FDataModel->fileSystemModel()->setCurrentDir(path);
  QModelIndex ind = FDataModel->fileSystemModel()->getIndexForPath(path);
  m_DirTree->setExpanded(ind, true);
  m_DirTree->scrollTo(ind);
  m_DirTree->setCurrentIndex(ind);
// end ATZ
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

  if (FThumbCount == 0) {
    // setting scene to null dimensions disappears unneeded scrollbars
    FFilesScene->setSceneRect(0,0,0,0);
// ATZ
    // there is nothing to display, show the pathbar, empty the files scene
    FDataModel->populateThumbs(FFilesScene);
    this->layoutAll();
// end ATZ
  } else {
    FLayouter->LazyInit(FThumbCount);
    FDataModel->populateThumbs(FFilesScene);  // non-blocking, returns almost immediately
    this->layoutAll();
  }

  ColorLabel1->setSelectedLabel(0);
  StarRating1->setStarCount(0);
  StarRating2->setStarCount(0);
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
      break;
    }
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
// ATZ
    FDataModel->fileSystemModel()->setCurrentDir(lastDir);
    QModelIndex ind = FDataModel->fileSystemModel()->getIndexForPath(lastDir);
    m_DirTree->setExpanded(ind, true);
    m_DirTree->scrollTo(ind);
    m_DirTree->setCurrentIndex(ind);
// end ATZ

    // First call base class showEvent, then start thumbnail loading to ensure
    // the file manager is visible before ressource heavy actions begin.
    QWidget::showEvent(event);
    if (!InStartup) {
      this->displayThumbnails(lastDir, lastDirType);
    }

    FIsFirstShow = false;
    return;
  }

  focusThumbnail(focusedThumbIdx());
  if (!event->spontaneous()) {
    QWidget::showEvent(event);

    // Thumbnails are cleared to free memory when the fm window is closed,
    // i.e. we need to refresh the display when opening it again.
// ATZ: thumbnails are NOT cleared - no need to build it again
//    displayThumbnails();
  updateThumbList();
// end ATZ
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
    int newIdx = FLayouter->MoveIndex(focusedThumbIdx(), (QKeyEvent*)event);
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
    ptGraphicsThumbGroup* thumb = FDataModel->thumbGroupList()->at(index);
    FFilesScene->clearSelection();
    thumb->setSelected(true);
    FFilesScene->setFocusItem(thumb);
    m_FilesView->ensureVisible(thumb, 0, 0);
    m_FilesView->setFocus();
  } else {
    FFilesScene->clearSelection();
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
ptGraphicsThumbGroup* ptFileMgrWindow::focusedThumb() {
  if (FFilesScene->selectedItems().count() > 0) {
    return dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().first());
  }

  return NULL;
}

//------------------------------------------------------------------------------
int ptFileMgrWindow::focusedThumbIdx() {
  if (FFilesScene->selectedItems().count() > 0) {
    for (int i = 0; i < FDataModel->thumbGroupList()->count(); ++i) {
      if (FDataModel->thumbGroupList()->at(i) == FFilesScene->selectedItems().first()) {
        return i;
      }
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::thumbSelectionChanged() {
  ptGraphicsThumbGroup* thumb = focusedThumb();
  if (thumb != NULL && thumb->fsoType() == fsoFile) {
    this->loadForImageView(thumb->fullPath());
  }

  if (FFilesScene->selectedItems().count() == 1) {
    StarRating3->setStarCount(thumb->imageRating());
    ColorLabel2->setSelectedLabel(thumb->imageColorLabel());
  } else {
    StarRating3->setStarCount(-1);
    ColorLabel2->setSelectedLabel(-1);
  }
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::updateThumbList() {
  FDataModel->updateThumbList(FFilesScene);
  layoutVisibleItems();
}

//------------------------------------------------------------------------------
void ptFileMgrWindow::saveThumbnail() {
  ptGraphicsThumbGroup* thumb = focusedThumb();
  if (thumb != NULL) {
    QString   hFileName = thumb->fullPath();
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
    changeDir(location);

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
// ATZ: do not refresh filemanager each time when it is activated
  return;
// end ATZ

  if (!event->spontaneous()) {
    event->accept();
    // free memory occupied by thumbnails and thumb cache
    // clear() includes stopping thumbnail generation
    FDataModel->clear();
    FImageView->clear();
    this->clearScene();
  } else {
    event->ignore();
  }
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
    // ATZ: was: Keyboard actions for image viewer
    // now: set color label
    switch (event->key()) {
      case Qt::Key_0: ColorLabel2->setSelectedLabel(0); colorLabelChanged(); break;
      case Qt::Key_1: ColorLabel2->setSelectedLabel(1); colorLabelChanged(); break;
      case Qt::Key_2: ColorLabel2->setSelectedLabel(2); colorLabelChanged(); break;
      case Qt::Key_3: ColorLabel2->setSelectedLabel(3); colorLabelChanged(); break;
      case Qt::Key_4: ColorLabel2->setSelectedLabel(4); colorLabelChanged(); break;
      case Qt::Key_5: ColorLabel2->setSelectedLabel(5); colorLabelChanged(); break;
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

//------------------------------------------------------------------------------
void ptFileMgrWindow::bookmarkCurrentDir() {
  FDataModel->tagModel()->appendRow(QDir::toNativeSeparators(FDataModel->currentDir()),
                                     FDataModel->currentDir());
}

// ATZ
void ptFileMgrWindow::removeBookmark() {
  FTagList->removeBookmark();
}

void ptFileMgrWindow::ensureHaveSettingsFile(const QString& fileName) {
  if (!QFile::exists(fileName)) {
    // settings file does not exist. Try to get it from "StartupSettings"
    if (Settings->GetInt("StartupSettings") == 1 && QFile::exists((Settings->GetString("StartupSettingsFile")))) {
       QFile::copy(Settings->GetString("StartupSettingsFile"), fileName);
    } else {
      // no startup settings? create empty but valid pts file
      QSettings ptsFile(fileName, QSettings::IniFormat);
      ptsFile.setValue("Magic", "photivoSettingsFile");
    }
  }
}

void ptFileMgrWindow::starRatingChanged() {
  if (Settings->GetInt("FileMgrShowDeleted") == 1) return;

  for (int i = 0; i < FFilesScene->selectedItems().count(); i++) {
    ptGraphicsThumbGroup* thumb = dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().at(i));
    if (thumb->fsoType() == fsoFile) {
      QFileInfo PathInfo(thumb->fullPath());
      QString baseFileName = PathInfo.dir().absolutePath() + QDir::separator() + PathInfo.completeBaseName();
      QString ptsFileName = baseFileName + ".pts";
      ensureHaveSettingsFile(ptsFileName);
      QSettings ptsFile(ptsFileName, QSettings::IniFormat);
      ptsFile.setValue("ImageRating", StarRating3->starCount());
    }
  }
  updateThumbList();
  // if thumblist is filtered by a star rating, reflect the changes
  filterChanged();
}

void ptFileMgrWindow::colorLabelChanged() {
  if (Settings->GetInt("FileMgrShowDeleted") == 1) return;

  for (int i = 0; i < FFilesScene->selectedItems().count(); i++) {
    ptGraphicsThumbGroup* thumb = dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().at(i));
    if (thumb->fsoType() == fsoFile) {
      QFileInfo PathInfo(thumb->fullPath());
      QString baseFileName = PathInfo.dir().absolutePath() + QDir::separator() + PathInfo.completeBaseName();
      QString ptsFileName = baseFileName + ".pts";
      ensureHaveSettingsFile(ptsFileName);
      QSettings ptsFile(ptsFileName, QSettings::IniFormat);
      ptsFile.setValue("ColorLabel", ColorLabel2->selectedLabel());
    }
  }
  updateThumbList();
  // if thumblist is filtered by a color label, reflect the changes
  filterChanged();
}

void ptFileMgrWindow::filterChanged() {
  // star rating
  int starFrom = StarRating1->starCount();
  int starTo = StarRating2->starCount();
  if (starTo < starFrom) {
    starTo = starFrom;
    StarRating2->setStarCount(starTo);
  }
  bool starSelected = starTo != 0;

  // color label
  QList<int> colorLabels = ColorLabel1->selectedLabels();
  bool colorLabelSelected = ColorLabel1->selectedLabel() != 0;

  bool isTrash = Settings->GetValue("FileMgrShowDeleted") == 1;
  if (isTrash) {
    starSelected = false;
    colorLabelSelected = false;
  }

  for (int i = 0; i < FDataModel->thumbGroupList()->count(); i++) {
    ptGraphicsThumbGroup* thumb = FDataModel->thumbGroupList()->at(i);
    if (thumb->fsoType() == fsoFile) {
      int star = thumb->imageRating();
      int colorLabel = thumb->imageColorLabel();
      bool visible = true;
      if (colorLabelSelected) {
        visible = colorLabels.contains(colorLabel);
      }
      if (starSelected) {
        visible = visible & (star >= starFrom && star <= starTo);
      }
      thumb->setVisible(visible);
    }
  }
  layoutVisibleItems();
}

void ptFileMgrWindow::fileTypeFilterChanged(int index) {
  Settings->SetValue("FileMgrShowDeleted", 0);

  switch(index) {
    // all files
    case 0:
      Settings->SetValue("FileMgrShowRAWs", 0);
      Settings->SetValue("FileMgrShowBitmaps", 0);
      break;
    // raw files
    case 1:
      Settings->SetValue("FileMgrShowRAWs", 1);
      Settings->SetValue("FileMgrShowBitmaps", 0);
      break;
    // bitmap files
    case 2:
      Settings->SetValue("FileMgrShowRAWs", 0);
      Settings->SetValue("FileMgrShowBitmaps", 1);
      break;
    // raw+bitmap files
    case 3:
      Settings->SetValue("FileMgrShowRAWs", 1);
      Settings->SetValue("FileMgrShowBitmaps", 1);
      break;
    // deleted files
    case 4:
      Settings->SetValue("FileMgrShowDeleted", 1);
      break;
  }

  bool isTrash = Settings->GetValue("FileMgrShowDeleted") == 1;

  RestoreImageButton->setVisible(isTrash);
  ColorLabel2->setVisible(!isTrash);
  StarRating3->setVisible(!isTrash);
  SendToBatchButton->setVisible(!isTrash);

  displayThumbnails();
}


void ptFileMgrWindow::layoutVisibleItems() {
  // gets a number of visible items first
  int visibleCount = 0;
  for (int i = 0; i < FDataModel->thumbGroupList()->count(); i++) {
    ptGraphicsThumbGroup* thumb = FDataModel->thumbGroupList()->at(i);
    if (thumb->isVisible()) {
      visibleCount++;
    }
  }

  // now layout visible items
  FLayouter->Init(visibleCount, m_FilesView->font());
  for (int i = 0; i < FDataModel->thumbGroupList()->count(); i++) {
    ptGraphicsThumbGroup* thumb = FDataModel->thumbGroupList()->at(i);
    if (thumb->isVisible()) {
      FLayouter->Layout(thumb);
    }
  }
}

QString ptFileMgrWindow::getCurrentDir() const {
  return FDataModel->currentDir();
}


QFileInfoList ptFileMgrWindow::getFilteredFileInfoList() const {
  QFileInfoList list;
  for (int i = 0; i < FDataModel->thumbGroupList()->count(); i++) {
    ptGraphicsThumbGroup* thumb = FDataModel->thumbGroupList()->at(i);
    if (thumb->isVisible() && thumb->fsoType() == fsoFile) {
      QFileInfo PathInfo(thumb->fullPath());
      list << PathInfo;
    }
  }
  return list;
}

void ptFileMgrWindow::OnRestoreImageButtonClicked() {
  if (Settings->GetInt("FileMgrShowDeleted") == 0) return;
  if (FFilesScene->selectedItems().count() == 0) return;

  // process selected images
  for (int i = 0; i < FFilesScene->selectedItems().count(); i++) {
    ptGraphicsThumbGroup* thumb = dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().at(i));
    if (thumb->fsoType() == fsoFile) {
      QString imageFileName = thumb->fullPath();
      // remove the .trash part
      QString newImageFileName = imageFileName;
      newImageFileName.chop(6); 
      QFile::rename(imageFileName, newImageFileName);
    }
  }
  updateThumbList();
}

void ptFileMgrWindow::OnDeleteImageButtonClicked() {
  if (FFilesScene->selectedItems().count() == 0) {
    return;
  }

  bool isTrashFile = Settings->GetInt("FileMgrShowDeleted") == 1;

  ptMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(QObject::tr("Photivo: Delete image(s)"));
  if (isTrashFile) {
    msgBox.setText(QObject::tr("Do you want to COMPLETELY delete selected image(s)?"));
  } else {
    msgBox.setText(QObject::tr("Do you want to delete selected image(s)?\n\nNote: Photivo just renames the image to filename.ext.trash"));
  }
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);

  int userChoice = msgBox.exec();
  if (userChoice != QMessageBox::Yes) {
    return;
  }

  // process selected images
  for (int i = 0; i < FFilesScene->selectedItems().count(); i++) {
    ptGraphicsThumbGroup* thumb = dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().at(i));
    if (thumb->fsoType() == fsoFile) {
      QString imageFileName = thumb->fullPath();
      if (isTrashFile) {
        // delete completely
        QFile::remove(imageFileName);
      } else {
        // instead of deleting the image file, just rename it to "filename.ext.trash"
        QFile::rename(imageFileName, imageFileName + ".trash");
      }
      // delete the settings file if any
      QFileInfo PathInfo(imageFileName);
      QString ptsFileName = PathInfo.dir().absolutePath() + QDir::separator() + PathInfo.completeBaseName() + ".pts";
      if (QFile::exists(ptsFileName)) {
        QFile::remove(ptsFileName);
      }
    }
  }
  updateThumbList();
}

void ptFileMgrWindow::OnSendToBatchButtonClicked() {
  if (Settings->GetInt("FileMgrShowDeleted") == 1) return;

  // process selected images
  for (int i = 0; i < FFilesScene->selectedItems().count(); i++) {
    ptGraphicsThumbGroup* thumb = dynamic_cast<ptGraphicsThumbGroup*>(FFilesScene->selectedItems().at(i));
    if (thumb->fsoType() == fsoFile) {
      QString imageFileName = thumb->fullPath();
      QFileInfo PathInfo(imageFileName);
      QString ptsFileName = PathInfo.dir().absolutePath() + QDir::separator() + PathInfo.completeBaseName() + ".pts";

      ensureHaveSettingsFile(ptsFileName);
      BatchWindow->AddJobToList(ptsFileName, imageFileName);
    }
  }
}

void ptFileMgrWindow::OnBatchButtonClicked() {
  CB_BatchButton();
}

void ptFileMgrWindow::OnProcessingButtonClicked() {
  closeWindow();
}

void ptFileMgrWindow::OnFullScreenButtonClicked() {
  if (FullScreenButton->isChecked())
    CB_FullScreenButton(1);
  else
    CB_FullScreenButton(0);
}

// end ATZ
