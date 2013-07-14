/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptFilterFactory.h"
#include "../ptInfo.h"

//==============================================================================

ptFilterFactory* ptFilterFactory::FInstance = nullptr;

//==============================================================================

ptFilterFactory::ptFilterFactory(QObject *parent) :
  QObject(parent)
{
}

//==============================================================================

ptFilterFactory::~ptFilterFactory()
{
}

//==============================================================================

void ptFilterFactory::RegisterFilter(const ptFilterFactoryMethod AMethod, const QString AName)
{
  // Every identifier should be registered only once.
  if (FRegisteredFilters.contains(AName))
    GInfo->Raise("Filter already registered: " + AName, AT);

  FRegisteredFilters.insert(AName, AMethod);
}

//==============================================================================

QStringList ptFilterFactory::AvailableFilters()
{
  return FRegisteredFilters.uniqueKeys();
}

//==============================================================================

ptFilterBase *ptFilterFactory::CreateFilterFromName(const QString AName) const
{
  ptFilterFactoryMethod AConstructor = FRegisteredFilters.value(AName, nullptr);
  if (!AConstructor)
    GInfo->Raise("Filter does not exist: " + AName, AT);

  return AConstructor();
}

//==============================================================================

ptFilterFactory *ptFilterFactory::GetInstance()
{
  if (!FInstance) {
    FInstance = new ptFilterFactory(nullptr);
  }

  return FInstance;
}

//==============================================================================

void ptFilterFactory::DestroyInstance()
{
  if (FInstance) delete FInstance;
  FInstance = nullptr;
}

//==============================================================================
