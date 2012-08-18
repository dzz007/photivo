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

// Qt includes
#include <QDateTime>

// local includes
#include "ptXmpBase.h"
#include "ptMetadataUtil.h"


// protected ///////////////////////////////////////////////////////////////////

std::string ptXmpBase::date() //static
{
  QDateTime now = QDateTime::currentDateTime();
  QDateTime utc = now;
  utc.setTimeSpec(Qt::UTC);

  int offsetSec = now.secsTo(utc);//complete time difference in sec

  //int division always truncates, so bounding is correct
  unsigned int minutes = static_cast<unsigned int>((offsetSec % 3600) / 60);
  unsigned int hours   = static_cast<unsigned int>( offsetSec / 3600);

  //add leading 0 if necessery
  std::string hhOffset = (minutes < 10) ? ("0" + num2str(hours))   : num2str(hours);
  std::string mmOffset = (minutes < 10) ? ("0" + num2str(minutes)) : num2str(minutes);
  std::string sign = (offsetSec < 0) ? "-" : "+";

  return now.toString("yyyy-MM-ddThh:mm:ss").toStdString()
         + sign + hhOffset + ":" + mmOffset;
}

//==============================================================================

unsigned long ptXmpBase::arraySize(const Exiv2::XmpData &xmp,
                                   const std::string    &strKey) //static
{
  auto          end  = xmp.end();
  unsigned long size = 1; //Remember: Exiv2 starts counting at 1

  while (xmp.findKey(Exiv2::XmpKey(strKey+'['+num2str(size)+']')) != end)
    ++size;

  return --size;
}

//==============================================================================

XmpList ptXmpBase::getList(const Exiv2::XmpData &xmp,
                           const std::string    &strKey) //static
{
  XmpList            list;
  const std::string  strCompare = strKey + "[";
  const unsigned int keysize    = strCompare.size();

  for (Exiv2::Xmpdatum item : xmp) {
    std::string key = item.key();
    if ((key.size() > keysize) && (key.substr(0, keysize) == strCompare)) {
      list.push_back(item);
    }
  }

  return list;
}

//==============================================================================
