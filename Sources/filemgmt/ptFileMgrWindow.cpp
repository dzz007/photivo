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

#include <QFileSystemModel>
#include <QFontMetrics>
#include <QList>
#include <QDir>
#include <QMenu>
#include <QAction>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "ptFileMgrWindow.h"
#include "ptGraphicsSceneEmitter.h"
#include "ptRowGridThumbnailLayouter.h"
#include "ptColumnGridThumbnailLayouter.h"

extern void CB_MenuFileOpen(const short HaveFile);

extern ptSettings*  Settings;
extern ptTheme*     Theme;
extern QString      ImageFileToOpen;
extern short        InStartup;

//==============================================================================

ptFileMgrWindow::ptFileMgrWindow(QWidget* parent)
: QWidget(parent),
  m_IsFirstShow(true),
  m_ThumbCount(-1),
  m_ThumbListIdx(0)
{
  setupUi(this);
  setMouseTracking(true);
  ptGraphicsSceneEmitter::ConnectThumbnailAction(
      this, SLOT(execThumbnailAction(ptThumbnailAction,QString)) );
  m_Progressbar->hide();
  FMTreePane->setVisible(Settings->GetInt("FileMgrShowTreePane"));

  // We setup our data module
  m_DataModel = ptFileMgrDM::GetInstance();

  m_DirTree->setModel(m_DataModel->treeModel());
  m_DirTree->setRootIndex(m_DataModel->treeModel()->index(m_DataModel->treeModel()->rootPath()));
  m_DirTree->setColumnHidden(1, true);
  m_DirTree->setColumnHidden(2, true);
  m_DirTree->setColumnHidden(3, true);
  connect(m_DirTree, SIGNAL(clicked(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));
  connect(m_DirTree, SIGNAL(activated(QModelIndex)), this, SLOT(changeTreeDir(QModelIndex)));

  // Setup the graphics view/scene
  m_FilesScene = new QGraphicsScene(m_FilesView);
  m_FilesScene->installEventFilter(this);
  m_FilesView->installEventFilter(this);
  m_FilesView->verticalScrollBar()->installEventFilter(this);
  m_FilesView->horizontalScrollBar()->installEventFilter(this);
  m_FilesView->setScene(m_FilesScene);
  connect(m_DataModel->thumbnailer(), SIGNAL(newThumbNotify(const bool)),
          this, SLOT(fetchNewThumbs(const bool)));
  connect(m_DataModel->thumbnailer(), SIGNAL(newImageNotify(ptGraphicsThumbGroup*,QImage*)),
          this, SLOT(fetchNewImages(ptGraphicsThumbGroup*,QImage*)));

  // Layouter must be set after the data model is created!
  setLayouter((ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType"));

  // Filemgr windwow layout
  if (Settings->m_IniSettings->contains("FileMgrMainSplitter")) {
    m_MainSplitter->restoreState(
        Settings->m_IniSettings->value("FileMgrMainSplitter").toByteArray());
  } else {
    QList <int> SizesList;
    SizesList.append(250);
    SizesList.append(1000); // Value obtained to avoid resizing at startup.
    m_MainSplitter->setSizes(SizesList);
  }
  m_MainSplitter->setStretchFactor(1,1);

  ConstructContextMenu();
}

//==============================================================================

ptFileMgrWindow::~ptFileMgrWindow() {
  Settings->SetValue("LastFileMgrLocation",
      m_DataModel->treeModel()->filePath(m_DirTree->currentIndex()) );
  Settings->m_IniSettings->
      setValue("FileMgrMainSplitter", m_MainSplitter->saveState());

  DelAndNull(m_Layouter);

  // Make sure to destroy all thumbnail related things before the singletons!
  ptFileMgrDM::DestroyInstance();
  ptGraphicsSceneEmitter::DestroyInstance();

  // context menu actions
  DelAndNull(ac_VerticalThumbs);
  DelAndNull(ac_HorizontalThumbs);
  DelAndNull(ac_DetailedThumbs);
  DelAndNull(ac_ThumbLayoutGroup);
  DelAndNull(ac_ToggleNaviPane);
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
    DisplayThumbnails(m_DirTree->currentIndex());
  } else {
    LayoutAll();
  }
}

//==============================================================================

void ptFileMgrWindow::changeTreeDir(const QModelIndex& index) {
  DisplayThumbnails(index, false);
}

//==============================================================================

void ptFileMgrWindow::DisplayThumbnails(const QModelIndex& index, bool clearCache /*= false*/) {
  if (clearCache) {
    m_DataModel->Clear();
  }

  m_PathInput->setText(
      QDir::toNativeSeparators(m_DataModel->treeModel()->filePath(m_DirTree->currentIndex())));

  m_DataModel->StopThumbnailer();
  ClearScene();
  m_FilesView->horizontalScrollBar()->setValue(0);
  m_FilesView->verticalScrollBar()->setValue(0);
  m_ThumbListIdx = 0;
  m_ThumbCount = m_DataModel->setThumbnailDir(index);

  if (m_ThumbCount > -1) {
    m_Layouter->LazyInit(m_ThumbCount);
    m_Progressbar->setValue(0);
    m_Progressbar->setMaximum(m_ThumbCount);
    m_DataModel->StartThumbnailer();
  }
}

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
    m_PathInput->hide();
  }
}

//==============================================================================

void ptFileMgrWindow::fetchNewImages(ptGraphicsThumbGroup* group, QImage* pix) {
  m_Progressbar->setValue(m_Progressbar->value() + 1);

  if (pix != NULL) {
    // Adding the image to the group must be done from the main GUI thread.
    group->addImage(pix);
  }

  if (m_Progressbar->value() >= m_ThumbCount) {
    m_Progressbar->hide();
    m_PathInput->show();
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
    // Execute only when the file manager is opened for the first time
    // Theme and layout stuff
    setStyle(Theme->ptStyle);
    setStyleSheet(Theme->ptStyleSheet);
    m_TreePaneLayout->setContentsMargins(0,0,0,0);
    m_FileListLayout->setContentsMargins(10,10,10,10);
    m_FileListLayout->setSpacing(10);
    m_Progressbar->setFixedHeight(m_PathInput->height());

    // set initally selected directory
    QString lastDir = Settings->GetString("LastFileMgrLocation");
    QFileSystemModel* fsmodel = qobject_cast<QFileSystemModel*>(m_DataModel->treeModel());

    if (lastDir != "" && QDir(lastDir).exists()) {
      m_DirTree->setCurrentIndex(fsmodel->index(lastDir));
    } else {
      m_DirTree->setCurrentIndex(fsmodel->index(QDir::homePath()));
    }

    m_PathInput->setText(
        QDir::toNativeSeparators(m_DataModel->treeModel()->filePath(m_DirTree->currentIndex())));

    QWidget::showEvent(event);

    if (!InStartup)
      DisplayThumbnails(m_DirTree->currentIndex());

    ConstructContextMenu();
    m_IsFirstShow = false;
    return;
  }


  QWidget::showEvent(event);

  // Thumbnails are cleared to free memory when the fm window is closed,
  // i.e. we need to refresh the display when opening it again.
  DisplayThumbnails(m_DirTree->currentIndex());
}

//==============================================================================

bool ptFileMgrWindow::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_FilesView && event->type() == QEvent::Resize) {
    // Resize event: Rearrange thumbnails when size of viewport changes
    LayoutAll();
    return false;   // handle event further

//------------------------------------------------------------------------------

  } else if ((obj == m_FilesView->verticalScrollBar() ||
              obj == m_FilesView->horizontalScrollBar()) &&
              event->type() == QEvent::Wheel)
  {
    // Wheel event
    int dir = ((QWheelEvent*)event)->delta() > 0 ? -1 : 1;
    if (m_FilesView->verticalScrollBar()->isVisible()) {
      m_FilesView->verticalScrollBar()->setValue(
            m_FilesView->verticalScrollBar()->value() + m_Layouter->Step()*dir);

    } else if (m_FilesView->horizontalScrollBar()->isVisible()) {
      m_FilesView->horizontalScrollBar()->setValue(
            m_FilesView->horizontalScrollBar()->value() + m_Layouter->Step()*dir);
    }
    return true;    // prevent further event handling

//------------------------------------------------------------------------------

  } else if (obj == m_FilesScene && (event->type() == QEvent::GraphicsSceneDragEnter ||
                                     event->type() == QEvent::GraphicsSceneDrop))
  {
    event->ignore();
    return true;

//------------------------------------------------------------------------------

  } else {
    // make sure parent event filters are executed
    return QWidget::eventFilter(obj, event);
  }
}

//==============================================================================

void ptFileMgrWindow::execThumbnailAction(const ptThumbnailAction action, const QString location) {
  if (action == tnaLoadImage) {
    closeWindow();
    ImageFileToOpen = location;
    CB_MenuFileOpen(1);

  } else if (action == tnaChangeDir) {
    m_DirTree->setCurrentIndex(m_DataModel->treeModel()->index(location));
    DisplayThumbnails(m_DirTree->currentIndex());
  }
}

//==============================================================================

void ptFileMgrWindow::on_m_PathInput_returnPressed() {
  QDir dir = QDir(m_PathInput->text());
  QString treeDir = m_DataModel->treeModel()->filePath(m_DirTree->currentIndex());

  if (dir.exists() && (dir != QDir(treeDir))) {
    m_PathInput->setText(dir.absolutePath());
    m_DirTree->setCurrentIndex(m_DataModel->treeModel()->index(dir.absolutePath()));
    DisplayThumbnails(m_DirTree->currentIndex());

  } else {
    m_PathInput->setText(QDir::toNativeSeparators(treeDir));
  }
}

//==============================================================================

void ptFileMgrWindow::UpdateTheme() {
  // Update file manager window appearance
  setStyle(Theme->ptStyle);
  setStyleSheet(Theme->ptStyleSheet);
  m_Progressbar->setFixedHeight(m_PathInput->height());

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
  m_DataModel->StopThumbnailer();
  // free memory occupied by thumbnails
  ClearScene();
  m_DataModel->Clear();
  event->accept();
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
    DisplayThumbnails(m_DirTree->currentIndex());
  }
  // Shift+F5: clear cache and refresh thumbnails
  else if (event->key() == Qt::Key_F5 && event->modifiers() == Qt::ShiftModifier) {
    DisplayThumbnails(m_DirTree->currentIndex(), true);
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
  // Space: toggles tree pane
  else if (event->key() == Qt::Key_Space && event->modifiers() == Qt::NoModifier) {
    toggleNaviPane();
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

  ac_ThumbLayoutGroup = new QActionGroup(this);
  ac_ThumbLayoutGroup->setExclusive(true);
  ac_ThumbLayoutGroup->addAction(ac_VerticalThumbs);
  ac_ThumbLayoutGroup->addAction(ac_HorizontalThumbs);
//TODO: re-enable  ac_ThumbLayoutGroup->addAction(ac_DetailedThumbs);

  // actions for main context menu
  ac_ToggleNaviPane = new QAction(tr("Show &navigation pane") + "\t" + tr("Space"), this);
  ac_ToggleNaviPane->setCheckable(true);
  connect(ac_ToggleNaviPane, SIGNAL(triggered()), this, SLOT(toggleNaviPane()));

  ac_CloseFileMgr = new QAction(tr("&Close file manager") + "\t" + tr("Esc"), this);
  connect(ac_CloseFileMgr, SIGNAL(triggered()), this, SLOT(closeWindow()));
}

//==============================================================================

void ptFileMgrWindow::contextMenuEvent(QContextMenuEvent* event) {
  ptThumbnailLayout currLayout = (ptThumbnailLayout)Settings->GetInt("FileMgrThumbLayoutType");

  // thumbnail view submenu
  QMenu MenuThumbLayout(tr("Thumbnail &view"));
  MenuThumbLayout.setPalette(Theme->ptMenuPalette);
  MenuThumbLayout.setStyle(Theme->ptStyle);
  MenuThumbLayout.addActions(ac_ThumbLayoutGroup->actions());
  ac_VerticalThumbs->setChecked(currLayout == tlVerticalByRow);
  ac_HorizontalThumbs->setChecked(currLayout == tlHorizontalByColumn);
  ac_DetailedThumbs->setChecked(currLayout == tlDetailedList);

  // main context menu
  QMenu Menu(NULL);
  Menu.setPalette(Theme->ptMenuPalette);
  Menu.setStyle(Theme->ptStyle);
  Menu.addMenu(&MenuThumbLayout);
  Menu.addSeparator();
  Menu.addAction(ac_ToggleNaviPane);
  ac_ToggleNaviPane->setChecked(FMTreePane->isVisible());

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

void ptFileMgrWindow::toggleNaviPane() {
  FMTreePane->setVisible(!FMTreePane->isVisible());
  Settings->SetValue("FileMgrShowTreePane", (int)FMTreePane->isVisible());
}

//==============================================================================