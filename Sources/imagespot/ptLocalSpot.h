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

#include <memory>
using std::unique_ptr;

#include "ptImageSpot.h"
#include "../ptCurve.h"
#include "../ptSettings.h"

//==============================================================================

class ptLocalSpot: public ptImageSpot {
public:
  /*! Static factory method returning a new default constructed \c ptLocalSpot object. */
  static ptLocalSpot *CreateSpot() { return new ptLocalSpot(); }

//------------------------------------------------------------------------------

  explicit ptLocalSpot(QSettings *APtsFile = nullptr);

  /*!
   *  \name Standard getters.
   *  Standard getter methods. See the the setters documentation for detailed descriptions.
   */
  ///@{
  bool              hasMaxRadius() { return FHasMaxRadius >> Settings->GetInt("Scaled"); }
  bool              isAdaptiveSaturation() { return FIsAdaptiveSaturation; }
  bool              isEdgeAware() { return FIsEdgeAware; }
  /*! Returns a pointer to the spot’s luminance curve. The curve is a read only member. */
  ptCurve           *lumaCurve() { return FLumaCurve.get(); }
  float             lumaWeight() { return FLumaWeight; }
  uint              maxRadius() { return FMaxRadius >> Settings->GetInt("Scaled"); }
  ptLocalAdjustMode mode() { return FMode; }
  float             saturation() { return FSaturation; }
  float             threshold() { return FThreshold; }
  ///@}

  /*!
   *  \name Standard setters.
   */
  ///@{
  void setHasMaxRadius(const bool AHasMaxRadius) { FHasMaxRadius = AHasMaxRadius; }
  void setAdaptiveSaturation(const bool AIsAdaptive) { FIsAdaptiveSaturation = AIsAdaptive; }
  void setEdgeAware(const bool AIsEdgeAware) { FIsEdgeAware = AIsEdgeAware; }
  void setLumaWeight(const float AWeight) { FLumaWeight = AWeight; }
  void setMaxRadius(const uint ARadius) { FMaxRadius = ARadius << Settings->GetInt("Scaled"); }
  void setMode(const ptLocalAdjustMode AMode) { FMode = AMode; }
  void setSaturation(const float ASaturation) { FSaturation = ASaturation; }
  void setThreshold(const float AThreshold) { FThreshold = AThreshold; }
  ///@}

  /*! Writes all data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this.
  */
  void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

private:
  bool                FHasMaxRadius;
  bool                FIsAdaptiveSaturation;
  bool                FIsEdgeAware;
  unique_ptr<ptCurve> FLumaCurve;
  float               FLumaWeight;
  uint                FMaxRadius;
  ptLocalAdjustMode   FMode;
  float               FSaturation;
  float               FThreshold;


};
#endif // PTLOCALSPOT_H
