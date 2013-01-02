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

#include <cassert>

#include <QMouseEvent>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>

#include "../ptDefines.h"
#include "../ptTheme.h"
#include "ptPathBar.h"
#include "ptFileMgrConstants.h"

#ifdef Q_OS_WIN
  #include "../ptWinApi.h"
#endif

extern ptTheme* Theme;

//==============================================================================

ptPathBar::ptPathBar(QWidget* parent)
: QWidget(parent),
  m_IsMyComputer(false)  // always stays at false on non-Windows OSes
{
  this->setContextMenuPolicy(Qt::PreventContextMenu);

  // Init QDir for generating subdirs lists
  m_DirInfo.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives);
  m_DirInfo.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

  //-------------------------------------

  m_PrettyDisplay = new QWidget(this);
  m_PrettyDisplay->setObjectName("PBPrettyDisplay");
  m_PrettyDisplay->setStyleSheet(Theme->stylesheet());

  m_Editor = new QLineEdit(this);
  m_Editor->setObjectName("PBEditor");
  m_Editor->installEventFilter(this);
  connect(m_Editor, SIGNAL(editingFinished()), this, SLOT(afterEditor()));

  m_WidgetStack = new QStackedLayout(this);
  m_WidgetStack->setContentsMargins(0,0,0,0);
  m_WidgetStack->setSpacing(0);
  m_WidgetStack->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_WidgetStack->addWidget(m_PrettyDisplay);
  m_WidgetStack->addWidget(m_Editor);
  m_WidgetStack->setCurrentIndex(0);
  this->setLayout(m_WidgetStack);

  //-------------------------------------

  // tolbuttons to navigate through paths too long to display
  m_GoLeftButton = new QToolButton(m_PrettyDisplay);
  m_GoLeftButton->hide();
  m_GoLeftButton->setIconSize(QSize(16, 16));
  m_GoLeftButton->setText("");
  m_GoLeftButton->setIcon(QIcon(Theme->IconGoPrevious));
  connect(m_GoLeftButton, SIGNAL(clicked()), this, SLOT(goLeftClicked()));
  m_GoRightButton = new QToolButton(m_PrettyDisplay);
  m_GoRightButton->hide();
  m_GoRightButton->setIconSize(QSize(16, 16));
  m_GoRightButton->setText("");
  m_GoRightButton->setIcon(QIcon(Theme->IconGoNext));
  connect(m_GoRightButton, SIGNAL(clicked()), this, SLOT(goRightClicked()));

  // this is the actual display area for the interactive path
  m_InnerContainer = new QWidget(m_PrettyDisplay);
  m_InnerContainer->setObjectName("PBInnerContainer");

  QHBoxLayout* l = new QHBoxLayout(m_PrettyDisplay);
  l->setContentsMargins(0,0,0,0);
  l->setSpacing(3);
  l->addWidget(m_GoLeftButton);
  l->addWidget(m_InnerContainer);
  l->addWidget(m_GoRightButton);
  m_PrettyDisplay->setLayout(l);

  //-------------------------------------

  m_Tokens = new QWidget(m_InnerContainer);

  m_TokenLayout = new QHBoxLayout(m_Tokens);
  m_TokenLayout->setContentsMargins(0,0,0,0);
  m_TokenLayout->setSpacing(0);
  m_TokenLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_Tokens->setLayout(m_TokenLayout);

  //-------------------------------------

  m_Animation.setTargetObject(m_Tokens);
  m_Animation.setPropertyName("pos");
  m_Animation.setDuration(100);
}

//==============================================================================

ptPathBar::~ptPathBar() {
  Clear();
}

//==============================================================================

ptPathBar::pbParseResult ptPathBar::Parse(QString path) {
#ifdef Q_OS_WIN
  m_IsMyComputer = (path == MyComputerIdString);
#endif

  if (!m_IsMyComputer) {
    // Clean up path, ensure / separators and expand leading ~
    path = QDir::fromNativeSeparators(path);
    if (path == "~" || path.leftRef(2) == "~/") {
      path = QDir::homePath() + path.mid(1);
    }
    path = QDir::cleanPath(path);

    if (QDir(BuildPath(m_TokenList.count()-1)) == QDir(path)) {
      return prSamePath;
    }

    // PathBar is for navigation through existing paths
    if (!QDir().exists(path)) return prFail;
    m_DirInfo.setPath(path);
  }

  QString pathSoFar;

#ifdef Q_OS_WIN
  if (!m_IsMyComputer) {
    // Paths that include a folder must start with "D:/" where D is a drive letter
    if (path.length() > 2 && path.mid(1,2) != ":/") return prFail;
  }

  Clear();

  // “My Computer” is always the first entry
  m_TokenList.append(CreateToken(0, MyComputerIdString, ""));

  // drive name/letter
  if (!m_IsMyComputer) {
    pathSoFar = path.left(2);
    m_TokenList.append(CreateToken(m_TokenList.count(),
                                   pathSoFar,
                                   WinApi::VolumeNamePretty(pathSoFar)));
    path.remove(0, 3);
  }

#else
  if (path.left(1) != "/") return prFail;  // paths must be absolute, i.e. start with "/"
  Clear();
  m_TokenList.append(CreateToken(0, "/", "/"));
  path.remove(0, 1);
  // pathSoFar must stay empty here, otherwise we’ll end up with a double slash at its beginning.
#endif

  // build widget lists for tokens and separators
  if (!m_IsMyComputer) {
    while (path.length() > 0) {
      int p = path.indexOf("/");
      pathSoFar = pathSoFar % "/" % path.mid(0, p);
      m_TokenList.append(CreateToken(m_TokenList.count(), pathSoFar, path.mid(0, p)));

      if (p == -1) {
        path.clear();
      } else {
        path.remove(0, p+1);
      }
    }
  }

  // add new widgets to layout
  for (int i = 0; i < m_TokenList.count(); i++) {
    m_TokenLayout->addWidget(m_TokenList.at(i));
    m_TokenList.at(i)->show();
  }
  m_Tokens->adjustSize();

  return prSuccess;
}

//==============================================================================

void ptPathBar::MoveVisibleArea(pbMovement dir, bool fromButton /*= false*/) {
  if (m_TokenList.count() <= 0) return;
  int newLeft = 0;

  if (m_Tokens->width() <= m_PrettyDisplay->width()) {
    // path is short enough to be displayed completely
    m_GoLeftButton->hide();
    m_GoRightButton->hide();
    newLeft = 0;

  } else {
    m_GoLeftButton->show();
    m_GoRightButton->show();
    int maxLeft = m_InnerContainer->width() - m_Tokens->width();

    switch (dir) {
      case mvResizeCheck:
        if (maxLeft - m_Tokens->x() > 0) {
          // resize produced free space on the right
          newLeft = maxLeft;
        } else {
          newLeft = m_Tokens->x();
        }
        break;

      case mvLeftMost:
        newLeft = 0;
        break;

      case mvRightMost:
        newLeft = maxLeft;
        break;

      case mvToLeft: {
        newLeft = qMin(0, m_Tokens->x() + m_InnerContainer->width());
        // negatives because FindNextTokenPos needs and returns m_Tokens coordinates.
        int clamped = 0-FindNextTokenPos(0-newLeft, dir);
        if (clamped > m_Tokens->x()) newLeft = clamped;
        break;
      }

      case mvToRight: {
        newLeft = qMax(maxLeft, m_Tokens->x() - m_InnerContainer->width());
        int clamped = 0-FindNextTokenPos(0-newLeft, dir);
        if (clamped < m_Tokens->x()) newLeft = clamped;
        break;
      }

      default:
        assert(!"Unknown pbWidgetPos!");
        break;
    }
  }

  if (fromButton) {
    m_Animation.setStartValue(m_Tokens->pos());
    m_Animation.setEndValue(QPoint(newLeft, 0));
    m_Animation.start();
  } else {
    m_Tokens->move(newLeft, 0);
  }
}

//==============================================================================

int ptPathBar::FindNextTokenPos(int pos, pbMovement dir) {
  if (dir == mvRightMost) {
    return m_TokenList.last()->x();
  } else if (dir == mvLeftMost) {
    return 0;
  }

  int x = pos;
  QVectorIterator<pbToken*> i(m_TokenList);
  while (i.hasNext()) {
    if ((i.peekNext()->x() <= x) &&
        (i.peekNext()->x() + i.peekNext()->width() >= x))
    { // we found the token containing x
      if (dir == mvToLeft) {
        x = i.peekNext()->x() + i.peekNext()->width() + 1;
      } else if (dir == mvToRight) {
        x = i.peekNext()->x();
      }
    }
    i.next();
  }

  return x;
}

//==============================================================================

bool ptPathBar::setPath(const QString& path) {
  pbParseResult result = Parse(path);
  if (result >= prSuccess) {
    MoveVisibleArea(mvRightMost);
  }
  return result >= prSuccess || result == prSamePath;
}

//==============================================================================

void ptPathBar::Clear() {
  // kill tokens
  for (int i = 0; i < m_TokenList.count(); i++) {
    delete m_TokenList.at(i);
  }
  m_TokenList.clear();
}

//==============================================================================

QString ptPathBar::path(const bool nativeSeparators /*= false*/) {
  if (nativeSeparators) {
    return QDir::fromNativeSeparators(BuildPath(m_TokenList.count()-1));
  } else {
    return BuildPath(m_TokenList.count()-1);
  }
}

//==============================================================================

QString ptPathBar::BuildPath(const int untilIdx) {
#ifdef Q_OS_WIN
  if (m_IsMyComputer) return MyComputerIdString;
#endif

  if (untilIdx >= 0 && untilIdx < m_TokenList.count()) {
    return m_TokenList.at(untilIdx)->path();
  } else {
    return "";
  }
}

//==============================================================================

void ptPathBar::afterEditor() {
  if (Parse(m_Editor->text()) >= prSuccess) {
    MoveVisibleArea(mvRightMost);
    emit changedPath(BuildPath(m_TokenList.count()-1));
  }
  m_WidgetStack->setCurrentWidget(m_PrettyDisplay);
}

//==============================================================================

void ptPathBar::goLeftClicked() {
  if (m_Tokens->x() != 0) {
    MoveVisibleArea(mvToLeft, true);
  }
}

//==============================================================================

void ptPathBar::goRightClicked() {
  if (m_Tokens->x() != (m_InnerContainer->width() - m_Tokens->width())) {
    MoveVisibleArea(mvToRight, true);
  }
}

//==============================================================================

bool ptPathBar::eventFilter(QObject* obj, QEvent* event) {
  if (obj == m_Editor) {
    if(event->type() == QEvent::KeyPress) {
      if (((QKeyEvent*)event)->key() == Qt::Key_Escape &&
          ((QKeyEvent*)event)->modifiers() == Qt::NoModifier)
      {
        // ESC pressed: abort path text editing
        m_WidgetStack->setCurrentWidget(m_PrettyDisplay);
        return true;
      }
    }
  }

  // unhandled events fall through to here
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
    m_Editor->setText(QDir::toNativeSeparators(BuildPath(m_TokenList.count()-1)));
    m_Editor->end(false);
    m_Editor->selectAll();
    m_Editor->setFocus(Qt::MouseFocusReason);
    m_Editor->blockSignals(false);
    m_WidgetStack->setCurrentWidget(m_Editor);
  }
}

//==============================================================================

void ptPathBar::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  MoveVisibleArea(mvResizeCheck);
}

//==============================================================================

void ptPathBar::showEvent(QShowEvent *event) {
  this->setStyleSheet(Theme->stylesheet());
  m_InnerContainer->setStyleSheet(Theme->stylesheet());
  QWidget::showEvent(event);
}

//==============================================================================

ptPathBar::pbToken* ptPathBar::CreateToken(int idx, const QString& fullPath, QString dirName) {
  pbToken* token = new pbToken(this, fullPath);
  token->setContextMenuPolicy(Qt::PreventContextMenu);

  QHBoxLayout* layout = new QHBoxLayout(token);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);
  token->setLayout(layout);

  // create button for the directory’s name
  pbButton* caption = new pbButton(token, idx);
#ifdef Q_OS_WIN
  if (fullPath == MyComputerIdString) {
    caption->setIcon(QIcon(QString::fromUtf8(":/dark/icons/my-computer.png")));
  }
#endif
  caption->setText(dirName);
  caption->setObjectName("PBDirCaption");
  connect(caption, SIGNAL(clicked()), this, SLOT(buttonClicked()));
  layout->addWidget(caption);

  // if the folder has subfolders, create a separator
  m_DirInfo.setPath(fullPath);
  if (m_DirInfo.entryList().size() > 0 || fullPath == MyComputerIdString) {
    pbButton* separator = new pbButton(token, idx);
    separator->setCheckable(true);
    separator->setIconSize(QSize(16, 16));
    separator->setIcon(QIcon(QString::fromUtf8(":/dark/ui-graphics/path-separator-normal.png")));
    separator->setObjectName("PBDirSeparator");
    connect(separator, SIGNAL(clicked(bool)), this, SLOT(separatorClicked(bool)));
    separator->setFixedHeight(this->height());
    layout->addWidget(separator);
  }

  return token;
}

//==============================================================================

void ptPathBar::buttonClicked() {
  pbButton* button = dynamic_cast<pbButton*>(this->sender());
  if (button == NULL) return;

  if (Parse(BuildPath(button->index())) >= prSuccess) {
    MoveVisibleArea(mvRightMost);
    emit changedPath(BuildPath(m_TokenList.count()-1));
  }
}

//==============================================================================

void ptPathBar::separatorClicked(bool checked) {
  if (!checked) return;
  pbButton* button = dynamic_cast<pbButton*>(this->sender());
  if (button == NULL) return;

  button->setIcon(QIcon(QString::fromUtf8(":/dark/ui-graphics/path-separator-menuopen.png")));
  m_DirInfo.setPath(BuildPath(button->index()));
  QStringList subdirs;

#ifdef Q_OS_WIN
  if (button->index() == 0)   // index 0 means “My Computer”
    subdirs = WinApi::DrivesListPretty();
  else
#endif
    subdirs = m_DirInfo.entryList();

  // build a list of menu actions from the subdirs list
  QList<QAction*> actions;
#if (QT_VERSION >= 0x40700)
  actions.reserve(subdirs.count());
#endif

  for (int i = 0; i < subdirs.count(); i++) {
    actions.append(new QAction(QPixmap(QString::fromUtf8(":/dark/icons/folder.png")),
                               subdirs.at(i),
                               NULL) );
  }

  // show subdirs menu directly below the clicked separator icon
  QMenu* Menu = new QMenu(NULL);
  Menu->setPalette(Theme->menuPalette());
  Menu->setStyle(Theme->style());
  Menu->addActions(actions);
  QAction* clicked = Menu->exec(button->mapToGlobal(QPoint(0, button->height())));

  // determine new path and signal FileMgrWindow
  if (clicked) {
    QString activatedPath;
#ifdef Q_OS_WIN
    if (button->index() == 0) {
      // since index 0 is “My Computer” clicked path must be a drive
      activatedPath = clicked->text().right(3);
      activatedPath.chop(1);
    } else {
      activatedPath = BuildPath(button->index()) + "/" + clicked->text();
    }
#else
    activatedPath = BuildPath(button->index()) + "/" + clicked->text();
#endif
    if (Parse(activatedPath) >= prSuccess) {
      MoveVisibleArea(mvRightMost);
      emit changedPath(activatedPath);
    }
  } else {
    button->setIcon(QIcon(QString::fromUtf8(":/dark/ui-graphics/path-separator-normal.png")));
  }

  DelAndNull(Menu);
  for (int i = 0; i < actions.count(); i++) {
    delete actions.at(i);
  }
  actions.clear();
}

//==============================================================================
