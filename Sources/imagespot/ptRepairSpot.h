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
/*!
  \class ptRepairSpot

  \brief Class for storing repair spot data.

  This class stores the data for a single repair spot. The same rules as for \c ptImageSpot apply,
  i.e. values you pass to a \c ptRepairSpot object as well as return values are in current pipe
  size scale.
*/

#ifndef PTREPAIRSPOT_H
#define PTREPAIRSPOT_H

//==============================================================================

#include <stdint.h>

#include <QtGlobal>
#include <QPoint>

#include "ptImageSpot.h"
#include "../ptConstants.h"

//==============================================================================

class ptRepairSpot: public ptImageSpot {
public:
  /*! Static factory method returning a new default constructed \c ptRepairSpot object. */
  static ptRepairSpot *CreateSpot() { return new ptRepairSpot(); }

//------------------------------------------------------------------------------

  /*! Creates an empty and disabled repair spot using the clone algorithm or loads a spot
    from the current ini file.
    \param Ini
      Optional QSettings object representing a pts file. When is parameter is non-NULL
      the constructor will attempt to initialise the spot with data read from \c Ini.
      All values not present in the pts file are set to their default values. The ini’s
      \c ReadArray() must be set appropriately before you can use this.
  */
  explicit ptRepairSpot(QSettings *APtsFile = nullptr);

  /*! Create a repair spot with specific values.  */
// BJ: this constructor probably superfluous
//  ptRepairSpot(const uint spotX,
//               const uint spotY,
//               const uint radiusX,
//               const uint radiusY,
//               const short isEnabled,
//               const QString &AName,
//               const float angle,
//               const uint edgeRadius,
//               const float edgeSoftness,
//               const float opacity,
//               const ptSpotRepairAlgo algorithm,
//               const short hasRepairer = 0,
//               const uint repairerX = 0,
//               const uint repairerY = 0);

  /*! Returns the spot's rotation angle in degrees clockwise. */
  inline float angle() const { return FAngle; }

  /*! Returns the edge blur value. */
  inline float edgeSoftness() const { return FEdgeSoftness; }

  /*! Returns the radius of the blurred outer edge. */
  uint edgeRadius() const;

  /*! Returns \c 1 if the spot has a repairer, \c 0 otherwise.
    A repairer is only applicable for algorithms that need data from somewhere else in the image
    as the source for the repair action. The simplest example is clone that copies the image data
    from the repairer to the spot.
  */
  inline short hasRepairer() const { return FHasRepairer; }

  /*! Returns the global opacity. \c 0.0 is fully transparent and
      \c 1.0 is fully opaque. */
  inline float opactiy() const { return FOpacity; }

  /*! Returns the vertical radius. */
  uint radiusY() const;

  /*! Returns the position of the repairer’s center. */
  QPoint repairerPos() const;

  /*! Returns algorithm used for repairing. */
  inline ptSpotRepairAlgo algorithm() const { return FAlgorithm; }

  /*! Removes the repairer */
  inline void removeRepairer() { FHasRepairer = 0; }

  /*! Sets the repair algorithm. */
  void setAlgorithm(const ptSpotRepairAlgo algorithm) { FAlgorithm = algorithm;printf("#######algo: %d\n",FAlgorithm); }

  /*! Sets the spot's rotation angle in degrees clockwise. */
  void setAngle(float angle);

  /*! Sets edge blur. */
  void setEdgeSoftness(const float ASoftness);

  /*! Sets the size of the blurred edge. */
  void setEdgeRadius(uint ARadius);

  /*! Sets the spot's global opacity.
      \param opacity
        Opacity in the range from \c 0.0 (fully transparent) to \c 1.0
        (fully opaque). */
  void setOpacity(const float AOpacity);

  /*! Moves the complete spot (including a repairer) to a new position.
      Coordinates are the topleft position of the spot’s (not the complete shape’s!)
      bounding rectangle.
  */
  void setPos(uint Ax, uint Ay);

  /*! Sets the vertical radius in pixels. */
  void setRadiusY(uint ARadius);

  /*! Moves only the repairer to a new position. The spot stays at its original location.
      Use this function for setting the repairer’s initial position as well.
      Coordinates are the topleft position of the repairer’s bounding rectangle.
  */
  void setRepairerPos(const uint Ax, const uint Ay);

  /*! Moves only the spot to a new position. The repairer stays at its original location.
      Coordinates are the topleft position of the spot’s bounding rectangle.
  */
  void setSpotPos(const uint Ax, const uint Ay);

  /*! Writes all data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this.
  */
  void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

private:
  ptSpotRepairAlgo  FAlgorithm;
  float             FAngle;
  uint              FEdgeRadius;
  float             FEdgeSoftness;
  bool              FHasRepairer;
  bool              FInit;
  float             FOpacity;   // global transparency percentage
  uint              FRadiusY;
  QPoint            FRepairerPos;   // the spot providing the source for the repair data
  uint16_t          *FWeightMatrix;

  void UpdateWeight();


};
#endif // PTREPAIRSPOT_H
