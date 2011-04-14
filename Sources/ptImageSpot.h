/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTIMAGESPOT_H
#define PTIMAGESPOT_H

#include <QtGlobal>
#include <QPoint>


class ptImageSpot {
////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////
private:
  short m_IsEnabled;
  short m_HasRepairer;
  short m_Mode;         // repair algorithm: use ptSpotMode constants
  double m_Angle;
  uint m_EdgeRadius;
  uint m_RadiusW;
  uint m_RadiusH;
  // Spot = the problematic spot to be repaired
  // Repairer = the spot providing the repair data
  // Position is the center of each spot
  QPoint m_SpotPos;
  QPoint m_RepairerPos;

  void UpdateTransparency();

////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////
public:
  ptImageSpot(const short isEnabled,
              const short hasRepairer,
              const short Mode,
              const double Angle,
              const uint EdgeRadius,
              const uint RadiusW, const uint RadiusH,
              const uint SpotX, const uint SpotY,
              const uint RepairerX = 0, const uint RepairerY = 0);

  // standard getter and setter methods
  inline short isEnabled() const { return m_IsEnabled; }
  inline short hasRepairer() const { return m_HasRepairer; }
  inline double Angle() const { return m_Angle; }
  inline uint EdgeRadius() const { return m_EgdeRadius; }
  inline uint RadiusW() const { return m_RadiusW; }
  inline uint RadiusH() const { return m_RadiusH; }
  inline QPoint SpotPos() const { return m_SpotPos; }
  inline QPoint RepairerPos() const { return m_RepairerPos; }

  inline void setEnabled(const short state) { m_IsEnabled = state; }
  void setAngle(double angle);
  void setEdgeRadius(uint radius);
  void setRadiusW(uint radius);
  void setRadiusH(uint radius);
  void setRepairer(const uint CenterX, const uint CenterY);
  inline void removeRepairer() { m_HasRepairer = 0; }

  // Move source and target spot around
  void MoveSpotTo(uint x, y);
  void MoveRepairerTo(uint x, y);
  void MoveTo(uint x, y);   // Moves both, keeps their relative position towards each other the same
};

#endif
