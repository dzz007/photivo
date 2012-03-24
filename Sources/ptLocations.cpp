/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#include <cassert>

#include <QCoreApplication>
#include <QDir>

#include "ptLocations.h"
#include "ptWinApi.h"

//==============================================================================

shared_ptr<ptLocations> Instance(nullptr);

//==============================================================================

/*static*/
shared_ptr<ptLocations> ptLocations::get() {
  if (Instance.use_count() == 0) {
    // not via make_shared because then ptLocation constructor would have to be public
    Instance = std::shared_ptr<ptLocations>(new ptLocations(), DestroyInstance);
  }
  return Instance;
}

//==============================================================================

/*static*/
void ptLocations::DestroyInstance(ptLocations *AObject) {
   delete AObject;
}

//==============================================================================

ptLocations::ptLocations()
: FExePath(QCoreApplication::applicationDirPath() + "/"),
  FUserPath(WinApi::AppdataFolder())
{
  // Create the QMap with all the file/folder info. For more documentation see the comment at
  // the top of ptLocations.h
  //
  // insert FOLDERS into map
  //                ID                folder path/name   location type                      isDir   file filter
  FLocations.insert(ChannelMixers,   {"ChannelMixers",   GlobalLocation | UserLocation,     true,   "*.ptm"});
  FLocations.insert(Curves,          {"Curves",          GlobalLocation | UserLocation,     true,   "*.ptc"});
  FLocations.insert(Presets,         {"Presets",         GlobalLocation | UserLocation,     true,   "*.pts"});
  FLocations.insert(IccProfiles,     {"Profiles",        GlobalLocation | UserLocation,     true,   "*.icc"});
  FLocations.insert(Themes,          {"Themes",          GlobalLocation,                    true,   "*.ptheme"});
  FLocations.insert(Translations,    {"Translations",    GlobalLocation,                    true,   "*.qm"});
  FLocations.insert(UISettings,      {"UISettings",      GlobalLocation | UserLocation,     true,   "*.ptu"});

  // insert FILES into map
  //                ID                file path/name     location type                      isDir   <filter is ignored for files>
  FLocations.insert(photivo_ini,     {"photivo.ini",     UserLocation,                      false,  ""});
  FLocations.insert(tags_ini,        {"tags.ini",        UserLocation,                      false,  ""});
}

//==============================================================================

ptLocations::~ptLocations()
{}

//==============================================================================

QString ptLocations::filter(const TLocation ALocation) {
  return  FLocations[ALocation].Filter;
}

//==============================================================================

QString ptLocations::name(const TLocation ALocation) const {
  return FLocations[ALocation].Name;
}

//==============================================================================

QString ptLocations::nativePath(const TLocation ALocation, const TLocationType ALocationType) const {
  return QDir::toNativeSeparators(this->path(ALocation, ALocationType));
}

//==============================================================================

QString ptLocations::path(const TLocation ALocation, const TLocationType ALocationType) const {
  QString hName = FLocations[ALocation].Name + (FLocations[ALocation].isDir ? "/" : "");

  switch (ALocationType) {
    case GlobalLocation:
      return FExePath + hName;

    case UserLocation:
      return FUserPath + hName;

    default:
      assert (!"Unhandled TLocationType entry.");
      return "";
  }
}

//==============================================================================

ptLocations::TLocationTypes ptLocations::type(const TLocation ALocation) const {
  return FLocations[ALocation].Type;
}

//==============================================================================
