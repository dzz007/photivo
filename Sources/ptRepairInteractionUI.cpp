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

#include "ptRepairInteractionUI.h"

ptRepairInteractionUI::ptRepairInteractionUI(QObject *parent)
: QDockWidget(parent)
{
  DockContainer = QWidget(this);
  DockContainer.setObjectName("DockContainer");

  GeneralHLayout = QHBoxLayout(DockContainer);
  GeneralHLayout.setObjectName("GeneralHLayout");
  SpotEnabled     = ptCheck(DockContainer,  "SpotEnabled",     "DockContainer",    0, tr("Enabled"), tr(""));
  Algorithm       = ptChoice(DockContainer, "Algorithm",       "GeneralHLayout",   1, 0, , tr("Repair mode"));
  HSpacer1 = QSpacerItem(20, 10, QSizePolicy::Expanding);

  // TODO SR: ptGroupBox SpotGroup
  SpotGroupWidget = QWidget(SpotGroup);
  SpotPosHLayout = QHBoxLayout(SpotGroupWidget);
  SpotPosHLayout.setObjectName("SpotPosHLayout");
  SpotPosLabel = QLabel(tr("Position"), SpotPosHLayout);
  SpotPosX = ptInput(DockContainer, "SpotPosX", "SpotPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0, tr("left"), tr("Horizontal position of spot's center"), 0);
  SpotPosY = ptInput(DockContainer, "SpotPosY", "SpotPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0, tr("top"), tr("Vertical position of spot's center"), 0);

  RadiusHLayout = QHBoxLayout(SpotGroupWidget);
  RadiusHLayout.setObjectName("RadiusHLayout");
  RadiusLabel = QLabel(tr("Radius"), RadiusHLayout);
  RadiusW = ptInput(DockContainer, "RadiusW", "RadiusHLayout", 0, 0, 1, 0, 0, 5000, 1, 0, tr("w"), tr("Width radius"), 0);
  RadiusH = ptInput(DockContainer, "RadiusH", "RadiusHLayout", 0, 0, 1, 0, 0, 5000, 1, 0, tr("h"), tr("Height radius"), 0);

  Angle = ptInput(DockContainer, "Angle", "SpotGroupWidget", 1, 0, 1, 0.0, -180.0, 180.0, 1, 1, tr("Angle"), tr("Rotate the spot"), 0);

  EdgeRadHLayout = QHBoxLayout(SpotGroupWidget);
  EdgeRadHLayout.setObjectName("EdgeRadHLayout");
  EdgeRadLabel = QLabel(tr("Edge radius"), EdgeRadHLayout);
  EdgeRadius = ptInput(DockContainer, "EdgeRadius", "EdgeRadHLayout", 0, 0, 1, 0, 0, 5000, 1, 0, tr("px"), tr("Radius of the spot's edge"), 0);

  EdgeBlur = ptInput(DockContainer, "EdgeBlur", "SpotGroupWidget", 1, 0, 1, 0.0, 0.0, 1.0, 0.05, 2, tr("Edge blur"), tr("Strength of edge blurring"), 0);
  Opacity =  ptInput(DockContainer, "Opacity",  "SpotGroupWidget", 1, 0, 1, 1.0, 0.0, 1.0, 0.05, 2, tr("Opacity"), tr("Opacity"), 0);


  // TODO SR: ptGroupBox RepairerGroup;
  RepGroupWidget = QWidget(RepairerGroup);
  RepairerEnabled = ptCheck(DockContainer, "RepairerEnabled", "RepGroupWidget", 0, tr("Enabled"), tr("Toggle repairer spot"));
  RepPosHLayout = QHBoxLayout(RepGroupWidget);
  RepPosHLayout.setObjectName("RepPosHLayout");
  RepPosLabel = QLabel(tr("Position"), RepPosHLayout);
  SpotPosX = ptInput(DockContainer, "RepPosX", "RepPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0, tr("left"), tr("Horizontal position of repairer's center"), 0);
  SpotPosY = ptInput(DockContainer, "RepPosY", "RepPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0, tr("top"), tr("Vertical position of repairer's center"), 0);
}
