/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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

#ifndef PTFILTERCONFIG_H
#define PTFILTERCONFIG_H

//==============================================================================

#include <QMap>
#include <QVariant>
#include <QStringList>

//forwards
class QSettings;

//==============================================================================

typedef QMap<QString, QVariant> TConfigStore;

//==============================================================================

class ptFilterConfig {
public:
  /*! Creates a new \c ptFilterConfig instance. */
  ptFilterConfig();

  /*! Copy constructor. */
  ptFilterConfig(const ptFilterConfig &AOther);


  /*! \group Management of the default data store. */
  ///@{
  /*! Initializes the data store with new key/value pairs. All old data is removed.
      \param AInitData
        A \c QMap with all the keys handled by this \c ptFilterConfig instance set
        to their default values. Also defines the valid keys for the \c getValue() and
        \c setValue() methods.
      \see update()
   */
  void            init(const TConfigStore &AInitData);

  /*! Updates the data store with new key/value pairs. Existing keys are updated with the
      new value, non-existing keys are ignored. Keys not present in \c AInitData are not touched.
      \param AInitData
        A \c QMap containing the new data.
      \see init()
   */
  void            update(const TConfigStore &AInitData);

  /*! Returns the value for the config item \c AKey. */
  QVariant        getValue(const QString &AKey) const;

  /*! Updates the config item \c AKey with \c AValue. */
  void            setValue(const QString &AKey, const QVariant &AValue);
  ///@}

  /*! Management of additional custom data stores. */
  ///@{
  TConfigStore       *newStore(const QString &AId, const TConfigStore ADefaults = TConfigStore());
  TConfigStore       *getStore(const QString &AId);
  const QStringList   storeIds() const { return FStoreIds; }
  void                clearCustomStores();
  ///@}


private:
  TConfigStore         FDataStore;
  QList<TConfigStore>  FCustomStores;
  QStringList          FStoreIds;

};

#endif // PTFILTERCONFIG_H
