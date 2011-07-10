/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
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
/*!
  \class ptBaseFoldBox

  \brief A simple foldable container widget.

  This class provides a container for grouping UI elements. It consists of a
  head with a caption and the actual content below. It can be folded to show
  only the head.
*/
#ifndef PTBASEFOLDBOX_H
#define PTBASEFOLDBOX_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>


class ptBaseFoldBox : public QWidget {
Q_OBJECT
////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////
public:
  explicit ptBaseFoldBox(QWidget* Parent,
                          const QString Title,
                          const QString ObjectName,
                          QWidget* Body = new QWidget());
  ~ptBaseFoldBox();

  inline QWidget* head() const { return m_Head; }
  inline QWidget* body() const { return m_Body; }
  inline short enabled() const { return m_Enabled; }
  inline short folded() const { return m_Folded; }
  inline QString title() const { return m_Title; }

  void toggleFolded();

////////////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
////////////////////////////////////////////////////////////////////////////////
protected:
  QWidget* m_Head;
  QWidget* m_Body;

  QLabel* m_ArrowLabel;
  QPixmap m_DefaultArrow[2];
  short m_Enabled;
  short m_Folded;
  QString m_Title;
  QLabel* m_TitleLabel;

  bool eventFilter(QObject* object, QEvent* event);
  void handleMousePress(QMouseEvent* event);

////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////
private:


////////////////////////////////////////////////////////////////////////////////
//
// slots
//
////////////////////////////////////////////////////////////////////////////////
public slots:
  void setEnabled(bool enabled);


////////////////////////////////////////////////////////////////////////////////
//
// signals
//
////////////////////////////////////////////////////////////////////////////////
signals:

};

#endif // PTBASEFOLDBOX_H
