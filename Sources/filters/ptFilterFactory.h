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

#ifndef PTFILTERFACTORY_H
#define PTFILTERFACTORY_H

//==============================================================================

#include <QObject>
#include <QStringList>
#include <QMap>

#include "ptFilterBase.h"

//==============================================================================

/*! \class ptFilterFactory
  Singleton factory for the image filters.
  Provides a registration mechanism, to get informed about the
  available filterclasses.
  */

class ptFilterFactory : public QObject {
Q_OBJECT

public:
  /*! Public access to the singleton instance.*/
  static ptFilterFactory* GetInstance();

  /*! Registration with unique identifier. */
  void RegisterFilter(const ptFilterFactoryMethod AMethod,
                      const QString               AName);

  /*! List of available filters. */
  QStringList AvailableFilters();

  /*! Access to the registered filters. */
  ptFilterBase *CreateFilterFromName(const QString AName) const;

//-------------------------------------

private:
  ptFilterFactory(QObject *parent = 0);
  ~ptFilterFactory();

  /*! For clean up. */
  static void DestroyInstance();

  /*! Singleton instance.*/
  static ptFilterFactory* FInstance;

  /*! List of the registered filters.*/
  QMap<QString, ptFilterFactoryMethod> FRegisteredFilters;
};

#endif // PTFILTERFACTORY_H
