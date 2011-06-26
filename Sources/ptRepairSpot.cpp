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

/**
** Base class for local selections ("spots")
** Data storage and functionality to move etc. the spot.
** No image manipulation or UI interface.
**/

#include <cassert>

#include "ptRepairSpot.h"
#include "ptSettings.h"

extern ptSettings* Settings;


////////////////////////////////////////////////////////////////////////////////
//
// ptRepairSpot constructor
//
////////////////////////////////////////////////////////////////////////////////

ptRepairSpot::ptRepairSpot(const short isEnabled,
                           const uint spotX,
                           const uint spotY,
                           const uint radiusW,
                           const uint radiusH,
                           const float angle,
                           const uint edgeRadius,
                           const float edgeBlur,
                           const float opacity,
                           const short mode,
                           const short hasRepairer,
                           const uint repairerX,
                           const uint repairerY)
: ptImageSpot(isEnabled, spotX, spotY, radiusW, radiusH, angle, edgeRadius, edgeBlur, opacity),
  m_Mode(mode), m_HasRepairer(hasRepairer)
{
  m_RepairerPos = QPoint(repairerX, repairerY);
}


////////////////////////////////////////////////////////////////////////////////
//
// Setter methods
//
////////////////////////////////////////////////////////////////////////////////

void ptRepairSpot::setRepairer(const uint CenterX, const uint CenterY) {
  m_HasRepairer = 1;
  m_RepairerPos.setX(CenterX);
  m_RepairerPos.setY(CenterY);
}


////////////////////////////////////////////////////////////////////////////////
//
// Move spot and/or repairer
//
////////////////////////////////////////////////////////////////////////////////

void ptRepairSpot::MoveRepairerTo(uint x, uint y) {
  if (m_HasRepairer) {
    m_RepairerPos.setX(x);
    m_RepairerPos.setY(y);
  }
}

void ptRepairSpot::MoveSpotTo(uint x, uint y) {
  m_SpotPos.setX(x);
  m_SpotPos.setY(y);
}

void ptRepairSpot::MoveTo(uint x, uint y) {
  if (m_HasRepairer) {
    m_RepairerPos.setX(m_RepairerPos.x() + m_SpotPos.x() - x);
    m_RepairerPos.setY(m_RepairerPos.y() + m_SpotPos.y() - y);
  }
  m_SpotPos.setX(x);
  m_SpotPos.setY(y);
}


////////////////////////////////////////////////////////////////////////////////
//
// WriteToIni
//
////////////////////////////////////////////////////////////////////////////////

void ptRepairSpot::WriteToIni() {
  ptImageSpot::WriteToIni();
  Settings->m_IniSettings("HasRepairer", m_HasRepairer);
  Settings->m_IniSettings("Mode", m_Mode);
  Settings->m_IniSettings("RepairerPosX", m_RepairerPosX);
  Settings->m_IniSettings("RepairerPosY", m_RepairerPosY);
}
