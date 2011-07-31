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

#include "ptRepairInteraction.h"
#include "ptImageSpotList.h"
#include "ptRepairSpot.h"
#include "../ptDefines.h"
#include "../ptConstants.h"
#include "../ptMainWindow.h"

extern ptImageSpotList* RepairSpotList;
extern ptMainWindow* MainWindow;

///////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
///////////////////////////////////////////////////////////////////////////
ptRepairInteraction::ptRepairInteraction(QGraphicsView* View)
: ptAbstractInteraction(View),
  m_TheSpotShape(new QGraphicsItemGroup()),
  m_TheSpot(new QGraphicsItemGroup()),
  m_TheRepairer(new QGraphicsItemGroup()),
  m_Spot(new QGraphicsEllipseItem()),
  m_SpotBorder(new QGraphicsEllipseItem()),
  m_RadiusHandle(new QGraphicsRectItem()),
  m_Repairer(new QGraphicsEllipseItem()),
  m_Connector(new QGraphicsLineItem())
{
  m_TheSpot->addToGroup(m_Spot);
  m_TheSpot->addToGroup(m_SpotBorder);
  m_TheSpot->addToGroup(m_RadiusHandle);
  m_TheRepairer->addToGroup(m_Repairer);
  m_TheRepairer->addToGroup(m_Connector);
  m_TheSpotShape->addToGroup(m_TheSpot);
  m_TheSpotShape->addToGroup(m_TheRepairer);

  m_SpotBorder->setParentItem(m_Spot);
  m_RadiusHandle->setParentItem(m_Spot);

  if (MainWindow->RepairSpotsView->currentIndex().row() > -1) {
    UpdateSpotShape();
  }

  connect(this, SIGNAL(finished(ptStatus)), m_View, SLOT(finishInteraction(ptStatus)));
  connect(m_View, SIGNAL(mouseChanged(QMouseEvent*)), this, SLOT(mouseAction(QMouseEvent*)));
  connect(m_View, SIGNAL(keyChanged(QKeyEvent*)), this, SLOT(keyAction(QKeyEvent*)));
}


ptRepairInteraction::~ptRepairInteraction() {
  DelAndNull(m_Spot);
  DelAndNull(m_SpotBorder);
  DelAndNull(m_RadiusHandle);
  DelAndNull(m_Repairer);
  DelAndNull(m_Connector);
  DelAndNull(m_TheSpot);
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
// UpdateSpotShape()
//
///////////////////////////////////////////////////////////////////////////

void ptRepairInteraction::UpdateSpotShape() {
  int scale = 1 >> Settings->GetInt("PipeSize");
  ptRepairSpot* spotData = static_cast<ptRepairSpot*>(
      RepairSpotList->at(MainWindow->RepairSpotsView->currentIndex().row()));

  m_Spot->setPos(spotData->pos() * scale);

  if (spotData->hasRepairer()) {
    m_Repairer->setPos(spotData->pos() * scale);
    m_Connector->setLine(m_Spot->x(), m_Spot->y(), m_Repairer->x(), m_Repairer->y());
    m_TheRepairer->show();
  } else {
    m_TheRepairer->hide();
  }
}
