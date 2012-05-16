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

#include <memory>

#include <QList>

#include "ptImageSpot.h"
#include <filters/ptStorable.h>

//==============================================================================

class ptImageSpotList: public ptStorable {
public:
  ptImageSpotList(ptImageSpot::PCreateSpotFunc ASpotCreator);
  ~ptImageSpotList();

  /*! \group Implementation of the ptStorable interface. */
  ///@{
  TConfigStore  storeConfig(const QString &APrefix = "") const;
  void          loadConfig(const TConfigStore &AConfig, const QString &APrefix = "");
  ///@}

  /*! \group QList functionality needed on the outside. */
  ///@{
  ptImageSpot  &at(int i)     { &(FSpots[i].get()); }
  void          clear()       { FSpots.clear(); }
  int           count() const { FSpots.count(); }
  int           size() const  { FSpots.size(); }
  ///@}

  bool          hasEnabledSpots();


private:
  QList<std::unique_ptr<ptImageSpot> > FSpots;
  ptImageSpot::PCreateSpotFunc         FSpotCreator;

};

#endif // PTIMAGESPOTLIST_H
