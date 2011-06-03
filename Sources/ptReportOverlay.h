/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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
/**
** Displays the status overlays in the view window.
** The overlay is displayed for duration() milliseconds. If you set the
** duration to 0 the overlay is displayed until you call stop().
**/

#ifndef PTREPORTOVERLAY_H
#define PTREPORTOVERLAY_H

#include <QTimer>
#include <QLabel>


///////////////////////////////////////////////////////////////////////////
//
// class ptViewWindow
//
///////////////////////////////////////////////////////////////////////////

class ptReportOverlay : public QLabel
{
  Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  explicit ptReportOverlay(QWidget* parent,
                           const QString& text,
                           const QColor& color,
                           const QColor& backgroundColor,
                           const int duration,
                           const Qt::Alignment pos,
                           const int padding);
  ~ptReportOverlay();

  inline int duration() { return m_Timer->interval(); }
  void exec(const QString& newText = "");
  void setColors(const QColor& color, const QColor& backgroundColor);
  inline void setDuration(const int msec) { m_Timer->setInterval(msec); }
  void stop();


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  int m_Padding;
  Qt::Alignment m_Position;  // only left or right for now
  QTimer* m_Timer;

  void UpdatePosition();


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  void TimerExpired();
};

#endif // PTREPORTOVERLAY_H
