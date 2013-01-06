/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011-2012 Bernd Schoeler <brjohn@brother-john.net>
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
  \class ptImageSpot

  \brief Abstract base class for storing image spot data.

  This class stores the data for a single image spot. Values you pass to a
  \c ptImageSpot object must always be in current pipe size scale. Similarly
  \c ptImageSpot always return values in current pipe size scale.

  However, internally (and in the ini file) everything is stored in 1:1 pipe size scale.
  Derived classes are strongly recommended to do the same!
*/

#ifndef PTIMAGESPOT_H
#define PTIMAGESPOT_H

#include <functional>

#include <QVariant>
#include <QPoint>

#include <filters/ptStorable.h>

//==============================================================================

const QString CSpotIsEnabledId = "IsEnabled";
const QString CSpotNameId      = "Name";
const QString CSpotPosId       = "Pos";

//==============================================================================

class ptImageSpot: public ptStorable {
public:
  virtual ~ptImageSpot();

  /*! \group Standard getters and setters.
      These members can also be accessed via the generic getValue() and setValue() functions. */
  ///@{
  inline bool     isEnabled() const             { return FIsEnabled; }
  inline void     setEnabled(const bool AState) { FIsEnabled = AState; }
  inline QString  name() const                  { return FName; }
  inline void     setName(const QString &AName) { FName = AName; }
  QPoint          pos() const;
  void            setPos(const QPoint &APos);
  int             x() const;
  int             y() const;
  ///@}

  QVariant getValue(const QString &AKey) const;
  void     setValue(const QString &AKey, const QVariant AValue);

  /*! \group Implementation of the ptStorable interface. */
  ///@{
  TConfigStore storeConfig(const QString &APrefix = "") const;
  void         loadConfig(const TConfigStore &AConfig, const QString &APrefix = "");
  ///@}


protected:
  /*! Creates an enabled spot with no name at position (0,0). */
  ptImageSpot();

// Pragmas are here to stop the compiler complaining about unused parameters in the default
// implementations. Removing the parameter names would work too but be too obscure.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
  virtual TConfigStore  doStoreConfig(const QString &APrefix) const = 0;
  virtual void          doLoadConfig(const TConfigStore &AConfig, const QString &APrefix) = 0;
  virtual QVariant      doGetValue(const QString &AKey) const { return QVariant(); }
  virtual bool          doSetValue(const QString &AKey, const QVariant AValue) { return false; }
#pragma GCC diagnostic pop

  QMap<QString, QVariant> FDataStore;
  bool                    FIsEnabled;
  QString                 FName;
  QPoint                  FPos;

};

//==============================================================================

/*! Type definition for the function pointer to the spot factory method.
   There is no factory method returning a \c ptImageSpot because we only use
   specialised spots in derived classes. */
typedef std::function<ptImageSpot*()> PCreateSpotFunc;

#endif // PTIMAGESPOT_H
