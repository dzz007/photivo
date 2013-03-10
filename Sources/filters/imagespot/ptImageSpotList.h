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
#ifndef PTIMAGESPOTLIST_H
#define PTIMAGESPOTLIST_H

#include "ptImageSpot.h"
#include "../../ptStorable.h"
#include <QList>
#include <memory>

class ptImageSpotList: public ptStorable {
public:
  ptImageSpotList(PCreateSpotFunc ASpotCreator);
  ~ptImageSpotList();

  /*! \name QList functionality needed on the outside. *//*! @{*/
  void            append(ptImageSpot *spot)   { FSpots.append(spot); }
  ptImageSpot    *at(int i) const             { return FSpots[i]; }
  void            clear();
  int             count() const               { return FSpots.count(); }
  void            move(int from, int to)      { FSpots.move(from, to); }
  void            removeAt(int i)             { FSpots.removeAt(i); }
  void            replace(int i, ptImageSpot *spot) { FSpots.replace(i, spot); }
  int             size() const                { return FSpots.size(); }
  /*! @}*/

  bool          hasEnabledSpots();

protected:
  /*! \name Implementation of the ptStorable interface. *//*! @{*/
  TConfigStore  doStoreConfig(const QString &APrefix) const;
  void          doLoadConfig(const TConfigStore &AConfig, const QString &APrefix);
  /*! @}*/

private:
  QList<ptImageSpot*> FSpots;
  PCreateSpotFunc     FSpotCreator;

};

#endif // PTIMAGESPOTLIST_H
