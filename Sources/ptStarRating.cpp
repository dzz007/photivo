/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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

#include "ptStarRating.h"
#include <QPolygonF>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>

const int PaintingScaleFactor = 16;
QPolygonF starPolygon;

//------------------------------------------------------------------------------

void ptDrawStars(QPainter* painter, QWidget* widget, int x, int y, int size, int selectedStars, int totalStars) {
  if (starPolygon.isEmpty()) {
    for (int i = 0; i < 10; i++) {
      float radius = (i % 2) == 0 ? 0.5 : 0.2;
      starPolygon << QPointF(0.5 + radius * sin((i * 36) * 3.14 / 180), 0.5 - radius * cos((i * 36) * 3.14 / 180));
    }
  }

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->translate(x, y);
  painter->scale(size, size);

  for (int i = 0; i < totalStars; i++) {
    if (i < selectedStars) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(widget->palette().foreground());
    } else {
      painter->setPen(Qt::SolidLine);
      painter->setBrush(Qt::NoBrush);
    }
    painter->drawPolygon(starPolygon, Qt::WindingFill);
    painter->translate(1, 0);
  }
  painter->restore();
}

//------------------------------------------------------------------------------

ptStarRating::ptStarRating(QWidget *parent): QWidget(parent) {
  myStarCount = 0;
  myMaxStarCount = 6;
  setAutoFillBackground(true);

// add to QWidget specified in the ctor
  QHBoxLayout *Layout = new QHBoxLayout(parent);
  parent->setLayout(Layout);
  Layout->addWidget(this);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->setSpacing(0);
}

//------------------------------------------------------------------------------

QSize ptStarRating::sizeHint() const {
  return PaintingScaleFactor * QSize(myMaxStarCount, 1);
}

void ptStarRating::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);

  int yOffset = (rect().height() - PaintingScaleFactor) / 2;
  int x = rect().x();
  int y = rect().y() + yOffset;

  // draw resetter
  painter.setPen(Qt::SolidLine);
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(QPointF(x + PaintingScaleFactor / 2, y + PaintingScaleFactor / 2), 5, 5);

  ptDrawStars(&painter, this, x + PaintingScaleFactor, y, PaintingScaleFactor, myStarCount, myMaxStarCount);

  painter.restore();
}

//------------------------------------------------------------------------------

void ptStarRating::mouseReleaseEvent(QMouseEvent *event) {
  int star = starAtPosition(event->x());

  if (star != myStarCount && star != -1) {
    myStarCount = star;
    update();
    emit valueChanged();
  }
}

//------------------------------------------------------------------------------

int ptStarRating::starAtPosition(int x) {
  int star = x / (sizeHint().width() / myMaxStarCount);

  if (star < 0 || star >= myMaxStarCount) {
    return -1;
  }

  return star;
}

