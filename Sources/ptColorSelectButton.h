/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2015 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTCOLORSELECTBUTTON_H
#define PTCOLORSELECTBUTTON_H

#include "ptWidget.h"
#include <QColor>
#include <QToolButton>

class ptColorSelectButton: public ptWidget {
  Q_OBJECT

public:
  ptColorSelectButton(QWidget* AParent = nullptr);
  ptColorSelectButton(const ptCfgItem& ACfgItem, QWidget* AParent);

  void init(const ptCfgItem& ACfgItem) override;
  void setValue(const QVariant& AValue) override;

private:
  void updateColor(const QColor& AColor);
  QToolButton* FButton;
  QColor FCurrColor;

private slots:
  void onButtonClicked();
};

#endif // PTCOLORSELECTBUTTON_H
