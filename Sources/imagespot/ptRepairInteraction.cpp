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

#include <QPen>

#include "ptRepairInteraction.h"
#include "ptImageSpotList.h"
#include "ptRepairSpot.h"
#include "ptRepairSpotListView.h"
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
  m_FullSpotGroup(new QGraphicsItemGroup()),
  m_SpotGroup(new QGraphicsItemGroup()),
  m_RepairerGroup(new QGraphicsItemGroup()),
  m_Spot(new QGraphicsEllipseItem()),
  m_SpotBorder(new QGraphicsEllipseItem()),
  m_RadiusHandle(new QGraphicsRectItem(0.0, 0.0, 10.0, 10.0)),
  m_Repairer(new QGraphicsEllipseItem()),
  m_Connector(new QGraphicsLineItem())
{
  QPen solidPen = QPen(QColor(150,150,150));

  m_Spot->setPen(solidPen);
  m_SpotBorder->setPen(QPen(QColor(150,150,150), 0, Qt::DashLine));
  m_RadiusHandle->setPen(solidPen);
  m_Repairer->setPen(solidPen);
  m_Connector->setPen(solidPen);

  m_SpotBorder->setParentItem(m_Spot);
  m_RadiusHandle->setParentItem(m_Spot);

  m_SpotGroup->addToGroup(m_Spot);
  m_RepairerGroup->addToGroup(m_Repairer);
  m_RepairerGroup->addToGroup(m_Connector);
  m_FullSpotGroup->addToGroup(m_SpotGroup);
  m_FullSpotGroup->addToGroup(m_RepairerGroup);

  if (MainWindow->RepairSpotListView->currentIndex().row() > -1) {
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
  DelAndNull(m_SpotGroup);
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
  ptRepairSpot* spotData = static_cast<ptRepairSpot*>(
      RepairSpotList->at(MainWindow->RepairSpotListView->currentIndex().row()));

  // spot outer edge
  m_Spot->setRect(spotData->pos().x() - spotData->radiusH(),
                  spotData->pos().y() - spotData->radiusW(),
                  spotData->radiusH() * 2,
                  spotData->radiusW() * 2);
  m_Spot->setRotation(spotData->angle());

  // spot inner edge
  if (spotData->edgeRadius() == 0) {
    m_SpotBorder->hide();
  } else {
    m_SpotBorder->setRect(spotData->edgeRadius(), spotData->edgeRadius(),
                          spotData->radiusH() - 2 * spotData->edgeRadius(),
                          spotData->radiusW() - 2 * spotData->edgeRadius() );
    m_SpotBorder->show();
  }

  // repairer and connector line between spot and repairer
  if (spotData->hasRepairer()) {
    m_Repairer->setRect(spotData->repairerPos().x() - spotData->radiusH(),
                        spotData->repairerPos().y() - spotData->radiusW(),
                        spotData->radiusH() * 2,
                        spotData->radiusW() * 2);
    m_Repairer->setRotation(spotData->angle());
    m_Connector->setLine(m_Spot->x(), m_Spot->y(), m_Repairer->x(), m_Repairer->y());
    m_RepairerGroup->show();
  } else {
    m_RepairerGroup->hide();
  }
}
