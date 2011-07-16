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
** Specialised data storage class for spot repair spots.
**/

#ifndef PTREPAIRSPOT_H
#define PTREPAIRSPOT_H

#include <QtGlobal>
#include <QPoint>

#include "ptImageSpot.h"


class ptRepairSpot: public ptImageSpot {
////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////
private:
  short m_HasRepairer;
  short m_Mode;           // repair algorithm: use ptSpotRepairAlgo constants
  QPoint m_RepairerPos;   // the spot providing the repair data

////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////
public:
  ptRepairSpot(const short isEnabled,
               const uint spotX,
               const uint spotY,
               const uint radiusW,
               const uint radiusH,
               const float angle,
               const uint edgeRadius,
               const float edgeBlur,
               const float opacity,
               const short mode,
               const short hasRepairer = 0,
               const uint repairerX = 0,
               const uint repairerY = 0);

  // standard getter and setter methods
  inline short hasRepairer() const { return m_HasRepairer; }
  inline QPoint RepairerPos() const { return m_RepairerPos; }
  inline short mode() const { return m_Mode; }

  void setRepairer(const uint CenterX, const uint CenterY);
  inline void removeRepairer() { m_HasRepairer = 0; }

  // more functionality
  void MoveRepairerTo(uint x, uint y);
  void MoveSpotTo(uint x, uint y);
  void MoveTo(uint x, uint y);   // move both spot and repairer
  void WriteToIni();
};

#endif
