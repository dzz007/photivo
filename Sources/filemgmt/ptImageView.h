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

#ifndef PTIMAGEVIEW_H
#define PTIMAGEVIEW_H

//==============================================================================

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

#include "../ptReportOverlay.h"

//==============================================================================

class ptImageView: public QGraphicsView {
Q_OBJECT
public:
  explicit ptImageView(QObject* parent = NULL);
  ~ptImageView();

  void setImage(QImage* image);

protected:
  void resizeEvent(QResizeEvent* event);
  void wheelEvent(QWheelEvent* event);


private:
  void ZoomTo(float factor);
  int ZoomToFit(const bool withMsg = true);

  const short MinZoom;
  const short MaxZoom;

  QGraphicsScene*       m_Scene;
  ptReportOverlay*      m_ZoomSizeOverlay;
  QGraphicsPixmapItem*  m_PixItem;
  qreal                 m_ZoomFactor;
  short                 m_ZoomMode;


//==============================================================================
};
#endif // PTIMAGEVIEW_H
