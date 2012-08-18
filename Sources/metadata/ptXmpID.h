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

#ifndef PTXMPID_H
#define PTXMPID_H

// Qt includes
#include <QString>

class ptXmpID
{
public: ////////////////////////////////////////////////////////////////////////
  enum Types { other, did, iid };

  ptXmpID(const ptXmpID::Types &type);
  ptXmpID(const QString        &id);
  ptXmpID(const std::string    &id);
  ptXmpID(const ptXmpID        &xmpID, const ptXmpID::Types &type);
  ptXmpID(const ptXmpID        &rhs);//copy constructor
  ptXmpID();

  QString pureID()     { return id_; }
  QString id()         { return prefix_ + ":" + id_; }
  QString typePrefix() { return prefix_; }
  Types   type();

  static QString createUuid();

protected: /////////////////////////////////////////////////////////////////////
  QString id_;
  QString prefix_;

  static const QString IID;// = "xmp.iid";
  static const QString DID;// = "xmp.did";

 QString typeAsString(const Types &type);

private: ///////////////////////////////////////////////////////////////////////

};

#endif // PTXMPID_H
