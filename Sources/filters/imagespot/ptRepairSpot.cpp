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

//==============================================================================

ptRepairSpot::ptRepairSpot(QSettings *APtsFile /*= nullptr*/)
: ptImageSpot(APtsFile),
  FAlgorithm    ((ptSpotRepairAlgo)
                 Settings->GetDefaultValue("RepairAlgorithm").toInt()),
  FAngle        (Settings->GetDefaultValue("RepairAngle").toFloat()),
  FEdgeRadius   (Settings->GetDefaultValue("RepairEdgeRadius").toUInt()),
  FEdgeSoftness (Settings->GetDefaultValue("RepairEdgeSoftness").toFloat()),
  FHasRepairer  (true),
  FOpacity      (Settings->GetDefaultValue("RepairOpacity").toFloat()),
  FRadiusSync   (Settings->GetDefaultValue("RepairRadiusSync").toBool()),
  FRadiusX      (Settings->GetDefaultValue("RepairRadiusX").toInt()),
  FRadiusY      (Settings->GetDefaultValue("RepairRadiusY").toInt()),
  FRepairerPos  (QPoint(0,0)),
  FWeightMatrix (nullptr)
{
  if (APtsFile != nullptr) {
    FAlgorithm      = (ptSpotRepairAlgo)
                      APtsFile->value("Algorithm",    FAlgorithm).toInt();
    FAngle          = APtsFile->value("Angle",        FAngle).toFloat();
    FEdgeRadius     = APtsFile->value("EdgeRadius",   FEdgeRadius).toUInt();
    FEdgeSoftness   = APtsFile->value("EdgeSoftness", FEdgeSoftness).toFloat();
    FHasRepairer    = APtsFile->value("HasRepairer",  FHasRepairer).toInt();
    FOpacity        = APtsFile->value("Opacity",      FOpacity).toFloat();
    FRadiusSync     = APtsFile->value("RadiusSync",   FRadiusSync).toBool();
    FRadiusX        = APtsFile->value("RadiusX",      FRadiusX).toUInt();
    FRadiusY        = APtsFile->value("RadiusY",      FRadiusY).toUInt();
    FRepairerPos.setX(APtsFile->value("RepairerPosX", 0).toInt());
    FRepairerPos.setY(APtsFile->value("RepairerPosY", 0).toInt());
  }
}

//==============================================================================

uint ptRepairSpot::edgeRadius() const {
  return FEdgeRadius >> Settings->GetInt("Scaled");
}

//==============================================================================

uint ptRepairSpot::radiusY() const {
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

void ptRepairSpot::setValue(const QString &AKey, const QVariant AValue) {
  bool hConvertOk = true;
  if      (AKey == "RepairAlgorithm")     setAlgorithm((ptSpotRepairAlgo)AValue.toInt(&hConvertOk));
  else if (AKey == "RepairRadiusX")       setRadiusX(AValue.toInt(&hConvertOk));
  else if (AKey == "RepairRadiusY")       setRadiusY(AValue.toInt(&hConvertOk));
  else if (AKey == "RepairAngle")         setAngle(AValue.toFloat(&hConvertOk));
  else if (AKey == "RepairEdgeRadius")    setEdgeRadius(AValue.toUInt(&hConvertOk));
  else if (AKey == "RepairEdgeSoftness")  setEdgeSoftness(AValue.toFloat(&hConvertOk));
  else if (AKey == "RepairOpacity")       setOpacity(AValue.toFloat(&hConvertOk));
  else if (AKey == "RepairRadiusSync")    setRadiusSync(AValue.toBool());
  else {
    printf("%s, line %d: Unrecognized member ID: %s\n", __FILE__, __LINE__, AKey.toLocal8Bit().data());
    assert(false);
  }

  if (!hConvertOk) {
    printf("%s, line %d: Could not convert from QVariant for member ID: %s\n",
           __FILE__, __LINE__, AKey.toLocal8Bit().data());
    assert(false);
  }
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

void ptRepairSpot::setRadiusX(uint ARadius) {
  FRadiusX = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
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
  APtsFile->setValue("Algorithm",     FAlgorithm);
  APtsFile->setValue("Angle",         FAngle);
  APtsFile->setValue("EdgeRadius",    FEdgeRadius);
  APtsFile->setValue("EdgeSoftness",  FEdgeSoftness);
  APtsFile->setValue("HasRepairer",   (int)FHasRepairer);
  APtsFile->setValue("Opacity",       FOpacity);
  APtsFile->setValue("RadiusSync",    FRadiusSync);
  APtsFile->setValue("RadiusX",       FRadiusX);
  APtsFile->setValue("RadiusY",       FRadiusY);
  APtsFile->setValue("RepairerPosX",  FRepairerPos.x());
  APtsFile->setValue("RepairerPosY",  FRepairerPos.y());
}

//==============================================================================

