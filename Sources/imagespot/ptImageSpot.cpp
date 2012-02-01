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

#include "ptImageSpot.h"
#include "../ptSettings.h"

extern ptSettings* Settings;

//==============================================================================

ptImageSpot::ptImageSpot(QSettings *APtsFile /*= NULL*/)
: FAngle(0.0),
  FEdgeSoftness(0.0),
  FEdgeRadius(0),
  FIsEnabled(0),
  FOpacity(1.0),
  FRadiusX(0),
  FRadiusY(0),
  FWeightMatrix(NULL)
{
  FPos = QPoint();

  if (APtsFile != NULL) {
    FAngle = APtsFile->value("Angle", 0.0).toFloat();
    FEdgeSoftness = APtsFile->value("EdgeBlur", 0.0).toFloat();
    FEdgeRadius = APtsFile->value("EdgeRadius", 0).toUInt();
    FIsEnabled = APtsFile->value("IsEnabled", 0).toInt();
    FOpacity = APtsFile->value("Opacity", 1.0).toFloat();
    FRadiusX = APtsFile->value("RadiusX", 0).toUInt();
    FRadiusY = APtsFile->value("RadiusY", 0.).toUInt();
    FPos.setX(APtsFile->value("SpotPosX", 0).toInt());
    FPos.setY(APtsFile->value("SpotPosY", 0).toInt());
  }
  printf("=========spotdata at spot ini constructor===========\n"
         "x %d  y %d\n"
         "w %d  h %d\n"
         "angle %f\n==========================\n",
         FPos.x(), FPos.y(), FRadiusY, FRadiusX, FAngle
         );
}

//==============================================================================

ptImageSpot::ptImageSpot(const short isEnabled,
                         const uint spotX,
                         const uint spotY,
                         const uint radiusX,
                         const uint radiusY,
                         const float angle,
                         const uint edgeRadius,
                         const float edgeBlur,
                         const float opacity)
{
  int toFullPipe = 1 << Settings->GetInt("PipeSize");
  FEdgeRadius = edgeRadius * toFullPipe;

  FAngle = angle * toFullPipe;
  FIsEnabled = isEnabled * toFullPipe;
  FRadiusX = radiusX * toFullPipe;
  FRadiusY = radiusY * toFullPipe;

  FInit = 1;
  FPos = QPoint(spotX * toFullPipe, spotY * toFullPipe);
  setEdgeBlur(edgeBlur);
  setOpacity(opacity);
  UpdateWeight();
  FInit = 0;

//  printf("=========spotdata at spot normal constructor===========\n"
//         "x %d  y %d\n"
//         "w %d  h %d\n"
//         "angle %f\n==========================\n",
//         m_Pos.x(), m_Pos.y(), m_RadiusY, m_RadiusX, m_Angle
//         );
}

//==============================================================================

QPoint ptImageSpot::pos() const {
  return QPoint(FPos.x() >> Settings->GetInt("Scaled"),
                FPos.y() >> Settings->GetInt("Scaled") );
}

//==============================================================================

void ptImageSpot::setAngle(float FAngle) {
  FAngle = FAngle;
  UpdateWeight();
}

//==============================================================================

void ptImageSpot::setEdgeBlur(const float ABlur) {
  if (ABlur > 1.0f) {
    FEdgeSoftness = 1.0;
  } else if (ABlur < 0.0f){
    FEdgeSoftness = 0.0;
  } else {
    FEdgeSoftness = ABlur;
  }
  if (!FInit) {
    UpdateWeight();
  }
}

//==============================================================================

void ptImageSpot::setEdgeRadius(uint ARadius) {
  FEdgeRadius = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
}

//==============================================================================

void ptImageSpot::setRadiusY(uint ARadius) {
  FRadiusY = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
}

//==============================================================================

void ptImageSpot::setRadiusX(uint ARadius) {
  FRadiusX = ARadius << Settings->GetInt("Scaled");
  UpdateWeight();
}

//==============================================================================

void ptImageSpot::setOpacity(const float AOpacity) {
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

void ptImageSpot::setPos(uint Ax, uint Ay) {
  FPos.setX(Ax);
  FPos.setY(Ay);
}

//==============================================================================

void ptImageSpot::UpdateWeight() {
  // TODO SR: alpha channel calculation
}

//==============================================================================

void ptImageSpot::WriteToFile(QSettings *APtsFile) {
  APtsFile->setValue("Angle", FAngle);
  APtsFile->setValue("EdgeBlur", FEdgeSoftness);
  APtsFile->setValue("EdgeRadius", FEdgeRadius);
  APtsFile->setValue("IsEnabled", FIsEnabled);
  APtsFile->setValue("Opacity", FOpacity);
  APtsFile->setValue("RadiusX", FRadiusX);
  APtsFile->setValue("RadiusY", FRadiusY);
  APtsFile->setValue("SpotPosX", FPos.x());
  APtsFile->setValue("SpotPosY", FPos.y());
}

//==============================================================================

