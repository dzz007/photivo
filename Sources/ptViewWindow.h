/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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

#ifndef PTVIEWWINDOW_H
#define PTVIEWWINDOW_H

#include <QLine>
#include <QMenu>
#include <QGraphicsView>

#include "ptImage.h"
#include "ptMainWindow.h"

///////////////////////////////////////////////////////////////////////////
//
// class ptViewWindow
//
///////////////////////////////////////////////////////////////////////////

class ptViewWindow : public QGraphicsView {

Q_OBJECT

///////////////////////////////////////////////////////////////////////////
//
// PUBLIC members
//
///////////////////////////////////////////////////////////////////////////
public:
  ptViewWindow(QWidget* Parent, ptMainWindow* mainWin);
  ~ptViewWindow();

  void UpdateView(const ptImage* relatedImage = NULL);
  inline int zoomPercent() { return qRound(m_ZoomFactor * 100); }
  inline float zoomFactor() { return m_ZoomFactor; }
  int ZoomTo(const float factor);  // 1.0 means 100%
  int ZoomToFit();


///////////////////////////////////////////////////////////////////////////
//
// PROTECTED members
//
///////////////////////////////////////////////////////////////////////////
protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void paintEvent(QPaintEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE members
//
///////////////////////////////////////////////////////////////////////////
private:
  const float MinZoom;
  const float MaxZoom;


  QGraphicsScene* m_ImageScene;
  QGraphicsPixmapItem* m_8bitImageItem;

  QLine* m_DragDelta;
  short m_LeftMousePressed;
  float m_ZoomFactor;

  void ZoomView(const quint32 width, const quint32 height);


  // context menu stuff
  void ConstructContextMenu();
  QMenu* m_Menu;
  QMenu* m_Menu_Mode;
  QMenu* m_Menu_Clip;
  QAction* ac_ZoomFit;
  QAction* ac_Zoom100;
  QAction* ac_Mode_RGB;
  QAction* ac_Mode_Structure;
  QAction* ac_Mode_L;
  QAction* ac_Mode_A;
  QAction* ac_Mode_B;
  QAction* ac_Mode_Gradient;
  QAction* ac_Clip_Indicate;
  QAction* ac_Clip_Over;
  QAction* ac_Clip_Under;
  QAction* ac_Clip_R;
  QAction* ac_Clip_G;
  QAction* ac_Clip_B;
  QAction* ac_SensorClip;
  QAction* ac_SensorClipSep;
  QAction* ac_ShowTools;
  QAction* ac_ShowZoomBar;
  QAction* ac_Fullscreen;
  QActionGroup* ac_ModeGroup;

  ptMainWindow* MainWindow;


///////////////////////////////////////////////////////////////////////////
//
// PRIVATE SLOTS
//
///////////////////////////////////////////////////////////////////////////
private slots:
  // context menu stuff
  void MenuExpIndicate();
  void MenuExpIndOver();
  void MenuExpIndUnder();
  void MenuExpIndR();
  void MenuExpIndG();
  void MenuExpIndB();
  void MenuExpIndSensor();
  void MenuShowBottom();
  void MenuShowTools();
  void MenuFullScreen();
  void MenuZoomFit();
  void MenuZoom100();
  void MenuMode();
};

#endif
