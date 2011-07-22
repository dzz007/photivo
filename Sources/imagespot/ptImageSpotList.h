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

#ifndef PTIMAGESPOTLIST_H
#define PTIMAGESPOTMLIST_H

#include <QList>
#include <QString>

#include "ptImageSpot.h"

class ptImageSpotList: public QList<ptImageSpot*> {
////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
////////////////////////////////////////////////////////////////////////////////
public:
  // IniPrefix is a unique string that identifies this image spot list. The ini
  // file needs this so we can store multiple lists in it.
  ptImageSpotList(const QString IniPrefix = "");
  ~ptImageSpotList();

  inline QString iniName() const { return m_IniName; }

  void clear();
  void removeAt(int i);
  void removeFirst();
  void removeLast();
  void replace(int i, ptImageSpot *const& NewSpot);
  void WriteToIni();

////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
////////////////////////////////////////////////////////////////////////////////
private:
  QString m_IniName;


};

#endif
