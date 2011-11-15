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

#ifndef PTPATHBAR_H
#define PTPATHBAR_H

//==============================================================================

#include <QWidget>
#include <QVector>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QEvent>
#include <QPoint>
#include <QLineEdit>
#include <QDir>
#include <QToolButton>
#include <QStackedLayout>

//==============================================================================
/*!
  \class ptPathBar

  \brief Provides an interactive display widget for file system paths.

  ptPathBar’s widget structure is a bit complex. For a graphical overview see
  ReferenceMaterial/ptPathbar widget structure.svg
*/
class ptPathBar: public QWidget {
Q_OBJECT
public:
  /*! Creates a new \c ptPathBar instance. */
  explicit ptPathBar(QWidget* parent = NULL);

  /*! Destroys a \c ptPathBar object. */
  ~ptPathBar();

  /*! Returns the path the bar is currently set to.
    \param nativeSeparators
      If set to \c true returns the path with native directory separators, e.g.
      “/” on Linux or “\” on Windows. Otherwise “/” will be used regardless of
      the underlying OS. Default is \c false.

      On Windows returns \c MyComputerIdString if the path is set to “My Computer”.
  */
  QString path(const bool nativeSeparators = false);

  /*! Sets the path. You can use native as well as Qt separators.
    Returns \c true if the path actually exists in the file system
    and could be parsed by the PathBar. Also returns \c true if the current and
    new path are same, but the PathBar is not updated in that case.
    \param path
      The path. Must be an absolute path corresponding to an existing file system
      location.

      On Linux must start with “/”.

      On Windows must start with “D:” where D is a drive letter. \c path can also be
      the special string indicating “My Computer” that is defined in the
      \c MyComputerIdString constant.
  */
  bool setPath(const QString& path);


protected:
  bool eventFilter(QObject* obj, QEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void resizeEvent(QResizeEvent* event);
  void showEvent(QShowEvent* event);


private:
  class pbToken: public QWidget {
  public:
    pbToken(QWidget* parent, QString path): QWidget(parent), m_Path(path) {}
    QString path() { return m_Path; }
  private:
    QString m_Path;
  };

  class pbButton: public QToolButton {
  public:
    pbButton(QWidget* parent, int idx): QToolButton(parent), m_Index(idx) {}
    int index() { return m_Index; }
  private:
    int m_Index;
  };

  // successes must be >=0, failures <0
  enum pbParseResult {
    prSamePath  = -2,
    prFail      = -1,
    prSuccess   = 0
  };

  enum pbWidgetPos {
    wpLeftMost  = -1,
    wpRightMost = 1
  };

  struct pbRange {
    int begin;
    int end;
  };

  QString       BuildPath           (const int untilIdx);
  void          Clear               ();
  pbToken*      CreateToken         (int idx, const QString& fullPath, QString dirName);
  pbParseResult Parse               (QString path);
  void          ShowSubdirMenu      (int idx);
  void          ShowVisibleWidgets  (int startIdx, pbWidgetPos pos);

  QDir                m_DirInfo;
  QWidget*            m_PrettyDisplay;
  QLineEdit*          m_Editor;
  QToolButton*        m_GoLeftButton;
  QToolButton*        m_GoRightButton;
  QWidget*            m_InnerContainer;
  bool                m_IsMyComputer;
  QWidget*            m_Tokens;
  QHBoxLayout*        m_TokenLayout;
  QVector<pbToken*>   m_TokenList;
  pbRange             m_VisibleRange;   // refers to the resp. m_Tokens indexes
  QStackedLayout*     m_WidgetStack;


private slots:
  void afterEditor();
  void buttonClicked();
  void goLeftClicked();
  void goRightClicked();
  void separatorClicked(bool checked);


signals:
  void changedPath(const QString& newPath);


//==============================================================================
};
#endif // PTPATHBAR_H
