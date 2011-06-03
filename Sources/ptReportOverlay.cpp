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

#include <cassert>

#include "ptReportOverlay.h"


ptReportOverlay::ptReportOverlay(QWidget* parent, const QString& text, const QColor& color,
                                 const QColor& backgroundColor, const int duration,
                                 const Qt::Alignment pos, const int padding)
: QLabel(text, parent),
  m_Padding(padding), m_Position(pos)
{
  setVisible(false);
  //setTextFormat(Qt::RichText);
  setTextFormat(Qt::PlainText);
  setAlignment(Qt::AlignCenter);
  setTextInteractionFlags(Qt::NoTextInteraction);
  QFont TheFont(this->font());
  TheFont.setBold(true);
  TheFont.setPixelSize(22);
  setFont(TheFont);
  setColors(color, backgroundColor);
  UpdatePosition();

  m_Timer = new QTimer(this);
  m_Timer->setSingleShot(true);
  m_Timer->setInterval(duration);
  connect(m_Timer, SIGNAL(timeout()), this, SLOT(TimerExpired()));
}



ptReportOverlay::~ptReportOverlay() {
  delete m_Timer;
}



void ptReportOverlay::setColors(const QColor& color, const QColor& backgroundColor) {
  QString stylesheet =
    "QLabel {"
    "  color: " + color.name() + ";"
    "  background-color: " + backgroundColor.name() + ";"
    "  border: 10px solid " + color.name() + ";"
    "  border-radius: 25px;"
    "  padding: 12px 24px 12px 24px;"
    "  text-align: center;"
    "}";
  setStyleSheet(stylesheet);
}



void ptReportOverlay::exec(const QString& newText /*= ""*/) {
  if (m_Timer->isActive()) {
    m_Timer->stop();
    setVisible(false);
  }

  if (newText != "") {
    setText(newText);
    UpdatePosition();
  }

  setVisible(true);

  if (m_Timer->interval() > 0) {
    m_Timer->start();
  }
}



void ptReportOverlay::stop() {
  m_Timer->stop();
  setVisible(false);
}



void ptReportOverlay::TimerExpired() {
  setVisible(false);
}



void ptReportOverlay::UpdatePosition() {
  if (m_Position == Qt::AlignLeft) {
    move(m_Padding, m_Padding);
  } else if (m_Position == Qt::AlignRight) {
    move(parentWidget()->width() - m_Padding - this->width(), m_Padding);
  } else {
    assert(!"Only AlignLeft or AlignRight allowed!");
  }
}
