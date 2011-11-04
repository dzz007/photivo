/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include <QMouseEvent>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>

#include <cassert>

#include "../ptDefines.h"
#include "../ptTheme.h"
#include "ptPathBar.h"
#include "ptFileMgrConstants.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

extern ptTheme* Theme;

//==============================================================================

ptPathBar::ptPathBar(QWidget *parent): QWidget(parent) {
  m_IsMyComputer = false;
  this->setObjectName("PathBar");   // for CSS theme support
  this->setContextMenuPolicy(Qt::PreventContextMenu);

  // Editor and pretty display widgets are arranged in a vertical layout
  // but only one of them is ever shown at the same time, depending on
  // which mode is active (interactive display or text edit)
  m_Display = new QWidget(this);
  m_Display->setLayout(&m_Layout);
  m_Display->installEventFilter(this);

  m_Editor = new QLineEdit(this);
  m_Editor->hide();
  m_Editor->installEventFilter(this);
  connect(m_Editor, SIGNAL(editingFinished()), this, SLOT(afterEditor()));

  QVBoxLayout* MainLayout = new QVBoxLayout(this);
  MainLayout->setContentsMargins(0,0,0,0);
  MainLayout->setSpacing(0);
  this->setLayout(MainLayout);
  MainLayout->addWidget(m_Display);
  MainLayout->addWidget(m_Editor);

  // stretch for the pretty display layout to keep left alignment
  m_Stretch = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

  // Init QDir for generating subdirs lists
  m_DirInfo.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
  m_DirInfo.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
}

//==============================================================================

ptPathBar::~ptPathBar() {
  Clear();
  DelAndNull(m_Stretch);
}

//==============================================================================

ptPathBar::pbParseResult ptPathBar::Parse(QString path) {
  // Cleanup path and ensure "/" as directory separator
  path = QDir::cleanPath(QDir::fromNativeSeparators(path));

  if (QDir::match(BuildPath(m_Tokens.count()-1), path)) {
    return prSamePath;
  }

  // PathBar is for navigation through existing paths
  if (!QDir().exists(path)) return prFail;
  m_DirInfo.setPath(path);

#ifdef Q_OS_WIN
  if (path == MyComputerIdString) {
    m_IsMyComputer = true;
    Clear();
    m_Tokens.append(CreateToken(QObject::tr("My Computer"), 0));

  } else {
    m_IsMyComputer = false;
    // Paths that include a folder must start with "D:/" where D is a drive letter
    if (path.length() > 2 && path.mid(1,2) != ":/") return prFail;
    Clear();

    // Extract drive
    m_Tokens.append(CreateToken(WinApi::VolumeNamePretty(path.left(2)), 0));
    m_Tokens.at(0)->setDrive(true);
    path.remove(0, 3);
  }
#else
  if (path.left(1) != "/") return prFail;  // paths must be absolute, i.e. start with "/"
  Clear();
  m_Tokens.append(CreateToken("/", 0));
  path.remove(0, 1);
#endif

  int i = 1;
  QStringList subdirs;
  if (!m_IsMyComputer) {
    while (path.length() > 0) {
      int p = path.indexOf("/");

      m_Separators.append(CreateSeparator(i-1));
      m_Tokens.append(CreateToken(path.mid(0, p), i));

      if (p == -1) {
        path.clear();
      } else {
        path.remove(0, p+1);
      }

      i++;
    }

    subdirs = m_DirInfo.entryList();
  } else {
#ifdef Q_OS_WIN
    subdirs = WinApi::DrivesListPretty();
#else
    // should not happen on linux
    assert(!"Unhandled subdirs!");
#endif
  }

  if (subdirs.count() > 0) {
    m_Separators.append(CreateSeparator(i-1));
  }

  return prSuccess;
}

//==============================================================================

void ptPathBar::BuildWidgets() {
  for (int i = 0; i < m_Tokens.count(); i++) {
    m_Layout.addWidget(m_Tokens.at(i));
    if (i < m_Separators.count()) {
      m_Layout.addWidget(m_Separators.at(i));
    }
  }
  m_Layout.addItem(m_Stretch);
}

//==============================================================================

bool ptPathBar::setPath(const QString& path) {
  pbParseResult result = Parse(path);
  if (result >= prSuccess) {
    BuildWidgets();
  }
  return result >= prSuccess || result == prSamePath;
}

//==============================================================================

void ptPathBar::Clear() {
  // kill separators
  for (int i = 0; i < m_Separators.count(); i++) {
    delete m_Separators.at(i);
  }
  m_Separators.clear();

  // kill tokens
  for (int i = 0; i < m_Tokens.count(); i++) {
    delete m_Tokens.at(i);
  }
  m_Tokens.clear();

  // remove stretch from layout, makes re-building the layout easier
  m_Layout.removeItem(m_Stretch);
}

//==============================================================================

QString ptPathBar::path(const bool nativeSeparators /*= false*/) {
  if (nativeSeparators) {
    return QDir::fromNativeSeparators(BuildPath(m_Tokens.count()-1));
  } else {
    return BuildPath(m_Tokens.count()-1);
  }
}

//==============================================================================

QString ptPathBar::BuildPath(const int untilIdx) {
#ifdef Q_OS_WIN
  if (m_IsMyComputer) return MyComputerIdString;
#endif

  QString p;
  for (int i = 0; i <= untilIdx; i++) {
    if (m_Tokens.at(i)->isDrive()) {
      QString d = m_Tokens.at(i)->text().right(3);
      d.chop(1);
      p.append(d).append("/");
    } else {
      p.append(m_Tokens.at(i)->text()).append("/");
    }
  }

  p.chop(1);    // remove separator at the end
  return p;
}

//==============================================================================

ptPathBar::pbItem* ptPathBar::CreateSeparator(const int index) {
  pbItem* sep = new pbItem(this, index, false);
  sep->setTextFormat(Qt::PlainText);
  sep->setTextInteractionFlags(Qt::NoTextInteraction);
  sep->setPixmap(QString::fromUtf8(":/dark/ui-graphics/path-separator-normal.png"));
  sep->setContentsMargins(0, 0, 0, 0);
  sep->setObjectName("PathBarSeparator");
  sep->installEventFilter(this);
  return sep;
}

//==============================================================================

ptPathBar::pbItem* ptPathBar::CreateToken(const QString& text, const int index) {
  pbItem* token = new pbItem(this, index, true);
  token->setText(text);
  token->setTextFormat(Qt::PlainText);
  token->setContentsMargins(2, 0, 2, 0);
  token->setObjectName("PathBarToken");
  token->installEventFilter(this);
  return token;
}

//==============================================================================

void ptPathBar::afterEditor() {
  if (Parse(m_Editor->text()) >= prSuccess) {
    BuildWidgets();
    emit changedPath(BuildPath(m_Tokens.count()-1));
  }
  m_Editor->hide();
  m_Display->show();
}

//==============================================================================

void ptPathBar::ShowSubdirMenu(int idx) {
  pbItem* sender = m_Separators.at(idx);
  QString path = BuildPath(idx);
  m_DirInfo.setPath(path);
  QStringList subdirs = m_DirInfo.entryList();
  sender->setPixmap(QString::fromUtf8(":/dark/ui-graphics/path-separator-menuopen.png"));

  // build a list of menu actions from the subdirs list
  QList<QAction*> actions;
  actions.reserve(subdirs.count());

  for (int i = 0; i < subdirs.count(); i++) {
    actions.append(new QAction(QPixmap(QString::fromUtf8(":/dark/icons/folder.png")),
                               subdirs.at(i),
                               NULL) );
  }

  // show subdirs menu directly below the clicked separator icon
  QMenu* Menu = new QMenu(NULL);
  Menu->setPalette(Theme->ptMenuPalette);
  Menu->setStyle(Theme->ptStyle);
  Menu->addActions(actions);
  QAction* clicked = Menu->exec(sender->mapToGlobal(QPoint(0, sender->height())));

  // determine new path and signal FileMgrWindow
  if (clicked) {
#ifdef Q_OS_WIN
    if (path == MyComputerIdString) {
      path = clicked->text().right(3);
      path.chop(1);
    } else {
      path += "/" + clicked->text();
    }
#else
    path += "/" + clicked->text();
#endif
    emit changedPath(path);
  } else {
    sender->setPixmap(QString::fromUtf8(":/dark/ui-graphics/path-separator-normal.png"));
  }

  DelAndNull(Menu);
  for (int i = 0; i < actions.count(); i++) {
    delete actions.at(i);
  }
  actions.clear();
}

//==============================================================================

bool ptPathBar::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_Editor) {
    if(event->type() == QEvent::KeyPress) {
      if (((QKeyEvent*)event)->key() == Qt::Key_Escape &&
          ((QKeyEvent*)event)->modifiers() == Qt::NoModifier)
      {
        // ESC pressed: abort path text editing
        m_Editor->hide();
        m_Display->show();
        return true;
      }
    }
  }

  else {
    // from here on, only token and separator events
    pbItem* sender = dynamic_cast<pbItem*>(obj);
    if (sender == NULL) {
      event->ignore();
      return QWidget::eventFilter(obj, event);
    }

    if (sender->isToken() && event->type() == QEvent::MouseButtonRelease) {
      if (((QMouseEvent*)event)->button() == Qt::LeftButton) {
        emit changedPath(BuildPath(sender->index()));
        return true;
      }
    }

    else if (!sender->isToken() && event->type() == QEvent::MouseButtonPress) {
      if (((QMouseEvent*)event)->button() == Qt::LeftButton) {
        ShowSubdirMenu(sender->index());
        return true;
      }
    }
  }

  event->ignore();
  return QWidget::eventFilter(obj, event);
}

//==============================================================================

void ptPathBar::mousePressEvent(QMouseEvent* event) {
  event->accept();
}

//==============================================================================

void ptPathBar::mouseReleaseEvent(QMouseEvent* event) {
  event->accept();
  if (event->button() == Qt::RightButton) {
    m_Editor->blockSignals(true);
    m_Editor->setText(BuildPath(m_Tokens.count()-1));
    m_Editor->end(false);
    m_Editor->selectAll();
    m_Editor->show();
    m_Editor->setFocus(Qt::MouseFocusReason);
    m_Editor->blockSignals(false);
    m_Display->hide();
  }
}

//==============================================================================
