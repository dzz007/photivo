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
  \class ptImageSpot

  \brief Base class for storing image spot data.

  This class stores the data for a single image spot. Values you pass to a
  \c ptImageSpot object must always be in current pipe size scale. Similarly
  \c ptImageSpot always return values in current pipe size scale.

  However, internally (and in the ini file) everything is stored in 1:1 pipe size scale.
*/

#ifndef PTIMAGESPOT_H
#define PTIMAGESPOT_H
//==============================================================================

#include <QtGlobal>
#include <QPoint>
#include <stdint.h>

#include "../ptSettings.h"

//==============================================================================

class ptImageSpot {
public:
  /*! Creates an empty and disabled image spot or loads a spot from the current ini file.
    \param Ini
      Optional QSettings object representing a pts file. When is parameter is non-NULL
      the constructor will attempt to initialise the spot with data read from \c Ini.
      All values not present in the pts file are set to their default values. The ini’s
      \c ReadArray() must be set appropriately before you can use this.
  */
  ptImageSpot(QSettings *APtsFile = NULL);

  /*! Create an image spot with specific values. */
  ptImageSpot(const short isEnabled,
              const uint spotX,
              const uint spotY,
              const uint radiusX,
              const uint radiusY,
              const float angle,
              const uint edgeRadius,
              const float edgeBlur,
              const float opacity);

  /*! Returns the spot's rotation angle in degrees clockwise. */
  inline float angle() const { return FAngle; }

  /*! Returns the edge blur value. */
  inline float edgeBlur() const { return FEdgeSoftness; }

  /*! Returns the radius of the blurred outer edge. */
  inline uint edgeRadius() const { return FEdgeRadius >> Settings->GetInt("Scaled"); }

  /*! Returns the spot's enabled status. */
  inline short isEnabled() const { return FIsEnabled; }

  /*! Returns the global opacity. \c 0.0 is fully transparent and
      \c 1.0 is fully opaque. */
  inline float opactiy() const { return FOpacity; }

  /*! Returns the horizontal radius. */
  inline uint radiusX() const { return FRadiusY >> Settings->GetInt("Scaled"); }

  /*! Returns the vertical radius. */
  inline uint radiusY() const { return FRadiusX >> Settings->GetInt("Scaled"); }

  /*! Returns the topleft position of the spot’s bounding rectangle. */
  QPoint pos() const;

  /*! Sets the spot's rotation angle in degrees clockwise. */
  void setAngle(float angle);

  /*! Sets edge blur. */
  void setEdgeBlur(const float ABlur);

  /*! Sets the size of the blurred edge. */
  void setEdgeRadius(uint ARadius);

  /*! Enables or disables the spot. Disabled spots are ignored when running the pipe.
      Values are \c 0 (disabled) or \c 2 (enabled), corresponding to what \c QListView
      checkboxes use. */
  inline void setEnabled(const short state) { FIsEnabled = state; }

  /*! Sets the spot's global opacity.
      \param opacity
        Opacity in the range from \c 0.0 (fully transparent) to \c 1.0
        (fully opaque). */
  void setOpacity(const float AOpacity);

  /*! Moves the spot to a new position.
      Coordinates are the topleft position of the spot’s bounding rectangle.

      Derived classes should re-implement \c setPos() to move the complete spot including repairers
      or any other additional elements. The default \c ptImageSpot implementation moves only the
      spot itself. */
  virtual void setPos(uint Ax, uint Ay);

  /*! Sets the horizontal radius in pixels. */
  void setRadiusX(uint ARadius);

  /*! Sets the vertical radius in pixels. */
  void setRadiusY(uint ARadius);

  /*! Writes the spot’s data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this. */
  virtual void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

protected:
  float     FAngle;
  float     FEdgeSoftness;
  uint      FEdgeRadius;
  short     FIsEnabled;
  float     FOpacity;   // global transparency percentage
  uint      FRadiusX;
  uint      FRadiusY;
  QPoint    FPos;       // Position is the center of the spot
  uint16_t  *FWeightMatrix;

  void UpdateWeight();

//------------------------------------------------------------------------------

private:
  short FInit;


};
#endif
