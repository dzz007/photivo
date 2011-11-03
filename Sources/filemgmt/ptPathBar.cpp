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

#include <QDir>
#include <QMouseEvent>
#include <QAction>
#include <QMenu>

#include "../ptDefines.h"
#include "ptPathBar.h"
#include "ptFileMgrConstants.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

//==============================================================================

ptPathBar::ptPathBar(QWidget *parent): QWidget(parent) {
  m_IsMyComputer = false;
  m_TokenCount = 0;
  m_SeparatorCount = 0;
  this->setLayout(&m_Layout);
  this->setObjectName("PathBar");   // for CSS theme support

  m_Stretch = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
}

//==============================================================================

ptPathBar::~ptPathBar() {
  Clear();
  DelAndNull(m_Stretch);
}

//==============================================================================

bool ptPathBar::Parse(QString path) {
  // Cleanup path and ensure "/" as directory separator
  path = QDir::cleanPath(QDir::fromNativeSeparators(path));
  // PathBar is for navigation through existing paths
  if (!QDir().exists(path)) return false;

#ifdef Q_OS_WIN
  if (path == MyComputerIdString) {
    m_IsMyComputer = true;
    Clear();
    m_Tokens.append(CreateToken(QObject::tr("My Computer"), 0));

  } else {
    m_IsMyComputer = false;
    // Paths that include a folder must start with "D:/" where D is a drive letter
    if (path.length() > 2 && path.mid(1,2) != ":/") return false;
    Clear();

    // Extract drive
    m_Tokens.append(CreateToken(WinApi::VolumeNamePretty(path.left(2)), 0));
    m_Tokens.at(0)->setDrive(true);
    path.remove(0, 3);
  }
#else
  if (path.left(1) != "/") return false;  // paths must be absolute, i.e. start with "/"
  Clear();
  m_Tokens.append(CreateToken("/", 0));
  path.remove(0, 1);
#endif

  int i = 1;
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
    m_Separators.append(CreateSeparator(i-1));
  }

  m_SeparatorCount = m_Separators.count();
  m_TokenCount = m_Tokens.count();

  return true;
}

//==============================================================================

void ptPathBar::BuildWidgets() {
  for (int i = 0; i < m_TokenCount; i++) {
    m_Layout.addWidget(m_Tokens.at(i));
    if (i < m_SeparatorCount) {
      m_Layout.addWidget(m_Separators.at(i));
    }
  }
  m_Layout.addItem(m_Stretch);
}

//==============================================================================

bool ptPathBar::setPath(const QString& path) {
  bool result = Parse(path);
  if (result) BuildWidgets();
  return result;
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
    return QDir::fromNativeSeparators(BuildPath(m_TokenCount));
  } else {
    return BuildPath(m_TokenCount);
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

void ptPathBar::ShowSubdirMenu(const QPoint& pos, int idx) {
  QString path = BuildPath(idx);
  QDir dir = QDir(path);
  dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
  dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
  QStringList subdirs = dir.entryList();
  QList<QAction*> actions;
  actions.reserve(subdirs.count());

  for (int i = 0; i < subdirs.count(); i++) {
    actions.append(new QAction(QPixmap(QString::fromUtf8(":/dark/icons/folder.png")),
                               subdirs.at(i),
                               this) );
  }

  QMenu Menu;
  Menu.addActions(actions);
  QAction* clicked = Menu.exec(pos);

  if (clicked) {
    path += "/" + clicked->text();

    for (int i = 0; i < actions.count(); i++) {
      delete actions.at(i);
    }

    emit changedPath(path);
  }
}

//==============================================================================

bool ptPathBar::eventFilter(QObject* obj, QEvent* event) {
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
      ShowSubdirMenu(sender->mapToGlobal(sender->pos()), sender->index());
    }
  }

  event->ignore();
  return QWidget::eventFilter(obj, event);
}

//==============================================================================
