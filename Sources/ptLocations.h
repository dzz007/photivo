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
/*!
 * \class ptLocations
 * \brief Provides paths to Photivo’s data files and folders.
 *
 * How to add a new location:\n
 * \li In *ptLocations.h* add a new entry to the \c TLocation enum. The order of the entries is
 *     not critical, just keep folders and files separate. For filenames replace dots (.)
 *     with underscores (_).
 * \li In *ptLocations.cpp* in the constructor’s implementation add a new \c FLocations.insert(...)
 *     line. For the file/folder name enter the path relative to the Photivo install dir or the
 *     Photivo user profile dir. Always use forward slashes (/) as the path delimiter. Do not use
 *     any leading or trailing slashes.
 */

#ifndef PTLOCATIONS_H
#define PTLOCATIONS_H

//==============================================================================

#include <memory>
using std::shared_ptr;

#include <QString>
#include <QMap>
#include <QFlags>

//==============================================================================

class ptLocations {
public:
  /*! Returns a smart pointer to the ptLocations singleton object. */
  static shared_ptr<ptLocations> get();

  /*!
   * The \c TLocation enum
   */
  enum TLocation {
    // folders
    ChannelMixers,
    Curves,
    Presets,
    IccProfiles,
    Themes,
    Translations,
    UISettings,
    //files
    photivo_ini,
    tags_ini
  };

  /*!
   * The \c TLocationType enum defines where data files and folders can be located. That can be
   * a global location for all users where typically only admins have write permission or a
   * per-user location, typically somewhere in the userâ€™s profile, or both.
   *
   * Photivo treats global locations as read-only. Data created by the user will always be stored
   * in the per-user location.
   *
   * Common location base folders are:\n
   * Windows:
   * \li <tt>Global</tt>: C:\Program Files\Photivo
   * \li <tt>User</tt>: C:\Users\username\AppData\Roaming\Photivo
   * Linux:
   * \li <tt>Global</tt>: /usr/share/photivo
   * \li <tt>User</tt>: ~/.photivo
   */
  enum TLocationType {
    GlobalLocation = 0x0, /*!< File/folder is allowed in the global location. */
    UserLocation   = 0x1  /*!< File/folder is allowed in the per-user location. */
  };
  typedef QFlags<TLocationType> TLocationTypes;

  /*! Returns the filename filter for the given \c ALocation. */
  QString filter(const TLocation ALocation);

  /*! Returns the name/path of \c ALocation relative to the base folder. */
  QString name(const TLocation ALocation) const;

  /*! Returns the full path (incl. filename if applicable) to \c ALocation
   *  using native path delimiters. */
  QString nativePath(const TLocation ALocation, const TLocationType ALocationType) const;

  /*! Same as \c nativePath() but always uses forward slashes (/) as path delimiters. */
  QString path(const TLocation ALocation, const TLocationType ALocationType) const;

  /*! Returns the type of \c ALocation. */
  TLocationTypes type(const TLocation ALocation) const;

//------------------------------------------------------------------------------

private:
  static void DestroyInstance(ptLocations *AObject);

  struct TEntity {
    QString        Name;
    TLocationTypes Type;
    bool           isDir;
    QString        Filter;
  };

  ptLocations();
  ~ptLocations();

  QString                   FExePath;
  QMap<TLocation, TEntity>  FLocations;
  QString                   FUserPath;


};

//==============================================================================

Q_DECLARE_OPERATORS_FOR_FLAGS(ptLocations::TLocationTypes)

//==============================================================================
#endif // PTLOCATIONS_H
