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

#include "ptRepairInteraction.h"
#include "ptRepairSpotModel.h"
#include "../ptDefines.h"
#include "../ptConstants.h"


///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////
ptRepairInteraction::ptRepairInteraction(QGraphicsView* View, ptRepairSpotListView* ListView)
: ptAbstractInteraction(View),
  m_SpotData(NULL),
  m_ListView(ListView),
  m_SpotShape(new ptRepairSpotShape)
{
  assert(m_ListView != NULL);
  m_View->scene()->addItem(m_SpotShape);

  connect(this, SIGNAL(finished(ptStatus)), m_View, SLOT(finishInteraction(ptStatus)));
  connect(m_View, SIGNAL(mouseChanged(QMouseEvent*)), this, SLOT(mouseAction(QMouseEvent*)));
  connect(m_View, SIGNAL(keyChanged(QKeyEvent*)), this, SLOT(keyAction(QKeyEvent*)));

  connect(m_ListView, SIGNAL(rowChanged(QModelIndex)), this, SLOT(changeSpot(QModelIndex)));
  changeSpot(m_ListView->currentIndex());
}


ptRepairInteraction::~ptRepairInteraction() {
  m_View->scene()->removeItem(m_SpotShape);
  DelAndNull(m_SpotShape);
  // Warning: Never delete m_View, m_ListView, m_SpotData!
}


///////////////////////////////////////////////////////////////////////////
//
// stop()
//
///////////////////////////////////////////////////////////////////////////

void ptRepairInteraction::stop() {
  emit finished(stSuccess);
}


///////////////////////////////////////////////////////////////////////////
//
// Mouse interaction
//
///////////////////////////////////////////////////////////////////////////

void ptRepairInteraction::mouseAction(QMouseEvent *event) {
  switch (event->type()) {
    case QEvent::MouseButtonPress:
      //MousePressHandler(event);
      break;
    case QEvent::MouseButtonRelease:
      //MouseReleaseHandler(event);
      break;
    case QEvent::MouseButtonDblClick:
      //MouseDblClickHandler(event);
      break;
    case QEvent::MouseMove:
      //MouseMoveHandler(event);
      break;
    default:
      // ignore other mouse events
      break;
  }
}


void ptRepairInteraction::MousePressHandler(QMouseEvent *event) {

}



void ptRepairInteraction::keyAction(QKeyEvent *event) {

}


///////////////////////////////////////////////////////////////////////////
//
// changeSpot()
//
///////////////////////////////////////////////////////////////////////////

void ptRepairInteraction::changeSpot(const QModelIndex &index) {
  // Set new current spot
  if (index.isValid()) {
    m_SpotData = static_cast<ptRepairSpot*>(
        static_cast<ptRepairSpotModel*>(m_ListView->model())->spotList()->at(index.row()) );

  // No spot focused in list -> remove the reference to active spot.
  } else {
    m_SpotData = NULL;
  }

  m_SpotShape->Draw(m_SpotData);
}
