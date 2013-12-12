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

#ifndef PTCOLORLABEL_H
#define PTCOLORLABEL_H

#include <QWidget>
#include <QMetaType>
#include <QPointF>
#include <QVector>
#include <QColor>

void ptDrawSingleColorLabel(QPainter* painter, QWidget* widget, int lbl, const QRect& rect, bool isSelected);

class ptColorLabel : public QWidget
{
    Q_OBJECT

public:
    ptColorLabel(QWidget *parent = 0);

    QSize sizeHint() const;
    QList<int> selectedLabels() const { return mySelectedLabels; }
    int selectedLabel();
    int maxLabelCount() const { return myMaxLabelCount; }
    void setSelectedLabel(int selectedLabel);
    void setMaxLabelCount(int maxLabelCount) { myMaxLabelCount = maxLabelCount; }
    void setMultiSelect(bool value) { myMultiSelect = value; }

signals:
    void valueChanged();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QList<int> mySelectedLabels;
    int myMaxLabelCount;
    bool myMultiSelect;
    int labelAtPosition(int x);
};


#endif
