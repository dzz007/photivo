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
/**
** Base class for image interactions (crop, spot repair etc.).
** Do not instantiate directly.
**/

#ifndef PTIMAGEINTERACTION_H
#define PTIMAGEINTERACTION_H

#include <QObject>
#include <QGraphicsView>

#include "ptConstants.h"

///////////////////////////////////////////////////////////////////////////
//
// class ptAbstractInteraction
//
///////////////////////////////////////////////////////////////////////////
class ptAbstractInteraction : public QObject {
Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  explicit ptAbstractInteraction(QGraphicsView* View);


///////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
///////////////////////////////////////////////////////////////////////////
protected:
  QGraphicsView* m_View;

///////////////////////////////////////////////////////////////////////////
//
// PRIVATE slots
//
///////////////////////////////////////////////////////////////////////////
private slots:
  virtual void keyAction(QKeyEvent* event) = 0;
  virtual void mouseAction(QMouseEvent* event) = 0;

///////////////////////////////////////////////////////////////////////////
//
// signals
//
///////////////////////////////////////////////////////////////////////////
signals:
  void finished(ptStatus ExitStatus);

};

#endif // PTIMAGEINTERACTION_H
