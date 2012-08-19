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

//Qt includes
#include <QString>
#include <QUuid>

//local includes
#include "ptXmpID.h"


// public //////////////////////////////////////////////////////////////////////

ptXmpID::ptXmpID(const ptXmpID::Types &type)
  : id_(createUuid()),
    prefix_(typeAsString(type))
{}

//==============================================================================

ptXmpID::ptXmpID(const QString &id)
{
  int split = id.lastIndexOf(':');

  if (split < 0) {//no ':' found => no prefix
    id_     = id;
    prefix_ = QString();
  } else {//split prefix and id after last ':'
    id_     = id.mid(split + 1);
    prefix_ = id.left(split);
  }
}

//==============================================================================

ptXmpID::ptXmpID(const std::string &id)
{
  QString qid   = QString::fromStdString(id);
  int     split = qid.lastIndexOf(':');

  if (split < 0) {//no ':' found => no prefix
    id_     = qid;
    prefix_ = QString();
  } else {//split prefix and id after last ':'
    id_     = qid.mid(split + 1);
    prefix_ = qid.left(split);
  }
}

//==============================================================================

ptXmpID::ptXmpID(const ptXmpID &xmpID, const ptXmpID::Types &type)
  : id_(xmpID.id_),
    prefix_(typeAsString(type))
{}

//==============================================================================

//copy constructor
ptXmpID::ptXmpID(const ptXmpID &rhs)
  : id_     { rhs.id_ },
    prefix_ { rhs.prefix_ }
{}

//==============================================================================

ptXmpID::ptXmpID()
  : id_     { createUuid() },
    prefix_ { "unknown" }
{}


//==============================================================================

ptXmpID::Types ptXmpID::type()
{
  if (prefix_ == IID) return iid;
  if (prefix_ == DID) return did;

  return other;
}

    //==============================================================================

QString ptXmpID::createUuid() //static
{
  //create random UUID, remove "{" and " }"
  return QUuid::createUuid().toString().mid(1, 36);
}

// protected ///////////////////////////////////////////////////////////////////

// static const initializations
const QString ptXmpID::IID = "xmp.iid";
const QString ptXmpID::DID = "xmp.did";

//==============================================================================

QString ptXmpID::typeAsString(const ptXmpID::Types &type)
{
  if (type == iid) return IID;
  if (type == did) return DID;
  return QString();
}

//==============================================================================
