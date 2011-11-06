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
#include <QLabel>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QEvent>
#include <QPoint>
#include <QLineEdit>
#include <QDir>

//==============================================================================
/*!
  \class ptPathBar

  \brief Provides an interactive display widget for file system paths.
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


private:
  // slight extension of QLabel to include some special flags
  class pbItem: public QLabel {
  public:
    pbItem(QWidget* parent, const int index, const bool isToken)
      : QLabel(parent), m_IsDrive(false), m_IsToken(isToken), m_Index(index) {}
    int index() { return m_Index; }
    bool isDrive() { return m_IsDrive; }
    bool isToken() { return m_IsToken; }
    void setDrive(const bool isDrive) { m_IsDrive = isDrive; }
  private:
    bool m_IsDrive;
    bool m_IsToken;  // if it’s not a token it’s a separator
    int  m_Index;    // index in the m_Tokens or m_Separators QVector
  };

  // successes must be >=0, failures <0
  enum pbParseResult {
    prSamePath  = -2,
    prFail      = -1,
    prSuccess   = 0
  };

  QString BuildPath(const int untilIdx);
  void BuildWidgets();
  void Clear();
  pbItem* CreateSeparator(const int index);
  pbItem* CreateToken(const QString& text, const int index);
  pbParseResult Parse(QString path);
  void ShowSubdirMenu(int idx);

  QWidget*            m_Display;
  QLineEdit*          m_Editor;
  bool                m_IsMyComputer;
  QHBoxLayout         m_Layout;
  int                 m_SeparatorCount;
  QVector<pbItem*>    m_Separators;
  QDir                m_DirInfo;
  QSpacerItem*        m_Stretch;
  int                 m_TokenCount;
  QVector<pbItem*>    m_Tokens;


private slots:
  void afterEditor();


signals:
  void changedPath(const QString& newPath);


//==============================================================================
};
#endif // PTPATHBAR_H
