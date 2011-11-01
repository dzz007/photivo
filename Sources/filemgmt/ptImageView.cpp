/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#include <QGraphicsView>
#include <QList>

#include "../ptDefines.h"
#include "../ptSettings.h"
#include "../ptTheme.h"
#include "ptImageView.h"
#include "ptFileMgrWindow.h"

//==============================================================================

extern ptSettings*  Settings;
extern ptTheme*     Theme;

//==============================================================================

ptImageView::ptImageView(QWidget *parent, ptFileMgrDM* DataModule) :
  QGraphicsView(parent),
  // constants
  MinZoom(0.05),
  MaxZoom(4.0)
{
  assert(parent     != NULL);
  assert(DataModule != NULL);
  assert(Theme      != NULL);
  assert(Settings   != NULL);

  m_DataModule = DataModule;

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // Layout to always fill the complete image pane with ViewWindow
  m_parentLayout = new QGridLayout(parent);
  m_parentLayout->setContentsMargins(9,9,9,9);
  m_parentLayout->setSpacing(0);
  m_parentLayout->addWidget(this);
  this->setStyleSheet("QGraphicsView { border: none; }");

  // We create a Graphicsscene and connect it.
  m_Scene = new QGraphicsScene;
  setScene(m_Scene);
}

//==============================================================================

ptImageView::~ptImageView() {
  DelAndNull(m_Scene);
  DelAndNull(m_parentLayout);
}

//==============================================================================

void ptImageView::Display(const QString FileName) {
  QImage* Image = m_DataModule->getThumbnail(FileName, 0);
  if (Image != NULL)
    m_Scene->addPixmap(QPixmap::fromImage(*Image));
}

//==============================================================================
