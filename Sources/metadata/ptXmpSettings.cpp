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

//#include <forward_list>
#include <exiv2/image.hpp>
#include <exiv2/xmpsidecar.hpp>
#include <exiv2/exiv2.hpp>
#include <libkexiv2/kexiv2.h>
#include <QUuid>
#include <type_traits>
#include <QDateTime>


#include "ptXmpSettings.h"
#include "ptInfo.h"
#include "filters/ptFilterDM.h"
#include "filters/ptFilterBase.h"
#include "ptSettings.h" //Needed for input file name

/* NOTE:
 * For JPEG images Exiv2 currently limits size of XMP packages.
 * I did not reach this limit without intentionally provoking it,
 * but it should be remembered if this becomes a problem in future...
 * Related Exiv2 bug report: http://dev.exiv2.org/issues/show/599
 */


// public //////////////////////////////////////////////////////////////////////

void ptXmp::saveSettings(const std::string &filename) {
  if (GFilterDM == nullptr) {
      GInfo->Raise("GFilterDM not created yet!", AT);
      return;
  }

  registerMyNs();
  Exiv2::XmpData xmp;

  fetchSettings(xmp);
  Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
  image->setXmpData(xmp);
  image->writeMetadata();
}

//==============================================================================

void ptXmp::addFilterCfgTo(Exiv2::XmpData &xmp) {
  registerMyNs();
  fetchSettings(xmp);
}

//==============================================================================

void ptXmp::addIdentificationTo(Exiv2::XmpData &xmp) {
  updateIDs(xmp);
  header(xmp);
}

//==============================================================================

Exiv2::XmpData ptXmp::readXMP(const std::string &filePath) {
  Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filePath);
  assert(image.get() != 0);
  image->readMetadata();

  Exiv2::XmpData &xmp = image->xmpData();
  if (xmp.empty()) {
    std::cout << filePath << " contains no XMP metadata! Looking for XMP sidecar file.\n";
    std::string sidecar = sidecarFilePathForFile(filePath);
    if (sidecar.size()==0) {
      std::cout << "No sidecar found for " << filePath << "\n";
    }
  }

  printXMP(xmp);
  return xmp;
}

//==============================================================================

void ptXmp::printXMP(const Exiv2::XmpData xmp) const {
  Exiv2::XmpData::const_iterator end = xmp.end();
  for (Exiv2::XmpData::const_iterator i = xmp.begin(); i != end; ++i) {
    const char* tn = i->typeName();
    std::cout << std::setw(44) << std::setfill(' ') << std::left
              << i->key() << " "
              << "0x" << std::setw(4) << std::setfill('0') << std::right
              << std::hex << i->tag() << " "
              << std::setw(9) << std::setfill(' ') << std::left
              << (tn ? tn : "Unknown") << " "
              << std::dec << std::setw(3)
              << std::setfill(' ') << std::right
              << i->count() << "  "
              << std::dec << i->value()
              << "\n";
  }
}

//==============================================================================

void ptXmp::copyHistoryFromOrig(const Exiv2::XmpData &src, Exiv2::XmpData &dest) {
  if (src.empty() || dest.empty()) return;

  dest["Xmp.xmpMM.OriginalDocumentID"] =
      src.findKey(Exiv2::XmpKey("Xmp.xmpMM.OriginalDocumentID"))->value();

  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  dest.add(Exiv2::XmpKey("Xmp.xmpMM.History"), &seq);
  std::vector<Exiv2::Xmpdatum> list = getXmpdatumList(src, "Xmp.xmpMM.History");
  for (Exiv2::Xmpdatum item : list) dest.add(item);

  historyCounter = getArraySize(src, "Xmp.xmpMM.History");
}

//==============================================================================

std::string ptXmp::sidecarFilePathForFile(const std::string &filePath) {
  std::string sidecar[] = { filePath + ".xmp",
                            filePath + ".XMP",
                            filePath + ".Xmp",
                            filePath.substr(0, filePath.find_last_of('.')) + ".xmp",
                            filePath.substr(0, filePath.find_last_of('.')) + ".XMP",
                            filePath.substr(0, filePath.find_last_of('.')) + ".Xmp" };
  for (std::string path : sidecar) {
    std::cout << "Search for sidecar file: " << path << "\n";
    if (Exiv2::fileExists(path, true)) {
      std::cout << "Found sidecar file: " << path << "\n";
      return path;
    }
  }
  return std::string("");
}


// private /////////////////////////////////////////////////////////////////////

//Take care of the order!
//Dependencies have to be defined beforehand (order of declarations doesn't matter).
const std::string ptXmp::CnsURI     = "http://www.photivo.org/ns/pts/1.0/";
const std::string ptXmp::Cns        = "pt";
const std::string ptXmp::CFCfgNode  = "FilterCfg";
const std::string ptXmp::CFCfgKey   = "Xmp." + Cns + "." + CFCfgNode;
const std::string ptXmp::CFCfgItem  = Cns + ":CfgItem";
const std::string ptXmp::CFCfgValue = Cns + ":CfgValue";

unsigned long historyCounter = 0;

//==============================================================================

void ptXmp::fetchSettings(Exiv2::XmpData &xmp) {
  if (GFilterDM == nullptr) {
      GInfo->Raise("GFilterDM not created yet!", AT);
      return;
  }

  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  xmp.add(Exiv2::XmpKey(CFCfgKey), &seq);

  int cntFilter = 0;
  auto end = GFilterDM->activeEnd();
  for (auto filter = GFilterDM->activeBegin(); filter != end; filter++) {
//    std::cout << AT << " : " << filter->uniqueName().toStdString()
//                    << " : " << filter->filterName().toStdString() << "\n";
    std::string filterPath = CFCfgKey + "[" + num2str(++cntFilter) + "]/"
                             + Cns + ":" + filter->filterName().toStdString();
    xmp.add(Exiv2::XmpKey(filterPath), &seq);

    std::unordered_map<std::string, std::string> map = filter->exportPreset();
    int cntItem = 0;
    for (auto pair : map) {
//      std::cout << "  * " << pair.first << " : " << pair.second << "\n";
      std::string itemPath = filterPath + "[" + num2str(++cntItem) + "]/";
      xmp[itemPath + CFCfgItem ] = pair.first;
      xmp[itemPath + CFCfgValue] = pair.second;
    }
  }
}

//==============================================================================

void ptXmp::updateIDs(Exiv2::XmpData &xmp) {
  //
  // TODO:
  // * Check if DocumentID and VersionID are available and use them
  // * Link ID of the RAW file
  // * There are even more properties that could be useful...
  //

  std::string origUuid = writeInputXmp();
  std::string uuid     = createUuid();
  xmp["Xmp.xmpMM.DocumentID"]         = "xmp.did:" + uuid;//same for did and iid
  xmp["Xmp.xmpMM.InstanceID"]         = "xmp.iid:" + uuid;//as this is the first instance
  if (xmp.findKey(Exiv2::XmpKey("Xmp.xmpMM.OriginalDocumentID")) == xmp.end()) {
    xmp["Xmp.xmpMM.OriginalDocumentID"] = "xmp.did:" + origUuid;
  } else {
    origUuid = xmp.findKey(Exiv2::XmpKey("Xmp.xmpMM.OriginalDocumentID"))->toString();
  }
  //xmp["Xmp.xmpMM.VersionID" ] = "1";

  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  if (historyCounter == 0) {
    xmp.add(Exiv2::XmpKey("Xmp.xmpMM.History"), &seq);
  }
  std::string h = num2str(++historyCounter);
  xmp["Xmp.xmpMM.History["+h+"]/stEvt:action"] = "created";
  xmp["Xmp.xmpMM.History["+h+"]/stEvt:instanceID"] = "xmp.iid:" + uuid;//FIXME!!!
  xmp["Xmp.xmpMM.History["+h+"]/stEvt:when"] = date();
  //xmp["Xmp.xmpMM.History["+h+"]/stEvt:when"] = "2012-01-01T01:01:01+01:00";//FIXME!!!
  //xmp["Xmp.xmpMM.History[1]/stEvt:parameters"] = ...
  //xmp["Xmp.xmpMM.History[1]/stEvt:softwareAgent"] = ...

  xmp.add(Exiv2::XmpKey("Xmp.xmpMM.DerivedFrom"), &seq);
  xmp["Xmp.xmpMM.DerivedFrom[1]/stRef:documentID"] = "xmp.did:" + origUuid;
  xmp["Xmp.xmpMM.DerivedFrom[1]/stRef:instanceID"] = "xmp.iid:" + origUuid;//FIXME!!!
  //...
}

//==============================================================================

std::string ptXmp::writeInputXmp() {
  std::string inputFilePath = Settings->GetStringList("InputFileNameList")[0].toStdString();
  Exiv2::Image::AutoPtr xmpSidecar;
  xmpSidecar = Exiv2::ImageFactory::create(Exiv2::ImageType::xmp, inputFilePath+".xmp");

  Exiv2::XmpData xmp;
  std::string uuid = createUuid();//same as this is the first instance
  xmp["Xmp.xmpMM.DocumentID"]         = "xmp.did:" + uuid;
  xmp["Xmp.xmpMM.InstanceID"]         = "xmp.iid:" + uuid;
  xmp["Xmp.xmpMM.OriginalDocumentID"] = "xmp.iid:" + uuid;
    //FIXME: even a input file could have been derived from another file!

  xmpSidecar->setXmpData(xmp);
  xmpSidecar->writeMetadata();
  return uuid;
}

//==============================================================================

void ptXmp::header(Exiv2::XmpData &xmp) {
  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  xmp.add(Exiv2::XmpKey("Xmp.pt.Header"), &seq);
  int i = 0;
  xmp["Xmp.pt.Header["+num2str(++i)+"]/pt:XmpSerializerVersion"] = "0.9";
  xmp["Xmp.pt.Header["+num2str(++i)+"]/pt:XmpSerializerVariant"] = "Output";
}

//==============================================================================

template<class T>
std::string ptXmp::num2str(const T number) {//static
  static_assert(std::is_integral<T>::value,
      "num2str() is thought to convert only numbers of integral types.");
  std::stringstream ss;
  ss << number;
  return ss.str();
}

//==============================================================================

std::string ptXmp::createUuid() {//static
  //create random UUID, remove "{" and " }"
  return QUuid::createUuid().toString().toStdString().substr(1, 36);
}

//==============================================================================

void ptXmp::registerMyNs() {
  //TODO: Check if ns prefix is used otherwise and try another one?!
  static bool registered = false;
  if (!registered) {
    Exiv2::XmpProperties::registerNs(CnsURI, Cns);
    registered = true;
  }
}

//==============================================================================

unsigned long ptXmp::getArraySize(const Exiv2::XmpData &xmp,
                                  const std::string    strKey) const {
  auto          end  = xmp.end();
  unsigned long size = 1;

  while (xmp.findKey(Exiv2::XmpKey(strKey+'['+num2str(size)+']')) != end)
    ++size;

  return --size;
}

//==============================================================================

std::vector<Exiv2::Xmpdatum> ptXmp::getXmpdatumList(const Exiv2::XmpData xmp,
                                                            const std::string &strKey) const {
  std::vector<Exiv2::Xmpdatum> list;
  const std::string strCompare = strKey + "[";
  const unsigned int keysize = strCompare.size();
  for (Exiv2::Xmpdatum item : xmp) {
    std::string key = item.key();
    if ((key.size() > keysize) && (key.substr(0, keysize) == strCompare)) {
      list.push_back(item);
    }
  }
  return list;
}

//==============================================================================

std::string ptXmp::date() {
  QDateTime now = QDateTime::currentDateTime();
  QDateTime utc = now;
  utc.setTimeSpec(Qt::UTC);

  int offsetSec = now.secsTo(utc);//complete time difference in sec

  //int division always truncates, so bounding is correct
  unsigned int minutes = static_cast<unsigned int>((offsetSec % 3600) / 60);
  unsigned int hours   = static_cast<unsigned int>( offsetSec / 3600);

  //add leading 0 if necessery
  std::string hhOffset = (minutes < 10) ? ("0" + num2str(hours)) : num2str(hours);
  std::string mmOffset = (minutes < 10) ? ("0" + num2str(minutes)) : num2str(minutes);
  std::string sign = (offsetSec < 0) ? "-" : "+";

  return now.toString("yyyy-MM-ddThh:mm:ss").toStdString()
         + sign + hhOffset + ":" + mmOffset;
}
