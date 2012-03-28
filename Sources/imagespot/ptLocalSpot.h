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
  float             colorShift()           const { return FColorShift; }
  bool              hasMaxRadius()         const;
  bool              isAdaptiveSaturation() const { return FIsAdaptiveSaturation; }
  bool              isEdgeAware()          const { return FIsEdgeAware; }
  /*! Returns a pointer to the spot’s luminance curve. The curve is a read only member. */
  ptCurve           *lumaCurve()           const { return FLumaCurve.get(); }
  float             lumaWeight()           const { return FLumaWeight; }
  uint              maxRadius()            const;
  ptLocalAdjustMode mode()                 const { return FMode; }
  float             saturation()           const { return FSaturation; }
  float             threshold()            const { return FThreshold; }
  ///@}

  /*!
   *  \name Standard setters.
   */
  ///@{
  void setHasMaxRadius(      const bool AHasMaxRadius)      { FHasMaxRadius = AHasMaxRadius; }
  void setAdaptiveSaturation(const bool AIsAdaptive)        { FIsAdaptiveSaturation = AIsAdaptive; }
  void setColorShift        (const float AColorShift)       { FColorShift = AColorShift; }
  void setEdgeAware(         const bool AIsEdgeAware)       { FIsEdgeAware = AIsEdgeAware; }
  void setLumaWeight(        const float AWeight)           { FLumaWeight = AWeight; }
  void setMaxRadius(         const uint ARadius);
  void setMode(              const ptLocalAdjustMode AMode) { FMode = AMode; }
  void setSaturation(        const float ASaturation)       { FSaturation = ASaturation; }
  void setThreshold(         const float AThreshold)        { FThreshold = AThreshold; }
  ///@}

  /*! Writes all data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this.
  */
  void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

private:
  float               FColorShift;
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
