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

#ifndef PTXMPSETTINGS_H
#define PTXMPSETTINGS_H

//includes

//forward declarations
class ptFilterDM;
namespace Exiv2 { class XmpData; class Xmpdatum; }

//==============================================================================

class ptXmp
{
public://///////////////////////////////////////////////////////////////////////
  void saveSettings(const std::string &filename);
  void addFilterCfgTo(Exiv2::XmpData &xmp);
  void addIdentificationTo(Exiv2::XmpData &xmp);
  Exiv2::XmpData readXMP(const std::string &filePath);
  void printXMP(const Exiv2::XmpData xmp) const;
  void copyHistoryFromOrig(const Exiv2::XmpData &src, Exiv2::XmpData &dest);
  std::string sidecarFilePathForFile(const std::string &filePath);

private:////////////////////////////////////////////////////////////////////////
  static const std::string CnsURI,
                           Cns;
  static const std::string CFCfgNode,
                           CFCfgKey,
                           CFCfgItem,
                           CFCfgValue;
  unsigned long historyCounter;

  void fetchSettings(Exiv2::XmpData &xmp);
  void updateIDs(Exiv2::XmpData &xmp);
  std::string writeInputXmp();
  void header(Exiv2::XmpData &xmp);
  void registerMyNs();
  template<class T> static std::string num2str(const T number);
  static std::string createUuid();
  unsigned long getArraySize(const Exiv2::XmpData &xmp,
                             const std::string strKey) const;
  std::vector<Exiv2::Xmpdatum> getXmpdatumList(const Exiv2::XmpData xmp,
                                               const std::string &strKey) const;
  std::string date();
};//ptXmpSettings

//==============================================================================

#endif // PTXMPSETTINGS_H
