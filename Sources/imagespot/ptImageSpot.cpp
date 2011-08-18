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
#include "../ptSettings.h"

extern ptSettings* Settings;


////////////////////////////////////////////////////////////////////////////////
//
// ptImageSpot constructor
//
////////////////////////////////////////////////////////////////////////////////

ptImageSpot::ptImageSpot(QSettings* Ini /*= NULL*/)
: m_Angle(0.0),
  m_EdgeBlur(0.0),
  m_EdgeRadius(0),
  m_IsEnabled(0),
  m_Opacity(1.0),
  m_RadiusW(0),
  m_RadiusH(0),
  m_WeightMatrix(NULL)
{
  m_Pos = QPoint();

  if (Ini != NULL) {
    m_Angle = Ini->value("Angle", 0.0).toFloat();
    m_EdgeBlur = Ini->value("EdgeBlur", 0.0).toFloat();
    m_EdgeRadius = Ini->value("EdgeRadius", 0).toUInt();
    m_IsEnabled = Ini->value("IsEnabled", 0).toInt();
    m_Opacity = Ini->value("Opacity", 1.0).toFloat();
    m_RadiusW = Ini->value("RadiusW", 0).toUInt();
    m_RadiusH = Ini->value("RadiusH", 0.).toUInt();
    m_Pos.setX(Ini->value("SpotPosX", 0).toInt());
    m_Pos.setY(Ini->value("SpotPosY", 0).toInt());
  }
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
{
  int toFullPipe = 1 << Settings->GetInt("PipeSize");

  m_Angle = angle * toFullPipe;
  m_EdgeRadius = edgeRadius * toFullPipe;
  m_IsEnabled = isEnabled * toFullPipe;
  m_RadiusW = radiusW * toFullPipe;
  m_RadiusH = radiusH * toFullPipe;

  m_init = 1;
  m_Pos = QPoint(spotX * toFullPipe, spotY * toFullPipe);
  setEdgeBlur(edgeBlur);
  setOpacity(opacity);
  UpdateWeight();
  m_init = 0;
}


///////////////////////////////////////////////////////////////////////////
//
// Getter methods
//
///////////////////////////////////////////////////////////////////////////

QPoint ptImageSpot::pos() const {
  return QPoint(m_Pos.x() * (1 >> Settings->GetInt("PipeSize")),
                m_Pos.y() * (1 >> Settings->GetInt("PipeSize")) );
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
  m_EdgeRadius = radius * (1 << Settings->GetInt("PipeSize"));
  UpdateWeight();
}

void ptImageSpot::setRadiusH(uint radius) {
  m_RadiusH = radius * (1 << Settings->GetInt("PipeSize"));
  UpdateWeight();
}

void ptImageSpot::setRadiusW(uint radius) {
  m_RadiusW = radius * (1 << Settings->GetInt("PipeSize"));
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

void ptImageSpot::setPos(uint x, uint y) {
  m_Pos.setX(x);
  m_Pos.setY(y);
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

void ptImageSpot::WriteToIni(QSettings* Ini) {
  Ini->setValue("Angle", m_Angle);
  Ini->setValue("EdgeBlur", m_EdgeBlur);
  Ini->setValue("EdgeRadius", m_EdgeRadius);
  Ini->setValue("IsEnabled", m_IsEnabled);
  Ini->setValue("Opacity", m_Opacity);
  Ini->setValue("RadiusW", m_RadiusW);
  Ini->setValue("RadiusH", m_RadiusH);
  Ini->setValue("SpotPosX", m_Pos.x());
  Ini->setValue("SpotPosY", m_Pos.y());
}
