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

// Exiv2 includes
#include <exiv2/image.hpp>
#include <exiv2/xmpsidecar.hpp>
#include <exiv2/exiv2.hpp>
#include <libkexiv2/kexiv2.h>

// Qt includes
#include <QUuid>
#include <QDateTime>

// local includes
#include "ptXmp.h"
#include "ptXmpID.h"
#include "ptXmpIDMap.h"
#include "ptInfo.h"

/* NOTE:
 * For JPEG images Exiv2 currently limits size of XMP packages.
 * I did not reach this limit without intentionally provoking it,
 * but it should be remembered if this becomes a problem in future...
 * Related Exiv2 bug report: http://dev.exiv2.org/issues/show/599
 */

/* Writing filter configuration to XMP is incomplete und untested,
 * so keep is deactivated.
 * WARNING: Activating this is probably completely useless
 *          or could even be dangerous!                                       */
#define WRITE_FILTER_CONFIG_TO_XMP 0

// public //////////////////////////////////////////////////////////////////////

ptXmp::ptXmp(const QString  &filePath,
             Exiv2::XmpData &xmpData,
             XmpLocation    location /*= XMPLOCATION::Embedded*/ )
  : m_FilePath    { filePath },
    m_XmpData     { xmpData  },
    m_XmpMM       { xmpData  },
    m_XmpLocation { location }
{}

ptXmp::ptXmp(const QString &filePath)
  : m_FilePath { filePath }
{
  loadXmp(); //set m_XmpData and m_XmpLocation
}

//==============================================================================

void ptXmp::saveSettings()
{
#if WRITE_FILTER_CONFIG_TO_XMP == 1 // #########################################
  if (GFilterDM == nullptr) {
      GInfo->Raise("GFilterDM not created yet!", AT);
      return;
  }

  registerMyNs();
  fetchSettings(m_XmpData);

  //FIXME: writeMetadata() called like this will overwrite non-Xmp metadata!
  //       see ptXmp::save()
  Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(m_FilePath.toStdString());
  image->setXmpData(m_XmpData);
  image->writeMetadata();
#else
  std::cout << "WRITE_FILTER_CONFIG_TO_XMP = " << WRITE_FILTER_CONFIG_TO_XMP
            << ", ptXmp::header() called!";
#endif // WRITE_FILTER_CONFIG_TO_XMP ###########################################
}

//==============================================================================

void ptXmp::addFilterCfg()
{
  registerMyNs();
  fetchSettings(m_XmpData);
}

//==============================================================================

void ptXmp::addIdentification(ptXmp &origin)
{
  updateIDs(origin);
  header();
}

//==============================================================================

void ptXmp::loadXmp()
{
  const std::string     filePath = m_FilePath.toStdString();
  Exiv2::Image::AutoPtr image    = Exiv2::ImageFactory::open(filePath);

  image->readMetadata();
  m_XmpLocation = XmpLocation::None;//set 'None' as default

  if (m_XmpData.empty()) { // ## 1 No embedded XMP ##
    std::cout << filePath << " contains no XMP metadata! Looking for XMP sidecar file.\n";

    m_SidecarPath = sidecarFilePath();
    if (m_SidecarPath.isEmpty()) { // #1.1 No XMP at all
      // no XMP data: but m_XmpData is fully constructed, but empty
      std::cout << "No sidecar found for " << filePath << "\n";
      m_XmpMM       = ptXmpMM(m_XmpData);
      m_SidecarPath.clear();

    } else {                     // #1.2 XMP sidecar file
      std::cout << "Read XMP sidecar " <<  m_SidecarPath.toStdString() << "\n";
      Exiv2::Image::AutoPtr sidecar = Exiv2::ImageFactory::open(m_SidecarPath.toStdString());
      m_XmpData     = sidecar->xmpData();
      m_XmpLocation = XmpLocation::Sidecar;
      m_XmpMM       = ptXmpMM(m_XmpData);
    }

  } else {                 // ## 2 Embedded XMP ##
    std::cout << "Read XMP metadata contained in " << filePath << "\n";
    m_XmpMM       = ptXmpMM(m_XmpData);
    m_XmpLocation = XmpLocation::Embedded;
  }
}

//==============================================================================

void ptXmp::save()
{
  if (m_XmpMM.hasChanges() == false) return; //nothing to do

  m_XmpMM.syncToXmpData(m_XmpData);

  // embed XMP if some XMP is embedded already, otherwise save as sidecar
  Exiv2::Image::AutoPtr image;
  if (m_XmpLocation == XmpLocation::Embedded) {
    image = Exiv2::ImageFactory::open(m_FilePath.toStdString());
    image->readMetadata();//read available metadata, otherwise it will be removed!
  } else {
    if (m_SidecarPath.isEmpty())
      image = Exiv2::ImageFactory::create(Exiv2::ImageType::xmp,
                                          m_FilePath.toStdString() + ".xmp");
    else {
      image = Exiv2::ImageFactory::open(m_SidecarPath.toStdString());
      image->readMetadata();//read available metadata, otherwise it will be removed!
    }
  }

  image->setXmpData(m_XmpData);
  image->writeMetadata();
}

//==============================================================================

void ptXmp::registerIDs()
{
  ptXmpIDMap &map = ptXmpIDMap::getMap();

  map.setValue("xmp.iid/" + m_XmpMM.instanceID.pureID(), m_FilePath);
  map.setValue("xmp.did/" + m_XmpMM.documentID.pureID(), m_FilePath);
}

//==============================================================================

void ptXmp::printXMP() const
{
  printXMP(m_XmpData);
}


void ptXmp::printXMP(const Exiv2::XmpData &xmpData) //static
{
  Exiv2::XmpData::const_iterator end = xmpData.end();
  for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
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

QString ptXmp::sidecarFilePath() const
{
  return sidecarFilePathForFile(m_FilePath);
}


QString ptXmp::sidecarFilePathForFile(const QString &imageFilePath) //static
{
  // XMP sidecar could be <image.xmp> or <image.extension.xmp>
  // The standard doesn't specify one of them exactly, in practice both is used
  QString noExtension = imageFilePath.mid(0, imageFilePath.lastIndexOf('.'));
  QString sidecar[]   = { imageFilePath + ".xmp",
                          imageFilePath + ".XMP",
                          imageFilePath + ".Xmp",
                          noExtension   + ".xmp",
                          noExtension   + ".XMP",
                          noExtension   + ".Xmp" };

  for (const QString &path : sidecar) {
    std::cout << "Search for sidecar file: " << path.toStdString() << "\n";
    if (Exiv2::fileExists(path.toStdString(), true)) {
      std::cout << "Found sidecar file: " << path.toStdString() << "\n";
      return path;
    }
  }

  return QString();//empty String - no sidecar found
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

//==============================================================================

void ptXmp::fetchSettings(Exiv2::XmpData &xmp)
{
#if WRITE_FILTER_CONFIG_TO_XMP == 1 // #########################################

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
#else
  std::cout << "WRITE_FILTER_CONFIG_TO_XMP = " << WRITE_FILTER_CONFIG_TO_XMP
            << ", ptXmp::header() called!";
#endif // WRITE_FILTER_CONFIG_TO_XMP ###########################################
}

//==============================================================================

void ptXmp::updateIDs(ptXmp &origin)
{
  // preserve own IDs
  ptXmpID documentID = m_XmpMM.documentID;
  ptXmpID instanceID = m_XmpMM.instanceID;

  // copy original MM section and later on overwrite or add data for this file
  m_XmpMM = origin.m_XmpMM;

  // write back own IDs
  m_XmpMM.documentID = documentID;
  m_XmpMM.instanceID = instanceID;

  // add new History and DerivedFrom
  m_XmpMM.history.push_back(XmpMMHistory("created",
                                         documentID.id(),
                                         QString::fromStdString(date())));
  m_XmpMM.derivedFrom.push_back(XmpMMDerivedFrom(origin.m_XmpMM.documentID.id(),
                                                 origin.m_XmpMM.instanceID.id()));

  // now hand data from m_XmpMM over to m_XmpData
  m_XmpMM.syncToXmpData(m_XmpData);
}

//==============================================================================

void ptXmp::header()
{
#if WRITE_FILTER_CONFIG_TO_XMP == 1 // #########################################
  Exiv2::XmpArrayValue seq(Exiv2::xmpSeq);
  xmp.add(Exiv2::XmpKey("Xmp.pt.Header"), &seq);
  int i = 0;
  xmp["Xmp.pt.Header["+num2str(++i)+"]/pt:XmpSerializerVersion"] = "0.9";
  xmp["Xmp.pt.Header["+num2str(++i)+"]/pt:XmpSerializerVariant"] = "Output";
#else
  std::cout << "WRITE_FILTER_CONFIG_TO_XMP = " << WRITE_FILTER_CONFIG_TO_XMP
            << ", ptXmp::header() called!";
#endif // WRITE_FILTER_CONFIG_TO_XMP ###########################################
}

//==============================================================================

void ptXmp::registerMyNs()
{
  //TODO: Check if ns prefix is used otherwise and try another one?!
  static bool registered = false;
  if (!registered) {
    Exiv2::XmpProperties::registerNs(CnsURI, Cns);
    registered = true;
  }
}

//==============================================================================
