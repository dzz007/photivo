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
  Derived classes are strongly recommended to do the same!
*/

#ifndef PTIMAGESPOT_H
#define PTIMAGESPOT_H
//==============================================================================

#include <QtGlobal>
#include <QPoint>
#include <QString>

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
  ptImageSpot(QSettings *APtsFile = nullptr);

  /*! Create an image spot with specific values. */
  ptImageSpot(const uint ASpotX,
              const uint ASpotY,
              const uint ARadius,
              const short AIsEnabled,
              const QString &AName);

  /*! Returns the spot's enabled status. */
  inline short isEnabled() const { return FIsEnabled; }

  /*! Returns the horizontal radius. */
  inline uint radius() const { return FRadius >> Settings->GetInt("Scaled"); }

  /*! Returns the topleft position of the spot’s bounding rectangle. */
  QPoint pos() const;

  /*! Enables or disables the spot. Disabled spots are ignored when running the pipe.
      Values are \c 0 (disabled) or \c 2 (enabled), corresponding to what \c QListView
      checkboxes use. */
  inline void setEnabled(const short AState) { FIsEnabled = AState; }

  /*! Moves the spot to a new position.
      Coordinates are the topleft position of the spot’s bounding rectangle.

      Derived classes should re-implement \c setPos() to move the complete spot including repairers
      or any other additional elements. The default \c ptImageSpot implementation moves only the
      spot itself. */
  virtual void setPos(uint Ax, uint Ay);

  /*! Sets the horizontal radius in pixels. */
  void setRadius(uint ARadius);

  /*! Writes the spot’s data to the currently opened ini file.
    The ini’s \c WriteArray() must be set appropriately before you use this. */
  virtual void WriteToFile(QSettings *APtsFile);

//------------------------------------------------------------------------------

protected:
  short     FIsEnabled;
  QString   FName;
  uint      FRadius;
  QPoint    FPos;       // Position is the center of the spot

//------------------------------------------------------------------------------

private:


};
#endif
