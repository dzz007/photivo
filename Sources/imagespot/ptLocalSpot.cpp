/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptLocalSpot.h"
#include "../ptSettings.h"

//==============================================================================

ptLocalSpot::ptLocalSpot(QSettings *APtsFile)
: ptImageSpot(APtsFile),
  FColorShift(0.0),
  FHasMaxRadius(false),
  FIsAdaptiveSaturation(false),
  FIsEdgeAware(true),
  FLumaCurve(unique_ptr<ptCurve>(new ptCurve(ptCurveChannel_SpotLuma))),
  FLumaWeight(0.5),
  FMaxRadius(500),
  FMode(lamFloodFill),
  FSaturation(0.0),
  FThreshold(0.25)
{
  FLumaCurve->setId("LumaCurve");
  FLumaCurve.get()->SetNullCurve(ptCurveChannel_SpotLuma);

  if (APtsFile != nullptr) {
    FColorShift = APtsFile->value("ColorShift", 0.0).toFloat();
    FHasMaxRadius = APtsFile->value("HasMaxRadius", false).toBool();
    FIsAdaptiveSaturation = APtsFile->value("IsAdaptiveSaturation", false).toBool();
    FIsEdgeAware = APtsFile->value("IsEdgeAware", true).toBool();
    FLumaWeight = APtsFile->value("LumaWeight", 0.5).toFloat();
    FMaxRadius = APtsFile->value("MaxRadius", 500).toUInt();
    FMode = (ptLocalAdjustMode)APtsFile->value("Mode", lamFloodFill).toInt();
    FSaturation = APtsFile->value("Saturation", 0.0).toFloat();
    FThreshold = APtsFile->value("Threshold", 0.25).toFloat();
    FLumaCurve->ReadFromFile(APtsFile);
  }
}

//==============================================================================

bool ptLocalSpot::hasMaxRadius() const
{
  return FHasMaxRadius;
}

//==============================================================================

uint ptLocalSpot::maxRadius() const
{
  return FMaxRadius >> Settings->GetInt("Scaled");
}

//==============================================================================

void ptLocalSpot::setMaxRadius(const uint ARadius)
{
  FMaxRadius = ARadius << Settings->GetInt("Scaled");
}

//==============================================================================

void ptLocalSpot::WriteToFile(QSettings *APtsFile) {
  ptImageSpot::WriteToFile(APtsFile);

  APtsFile->setValue("ColorShift", FColorShift);
  APtsFile->setValue("HasMaxRadius", (int)FHasMaxRadius);
  APtsFile->setValue("IsAdaptiveSaturation", (int)FIsAdaptiveSaturation);
  APtsFile->setValue("IsEdgeAware", (int)FIsEdgeAware);
  APtsFile->setValue("LumaWeight", FLumaWeight);
  APtsFile->setValue("MaxRadius", FMaxRadius);
  APtsFile->setValue("Mode", FMode);
  APtsFile->setValue("Saturation", FSaturation);
  APtsFile->setValue("Threshold", FThreshold);

  FLumaCurve->WriteToFile(APtsFile);
}

//==============================================================================

