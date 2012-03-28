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

#include "ptImageSpot.h"
#include "../ptSettings.h"

extern ptSettings* Settings;

//==============================================================================

ptImageSpot::ptImageSpot(QSettings *APtsFile /*= nullptr*/)
: FIsEnabled(true),
  FName("")
{
  FPos = QPoint();

  if (APtsFile != nullptr) {
    FIsEnabled = APtsFile->value("IsEnabled", 0).toBool();
    FName = APtsFile->value("Name", "").toString();
    FPos.setX(APtsFile->value("SpotPosX", 0).toInt());
    FPos.setY(APtsFile->value("SpotPosY", 0).toInt());
  }
}

//==============================================================================

ptImageSpot::ptImageSpot(const uint ASpotX,
                         const uint ASpotY,
                         const uint ARadius,
                         const bool AIsEnabled,
                         const QString &AName)
{
  int hToFullPipe = 1 << Settings->GetInt("PipeSize");

  FPos        = QPoint(ASpotX * hToFullPipe, ASpotY * hToFullPipe);
  FIsEnabled  = AIsEnabled;
  FName       = AName;
}

//==============================================================================

QPoint ptImageSpot::pos() const {
  return QPoint(FPos.x() >> Settings->GetInt("Scaled"),
                FPos.y() >> Settings->GetInt("Scaled") );
}

//==============================================================================

void ptImageSpot::setPos(uint Ax, uint Ay) {
  FPos.setX(Ax << Settings->GetInt("Scaled"));
  FPos.setY(Ay << Settings->GetInt("Scaled"));
}

//==============================================================================

void ptImageSpot::WriteToFile(QSettings *APtsFile) {
  APtsFile->setValue("IsEnabled", (int)FIsEnabled);
  APtsFile->setValue("Name", FName);
  APtsFile->setValue("SpotPosX", FPos.x());
  APtsFile->setValue("SpotPosY", FPos.y());
}

//==============================================================================

int ptImageSpot::x() const
{
  return FPos.x() >> Settings->GetInt("Scaled");
}

//==============================================================================

int ptImageSpot::y() const
{
  return FPos.y() >> Settings->GetInt("Scaled");
}

//==============================================================================

