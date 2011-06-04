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
#include <stdint.h>


class ptImageSpot {
////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////
private:
  short m_init;

////////////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
////////////////////////////////////////////////////////////////////////////////
protected:
  float m_Angle;
  float m_EdgeBlur;
  uint m_EdgeRadius;
  short m_IsEnabled;
  float m_Opacity;  // global transparency percentage
  uint m_RadiusW;
  uint m_RadiusH;
  QPoint m_SpotPos;  // Position is the center of the spot
  uint16_t* m_WeightMatrix;

  void UpdateWeight();

////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////
public:
  ptImageSpot(const short isEnabled,
              const uint spotX,
              const uint spotY,
              const uint radiusW,
              const uint radiusH,
              const float angle,
              const uint edgeRadius,
              const float edgeBlur,
              const float opacity);

  // standard getter and setter methods
  inline float Angle() const { return m_Angle; }
  inline float EdgeBlur() const { return m_EdgeBlur; }
  inline uint EdgeRadius() const { return m_EdgeRadius; }
  inline short isEnabled() const { return m_IsEnabled; }
  inline float Opactiy() const { return m_Opacity; }
  inline uint RadiusW() const { return m_RadiusW; }
  inline uint RadiusH() const { return m_RadiusH; }
  inline QPoint SpotPos() const { return m_SpotPos; }

  void setAngle(float angle);
  void setEdgeBlur(const float blur);
  void setEdgeRadius(uint radius);
  inline void setEnabled(const short state) { m_IsEnabled = state; }
  void setOpacity(const float opacity);
  void setRadiusW(uint radius);
  void setRadiusH(uint radius);

  // more functionality
  virtual void MoveTo(uint x, uint y);
};

#endif
