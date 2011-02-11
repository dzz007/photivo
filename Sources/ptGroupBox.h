/*******************************************************************************
**
** Photivo
**
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

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
void SetHelpUri(const QString Uri);
void UpdateView();
void Update();
void SetEnabled(const short Enabled);

private slots:
void Hide();
void SetBlocked();
void Reset();
void PipeUpdate();
void SaveSettings();
void AppendSettings();

signals :

protected:
void mousePressEvent(QMouseEvent *event);
void paintEvent(QPaintEvent *);
void changeEvent(QEvent *);

private:
void WriteSettings(const short Append);
bool      m_Folded;
short     m_IsActive;
short     m_IsBlocked;
short     m_IsEnabled;
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
QLabel*   m_HelpIcon;
QString   m_Name;
QString   m_HelpUri;
QLabel*   test;
QAction*  m_AtnHide;
QAction*  m_AtnBlock;
QAction*  m_AtnReset;
QAction*  m_AtnSavePreset;
QAction*  m_AtnAppendPreset;
QTimer*   m_Timer;
int       m_NeedPipeUpdate;
};

#endif

////////////////////////////////////////////////////////////////////////////////

