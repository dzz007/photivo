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

  /*! Sets the path. Returns \c true if the path actually exists in the file system
    and could be parsed by the PathBar.
    \param path
      The path. Must be an absolute path corresponding to an existing file system
      location.

      On Linux must start with “/”.

      On Windows must start with “D:” where D is a drive letter. \c path can also be
      the special string indicating “My Computer” that is defined in the
      \c MyComputerIdString constant.
  */
  bool setPath(const QString& path);


private:
  void BuildWidgets();
  void Clear();
  QLabel* CreateSeparator();
  QLabel* CreateToken(const QString& text);
  /* Returns true if the path exists and could be parsed. */
  bool Parse(QString path);

  struct TokenItem {
    QLabel*   Token;
    int       Idx;
  };

  bool                m_IsMyComputer;
  QHBoxLayout         m_Layout;
  int                 m_SeparatorCount;
  QVector<QLabel*>    m_Separators;
  QSpacerItem*        m_Stretch;
  int                 m_TokenCount;
  QVector<TokenItem>  m_Tokens;


signals:
  void changedPath(const QString& newPath);

public slots:


//==============================================================================
};
#endif // PTPATHBAR_H
