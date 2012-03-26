/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptRepairSpot.h"
#include "../ptSettings.h"

extern ptSettings* Settings;

//==============================================================================

ptRepairSpot::ptRepairSpot(QSettings *APtsFile /*= nullptr*/)
: ptImageSpot(APtsFile),
  FAlgorithm(SpotRepairAlgo_Clone),
  FHasRepairer(1),
  FRepairerPos(QPoint())
{
  if (APtsFile != nullptr) {
    FAngle = APtsFile->value("Angle", 0.0).toFloat();
    FEdgeSoftness = APtsFile->value("EdgeBlur", 0.0).toFloat();
    FEdgeRadius = APtsFile->value("EdgeRadius", 0).toUInt();
    FOpacity = APtsFile->value("Opacity", 1.0).toFloat();
    FRadiusY = APtsFile->value("RadiusY", 0.).toUInt();
    FHasRepairer = APtsFile->value("HasRepairer", 1).toInt();
    FAlgorithm =
        (ptSpotRepairAlgo)(APtsFile->value("Algorithm", SpotRepairAlgo_Clone).toInt());
    FRepairerPos.setX(APtsFile->value("RepairerPosX", 0).toInt());
    FRepairerPos.setY(APtsFile->value("RepairerPosY", 0).toInt());
  }
}

//==============================================================================

uint ptRepairSpot::edgeRadius() const
{
  return FEdgeRadius >> Settings->GetInt("Scaled");
}

//==============================================================================

uint ptRepairSpot::radiusY() const
{
  return FRadiusY >> Settings->GetInt("Scaled");
}

//==============================================================================

//ptRepairSpot::ptRepairSpot(const uint spotX,
//                           const uint spotY,
//                           const uint radiusX,
//                           const uint radiusY,
//                           const short isEnabled,
//                           const QString &AName,
//                           const float angle,
//                           const uint edgeRadius,
//                           const float edgeSoftness,
//                           const float opacity,
//                           const ptSpotRepairAlgo algorithm,
//                           const short hasRepairer /*= 0*/,
//                           const uint repairerX /*= 0*/,
//                           const uint repairerY /*= 0*/)
//: ptImageSpot(spotX, spotY, radiusX, isEnabled, AName),
//  FAlgorithm(algorithm),
//  FAngle(angle),
//  FEdgeRadius(edgeRadius),
//  FEdgeSoftness(edgeSoftness),
//  FHasRepairer(hasRepairer),
//  FOpacity(opacity),
//  FRadiusY(radiusY)
//{
//  int hToFullPipe = 1 << Settings->GetInt("PipeSize");
//  FEdgeRadius = edgeRadius * hToFullPipe;
//  FRadiusY = radiusY * hToFullPipe;

//  FAngle = angle * hToFullPipe;
//  FInit = true;
//  setEdgeSoftness(edgeSoftness);
//  setOpacity(opacity);
//  UpdateWeight();
//  FInit = false;

//  FRepairerPos = QPoint(repairerX * (1 >> Settings->GetInt("PipeSize")),
//                         repairerY * (1 >> Settings->GetInt("PipeSize")) );
//}

//==============================================================================

QPoint ptRepairSpot::repairerPos() const {
  return QPoint(FRepairerPos.x() * (1 >> Settings->GetInt("PipeSize")),
                FRepairerPos.y() * (1 >> Settings->GetInt("PipeSize")) );
}

//==============================================================================

void ptRepairSpot::setPos(uint Ax, uint Ay) {
  Ax *= (1 << Settings->GetInt("PipeSize"));
  Ay *= (1 << Settings->GetInt("PipeSize"));

  if (FHasRepairer) {
    FRepairerPos.setX(FRepairerPos.x() + FPos.x() - Ax);
    FRepairerPos.setY(FRepairerPos.y() + FPos.y() - Ay);
  }
  FPos.setX(Ax);
  FPos.setY(Ay);
}

//==============================================================================

void ptRepairSpot::setRepairerPos(const uint Ax, const uint Ay) {
  FHasRepairer = true;
  FRepairerPos.setX(Ax * (1 << Settings->GetInt("PipeSize")));
  FRepairerPos.setY(Ay * (1 << Settings->GetInt("PipeSize")));
}

//==============================================================================

void ptRepairSpot::setSpotPos(const uint Ax, const uint Ay) {
  FPos.setX(Ax * (1 << Settings->GetInt("PipeSize")));
  FPos.setY(Ay * (1 << Settings->GetInt("PipeSize")));
}

//==============================================================================

void ptRepairSpot::setAngle(float FAngle) {
  FAngle = FAngle;
  UpdateWeight();
}

//==============================================================================

void ptRepairSpot::setEdgeSoftness(const float ASoftness) {
  if (ASoftness > 1.0f) {
    FEdgeSoftness = 1.0;
  } else if (ASoftness < 0.0f){
    FEdgeSoftness = 0.0;
  } else {
    FEdgeSoftness = ASoftness;
  }
  if (!FInit) {
    UpdateWeight();
  }
}

//==============================================================================

void ptRepairSpot::setEdgeRadius(uint ARadius) {
  FEdgeRadius = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
}

//==============================================================================

void ptRepairSpot::setOpacity(const float AOpacity) {
  if (AOpacity > 1.0f) {
    FOpacity = 1.0;
  } else {
    FOpacity = AOpacity;
  }
  if (!FInit) {
    UpdateWeight();
  }
}

//==============================================================================

void ptRepairSpot::setRadiusY(uint ARadius) {
  FRadiusY = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
}

//==============================================================================

void ptRepairSpot::UpdateWeight() {
  // TODO SR: alpha channel calculation
}

//==============================================================================

void ptRepairSpot::WriteToFile(QSettings *APtsFile) {
  ptRepairSpot::WriteToFile(APtsFile);
  APtsFile->setValue("Angle", FAngle);
  APtsFile->setValue("EdgeBlur", FEdgeSoftness);
  APtsFile->setValue("EdgeRadius", FEdgeRadius);
  APtsFile->setValue("Opacity", FOpacity);
  APtsFile->setValue("RadiusY", FRadiusY);
  APtsFile->setValue("HasRepairer", FHasRepairer);
  APtsFile->setValue("Algorithm", FAlgorithm);
  APtsFile->setValue("RepairerPosX", FRepairerPos.x());
  APtsFile->setValue("RepairerPosY", FRepairerPos.y());
}

//==============================================================================

