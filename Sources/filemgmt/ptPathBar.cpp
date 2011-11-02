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
    TokenItem item = { CreateToken(QObject::tr("My Computer")), 0 };
    m_Tokens.append(item);

  } else {
    m_IsMyComputer = false;
    // Paths that include a folder must start with "D:/" where D is a drive letter
    if (path.length() > 2 && path.mid(1,2) != ":/") return false;
    Clear();

    // Extract drive
    TokenItem item = { CreateToken(WinApi::VolumeNamePretty(path.left(2))), 0 };
    m_Tokens.append(item);
    path.remove(0, 3);
  }
#else
  if (path.left(1) != "/") return false;  // paths must be absolute, i.e. start with "/"
  Clear();
  TokenItem item = { CreateToken("/"), 0 };
  m_Tokens.append(item);
  path.remove(0, 1);
#endif

  int i = 1;
  if (!m_IsMyComputer) {
    while (path.length() > 0) {
      int p = path.indexOf("/");

      m_Separators.append(CreateSeparator());
      TokenItem item = { CreateToken(path.mid(0, p)), i };
      m_Tokens.append(item);

      if (p == -1) {
        path.clear();
      } else {
        path.remove(0, p+1);
      }

      i++;
    }
  }

  // after the loop i is 1 higher than the last token index
  m_SeparatorCount = i-1;
  m_TokenCount = i;

  return true;
}

//==============================================================================

void ptPathBar::BuildWidgets() {
  for (int i = 0; i < m_TokenCount; i++) {
    m_Layout.addWidget(m_Tokens.at(i).Token);
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
  for (int i = 0; i < m_Separators.count(); i++) {
    delete m_Separators.at(i);
  }
  m_Separators.clear();

  for (int i = 0; i < m_Tokens.count(); i++) {
    delete m_Tokens.at(i).Token;
  }
  m_Tokens.clear();

  m_Layout.removeItem(m_Stretch);
}

//==============================================================================

QString ptPathBar::path(const bool nativeSeparators /*= false*/) {
#ifdef Q_OS_WIN
  if (m_IsMyComputer) return MyComputerIdString;
#endif

  QString p;
  QString sep = "/";
  if (nativeSeparators) sep = QDir::separator();

  for (int i = 0; i < m_TokenCount; i++) {
    p.append(m_Tokens.at(i).Token->text()).append(sep);
  }

  p.chop(1);    // remove separator at the end
  return p;
}

//==============================================================================

QLabel* ptPathBar::CreateSeparator() {
  QLabel* sep = new QLabel(this);
  sep->setTextFormat(Qt::PlainText);
  sep->setTextInteractionFlags(Qt::NoTextInteraction);
  sep->setPixmap(QString::fromUtf8(":/dark/ui-graphics/path-separator-normal.png"));
  sep->setContentsMargins(5, 0, 5, 0);
  return sep;
}

//==============================================================================

QLabel* ptPathBar::CreateToken(const QString& text) {
  QLabel* token = new QLabel(text, this);
  token->setTextFormat(Qt::PlainText);
  token->setContentsMargins(2, 0, 2, 0);
  return token;
}

//==============================================================================
