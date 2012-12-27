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
  \class ptImageInteraction
  \brief Abstract base class for image interactions (crop, spot repair etc.).
         Do not instantiate directly!
*/

#ifndef PTIMAGEINTERACTION_H
#define PTIMAGEINTERACTION_H

//==============================================================================

#include <QObject>
#include <QGraphicsView>

#include "ptConstants.h"

//==============================================================================

class ptImageInteraction: public QObject {
Q_OBJECT

public:
  explicit ptImageInteraction(QGraphicsView *AView);

//------------------------------------------------------------------------------

protected:
  QGraphicsView *FView;

//------------------------------------------------------------------------------

signals:
  void finished(ptStatus AExitStatus);

};
#endif // PTIMAGEINTERACTION_H
