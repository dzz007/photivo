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

//local includes
#include "ptXmpMM.h"
#include "ptMetadataUtil.h"

// public /////////////////////////////////////////////////////////////////////

void ptXmpMM::load(Exiv2::XmpData &xmpData)
{
    loadIDs    (xmpData);
    loadHistory(xmpData);
    loadDerived(xmpData);

    ensureMinData();
}

//==============================================================================

void ptXmpMM::syncToXmpData(Exiv2::XmpData &xmpData)
{
    syncIDs    (xmpData);
    syncHistory(xmpData);
    syncDerived(xmpData);
}

//==============================================================================

bool ptXmpMM::hasChanges()
{
  //XXX: Photivo would add history or derivedFrom items, so checking for this is
  //     more efficient than actually comparing the whole data
  return (histSizeAtLoad < history.size()) || (derivSizeAtLoad < derivSizeAtLoad);
}

// private ////////////////////////////////////////////////////////////////////

void ptXmpMM::loadDerived(Exiv2::XmpData &xmpData)
{
  XmpList list = getList(xmpData, "Xmp.xmpMM.DerivedFrom");

  for (Exiv2::Xmpdatum &item : list) {
    std::string node = item.key() + "/stRef:";

    // Missing key/value pairs will result as an empty string
    QString documentID = QString::fromStdString(xmpData[node + "documentID"].toString());
    QString instanceID = QString::fromStdString(xmpData[node + "instanceID"].toString());
    derivedFrom.push_back(XmpMMDerivedFrom(documentID, instanceID));
  }

  derivSizeAtLoad = derivedFrom.size(); // save for hasChanges();
}

//==============================================================================

void ptXmpMM::loadHistory(Exiv2::XmpData &xmpData)
{
  const unsigned long size = arraySize(xmpData, "Xmp.xmpMM.History");

  unsigned long i = 0;
  while (++i <= size) {//Exiv2 counts beginning at 1
    std::string node = "Xmp.xmpMM.History[" + num2str(i) + "]/stEvt:";

    // Missing key/value pairs will result as an empty string
    QString action = QString::fromStdString(xmpData[node + "action"].toString());
    QString iid    = QString::fromStdString(xmpData[node + "instanceID"].toString());
    QString when   = QString::fromStdString(xmpData[node + "when"].toString());
    history.push_back(XmpMMHistory(action, iid, when));
  }

  histSizeAtLoad = history.size(); // save for hasChanges();
}

//==============================================================================

void ptXmpMM::loadIDs(Exiv2::XmpData &xmpData)
{
  // Missing key/value pairs will result as an empty string
  documentID     = xmpData["Xmp.xmpMM.DocumentID"].toString();
  instanceID     = xmpData["Xmp.xmpMM.InstanceID"].toString();
  origDocumentID = xmpData["Xmp.xmpMM.OriginalDocumentID"].toString();
}

//==============================================================================

void ptXmpMM::syncDerived(Exiv2::XmpData &xmpData)
{
  if (derivedFrom.empty()) return; //nothing to do

  // Add sequence node
  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  xmpData.add(Exiv2::XmpKey("Xmp.xmpMM.DerivedFrom"), &seq);

  // (over-)write DerivedFrom
  int i = 0;
  for (XmpMMDerivedFrom &item : derivedFrom) {
    ++i; //Exiv2 array count beginning at 1
    std::string node  = "Xmp.xmpMM.DerivedFrom[" + num2str(i) + "]/stRef:";

    xmpData[node + "documentID"] = item.documentID.id().toStdString();
    xmpData[node + "instanceID"] = item.instanceID.id().toStdString();
  }

  //TODO: delete derivedFrom elements that are in xmpData but not in derivedFrom vector???
}

//==============================================================================

void ptXmpMM::syncHistory(Exiv2::XmpData &xmpData)
{
  if (history.empty()) return; //nothing to do

  // Add sequence node
  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  xmpData.add(Exiv2::XmpKey("Xmp.xmpMM.History"), &seq);

  // (over-)write history
  int i = 0;
  for (XmpMMHistory &item : history) {
    ++i; //Exiv2 array count beginning at 1
    std::string node  = "Xmp.xmpMM.History[" + num2str(i) + "]/stEvt:";

    xmpData[node + "action"]     = item.action.toStdString();
    xmpData[node + "instanceID"] = item.instanceID.id().toStdString();
    xmpData[node + "when"]       = item.when.toStdString();
  }

  //TODO: delete history elements that are in xmpData but not in history vector???
}

//==============================================================================

void ptXmpMM::syncIDs(Exiv2::XmpData &xmpData)
{
  // Missing key/value pairs will result as an empty string
  xmpData["Xmp.xmpMM.DocumentID"]         = documentID.id().toStdString();
  xmpData["Xmp.xmpMM.InstanceID"]         = instanceID.id().toStdString();
  xmpData["Xmp.xmpMM.OriginalDocumentID"] = origDocumentID.id().toStdString();
}

//==============================================================================

void ptXmpMM::ensureMinData()
{
  if (instanceID.pureID().isEmpty())     instanceID     = ptXmpID(ptXmpID::iid);
  if (documentID.pureID().isEmpty())     documentID     = ptXmpID(instanceID, ptXmpID::did);
  if (origDocumentID.pureID().isEmpty()) origDocumentID = ptXmpID(documentID);

  if (history.size() == 0) {
    QString action = "created";
    QString when   = QString::fromStdString(date());
    history.push_back(XmpMMHistory(action, instanceID.id(), when));
  }

  if (derivedFrom.size() == 0) {
    //TODO: Valid to keep it empty?
  }
}
