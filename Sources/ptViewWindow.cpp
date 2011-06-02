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

#include <cassert>

#include "ptSettings.h"
#include "ptTheme.h"
#include "ptViewWindow.h"

extern ptTheme* Theme;
extern ptSettings* Settings;

////////////////////////////////////////////////////////////////////////////////
//
// ptViewWindow constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptViewWindow::ptViewWindow(QWidget* Parent, ptMainWindow* mainWin)
: QGraphicsView(Parent),
  // constants
  MinZoom(0.1), MaxZoom(400.0),
  // member variables
  m_LeftMousePressed(0),
  m_ZoomFactor(1.0),
  // always keep this at the end of the list
  MainWindow(mainWin)
{
  assert(MainWindow != NULL);    // Main window must exist before view window

  m_DragDelta = new QLine();

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setMouseTracking(true);   // Move events without pressed button. Needed for crop cursor change.

  // Layout to always fill the complete image pane with ViewWindow
  QGridLayout* Layout = new QGridLayout(Parent);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(this);
  this->setStyleSheet("QGraphicsView { border: none; }");

  // Init the scene
  m_ImageScene = new QGraphicsScene(Parent);
  m_8bitImageItem = m_ImageScene->addPixmap(QPixmap());
  m_8bitImageItem->setPos(0, 0);
  this->setScene(m_ImageScene);

  ConstructContextMenu();
  this->show();
}

////////////////////////////////////////////////////////////////////////////////
//
// ptViewWindow destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptViewWindow::~ptViewWindow() {
  delete m_Menu;
  delete m_Menu_Mode;
  delete m_Menu_Clip;
  delete m_DragDelta;
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateView()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::UpdateView(const ptImage* relatedImage /*= NULL*/) {
  // Convert the 16bit ptImage to a 8bit QPixmap. Mind R<->B and 16->8
  if (relatedImage) {
    QImage* Img8bit = new QImage(relatedImage->m_Width, relatedImage->m_Height, QImage::Format_RGB32);
    uint32_t Size = relatedImage->m_Height * relatedImage->m_Width;
    uint32_t* ImagePtr = (QRgb*) Img8bit->scanLine(0);

#pragma omp parallel for schedule(static)
    for (uint32_t i = 0; i < Size; i++) {
      uint8_t* Pixel = (uint8_t*) ImagePtr + (i<<2);
      for (short c=0; c<3; c++) {
        // Mind the R<->B swap !
        Pixel[2-c] = relatedImage->m_Image[i][c]>>8;
      }
      Pixel[3] = 0xff;
    }

    m_8bitImageItem->setPixmap(QPixmap::fromImage(*Img8bit, Qt::ColorOnly));
    m_8bitImageItem->setPos(0, 0);
    delete Img8bit;
  }

  this->horizontalScrollBar()->setRange(0, m_ImageScene->width() - viewport()->width());
  this->verticalScrollBar()->setRange(0, m_ImageScene->height() - viewport()->height());

}


////////////////////////////////////////////////////////////////////////////////
//
// Zoom
//
////////////////////////////////////////////////////////////////////////////////

int ptViewWindow::ZoomTo(const float factor) {
  if (factor >= MinZoom && factor <= MaxZoom) {
    m_ZoomFactor = factor;
    ZoomView(qRound(m_8bitImageItem->pixmap().width() * factor),
             qRound(m_8bitImageItem->pixmap().height() * factor) );
  }

  return m_ZoomFactor;
}


int ptViewWindow::ZoomToFit() {
printf("LEBT000\n");
if (m_8bitImageItem->pixmap().isNull()) {    printf("FUCK\n"); }
  float factorW = viewport()->width() / m_8bitImageItem->pixmap().width();
  float factorH = viewport()->height() / m_8bitImageItem->pixmap().height();
  m_ZoomFactor = qMin(factorW, factorH);
printf("LEBT\n");
  fitInView(m_8bitImageItem, Qt::KeepAspectRatio);
printf("LEBT NOCH\n");
//  ZoomView((quint32)(m_8bitImageItem->pixmap().width() * m_ZoomFactor),
//           (quint32)(m_8bitImageItem->pixmap().height() * m_ZoomFactor) );

  return m_ZoomFactor;
}


void ptViewWindow::ZoomView(const quint32 width, const quint32 height) {

}


////////////////////////////////////////////////////////////////////////////////
//
// paintEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::paintEvent(QPaintEvent* event) {
  // Fill viewport with background colour
  QPainter Painter(viewport());
  Painter.save();
  Painter.fillRect(0, 0, viewport()->width(), viewport()->height(), palette().color(QPalette::Window));
  Painter.restore();
  m_ImageScene->setBackgroundBrush(QBrush(palette().color(QPalette::Window)));

  QGraphicsView::paintEvent(event);
}


////////////////////////////////////////////////////////////////////////////////
//
// mousePressEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_LeftMousePressed = 1;
    m_DragDelta->setPoints(event->pos(), event->pos());
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// mouseReleaseEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_LeftMousePressed = 0;
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// mouseMoveEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseMoveEvent(QMouseEvent* event) {
  if (m_LeftMousePressed) {
    m_DragDelta->setP2(event->pos());
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - m_DragDelta->x2() + m_DragDelta->x1());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - m_DragDelta->y2() + m_DragDelta->y1());
    m_DragDelta->setP1(event->pos());
  }
}








////////////////////////////////////////////////////////////////////////////////
//
// Construct context menu
//
// Convenience function to keep constructor short. Is only called once from the
// constructor.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::ConstructContextMenu() {
  // Create actions for context menu
  ac_ZoomFit = new QAction(tr("Zoom fit"), this);
  connect(ac_ZoomFit, SIGNAL(triggered()), this, SLOT(MenuZoomFit()));
  QIcon ZoomFitIcon;
  ZoomFitIcon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag_h.png")));
  ac_ZoomFit->setIcon(ZoomFitIcon);
  ac_ZoomFit->setIconVisibleInMenu(true);

  ac_Zoom100 = new QAction(tr("Zoom 100%"), this);
  connect(ac_Zoom100, SIGNAL(triggered()), this, SLOT(MenuZoom100()));
  QIcon Zoom100Icon;
  Zoom100Icon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag1.png")));
  ac_Zoom100->setIcon(Zoom100Icon);
  ac_Zoom100->setIconVisibleInMenu(true);


  ac_Mode_RGB = new QAction(tr("RGB"), this);
  ac_Mode_RGB->setCheckable(true);
  connect(ac_Mode_RGB, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_Mode_Structure = new QAction(tr("Structure"), this);
  ac_Mode_Structure->setCheckable(true);
  connect(ac_Mode_Structure, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_Mode_L = new QAction(tr("L*"), this);
  ac_Mode_L->setCheckable(true);
  connect(ac_Mode_L, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_Mode_A = new QAction(tr("a*"), this);
  ac_Mode_A->setCheckable(true);
  connect(ac_Mode_A, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_Mode_B = new QAction(tr("b*"), this);
  ac_Mode_B->setCheckable(true);
  connect(ac_Mode_B, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_Mode_Gradient = new QAction(tr("Gradient"), this);
  ac_Mode_Gradient->setCheckable(true);
  connect(ac_Mode_Gradient, SIGNAL(triggered()), this, SLOT(MenuMode()));

  ac_ModeGroup = new QActionGroup(this);
  ac_ModeGroup->addAction(ac_Mode_RGB);
  ac_ModeGroup->addAction(ac_Mode_L);
  ac_ModeGroup->addAction(ac_Mode_A);
  ac_ModeGroup->addAction(ac_Mode_B);
  ac_ModeGroup->addAction(ac_Mode_Gradient);
  ac_ModeGroup->addAction(ac_Mode_Structure);


  ac_Clip_Indicate = new QAction(tr("Indicate"), this);
  ac_Clip_Indicate->setStatusTip(tr("Indicate clipping"));
  ac_Clip_Indicate->setCheckable(true);
  ac_Clip_Indicate->setChecked(Settings->GetInt("ExposureIndicator"));
  connect(ac_Clip_Indicate, SIGNAL(triggered()), this, SLOT(MenuExpIndicate()));

  ac_Clip_Over = new QAction(tr("Over exposure"), this);
  ac_Clip_Over->setCheckable(true);
  ac_Clip_Over->setChecked(Settings->GetInt("ExposureIndicatorOver"));
  connect(ac_Clip_Over, SIGNAL(triggered()), this, SLOT(MenuExpIndOver()));

  ac_Clip_Under = new QAction(tr("Under exposure"), this);
  ac_Clip_Under->setCheckable(true);
  ac_Clip_Under->setChecked(Settings->GetInt("ExposureIndicatorUnder"));
  connect(ac_Clip_Under, SIGNAL(triggered()), this, SLOT(MenuExpIndUnder()));

  ac_Clip_R = new QAction(tr("R"), this);
  ac_Clip_R->setCheckable(true);
  ac_Clip_R->setChecked(Settings->GetInt("ExposureIndicatorR"));
  connect(ac_Clip_R, SIGNAL(triggered()), this, SLOT(MenuExpIndR()));

  ac_Clip_G = new QAction(tr("G"), this);
  ac_Clip_G->setCheckable(true);
  ac_Clip_G->setChecked(Settings->GetInt("ExposureIndicatorG"));
  connect(ac_Clip_G, SIGNAL(triggered()), this, SLOT(MenuExpIndG()));

  ac_Clip_B = new QAction(tr("B"), this);
  ac_Clip_B->setCheckable(true);
  ac_Clip_B->setChecked(Settings->GetInt("ExposureIndicatorB"));
  connect(ac_Clip_B, SIGNAL(triggered()), this, SLOT(MenuExpIndB()));

  ac_SensorClip = new QAction(tr("Sensor"), this);
  ac_SensorClip->setCheckable(true);
  ac_SensorClip->setChecked(Settings->GetInt("ExposureIndicatorSensor"));
  connect(ac_SensorClip, SIGNAL(triggered()), this, SLOT(MenuExpIndSensor()));
  ac_SensorClipSep = new QAction(this);
  ac_SensorClipSep->setSeparator(true);

  ac_ShowZoomBar = new QAction(tr("Show zoom bar"), this);
  ac_ShowZoomBar->setCheckable(true);
  ac_ShowZoomBar->setChecked(Settings->GetInt("ShowBottomContainer"));
  connect(ac_ShowZoomBar, SIGNAL(triggered()), this, SLOT(MenuShowBottom()));

  ac_ShowTools = new QAction(tr("Show tools"), this);
  ac_ShowTools->setCheckable(true);
  ac_ShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  connect(ac_ShowTools, SIGNAL(triggered()), this, SLOT(MenuShowTools()));

  ac_Fullscreen = new QAction(tr("Full screen"), this);
  ac_Fullscreen->setCheckable(true);
  ac_Fullscreen->setChecked(0);
  connect(ac_Fullscreen, SIGNAL(triggered()), this, SLOT(MenuFullScreen()));


  // Create the menus themselves
  m_Menu_Mode = new QMenu(tr("Mode"), this);
  m_Menu_Mode->setPalette(Theme->ptMenuPalette);
  m_Menu_Mode->setStyle(Theme->ptStyle);
  m_Menu_Mode->addAction(ac_Mode_RGB);
  m_Menu_Mode->addAction(ac_Mode_Structure);
  m_Menu_Mode->addAction(ac_Mode_L);
  m_Menu_Mode->addAction(ac_Mode_A);
  m_Menu_Mode->addAction(ac_Mode_B);
  m_Menu_Mode->addAction(ac_Mode_Gradient);
  m_Menu_Mode->setTitle(tr("Mode"));

  m_Menu_Clip = new QMenu(tr("Clipping"), this);
  m_Menu_Clip->setPalette(Theme->ptMenuPalette);
  m_Menu_Clip->setStyle(Theme->ptStyle);
  m_Menu_Clip->addAction(ac_Clip_Indicate);
  m_Menu_Clip->addSeparator();
  m_Menu_Clip->addAction(ac_Clip_Over);
  m_Menu_Clip->addAction(ac_Clip_Under);
  m_Menu_Clip->addSeparator();
  m_Menu_Clip->addAction(ac_Clip_R);
  m_Menu_Clip->addAction(ac_Clip_G);
  m_Menu_Clip->addAction(ac_Clip_B);

  m_Menu = new QMenu(this);
  m_Menu->setPalette(Theme->ptMenuPalette);
  m_Menu->setStyle(Theme->ptStyle);
  m_Menu->addAction(ac_ZoomFit);
  m_Menu->addAction(ac_Zoom100);
  m_Menu->addSeparator();
  m_Menu->addMenu(m_Menu_Mode);
  m_Menu->addMenu(m_Menu_Clip);
  m_Menu->addSeparator();
  m_Menu->addAction(ac_SensorClip);
  m_Menu->addAction(ac_SensorClipSep);
  m_Menu->addAction(ac_ShowTools);
  m_Menu->addAction(ac_ShowZoomBar);
  m_Menu->addSeparator();
  m_Menu->addAction(ac_Fullscreen);
}


////////////////////////////////////////////////////////////////////////////////
//
// contextMenuEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::contextMenuEvent(QContextMenuEvent* event) {
  if (Settings->GetInt("SpecialPreview") == ptSpecialPreview_Structure) {
    ac_Mode_Structure->setChecked(true);
  } else if (Settings->GetInt("SpecialPreview") == ptSpecialPreview_Gradient) {
    ac_Mode_Gradient->setChecked(true);
  } else if (Settings->GetInt("SpecialPreview") == ptSpecialPreview_L) {
    ac_Mode_L->setChecked(true);
  } else if (Settings->GetInt("SpecialPreview") == ptSpecialPreview_A) {
    ac_Mode_A->setChecked(true);
  } else if (Settings->GetInt("SpecialPreview") == ptSpecialPreview_B) {
    ac_Mode_B->setChecked(true);
  } else {
    ac_Mode_RGB->setChecked(true);
  }

  ac_ShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  ac_ShowZoomBar->setChecked(Settings->GetInt("ShowBottomContainer"));
  ac_Fullscreen->setChecked(Settings->GetInt("FullscreenActive"));
  ac_Clip_Indicate->setChecked(Settings->GetInt("ExposureIndicator"));

  if (Settings->GetInt("ShowExposureIndicatorSensor") == 1) {
    ac_SensorClip->setEnabled(Settings->GetInt("ShowExposureIndicatorSensor"));
    ac_SensorClip->setVisible(true);
    ac_SensorClipSep->setVisible(true);
  } else {
    ac_SensorClip->setVisible(false);
    ac_SensorClipSep->setVisible(false);
  }

  m_Menu->exec(((QMouseEvent*)event)->globalPos());
}


////////////////////////////////////////////////////////////////////////////////
//
// slots for the context menu
//
////////////////////////////////////////////////////////////////////////////////

void Update(short Phase, short SubPhase = -1, short WithIdentify  = 1, short ProcessorMode = ptProcessorMode_Preview);
void ptViewWindow::MenuExpIndicate() {
  Settings->SetValue("ExposureIndicator",(int)ac_Clip_Indicate->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndOver() {
  Settings->SetValue("ExposureIndicatorOver",(int)ac_Clip_Over->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndUnder() {
  Settings->SetValue("ExposureIndicatorUnder",(int)ac_Clip_Under->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndR() {
  Settings->SetValue("ExposureIndicatorR",(int)ac_Clip_R->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndG() {
  Settings->SetValue("ExposureIndicatorG",(int)ac_Clip_G->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndB() {
  Settings->SetValue("ExposureIndicatorB",(int)ac_Clip_B->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndSensor() {
  Settings->SetValue("ExposureIndicatorSensor",(int)ac_SensorClip->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuShowBottom() {
  Settings->SetValue("ShowBottomContainer",(int)ac_ShowZoomBar->isChecked());
  MainWindow->UpdateSettings();
}

void ptViewWindow::MenuShowTools() {
  Settings->SetValue("ShowToolContainer",(int)ac_ShowTools->isChecked());
  MainWindow->UpdateSettings();
}

void CB_FullScreenButton(const int State);
void ptViewWindow::MenuFullScreen() {
  CB_FullScreenButton((int)ac_Fullscreen->isChecked());
}

void CB_ZoomFitButton();
void ptViewWindow::MenuZoomFit() {
  CB_ZoomFitButton();
}

void CB_ZoomFullButton();
void ptViewWindow::MenuZoom100() {
  CB_ZoomFullButton();
}

void ptViewWindow::MenuMode() {
  if (ac_Mode_RGB->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_RGB);
  else if (ac_Mode_L->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_L);
  else if (ac_Mode_A->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_A);
  else if (ac_Mode_B->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_B);
  else if (ac_Mode_Gradient->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_Gradient);
  else if (ac_Mode_Structure->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_Structure);

  Update(ptProcessorPhase_NULL);
}
