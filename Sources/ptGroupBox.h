////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com> (mike)
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef DLGROUPBOX_H
#define DLGROUPBOX_H

#include <QtGui>

////////////////////////////////////////////////////////////////////////////////
//
// ptGroupBox is a modified groupbox
//
////////////////////////////////////////////////////////////////////////////////

class ptGroupBox : public QWidget {

Q_OBJECT

public :

ptGroupBox(const QString Title,
     QWidget* Parent,
     const QString Name);

// Destructor.
~ptGroupBox();

QWidget*   m_Widget;

void SetActive(const short IsActive);
void UpdateView();
void Update();

private slots:
void Hide();
void SetBlocked();
void Reset();
void PipeUpdate();

signals :

protected:
void mousePressEvent(QMouseEvent *event);
void paintEvent(QPaintEvent *);
void changeEvent(QEvent *);

private:
bool      m_Folded;
int       m_IsActive;
int       m_IsBlocked;
QPixmap   RightArrow;
QPixmap   DownArrow;
QPixmap   ActiveRightArrow;
QPixmap   ActiveDownArrow;
QPixmap   BlockedRightArrow;
QPixmap   BlockedDownArrow;
QWidget*  m_Header;
QLabel*   m_Icon;
QLabel*   m_Symbol;
QLabel*   m_Title;
QString   m_Name;
QLabel*   test;
QAction*  m_AtnHide;
QAction*  m_AtnBlock;
QAction*  m_AtnReset;
QTimer*   m_Timer;
int       m_NeedPipeUpdate;
};

#endif

////////////////////////////////////////////////////////////////////////////////

