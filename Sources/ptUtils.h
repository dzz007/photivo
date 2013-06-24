/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Bernd Schoeler <brjohn@brother-john.net>
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
#ifndef PTUTILS_H
#define PTUTILS_H

#include <QString>

//------------------------------------------------------------------------------
/*!
  Returns *true* if *AValue* lies between *ALowBound* and *AHighBound*, *false* otherwise.
  Both boundary are included in the range.
*/
template<typename T>
inline bool isBetween(const T& ALowBound, const T& AValue, const T& AHighBound) {
  return (AValue >= ALowBound) && (AValue <= AHighBound);
}

//------------------------------------------------------------------------------
QString trailingSlash(const QString& AText);


#endif // PTUTILS_H
