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

#ifndef PTMETADATAUTIL_H
#define PTMETADATAUTIL_H

//==============================================================================

template<class T>
std::string num2str(const T number) //static
{
  static_assert(std::is_integral<T>::value,
      "num2str() is ment to convert only numbers of integral types.");
  std::stringstream ss;
  ss << number;
  return ss.str();
}

//==============================================================================

#endif // PTMETADATAUTIL_H
