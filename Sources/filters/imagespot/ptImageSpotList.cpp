/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptImageSpotList.h"
#include "../../ptDefines.h"

ptImageSpotList::ptImageSpotList(PCreateSpotFunc ASpotCreator)
: ptStorable(),
  FSpotCreator(ASpotCreator)
{}

//==============================================================================

ptImageSpotList::~ptImageSpotList() {
  this->clear();
}

//==============================================================================

TConfigStore ptImageSpotList::doStoreConfig(const QString &APrefix) const {
  TConfigStore hConfig;

  // number of spots in this list
  hConfig.insert(APrefix+"Spot/size", FSpots.size());

  for (int i = 0; i < FSpots.size(); ++i) {
    hConfig.unite(FSpots.at(i)->storeConfig(QString(APrefix+"Spot/%1/").arg(i)));
  }

  return hConfig;
}

//==============================================================================

void ptImageSpotList::doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FSpots.clear();
  int hSpotCount = AConfig.value(APrefix+"Spot/size").toInt();

  for (int i = 0; i < hSpotCount; ++i) {
    FSpots.append(FSpotCreator());
    FSpots.last()->loadConfig(AConfig, QString(APrefix+"Spot/%1/").arg(i));
  }
}

//==============================================================================

void ptImageSpotList::clear() {
  for (auto &hSpot: FSpots) {
    DelAndNull(hSpot);
  }
  FSpots.clear();
}

//==============================================================================

bool ptImageSpotList::hasEnabledSpots() {
  for (auto &hSpot: FSpots) {
    if (hSpot->isEnabled())
      return true;
  }
  return false;
}

//==============================================================================
