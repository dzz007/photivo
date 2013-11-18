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

#include <QtGui>
#include "ptColorLabel.h"

const int PaintingScaleFactor = 16;
QList<QColor> colorTable;

void ptDrawSingleColorLabel(QPainter* painter, QWidget* widget, int lbl, const QRect& rect, bool isSelected) {
  if (colorTable.isEmpty()) {
    colorTable << QColor("black") << QColor("red") << QColor("yellow") << QColor("green") << QColor("blue") << QColor("purple");
  }

  if (isSelected) {
    painter->setBrush(colorTable[lbl]);
  } else {
    painter->setBrush(widget->palette().background());
  }

  painter->setPen(Qt::NoPen);
  painter->drawRect(rect);

  if (isSelected) {
    if (lbl == 0) {
      painter->setPen(QColor(255,255,255));
    } else {
      painter->setPen(QColor(0,0,0));
    }
  } else {
    painter->setPen(colorTable[lbl]);
  }
  painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, QString::number(lbl));
}

ptColorLabel::ptColorLabel(QWidget *parent)
    : QWidget(parent)
{
  mySelectedLabels << 0;
  myMaxLabelCount = 6;
  myMultiSelect = false;
  setAutoFillBackground(true);

// add to QWidget specified in the ctor
  QHBoxLayout *Layout = new QHBoxLayout(parent);
  parent->setLayout(Layout);
  Layout->addWidget(this);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setMargin(0);
  Layout->setSpacing(0);
}

QSize ptColorLabel::sizeHint() const
{
  return PaintingScaleFactor * QSize(myMaxLabelCount, 1);
}

void ptColorLabel::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, false);

  int yOffset = (rect().height() - PaintingScaleFactor) / 2;
  QRect r(rect().x(), rect().y() + yOffset, PaintingScaleFactor, PaintingScaleFactor);

  for (int i = 0; i < myMaxLabelCount; ++i) {
    r.moveLeft(i * PaintingScaleFactor);
    ptDrawSingleColorLabel(&painter, this, i, r, mySelectedLabels.contains(i));
  }

  painter.restore();
}

void ptColorLabel::mouseReleaseEvent(QMouseEvent *event)
{
  int lbl = labelAtPosition(event->x());

  if (lbl != -1) {
    if (myMultiSelect && lbl != 0) {
      if (mySelectedLabels.contains(0)) {
        mySelectedLabels.removeAll(0);
      }
      if (mySelectedLabels.contains(lbl)) {
        mySelectedLabels.removeAll(lbl);
      } else {
        mySelectedLabels.append(lbl);
      }
      if (mySelectedLabels.count() == 0) {
        mySelectedLabels.append(0);
      }
    } else {
      setSelectedLabel(lbl);
    }
    update();
    emit valueChanged();
  }
}

int ptColorLabel::labelAtPosition(int x)
{
  int lbl = x / (sizeHint().width() / myMaxLabelCount);
  if (lbl < 0 || lbl >= myMaxLabelCount)
    return -1;

  return lbl;
}

int ptColorLabel::selectedLabel() { 
  if (mySelectedLabels.count() > 0) {
    return mySelectedLabels.first(); 
  }
  return -1;
}

void ptColorLabel::setSelectedLabel(int selectedLabel) { 
  mySelectedLabels.clear(); 
  mySelectedLabels << selectedLabel;
  update(); 
}
