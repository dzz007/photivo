/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012-2013 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptCfgItem.h"
#include "../ptStorable.h"
#include <QHash>
#include <QVariant>
#include <QStringList>

class QSettings;

/*!
  \brief The ptFilterConfig class manages the configuration of a filter and acts
  as its data storage.
*/
class ptFilterConfig {
public:
  ptFilterConfig();
  ~ptFilterConfig();

  void                clear();
  void                exportPreset(QSettings* APreset) const;
  void                importPreset(QSettings* APreset);
  const TCfgItemList& items() const;
  bool                isEmpty() const;


  /*! \name Access to the data store. *//*! @{*/
  void        initStores(const TCfgItemList& ACfgItemList);
  void        loadDefaults();

  QVariant    value(const QString& AKey) const;
  void        setValue(const QString& AKey, const QVariant& AValue);

  bool        containsObject(const QString& AId) const;
  ptStorable* object(const QString& AId);
  /*! @}*/

private:
  TCfgItemList                FItems;
  QHash<QString, QVariant>    FDefaultStore;
  QHash<QString, ptStorable*> FCustomStore;
};

#endif // PTFILTERCONFIG_H
