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

#ifndef PTXMPMM_H
#define PTXMPMM_H

// C++ includes
#include <vector>

// Exiv2 includes
#include <exiv2/xmp.hpp>

// local includes
#include "ptXmpBase.h"
#include "ptXmpID.h"
#include "ptMetadataUtil.h"

// using directives
using std::vector;

/* NOTE: As we only need to use known properties for this purpose we can use
 * simple structs. This simple, fast and less error prone.
 * For more generic requirements a hash map could be used instead. */

// forward declarations for some data structs declared below
struct XmpMMHistory;
struct XmpMMDerivedFrom;

/*! ***************************************************************************
 * Encapsulate and manage xmpMM-data
 */
class ptXmpMM : public ptXmpBase
{

public:

  ptXmpID origDocumentID;
  ptXmpID documentID;
  ptXmpID instanceID;
  vector<XmpMMDerivedFrom> derivedFrom;
  vector<XmpMMHistory>     history;

  ptXmpMM()                        = default;
  ptXmpMM(Exiv2::XmpData &xmpData) { load(xmpData); }

  void load(Exiv2::XmpData &xmpData);

  void syncToXmpData(Exiv2::XmpData &xmpData);

  bool hasChanges();

private:

  unsigned int histSizeAtLoad;
  unsigned int derivSizeAtLoad;

  void loadDerived(Exiv2::XmpData &xmpData);
  void loadHistory(Exiv2::XmpData &xmpData);
  void loadIDs    (Exiv2::XmpData &xmpData);

  void syncDerived(Exiv2::XmpData &xmpData);
  void syncHistory(Exiv2::XmpData &xmpData);
  void syncIDs    (Exiv2::XmpData &xmpData);

  void ensureMinData();

};

/*! ***************************************************************************
 * Simple data-only struct to encapsulate a single xmpMM "history" item
 */
struct XmpMMHistory
{
  QString action;
  ptXmpID instanceID;
  QString when;

  XmpMMHistory(QString action, QString instanceID, QString when)
    : action     { action },
      instanceID { instanceID },
      when       { when }
  {}
};

/*! ***************************************************************************
 * Simple data-only struct to encapsulate a single xmpMM "derivedFrom" item
 */
struct XmpMMDerivedFrom
{
  ptXmpID documentID;
  ptXmpID instanceID;

  XmpMMDerivedFrom(QString documentID, QString instanceID)
    : documentID { documentID },
      instanceID { instanceID }
  {}
};

#endif // PTXMPMM_H
