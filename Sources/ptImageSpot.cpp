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


////////////////////////////////////////////////////////////////////////////////
//
// ptImageSpot constructor
//
////////////////////////////////////////////////////////////////////////////////

ptImageSpot::ptImageSpot(const short isEnabled,
                         const short hasRepairer,
                         const short Mode,
                         const float Angle,
                         const uint EdgeRadius,
                         const uint RadiusW, const uint RadiusH,
                         const uint SpotX, const uint SpotY,
                         const uint RepairerX, const uint RepairerY)
{
  m_IsEnabled = isEnabled;
  m_HasRepairer = hasRepairer;
  m_Mode = Mode;
  m_Angle = Angle;
  m_EdgeRadius = EdgeRadius;
  m_RadiusW = RadiusW;
  m_RadiusH = RadiusH;
  m_SpotPos = QPoint(SpotX, SpotY);
  m_RepairerPos = QPoint(RepairerX, RepairerY);
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

void ptImageSpot::setRepairer(const uint CenterX, const uint CenterY) {
  m_HasRepairer = 1;
  m_RepairerPos.setX(CenterX);
  m_RepairerPos.setY(CenterY);
}

void ptImageSpot::setOpacity(const float opacity) {
  if (strength > 1.0f) {
    m_Opacity = 1.0;
  } else {
    m_Opacity = opacity;
  }
}

void ptImageSpot::setBlur(const float blur) {
  if (blur > 1.0f) {
    m_Blur = 1.0;
  } else {
    m_Blur = blur;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Move spot and/or repairer
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::MoveSpotTo(uint x, y) {
  m_SpotPos.setX(x);
  m_SpotPos.setY(y);
}

void ptImageSpot::MoveRepairerTo(uint x, y) {
  assert(m_HasRepairer != 0);
  m_RepairerPos.setX(x);
  m_RepairerPos.setY(y);
}

void ptImageSpot::MoveTo(uint x, y) {
  m_SpotPos.setX(x);
  m_SpotPos.setY(y);
  if (m_HasRepairer) {
    m_RepairerPos.setX(x);
    m_RepairerPos.setY(y);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateActivityMatrix
//
////////////////////////////////////////////////////////////////////////////////

void ptImageSpot::UpdateWeight() {
  // TODO SR: alpha channel calculation
}
