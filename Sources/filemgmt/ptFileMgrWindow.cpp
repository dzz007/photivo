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

#include <cassert>

#include <QVBoxLayout>
#include <QFontMetrics>
#include <QList>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QLabel>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "../ptImageHelper.h"
#include "../ptMessageBox.h"
#include "../ptImage8.h"
#include "ptFileMgrWindow.h"
#include "ptGraphicsSceneEmitter.h"
#include "ptRowGridThumbnailLayouter.h"
#include "ptColumnGridThumbnailLayouter.h"

extern void CB_MenuFileOpen(const short HaveFile);

extern ptSettings*  Settings;
extern ptTheme*     Theme;
extern QString      ImageFileToOpen;
extern short        InStartup;
extern QString      SaveBitmapPattern;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget* parent)
: QWidget(parent),
  m_IsFirstShow(true),
  m_ThumbCount(-1),
  m_ThumbListIdx(0)
{
  // Setup data model first because several UI elements need it.
  m_DataModel = ptFileMgrDM::GetInstance();

  // Main UI init
  setupUi(this);
  setMouseTracking(true);

  ptGraphicsSceneEmitter::ConnectThumbnailAction(
      this, SLOT(execThumbnailAction(ptThumbnailAction,QString)));
  ptGraphicsSceneEmitter::ConnectFocusChanged(this, SLOT(thumbFocusChanged()));

  //------------------------------------------------------------------------------

  // sidebar
  FMSidebar->setVisible(Settings->GetInt("FileMgrShowSidebar"));

  // Folder list
#ifdef Q_OS_WIN
  DirListLabel->setText(tr("Folders"));
#else
  DirListLabel->setText(tr("Directories"));
#endif
  m_DirList->setModel(m_DataModel->dirModel());
  connect(m_DirList, SIGNAL(activated(QModelIndex)), this, SLOT(changeListDir(QModelIndex)));


#ifdef Q_OS_WIN
  QString BookmarkTooltip = tr("Bookmark current folder (Ctrl+B)");
#else
  QString BookmarkTooltip = tr("Bookmark current directory (Ctrl+B)");
#endif

  // bookmark list in sidebar
  m_TagList = new ptTagList(this);
  m_TagList->setModel(m_DataModel->tagModel());
  connect(m_DataModel->tagModel(), SIGNAL(itemChanged(QStandardItem*)),
          this, SLOT(bookmarkDataChanged(QStandardItem*)));
  m_TagPaneLayout->addWidget(m_TagList);
  connect(m_TagList, SIGNAL(activated(QModelIndex)), this, SLOT(changeToBookmark(QModelIndex)));
  m_AddBookmarkButton->setToolTip(BookmarkTooltip);
  connect(m_AddBookmarkButton, SIGNAL(clicked()), this, SLOT(bookmarkCurrentDir()));

  //------------------------------------------------------------------------------

  //bookmark menu
  m_TagMenu = new QMenu(this);
  m_TagMenu->hide();
  m_TagMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_TagMenu->setObjectName("FMTagMenu");

  QLabel* label = new QLabel("<b>" + tr("Bookmarks") + "</b>", m_TagMenu);
  QToolButton* addButton = new QToolButton(m_TagMenu);
  addButton->setIcon(QIcon(Theme->IconAddBookmark));
  addButton->setToolTip(BookmarkTooltip);
  connect(addButton, SIGNAL(clicked()), this, SLOT(bookmarkCurrentDir()));

  QHBoxLayout* headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(0,0,0,0);
  headerLayout->addWidget(addButton);
  headerLayout->addWidget(label);

  m_TagMenuList = new ptTagList(m_TagMenu);
  m_TagMenuList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_TagMenuList->setModel(m_DataModel->tagModel());
  connect(m_TagMenuList, SIGNAL(activated(QModelIndex)),
          this, SLOT(changeToBookmarkFromMenu(QModelIndex)));

  QVBoxLayout* layout = new QVBoxLayout(m_TagMenu);
  layout->addLayout(headerLayout);
  layout->addWidget(m_TagMenuList);
  m_TagMenu->setLayout(layout);

  //------------------------------------------------------------------------------

  // Setup the graphics view/scene
  m_FilesScene = new QGraphicsScene(m_FilesView);
  m_FilesScene->setStickyFocus(true);
  m_FilesScene->installEventFilter(this);
  m_FilesView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  m_FilesView->installEventFilter(this);
  m_FilesView->verticalScrollBar()->installEventFilter(this);
  m_FilesView->horizontalScrollBar()->installEventFilter(this);
  m_FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbNotify(const bool)),
          this, SLOT(fetchNewThumbs(const bool)));
  connect(m_DataModel->thumbnailer(), SIGNAL(newImageNotify(ptGraphicsThumbGroup*,ptImage8*)),
          this, SLOT(fetchNewImages(ptGraphicsThumbGroup*,ptImage8*)));
  setLayouter((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType"));

  m_PathBar = new ptPathBar(m_PathContainer);
  m_PathBar->setObjectName("FMPathBar");
  connect(m_PathBar, SIGNAL(changedPath(QString)), this, SLOT(changeDir(QString)));
  m_PathLayout->addWidget(m_PathBar);
  m_Progressbar->hide();

  //------------------------------------------------------------------------------

  // construct the image view
  m_ImageView = new ptImageView(FMImageViewPane, m_DataModel);
  m_ImageView->installEventFilter(this);
  FMImageViewPane->setVisible((bool)Settings->GetInt("FileMgrShowImageView"));

  //------------------------------------------------------------------------------

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

  //------------------------------------------------------------------------------

  ConstructContextMenu();
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation", m_DataModel->currentDir());
  Settings->m_IniSettings->
      setValue("FileMgrMainSplitter", FMMainSplitter->saveState());
  Settings->m_IniSettings->
      setValue("FileMgrSidebarSplitter", FMSidebarSplitter->saveState());

  DelAndNull(m_Layouter);
  DelAndNull(m_PathBar);

  // Make sure to destroy all thumbnail related things before the singletons!
  ptFileMgrDM::DestroyInstance();
  ptGraphicsSceneEmitter::DestroyInstance();

  // context menu actions
  DelAndNull(ac_VerticalThumbs);
  DelAndNull(ac_HorizontalThumbs);
  DelAndNull(ac_DetailedThumbs);
  DelAndNull(ac_DirThumbs);
  DelAndNull(ac_ThumbLayoutGroup);
  DelAndNull(ac_ToggleSidebar);
  DelAndNull(ac_ToggleImageView);
  DelAndNull(ac_CloseFileMgr);
  DelAndNull(ac_SaveThumb);

  DelAndNull(m_ImageView);
}

//==============================================================================

void ptFileMgrWindow::setLayouter(const ptThumbnailLayout layout) {
  bool RestartThumbnailer = false;
  if (!InStartup) {
    if ((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType") == layout)
      return;
    RestartThumbnailer = m_DataModel->thumbnailer()->isRunning();
    m_DataModel->StopThumbnailer();
    DelAndNull(m_Layouter);
    Settings->SetValue("FileMgrThumbLayoutType", layout);
  }

  switch (layout) {
    case tlVerticalByRow:
      m_FilesView->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
      m_Layouter = new ptRowGridThumbnailLayouter(m_FilesView);
      break;

    case tlHorizontalByColumn:
      m_FilesView->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      m_Layouter = new ptColumnGridThumbnailLayouter(m_FilesView);
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
    DisplayThumbnails();
  } else {
    LayoutAll();
  }
}

//==============================================================================

void ptFileMgrWindow::changeListDir(const QModelIndex& index) {
  m_DataModel->dirModel()->ChangeDir(index);
  DisplayThumbnails(m_DataModel->dirModel()->absolutePath(), m_DataModel->dirModel()->pathType());
}

//==============================================================================

void ptFileMgrWindow::changeToBookmark(const QModelIndex& index) {
  changeDir(m_DataModel->tagModel()->path(index));
}

//==============================================================================

void ptFileMgrWindow::changeToBookmarkFromMenu(const QModelIndex& index) {
  m_TagMenu->hide();
  changeToBookmark(index);
}

//==============================================================================

void ptFileMgrWindow::changeDir(const QString& path) {
  m_DataModel->dirModel()->ChangeAbsoluteDir(path);
  DisplayThumbnails(path, m_DataModel->dirModel()->pathType());
}

//==============================================================================

#ifdef Q_OS_UNIX
// Linux does not need parameter fsoType. Disable compiler warning.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif
void ptFileMgrWindow::DisplayThumbnails(QString path /*= ""*/, ptFSOType fsoType /*= fsoDir*/) {
  if (path.isEmpty()) {
    path = m_DataModel->dirModel()->absolutePath();
    fsoType = m_DataModel->dirModel()->pathType();
  }

#ifdef Q_OS_WIN
  if (fsoType == fsoRoot) {
    // We are in “My Computer”
    path = MyComputerIdString;
  }
#endif

  m_PathBar->setPath(path);
  m_DataModel->StopThumbnailer();
  ClearScene();
  m_FilesView->horizontalScrollBar()->setValue(0);
  m_FilesView->verticalScrollBar()->setValue(0);
  m_ThumbListIdx = 0;
  m_ThumbCount = m_DataModel->setThumbnailDir(path);

  if (m_ThumbCount > 0) {
    m_Layouter->LazyInit(m_ThumbCount);
    m_Progressbar->setValue(0);
    m_Progressbar->setMaximum(m_ThumbCount);
    m_DataModel->StartThumbnailer();
  } else {
    // setting scene to null dimensions disappears unneeded scrollbars
    m_FilesScene->setSceneRect(0,0,0,0);
  }
}
#ifdef Q_OS_UNIX
#pragma GCC diagnostic pop
#endif

//==============================================================================

void ptFileMgrWindow::fetchNewThumbs(const bool isLast) {
  while (m_ThumbListIdx < m_DataModel->thumbList()->count()) {
    ptGraphicsThumbGroup* thumb = m_DataModel->thumbList()->at(m_ThumbListIdx);
    m_ThumbListIdx++;
    m_Layouter->Layout(thumb);
    m_FilesScene->addItem(thumb);
  }

  if (isLast) {
    m_ThumbListIdx = 0;
    m_Progressbar->show();
    m_PathContainer->hide();
  }
}

//==============================================================================

void ptFileMgrWindow::fetchNewImages(ptGraphicsThumbGroup* group, ptImage8 *pix) {
  m_Progressbar->setValue(m_Progressbar->value() + 1);
  if (pix != NULL) {
    // Adding the image to the group must be done from the main GUI thread.
    group->addImage(pix);
  }

  if (m_Progressbar->value() >= m_ThumbCount) {
    m_Progressbar->hide();
    m_PathContainer->show();
    m_FilesScene->setFocus();
    if (m_FilesScene->focusItem() == NULL) {
      FocusThumbnail(0);
    }
  }
}

//==============================================================================

void ptFileMgrWindow::LayoutAll() {
  m_Layouter->Init(m_DataModel->thumbList()->count(), m_FilesView->font());
  QListIterator<ptGraphicsThumbGroup*> i(*m_DataModel->thumbList());

  while (i.hasNext()) {
    m_Layouter->Layout(i.next());
  }
}

//==============================================================================

void ptFileMgrWindow::showEvent(QShowEvent* event) {
  if (m_IsFirstShow) {
    // Execute once when the file manager is opened for the first time.

    // Theme and layout stuff (wouldn’t work in constructor)
    UpdateTheme();

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
    m_DataModel->dirModel()->ChangeAbsoluteDir(lastDir);
    m_DataModel->setCurrentDir(lastDir);

    // First call base class showEvent, then start thumbnail loading to ensure
    // the file manager is visible before ressource heavy actions begin.
    QWidget::showEvent(event);
    if (!InStartup)
      DisplayThumbnails(lastDir, lastDirType);

    m_IsFirstShow = false;
    return;
  }

  if (!event->spontaneous()) {
    QWidget::showEvent(event);

    // Thumbnails are cleared to free memory when the fm window is closed,
    // i.e. we need to refresh the display when opening it again.
    DisplayThumbnails();
  }
}

//==============================================================================

bool ptFileMgrWindow::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_FilesView && event->type() == QEvent::Resize) {
    // Resize event: Rearrange thumbnails when size of viewport changes
    LayoutAll();
    return false;   // handle event further
  }

//------------------------------------------------------------------------------

  else if ((obj == m_FilesView->verticalScrollBar() ||
            obj == m_FilesView->horizontalScrollBar()) &&
            event->type() == QEvent::Wheel)
  { // Wheel event
    int dir = ((QWheelEvent*)event)->delta() > 0 ? -1 : 1;
    if (m_FilesView->verticalScrollBar()->isVisible()) {
      m_FilesView->verticalScrollBar()->setValue(
            m_FilesView->verticalScrollBar()->value() + m_Layouter->Step()*dir);

    } else if (m_FilesView->horizontalScrollBar()->isVisible()) {
      m_FilesView->horizontalScrollBar()->setValue(
            m_FilesView->horizontalScrollBar()->value() + m_Layouter->Step()*dir);
    }
    return true;    // prevent further event handling
  }

//------------------------------------------------------------------------------

  else if (obj == m_FilesScene && (event->type() == QEvent::GraphicsSceneDragEnter ||
                                   event->type() == QEvent::GraphicsSceneDrop))
  { // Make sure drag&drop events are passed on to MainWindow
    event->ignore();
    return true;
  }

//------------------------------------------------------------------------------

  else if (obj == m_FilesScene && event->type() == QEvent::KeyPress) {
    // Keyboard navigation in thumbnail list
    int newIdx = m_Layouter->MoveIndex(m_DataModel->focusedThumb(), (QKeyEvent*)event);
    if (newIdx >= 0) {
      FocusThumbnail(newIdx);
      return true;
    }
  }

//------------------------------------------------------------------------------

  // unhandled events fall through to here
  // make sure parent event filters are executed
  return QWidget::eventFilter(obj, event);
}

//==============================================================================

void ptFileMgrWindow::FocusThumbnail(int index) {
  if (index >= 0) {
    // focus new thumb
    ptGraphicsThumbGroup* thumb = m_DataModel->MoveFocus(index);
    m_FilesScene->setFocusItem(thumb);
    m_FilesView->ensureVisible(thumb, 0, 0);
    m_FilesView->setFocus();
    if (thumb->fsoType() == fsoFile) {
      m_ImageView->ShowImage(thumb->fullPath());
    }

  } else {
    m_FilesScene->clearFocus();
  }
}

//==============================================================================

void ptFileMgrWindow::thumbFocusChanged() {
  FocusThumbnail(m_DataModel->focusedThumb(m_FilesScene->focusItem()));
}

//==============================================================================

void ptFileMgrWindow::saveThumbnail()
{
  int hThumbIdx = m_DataModel->focusedThumb();

  if (hThumbIdx > -1 &&
      hThumbIdx < m_DataModel->thumbList()->count()) {
    QString   hFileName          = m_DataModel->thumbList()->at(hThumbIdx)->fullPath();
    ptImage8* hImage             = new ptImage8();
    m_DataModel->getThumbnail(hImage, hFileName, Settings->GetInt("FileMgrThumbSaveSize"));

    QFileInfo hPathInfo(hFileName);
    QString   hSuggestedFileName = hPathInfo.dir().path() + "/" + hPathInfo.completeBaseName() + "-thumb.jpg";

    QString hOutputName;
    hOutputName = QFileDialog::getSaveFileName(NULL,
                                               QObject::tr("Save File"),
                                               hSuggestedFileName,
                                               SaveBitmapPattern);
    if (0 == hOutputName.size()) return; // Operation cancelled.

    if (!(hImage->DumpImage(hOutputName.toAscii().data(), true))) {
      ptMessageBox::warning(0, QObject::tr("Error"), QObject::tr("Thumbnail could not be saved."));
    }

    if (!ptImageHelper::TransferExif(hFileName, hOutputName)) {
      ptMessageBox::warning(0, QObject::tr("Exif error"), QObject::tr("Exif data could not be written."));
    }

    delete hImage;
  }
}

//==============================================================================

void ptFileMgrWindow::execThumbnailAction(const ptThumbnailAction action, const QString location) {
  if (action == tnaLoadImage) {
    closeWindow();
    ImageFileToOpen = location;
    CB_MenuFileOpen(1);
  } else if (action == tnaChangeDir) {
    m_DataModel->dirModel()->ChangeAbsoluteDir(location);
    DisplayThumbnails(location, m_DataModel->dirModel()->pathType());
  } else if (action == tnaViewImage) {
    m_ImageView->ShowImage(location);
  }
}

//==============================================================================

void ptFileMgrWindow::UpdateTheme() {
  // Update file manager window appearance
  setStyle(Theme->style());
  setStyleSheet(Theme->stylesheet());

  // Thumbgroups don’t need to be updated because Photivo theme can only be changed
  // while fm is closed = thumbnail display cleared
}

//==============================================================================

void ptFileMgrWindow::closeWindow() {
  // File manager can’t close its window itself. That’s handled by the main window.
  // Signal tells main window to perform necessary closing actions.
  // FM internal cleanup is done in ptFileMgrWindow::hideEvent().
  emit FileMgrWindowClosed();
}

//==============================================================================

void ptFileMgrWindow::hideEvent(QHideEvent* event) {
  if (!event->spontaneous()) {
    event->accept();
    m_DataModel->StopThumbnailer();
    // free memory occupied by thumbnails
    ClearScene();
    m_DataModel->Clear();
  } else {
    event->ignore();
  }
}

//==============================================================================

void ptFileMgrWindow::ClearScene() {
  for (int i = 0; i < m_DataModel->thumbList()->count(); i++) {
    m_FilesScene->removeItem(m_DataModel->thumbList()->at(i));
    ptGraphicsThumbGroup::RemoveRef(m_DataModel->thumbList()->at(i));
  }

  m_DataModel->thumbList()->clear();
}

//==============================================================================

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
    DisplayThumbnails();
  }
  // Shift+F5: clear cache and refresh thumbnails
  else if (event->key() == Qt::Key_F5 && event->modifiers() == Qt::ShiftModifier) {
    m_DataModel->Clear();
    DisplayThumbnails();
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

  //------------------------------------------------------------------------------

  else if (event->modifiers() == Qt::NoModifier) {
    // Keyboard actions for image viewer
    switch (event->key()) {
      case Qt::Key_1: m_ImageView->zoomIn();  break;
      case Qt::Key_2: m_ImageView->zoom100(); break;
      case Qt::Key_3: m_ImageView->zoomOut(); break;
      case Qt::Key_4: m_ImageView->zoomFit(); break;
      default: break;
    }
  }
}

//==============================================================================

void ptFileMgrWindow::ConstructContextMenu() {
  // Actions for thumbnail view submenu
  ac_VerticalThumbs = new QAction(tr("&Vertical") + "\t" + tr("Alt+1"), this);
  ac_VerticalThumbs->setCheckable(true);
  connect(ac_VerticalThumbs, SIGNAL(triggered()), this, SLOT(verticalThumbs()));

  ac_HorizontalThumbs = new QAction(tr("&Horizontal") + "\t" + tr("Alt+2"), this);
  ac_HorizontalThumbs->setCheckable(true);
  connect(ac_HorizontalThumbs, SIGNAL(triggered()), this, SLOT(horizontalThumbs()));

  ac_DetailedThumbs = new QAction(tr("&Details") + "\t" + tr("Alt+3"), this);
  ac_DetailedThumbs->setCheckable(true);
  connect(ac_DetailedThumbs, SIGNAL(triggered()), this, SLOT(detailedThumbs()));

#ifdef Q_OS_WIN
  ac_DirThumbs = new QAction(tr("Show &folder thumbnails"), this);
#else
  ac_DirThumbs = new QAction(tr("Show &directory thumbnails"), this);
#endif
  ac_DirThumbs->setCheckable(true);
  connect(ac_DirThumbs, SIGNAL(triggered()), this, SLOT(toggleDirThumbs()));

  ac_ThumbLayoutGroup = new QActionGroup(this);
  ac_ThumbLayoutGroup->setExclusive(true);
  ac_ThumbLayoutGroup->addAction(ac_VerticalThumbs);
  ac_ThumbLayoutGroup->addAction(ac_HorizontalThumbs);
//TODO: re-enable  ac_ThumbLayoutGroup->addAction(ac_DetailedThumbs);

  // actions for main context menu
  ac_ToggleImageView = new QAction(tr("Show &image preview") + "\t" + tr("F3"), this);
  ac_ToggleImageView->setCheckable(true);
  connect(ac_ToggleImageView, SIGNAL(triggered()), this, SLOT(toggleImageView()));

  ac_ToggleSidebar = new QAction(tr("Show &sidebar") + "\t" + tr("F4"), this);
  ac_ToggleSidebar->setCheckable(true);
  connect(ac_ToggleSidebar, SIGNAL(triggered()), this, SLOT(toggleSidebar()));

  ac_SaveThumb = new QAction(tr("&Save thumbnail"), this);
  connect(ac_SaveThumb, SIGNAL(triggered()), this, SLOT(saveThumbnail()));

  ac_CloseFileMgr = new QAction(tr("&Close file manager") + "\t" + tr("Esc"), this);
  connect(ac_CloseFileMgr, SIGNAL(triggered()), this, SLOT(closeWindow()));
}

//==============================================================================

void ptFileMgrWindow::contextMenuEvent(QContextMenuEvent* event) {
  ptThumbnailLayout currLayout = (ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType");

  // thumbnail view submenu
  QMenu MenuThumbLayout(tr("Thumbnail &view"));
  MenuThumbLayout.setPalette(Theme->menuPalette());
  MenuThumbLayout.setStyle(Theme->style());
  MenuThumbLayout.addActions(ac_ThumbLayoutGroup->actions());
  MenuThumbLayout.addSeparator();
  MenuThumbLayout.addAction(ac_DirThumbs);
  ac_VerticalThumbs->setChecked(currLayout == tlVerticalByRow);
  ac_HorizontalThumbs->setChecked(currLayout == tlHorizontalByColumn);
  ac_DetailedThumbs->setChecked(currLayout == tlDetailedList);
  ac_DirThumbs->setChecked(Settings->GetInt("FileMgrShowDirThumbs"));

  // main context menu
  QMenu Menu(NULL);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  Menu.addMenu(&MenuThumbLayout);

  Menu.addSeparator();
  Menu.addAction(ac_ToggleImageView);
  ac_ToggleImageView->setChecked(FMImageViewPane->isVisible());
  Menu.addAction(ac_ToggleSidebar);
  ac_ToggleSidebar->setChecked(FMSidebar->isVisible());

  Menu.addSeparator();
  Menu.addAction(ac_SaveThumb);

  Menu.addSeparator();
  Menu.addAction(ac_CloseFileMgr);

  Menu.exec(((QMouseEvent*)event)->globalPos());
}

//==============================================================================

void ptFileMgrWindow::verticalThumbs() {
  setLayouter(tlVerticalByRow);
}

//==============================================================================

void ptFileMgrWindow::horizontalThumbs() {
  setLayouter(tlHorizontalByColumn);
}

//==============================================================================

void ptFileMgrWindow::detailedThumbs() {
  setLayouter(tlDetailedList);
}

//==============================================================================

void ptFileMgrWindow::toggleSidebar() {
  FMSidebar->setVisible(!FMSidebar->isVisible());
  Settings->SetValue("FileMgrShowSidebar", (int)FMSidebar->isVisible());
}

//==============================================================================

void ptFileMgrWindow::toggleImageView() {
  FMImageViewPane->setVisible(!FMImageViewPane->isVisible());
  Settings->SetValue("FileMgrShowImageView", (int)FMImageViewPane->isVisible());
}

//==============================================================================

void ptFileMgrWindow::toggleDirThumbs() {
  Settings->SetValue("FileMgrShowDirThumbs", 1 - Settings->GetInt("FileMgrShowDirThumbs"));
  DisplayThumbnails();
}

//==============================================================================

void ptFileMgrWindow::bookmarkCurrentDir() {
  m_DataModel->tagModel()->appendRow(QDir::toNativeSeparators(m_DataModel->currentDir()),
                                     m_DataModel->currentDir());
  if (m_TagMenu->isVisible()) {
    AdjustBookmarkMenuSize();
  }
}

//==============================================================================

void ptFileMgrWindow::on_m_BookmarkButton_clicked() {
  m_TagMenu->move(m_BookmarkButton->mapToGlobal(QPoint(0, m_BookmarkButton->height())));
  m_TagMenu->setPalette(Theme->menuPalette());
  m_TagMenu->setStyle(Theme->style());
  m_TagMenu->show();  // must be first or adjust size won’t work correctly
  AdjustBookmarkMenuSize();

  m_TagMenuList->setFocus();
}

//==============================================================================

void ptFileMgrWindow::AdjustBookmarkMenuSize() {
  QSize MenuSize(0, 0);
  QPoint MenuTopleft = m_BookmarkButton->mapTo(this, QPoint(0, m_BookmarkButton->height()));
  QSize MaxSize((this->width() - MenuTopleft.x()) * 0.85,
                (this->height() - MenuTopleft.y()) * 0.85);

  QFontMetrics metrics(m_TagMenuList->font());
  for (int i = 0; i < m_DataModel->tagModel()->rowCount(); i++) {
    QModelIndex curIndex = m_DataModel->tagModel()->index(i, 0);
    int width = metrics.width(curIndex.data().toString());
    if (width > MenuSize.width()) MenuSize.setWidth(width);
    MenuSize.setHeight(MenuSize.height() + m_TagMenuList->visualRect(curIndex).height());
  }

  m_TagMenu->setFixedSize(qBound(150,
                                 MenuSize.width() + 20 + m_TagMenuList->verticalScrollBar()->width(),
                                 MaxSize.width()),
                          qBound(m_TagMenuList->y() + 50,
                                 MenuSize.height() + m_TagMenuList->y() + 30,
                                 MaxSize.height()) );
}

//==============================================================================

void ptFileMgrWindow::bookmarkDataChanged(QStandardItem*) {
  if (m_TagMenu->isVisible()) {
    AdjustBookmarkMenuSize();
  }
}

//==============================================================================
