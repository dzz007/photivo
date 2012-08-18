/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Dominic Lyons <domlyons@googlemail.com>
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
** along with Photivo. If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef PTXMPBASE_H
#define PTXMPBASE_H

// C** includes
#include <vector>

// Exiv2 includes
#include <exiv2/xmp.hpp>

typedef std::vector<Exiv2::Xmpdatum> XmpList;

class ptXmpBase
{
protected:

  /*! **************************************************************************
   * Current date and time as yyy-MM-ddThh:mm:ss+hh:mm or
   * yyy-MM-ddThh:mm:ss-hh:mm (hh:mm after '+' or '-' is offset from UTC),
   * for example 2012-01-02T03:04:05+02:00
   */
  static std::string date();

  /*! **************************************************************************
   * Count all array elements that habe the key strKey
   */
  static unsigned long arraySize(const Exiv2::XmpData &xmp,
                                 const std::string    &strKey);

  /*! **************************************************************************
   * Get a list of all elements that have the key strKey
   */
  static XmpList getList(const Exiv2::XmpData &xmp,
                         const std::string    &strKey);

};

#endif // PTXMPBASE_H
