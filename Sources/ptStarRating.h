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

// ATZ: this is a deeply modified StarRating control from qt examples

#ifndef PTSTARRATING_H
#define PTSTARRATING_H

#include <QWidget>
#include <QMetaType>
#include <QPointF>
#include <QVector>

void ptDrawStars(QPainter* painter, QWidget* widget, int x, int y, int size, int selectedStars, int totalStars);

class ptStarRating : public QWidget
{
    Q_OBJECT

public:
    ptStarRating(QWidget *parent = 0);

    QSize sizeHint() const;
    int starCount() const { return myStarCount; }
    int maxStarCount() const { return myMaxStarCount; }
    void setStarCount(int starCount) { myStarCount = starCount; update(); }
    void setMaxStarCount(int maxStarCount) { myMaxStarCount = maxStarCount; }

signals:
    void valueChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    int myStarCount;
    int myMaxStarCount;
    int starAtPosition(int x);
};


#endif
