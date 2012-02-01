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

#include <cassert>

#include "ptRepairSpot.h"
#include "../ptSettings.h"

extern ptSettings* Settings;

//==============================================================================

ptRepairSpot::ptRepairSpot(QSettings *APtsFile /*= NULL*/)
: ptImageSpot(APtsFile),
  FHasRepairer(1),
  FAlgorithm(SpotRepairAlgo_Clone),
  m_RepairerPos(QPoint())
{
  if (APtsFile != NULL) {
    FHasRepairer = APtsFile->value("HasRepairer", 1).toInt();
    FAlgorithm =
        (ptSpotRepairAlgo)(APtsFile->value("Algorithm", SpotRepairAlgo_Clone).toInt());
    m_RepairerPos.setX(APtsFile->value("RepairerPosX", 0).toInt());
    m_RepairerPos.setY(APtsFile->value("RepairerPosY", 0).toInt());
  }
}

//==============================================================================

ptRepairSpot::ptRepairSpot(const short isEnabled,
                           const uint spotX,
                           const uint spotY,
                           const uint radiusX,
                           const uint radiusY,
                           const float angle,
                           const uint edgeRadius,
                           const float edgeBlur,
                           const float opacity,
                           const ptSpotRepairAlgo algorithm,
                           const short hasRepairer /*= 0*/,
                           const uint repairerX /*= 0*/,
                           const uint repairerY /*= 0*/)
: ptImageSpot(isEnabled, spotX, spotY, radiusX, radiusY, angle, edgeRadius, edgeBlur, opacity),
  FHasRepairer(hasRepairer),
  FAlgorithm(algorithm)
{
  m_RepairerPos = QPoint(repairerX * (1 >> Settings->GetInt("PipeSize")),
                         repairerY * (1 >> Settings->GetInt("PipeSize")) );
}

//==============================================================================

QPoint ptRepairSpot::repairerPos() const {
  return QPoint(m_RepairerPos.x() * (1 >> Settings->GetInt("PipeSize")),
                m_RepairerPos.y() * (1 >> Settings->GetInt("PipeSize")) );
}

//==============================================================================

void ptRepairSpot::setPos(uint Ax, uint Ay) {
  Ax *= (1 << Settings->GetInt("PipeSize"));
  Ay *= (1 << Settings->GetInt("PipeSize"));

  if (FHasRepairer) {
    m_RepairerPos.setX(m_RepairerPos.x() + FPos.x() - Ax);
    m_RepairerPos.setY(m_RepairerPos.y() + FPos.y() - Ay);
  }
  FPos.setX(Ax);
  FPos.setY(Ay);
}

//==============================================================================

void ptRepairSpot::setRepairerPos(const uint Ax, const uint Ay) {
  FHasRepairer = 1;
  m_RepairerPos.setX(Ax * (1 << Settings->GetInt("PipeSize")));
  m_RepairerPos.setY(Ay * (1 << Settings->GetInt("PipeSize")));
}

//==============================================================================

void ptRepairSpot::setSpotPos(const uint Ax, const uint Ay) {
  FPos.setX(Ax * (1 << Settings->GetInt("PipeSize")));
  FPos.setY(Ay * (1 << Settings->GetInt("PipeSize")));
}

//==============================================================================

void ptRepairSpot::WriteToFile(QSettings *APtsFile) {
  ptImageSpot::WriteToFile(APtsFile);
  APtsFile->setValue("HasRepairer", FHasRepairer);
  APtsFile->setValue("Algorithm", FAlgorithm);
  APtsFile->setValue("RepairerPosX", m_RepairerPos.x());
  APtsFile->setValue("RepairerPosY", m_RepairerPos.y());
}

//==============================================================================

