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
#include "ptTheme.h"
#include "ptGuiOptions.h"

extern ptTheme* Theme;

ptRepairInteractionUI::ptRepairInteractionUI(QWidget* parent)
: QDockWidget(parent)
{
  this->setStyle(Theme->ptStyle);
  this->setStyleSheet(Theme->ptStyleSheet);
  DockContainer = new QWidget(this);
  DockContainer->setObjectName("DockContainer");
  QVBoxLayout* MainVLayout = new QVBoxLayout();
  DockContainer->setLayout(MainVLayout);

  // Global enable and algorithm choice
  GeneralHLayout = new QHBoxLayout(DockContainer);
  GeneralHLayout->setObjectName("GeneralHLayout");

  SpotEnabled = new ptCheck(DockContainer,  "SpotEnabled", DockContainer->objectName(),
                            0, tr("Enabled"), tr(""));
  Algorithm = new ptChoice(DockContainer, "Algorithm", DockContainer->objectName(),
                           1, ptSpotRepairAlgo_Copy, GuiOptions->SpotRepairAlgorithm,
                           tr("Repair mode"), ptTimeout_Input);
  GeneralHLayout->addWidget(SpotEnabled);
  GeneralHLayout->addWidget(Algorithm);
  GeneralHLayout->addStretch();


  // Controls for the spot that is to be repaired
  SpotGroup = new ptBaseFoldBox(DockContainer, tr("Spot"), "SpotGroup");
  SpotPosHLayout = new QHBoxLayout(SpotGroup->body());
  SpotPosHLayout->setObjectName("SpotPosHLayout");
  SpotPosLabel = new QLabel(tr("Position"));
  SpotPosX = new ptInput(DockContainer, "SpotPosX", SpotGroup->body()->objectName(), 0, 0, 1, 0, 0, 10000, 1, 0,
                         tr("left"), tr("Horizontal position of spot's center"), 0);
  SpotPosY = new ptInput(DockContainer, "SpotPosY", SpotGroup->body()->objectName(), 0, 0, 1, 0, 0, 10000, 1, 0,
                         tr("top"), tr("Vertical position of spot's center"), 0);
  SpotPosHLayout->addWidget(SpotPosLabel);
  SpotPosHLayout->addWidget(SpotPosX);
  SpotPosHLayout->addWidget(SpotPosY);
  SpotPosHLayout->addStretch();

  RadiusHLayout = new QHBoxLayout(SpotGroup->body());
  RadiusHLayout->setObjectName("RadiusHLayout");
  RadiusLabel = new QLabel(tr("Radius"));
  RadiusW = new ptInput(DockContainer, "RadiusW", SpotGroup->body()->objectName(), 0, 0, 1, 0, 0, 5000, 1, 0,
                        tr("w"), tr("Width radius"), ptTimeout_Input);
  RadiusH = new ptInput(DockContainer, "RadiusH", SpotGroup->body()->objectName(), 0, 0, 1, 0, 0, 5000, 1, 0,
                        tr("h"), tr("Height radius"), ptTimeout_Input);
  RadiusHLayout->addWidget(RadiusLabel);
  RadiusHLayout->addWidget(RadiusW);
  RadiusHLayout->addWidget(RadiusH);
  RadiusHLayout->addStretch();

  Angle = new ptInput(DockContainer, "Angle", SpotGroup->body()->objectName(), 1, 0, 1,
                      0.0, -180.0, 180.0, 1, 1, tr("Angle"), tr("Rotate the spot"), ptTimeout_Input);

  EdgeRadHLayout = new QHBoxLayout(SpotGroup->body());
  EdgeRadHLayout->setObjectName("EdgeRadHLayout");
  EdgeRadLabel = new QLabel(tr("Edge radius"));
  EdgeRadius = new ptInput(DockContainer, "EdgeRadius", SpotGroup->body()->objectName(), 0, 0, 1, 0, 0, 5000, 1, 0,
                           tr("px"), tr("Radius of the spot's edge"), ptTimeout_Input);
  EdgeRadHLayout->addWidget(EdgeRadLabel);
  EdgeRadHLayout->addWidget(EdgeRadius);
  EdgeRadHLayout->addStretch();

  EdgeBlur = new ptInput(DockContainer, "EdgeBlur", SpotGroup->body()->objectName(), 1, 0, 1,
                         0.0, 0.0, 1.0, 0.05, 2, tr("Edge blur"), tr("Strength of edge blurring"), 0);
  Opacity =  new ptInput(DockContainer, "Opacity",  SpotGroup->body()->objectName(), 1, 0, 1,
                         1.0, 0.0, 1.0, 0.05, 2, tr("Opacity"), tr("Opacity"), 0);


  // Controls for the optional repairer
  RepairerGroup = new ptBaseFoldBox(DockContainer, tr("Repairer"), "RepairerGroup");
  RepairerEnabled = new ptCheck(DockContainer, "RepairerEnabled", RepairerGroup->body()->objectName(), 0,
                                tr("Enabled"), tr("Toggle repairer spot"));
  RepPosHLayout = new QHBoxLayout(RepairerGroup->body());
  RepPosHLayout->setObjectName("RepPosHLayout");
  RepPosLabel = new QLabel(tr("Position"));
  RepPosX = new ptInput(DockContainer, "RepPosX", "RepPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0,
                        tr("left"), tr("Horizontal position of repairer's center"), ptTimeout_Input);
  RepPosY = new ptInput(DockContainer, "RepPosY", "RepPosHLayout", 0, 0, 1, 0, 0, 10000, 1, 0,
                        tr("top"), tr("Vertical position of repairer's center"), ptTimeout_Input);
  RepPosHLayout->addWidget(RepPosLabel);
  RepPosHLayout->addWidget(RepPosX);
  RepPosHLayout->addWidget(RepPosY);
  RepPosHLayout->addStretch();
}
