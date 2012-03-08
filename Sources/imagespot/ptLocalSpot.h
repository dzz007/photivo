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

#ifndef PTLOCALSPOT_H
#define PTLOCALSPOT_H

//==============================================================================

#include "ptImageSpot.h"

//==============================================================================

class ptLocalSpot: public ptImageSpot {
public:
  explicit ptLocalSpot(QSettings *APtsFile = nullptr);

  // TODO BJ: not sure is this constructor is needed at all.
//  ptLocalSpot(const uint ASpotX,
//              const uint ASpotY,
//              const uint ARadius,
//              const short AIsEnabled,
//              const QString &AName,
//              const bool AHasMaxRadius,
//              const bool AIsAdaptiveSaturation,
//              const bool AIsEdgeAware,
//              const float ALumaWeight,
//              const uint AMaxRadius,
//              const ptLocalAdjustMode AMode,
//              const float ASaturation,
//              const float AThreshold);

  /*! Standard getters. */
  bool hasMaxRadius() { return FHasMaxRadius; }
  bool isAdaptiveSaturation() { return FIsAdaptiveSaturation; }
  bool isEdgeAware() { return FIsEdgeAware; }
  float lumaWeight() { return FLumaWeight; }
  uint maxRadius() { return FMaxRadius; }
  ptLocalAdjustMode mode() { return FMode; }
  float saturation() { return FSaturation; }
  float threshold() { return FThreshold; }

  /*! Standard setters. */
  void setHasMaxRadius(const bool AHasMaxRadius) { FHasMaxRadius = AHasMaxRadius; }
  void setAdaptiveSaturation(const bool AIsAdaptive) { FIsAdaptiveSaturation = AIsAdaptive; }
  void setEdgeAware(const bool AIsEdgeAware) { FIsEdgeAware = AIsEdgeAware; }
  void setLumaWeight(const float AWeight) { FLumaWeight = AWeight; }
  void setMaxRadius(const uint ARadius) { FMaxRadius = ARadius; }
  void setMode(const ptLocalAdjustMode AMode) { FMode = AMode; }
  void setSaturation(const float ASaturation) { FSaturation = ASaturation; }
  void setThreshold(const float AThreshold) { FThreshold = AThreshold; }

  /*! Writes all data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this.
  */
  void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

private:
  // TODO BJ: Add curves everywhere. They’re completely non-functional now.
  bool                FHasMaxRadius;
  bool                FIsAdaptiveSaturation;
  bool                FIsEdgeAware;
  float               FLumaWeight;
  uint                FMaxRadius;
  ptLocalAdjustMode   FMode;
  float               FSaturation;
  float               FThreshold;


};
#endif // PTLOCALSPOT_H
