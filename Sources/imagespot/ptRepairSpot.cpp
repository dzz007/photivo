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

#include "ptRepairSpot.h"
#include "../ptSettings.h"

extern ptSettings* Settings;


////////////////////////////////////////////////////////////////////////////////
//
// ptRepairSpot constructors
//
////////////////////////////////////////////////////////////////////////////////

ptRepairSpot::ptRepairSpot(QSettings* Ini /*= NULL*/)
: ptImageSpot(Ini),
  m_HasRepairer(1),
  m_Algorithm(SpotRepairAlgo_Clone),
  m_RepairerPos(QPoint())
{
  if (Ini != NULL) {
    m_HasRepairer = Ini->value("HasRepairer", 1).toInt();
    m_Algorithm =
        (ptSpotRepairAlgo)(Ini->value("Algorithm", SpotRepairAlgo_Clone).toInt());
    m_RepairerPos.setX(Ini->value("RepairerPosX", 0).toInt());
    m_RepairerPos.setY(Ini->value("RepairerPosY", 0).toInt());
  }
}


ptRepairSpot::ptRepairSpot(const short isEnabled,
                           const uint spotX,
                           const uint spotY,
                           const uint radiusW,
                           const uint radiusH,
                           const float angle,
                           const uint edgeRadius,
                           const float edgeBlur,
                           const float opacity,
                           const ptSpotRepairAlgo algorithm,
                           const short hasRepairer /*= 0*/,
                           const uint repairerX /*= 0*/,
                           const uint repairerY /*= 0*/)
: ptImageSpot(isEnabled, spotX, spotY, radiusW, radiusH, angle, edgeRadius, edgeBlur, opacity),
  m_HasRepairer(hasRepairer),
  m_Algorithm(algorithm)
{
  m_RepairerPos = QPoint(repairerX * (1 >> Settings->GetInt("PipeSize")),
                         repairerY * (1 >> Settings->GetInt("PipeSize")) );
}


///////////////////////////////////////////////////////////////////////////
//
// Getter methods
//
///////////////////////////////////////////////////////////////////////////

QPoint ptRepairSpot::repairerPos() const {
  return QPoint(m_RepairerPos.x() * (1 >> Settings->GetInt("PipeSize")),
                m_RepairerPos.y() * (1 >> Settings->GetInt("PipeSize")) );
}


////////////////////////////////////////////////////////////////////////////////
//
// Setter methods
//
////////////////////////////////////////////////////////////////////////////////

void ptRepairSpot::setPos(uint x, uint y) {
  x *= (1 << Settings->GetInt("PipeSize"));
  y *= (1 << Settings->GetInt("PipeSize"));

  if (m_HasRepairer) {
    m_RepairerPos.setX(m_RepairerPos.x() + m_Pos.x() - x);
    m_RepairerPos.setY(m_RepairerPos.y() + m_Pos.y() - y);
  }
  m_Pos.setX(x);
  m_Pos.setY(y);
}


void ptRepairSpot::setRepairerPos(const uint x, const uint y) {
  m_HasRepairer = 1;
  m_RepairerPos.setX(x * (1 << Settings->GetInt("PipeSize")));
  m_RepairerPos.setY(x * (1 << Settings->GetInt("PipeSize")));
}


void ptRepairSpot::setSpotPos(const uint x, const uint y) {
  m_Pos.setX(x * (1 << Settings->GetInt("PipeSize")));
  m_Pos.setY(y * (1 << Settings->GetInt("PipeSize")));
}


////////////////////////////////////////////////////////////////////////////////
//
// WriteToIni
//
////////////////////////////////////////////////////////////////////////////////

void ptRepairSpot::WriteToIni(QSettings* Ini) {
  ptImageSpot::WriteToIni(Ini);
  Ini->setValue("HasRepairer", m_HasRepairer);
  Ini->setValue("Algorithm", m_Algorithm);
  Ini->setValue("RepairerPosX", m_RepairerPos.x());
  Ini->setValue("RepairerPosY", m_RepairerPos.y());
}
