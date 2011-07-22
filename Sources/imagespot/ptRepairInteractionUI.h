/*******************************************************************************
**
** Photivo
**
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

#ifndef PTREPAIRINTERACTIONUI_H
#define PTREPAIRINTERACTIONUI_H

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>

#include "ptCheck.h"
#include "ptChoice.h"
#include "ptBaseFoldBox.h"
#include "ptInput.h"

class ptRepairInteractionUI : public QDockWidget {
Q_OBJECT

public:
  explicit ptRepairInteractionUI(QWidget* parent = 0);

  QWidget* DockContainer;

  QHBoxLayout* GeneralHLayout;
    ptCheck* SpotEnabled;
    ptChoice* Algorithm;

  ptBaseFoldBox* SpotGroup;
    QHBoxLayout* SpotPosHLayout;
      QLabel* SpotPosLabel;
      ptInput* SpotPosX;
      ptInput* SpotPosY;
    QHBoxLayout* RadiusHLayout;
      QLabel* RadiusLabel;
      ptInput* RadiusW;
      ptInput* RadiusH;
    ptInput* Angle;
    QHBoxLayout* EdgeRadHLayout;
      QLabel* EdgeRadLabel;
      ptInput* EdgeRadius;
    ptInput* EdgeBlur;
    ptInput* Opacity;

  ptBaseFoldBox* RepairerGroup;
    ptCheck* RepairerEnabled;
    QHBoxLayout* RepPosHLayout;
      QLabel* RepPosLabel;
      ptInput* RepPosX;
      ptInput* RepPosY;


signals:

public slots:

};

#endif // PTREPAIRINTERACTIONUI_H
