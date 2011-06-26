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

#include <cassert>

#include "ptImageSpot.h"
#include "ptSettings.h"

extern ptSettings* Settings;


////////////////////////////////////////////////////////////////////////////////
//
// ptImageSpot constructor
//
////////////////////////////////////////////////////////////////////////////////

ptImageSpot::ptImageSpot(const short CreateFromIni /*= 0*/)
: m_Angle(0.0),
  m_EdgeBlur(0.0),
  m_EdgeRadius(0),
  m_IsEnabled(0),
  m_Opacity(1.0),
  m_RadiusW(0),
  m_RadiusH(0),
  m_WeightMatrix(NULL)
{
  m_SpotPos = QPoint();
}

ptImageSpot::ptImageSpot(const short isEnabled,
                         const uint spotX,
                         const uint spotY,
                         const uint radiusW,
                         const uint radiusH,
                         const float angle,
                         const uint edgeRadius,
                         const float edgeBlur,
                         const float opacity)
  : m_IsEnabled(isEnabled),
    m_RadiusW(radiusW),
    m_RadiusH(radiusH),
    m_Angle(angle),
    m_EdgeRadius(edgeRadius)
{
  m_init = 1;
  m_SpotPos = QPoint(spotX, spotY);
  setEdgeBlur(edgeBlur);
  setOpacity(opacity);
  UpdateWeight();
  m_init = 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Setter methods
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::setAngle(float angle) {
  m_Angle = angle;
  UpdateWeight();
}

void ptImageSpot::setEdgeBlur(const float blur) {
  if (blur > 1.0f) {
    m_EdgeBlur = 1.0;
  } else if (blur < 0.0f){
    m_EdgeBlur = 0.0;
  } else {
    m_EdgeBlur = blur;
  }
  if (!m_init) {
    UpdateWeight();
  }
}

void ptImageSpot::setEdgeRadius(uint radius) {
  m_EdgeRadius = radius;
  UpdateWeight();
}

void ptImageSpot::setRadiusW(uint radius) {
  m_RadiusW = radius;
  UpdateWeight();
}

void ptImageSpot::setRadiusH(uint radius) {
  m_RadiusH = radius;
  UpdateWeight();
}

void ptImageSpot::setOpacity(const float opacity) {
  if (opacity > 1.0f) {
    m_Opacity = 1.0;
  } else {
    m_Opacity = opacity;
  }
  if (!m_init) {
    UpdateWeight();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Move spot
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::MoveTo(uint x, uint y) {
  m_SpotPos.setX(x);
  m_SpotPos.setY(y);
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateActivityMatrix
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::UpdateWeight() {
  // TODO SR: alpha channel calculation
}


////////////////////////////////////////////////////////////////////////////////
//
// WriteToIni
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::WriteToIni() {
  Settings->m_IniSettings->setValue("Angle", m_Angle);
  Settings->m_IniSettings->setValue("EdgeBlur", m_EdgeBlur);
  Settings->m_IniSettings->setValue("EdgeRadius", m_EdgeRadius);
  Settings->m_IniSettings->setValue("IsEnabled", m_IsEnabled);
  Settings->m_IniSettings->setValue("Opacity", m_Opacity);
  Settings->m_IniSettings->setValue("RadiusW", m_RadiusW);
  Settings->m_IniSettings->setValue("RadiusH", m_RadiusH);
  Settings->m_IniSettings->setValue("SpotPosX", m_SpotPos.x());
  Settings->m_IniSettings->setValue("SpotPosY", m_SpotPos.y());
}
