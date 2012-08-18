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

#ifndef PTXMP_H
#define PTXMP_H

// Qt includes
#include <QString>
#include <QSettings>

// local includes
#include "ptXmpBase.h"
#include "ptXmpMM.h"

//forward declarations
class ptFilterDM;
namespace Exiv2 { class XmpData; class Xmpdatum; }

// -----------------------------------------------------------------------------

class ptXmp : public ptXmpBase
{
public: ////////////////////////////////////////////////////////////////////////

  enum class XmpLocation { None, Embedded, Sidecar };

  ptXmp(const QString  &filePath,
        Exiv2::XmpData &xmpData,
        XmpLocation    location = XmpLocation::Embedded);
  ptXmp(const QString &filePath);

  void saveSettings();

  Exiv2::XmpData xmpData() { return m_XmpData; }

  void addFilterCfg();

  void addIdentification(ptXmp &origin);

  //Exiv2::XmpData readXMP(const QString &filePath);//TODO: fn neccessary?

  void loadXmp();
  void save();
  void registerIDs();

  static void printXMP(const Exiv2::XmpData &xmpData);
         void printXMP() const;

  //void copyHistoryFromOrig(const ptXmp &original);//TODO: remove?

  static QString sidecarFilePathForFile(const QString &imageFilePath);
         QString sidecarFilePath() const;

protected: /////////////////////////////////////////////////////////////////////
  //QSettings IDMap;
  QString        m_FilePath;
  QString        m_SidecarPath;
  Exiv2::XmpData m_XmpData;
  ptXmpMM        m_XmpMM;
  XmpLocation    m_XmpLocation;

private: ///////////////////////////////////////////////////////////////////////
  static const std::string CnsURI,
                           Cns;
  static const std::string CFCfgNode,
                           CFCfgKey,
                           CFCfgItem,
                           CFCfgValue;
  //unsigned long historyCounter;//TODO: remove?

  void fetchSettings(Exiv2::XmpData &xmp);
  ///void updateIDs();
  void updateIDs(ptXmp &orign);
  ///std::string updateInputXmp();//TODO: remove?
  void header();
  void registerMyNs();
};//ptXmp

// -----------------------------------------------------------------------------

#endif // PTXMP_H
