/*******************************************************************************
**
** Photivo
**
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
/**
** Reminder:
** current horizontal scale factor: this->transform().m11();
** current vertical scale factor: this->transform().m22();
** Because we do not change aspect ratio both factors are always the same.
** Use m_ZoomFactor whenever possible and m11() otherwise.
**/

#include <cassert>

#include "ptSettings.h"
#include "ptTheme.h"
#include "ptViewWindow.h"
#include "ptConstants.h"
#include "ptDefines.h"

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
  MinZoom(0.05), MaxZoom(4.0),
  // member variables
  m_DrawLine(NULL),
  m_SelectRect(NULL),
  m_Crop(NULL),
  m_Interaction(iaNone),
  m_LeftMousePressed(0),
  m_ZoomIsSaved(0),
  m_ZoomFactor(1.0),
  m_ZoomFactorSav(0.0),
  m_ZoomModeSav(ptZoomMode_Fit),
  // always keep this at the end of the initializer list
  MainWindow(mainWin)
{
  assert(MainWindow != NULL);    // Main window must exist before view window
  assert(Theme != NULL);
  assert(Settings != NULL);

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  m_DragDelta = new QLine();
  m_StatusOverlay = new ptReportOverlay(this, "", QColor(), QColor(), 0, Qt::AlignLeft, 20);
  m_ZoomSizeOverlay = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
                                          1000, Qt::AlignRight, 20);

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
  m_ImageScene = new QGraphicsScene(0, 0, 0, 0, Parent);
  m_8bitImageItem = m_ImageScene->addPixmap(QPixmap());
  m_8bitImageItem->setPos(0, 0);
  this->setScene(m_ImageScene);

  m_Grid = new ptGridInteraction(this);  // scene must already be alive
  ConstructContextMenu();
}


////////////////////////////////////////////////////////////////////////////////
//
// ptViewWindow destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptViewWindow::~ptViewWindow() {
  delete m_DragDelta;
  delete m_StatusOverlay;
  delete m_ZoomSizeOverlay;
  delete m_DrawLine;
  delete m_SelectRect;
  delete m_Grid;
}


////////////////////////////////////////////////////////////////////////////////
//
// UpdateImage()
//
// Convert a 16bit ptImage to an 8bit QPixmap. Mind R<->B. Also update the
// graphics scene and the viewport.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::UpdateImage(const ptImage* relatedImage) {
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
    DelAndNull(Img8bit);
    m_ImageScene->setSceneRect(0, 0,
                               m_8bitImageItem->pixmap().width(),
                               m_8bitImageItem->pixmap().height());
    m_8bitImageItem->setPos(0, 0);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Zoom
//
////////////////////////////////////////////////////////////////////////////////

// ZoomTo() is also called by wheelEvent() for mouse wheel zoom.
void ptViewWindow::ZoomTo(float factor) {
  Settings->SetValue("ZoomMode",ptZoomMode_NonFit);
  factor = qBound(MinZoom, factor, MaxZoom);

  if(((uint)(factor * 10000) % 10000) < 1) {
    // nearest neighbour resize for 200%, 300%, 400% zoom
    m_8bitImageItem->setTransformationMode(Qt::FastTransformation);
  } else {
    // bilinear resize for all others
    m_8bitImageItem->setTransformationMode(Qt::SmoothTransformation);
  }
  setTransform(QTransform(factor, 0, 0, factor, 0, 0));

  m_ZoomFactor = transform().m11();
  int z = qRound(m_ZoomFactor * 100);
  Settings->SetValue("Zoom", z);
  m_ZoomSizeOverlay->exec(QString::number(z) + "%");
}


int ptViewWindow::ZoomToFit(const short withMsg /*= 1*/) {
  Settings->SetValue("ZoomMode",ptZoomMode_Fit);

  if (!m_8bitImageItem->pixmap().isNull()) {
    // Always smooth scaling because we don't know the zoom factor in advance.
    m_8bitImageItem->setTransformationMode(Qt::SmoothTransformation);
    fitInView(m_8bitImageItem, Qt::KeepAspectRatio);
    m_ZoomFactor = transform().m11();

    if (withMsg) {
      m_ZoomSizeOverlay->exec(tr("Fit"));
    }
  }

  Settings->SetValue("Zoom",qRound(m_ZoomFactor * 100));
  return m_ZoomFactor;
}


////////////////////////////////////////////////////////////////////////////////
//
// Save and restore current zoom
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::RestoreZoom() {
  if (m_ZoomIsSaved) {
    if (m_ZoomModeSav == ptZoomMode_Fit) {
      ZoomToFit();
    } else {
      ZoomTo(m_ZoomFactorSav);
    }

    m_ZoomIsSaved = 0;
  }
}


void ptViewWindow::SaveZoom() {
  m_ZoomFactorSav = m_ZoomFactor;
  m_ZoomModeSav = Settings->GetValue("ZoomMode").toInt();
  m_ZoomIsSaved = 1;
}


////////////////////////////////////////////////////////////////////////////////
//
// setGrid()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::setGrid(const short enabled, const uint linesX, const uint linesY) {
  if (enabled) {
    m_Grid->show(linesX, linesY);
  } else {
    m_Grid->hide();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// paintEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::paintEvent(QPaintEvent* event) {
  // Fill viewport with background colour
  QPainter Painter(viewport());
  Painter.fillRect(0, 0, viewport()->width(), viewport()->height(), palette().color(QPalette::Window));

  // takes care of updating the scene
  QGraphicsView::paintEvent(event);
}


////////////////////////////////////////////////////////////////////////////////
//
// Mouse clicks
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
    m_LeftMousePressed = 1;
    m_DragDelta->setPoints(event->pos(), event->pos());
  }

  // Broadcast event to possible interaction handlers
  if (m_Interaction != iaNone) {
    emit mouseChanged(event);
  }
}


void ptViewWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && m_LeftMousePressed) {
    m_LeftMousePressed = 0;
    event->accept();
  } else {
    event->ignore();
  }

  // Broadcast event to possible interaction handlers
  if (m_Interaction != iaNone) {
    emit mouseChanged(event);
  }
}


void ptViewWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  // Broadcast event to possible interaction handlers
  if (m_Interaction != iaNone) {
    emit mouseChanged(event);
  } else {
    event->ignore();
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// mouseMoveEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseMoveEvent(QMouseEvent* event) {
  // drag image with left mouse button to scroll
  // Also Ctrl needed in crop mode
  short ImgDragging = m_LeftMousePressed && m_Interaction == iaNone;
  if (m_Interaction == iaCrop) {
    ImgDragging = m_LeftMousePressed && m_CtrlIsPressed;
  }

  if (ImgDragging) {
    m_DragDelta->setP2(event->pos());
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                    m_DragDelta->x2() +
                                    m_DragDelta->x1());
    verticalScrollBar()->setValue(verticalScrollBar()->value() -
                                  m_DragDelta->y2() +
                                  m_DragDelta->y1());
    m_DragDelta->setP1(event->pos());
    event->accept();

  } else {
    event->ignore();
    // Broadcast event to possible interaction handlers
    if (m_Interaction != iaNone) {
      emit mouseChanged(event);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// wheelEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::wheelEvent(QWheelEvent* event) {
  int ZoomIdx = -1;

  // zoom larger
  if (event->delta() > 0) {
    for (int i = 0; i < ZoomFactors.size(); i++) {
      if (ZoomFactors[i] > m_ZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }

  // zoom smaller
  } else if (event->delta() < 0) {
    for (int i = ZoomFactors.size() - 1; i >= 0; i--) {
      if (ZoomFactors[i] < m_ZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }
  }

  if (ZoomIdx != -1) {
    ZoomTo(ZoomFactors[ZoomIdx]);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// keyPressEvent() and keyReleaseEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::keyPressEvent(QKeyEvent* event) {
  // m_CtrlIsPressed is not a simple bool flag to account for keyboards with
  // multiple ctrl keys. There's a lot of those. ;-) For each ctrl press the
  // variable is increased, for each release it's decreased. This is necessary
  // because in cases like press left ctrl - press right ctrl - release left ctrl
  // Photivo should still recognise the ctrl key as being held down.
  if (event->key() == Qt::Key_Control) {
    m_CtrlIsPressed++;
    if (m_Interaction == iaNone || m_Interaction == iaCrop) {
      setCursor(Qt::ClosedHandCursor);
    }
  } else {
    event->ignore();  // necessary to forward unhandled keys to main window
  }

  if (m_Interaction != iaNone) {
    emit keyChanged(event);
  }
}

void ptViewWindow::keyReleaseEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Control) {
    m_CtrlIsPressed--;
    if (m_CtrlIsPressed == 0 && (m_Interaction == iaNone || m_Interaction == iaCrop)) {
      setCursor(Qt::ArrowCursor);
    }
  } else {
    event->ignore();  // necessary to forward unhandled keys to main window
  }

  if (m_Interaction != iaNone) {
    emit keyChanged(event);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Drag & Drop
//
// The two functions are necessary to enable d&d over the view window.
// The actual d&d action is handled by the resp. main window events.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::dragEnterEvent(QDragEnterEvent* event) {
  event->ignore();
}

void ptViewWindow::dropEvent(QDropEvent* event) {
  event->ignore();
}


////////////////////////////////////////////////////////////////////////////////
//
// resizeEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::resizeEvent(QResizeEvent* event) {
  if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit) {
    event->accept();
    ZoomToFit(0);
  } else {
    // takes care of positioning the scene inside the viewport on non-fit zooms
    QGraphicsView::resizeEvent(event);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// ShowStatus()
// Top left corner overlay for the processing status
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::ShowStatus(short mode) {
  switch (mode) {
    case ptStatus_Done:
      m_StatusOverlay->setColors(QColor(0,130,0), QColor(120,170,120));   // green
      m_StatusOverlay->setDuration(1500);
      m_StatusOverlay->exec(QObject::tr("Done"));
      break;

    case ptStatus_Updating:
      m_StatusOverlay->setColors(QColor(255,140,0), QColor(255,200,120));   // orange
      m_StatusOverlay->setDuration(0);
      m_StatusOverlay->exec(QObject::tr("Updating"));
      break;

    case ptStatus_Processing:
      m_StatusOverlay->setColors(QColor(255,75,75), QColor(255,190,190));   // red
      m_StatusOverlay->setDuration(0);
      m_StatusOverlay->exec(QObject::tr("Processing"));
      break;

    default:    // should not happen
      m_StatusOverlay->stop();
      break;
  }
}

void ptViewWindow::ShowStatus(const QString text) {
  m_StatusOverlay->setColors(QColor(75,150,255), QColor(190,220,255));    // blue
  m_StatusOverlay->setDuration(1500);
  m_StatusOverlay->exec(text);
}



////////////////////////////////////////////////////////////////////////////////
//
// StartLine()
// Start draw line interaction to determine rotation angle.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::StartLine() {
  if (m_Interaction == iaNone) {
    m_DrawLine = new ptLineInteraction(this);
    connect(m_DrawLine, SIGNAL(finished(ptStatus)), this, SLOT(finishInteraction(ptStatus)));
    connect(this, SIGNAL(mouseChanged(QMouseEvent*)), m_DrawLine, SLOT(mouseAction(QMouseEvent*)));
    connect(this, SIGNAL(keyChanged(QKeyEvent*)), m_DrawLine, SLOT(keyAction(QKeyEvent*)));
    m_Interaction = iaDrawLine;
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// StartSelectRect()
// Start simple selection interaction for spot WB and histogram "crop".
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::StartSimpleRect(void (*CB_SimpleRect)(const ptStatus, QRect)) {
  if (m_Interaction == iaNone) {
    assert(CB_SimpleRect != NULL);
    m_CB_SimpleRect = CB_SimpleRect;
    m_SelectRect = new ptSimpleRectInteraction(this);
    connect(m_SelectRect, SIGNAL(finished(ptStatus)), this, SLOT(finishInteraction(ptStatus)));
    connect(this, SIGNAL(mouseChanged(QMouseEvent*)), m_SelectRect, SLOT(mouseAction(QMouseEvent*)));
    connect(this, SIGNAL(keyChanged(QKeyEvent*)), m_SelectRect, SLOT(keyAction(QKeyEvent*)));
    m_Interaction = iaSelectRect;
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// StartCrop()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::StartCrop()
{
  if (m_Interaction != iaNone) {
    return;
  }

  // Get crop rect values from settings. The bit shift converts from 1:1 to
  // current pipe size.
  int x = Settings->GetInt("CropX") >> Settings->GetInt("Scaled");
  int y = Settings->GetInt("CropY") >> Settings->GetInt("Scaled");
  int width = Settings->GetInt("CropW") >> Settings->GetInt("Scaled");
  int height = Settings->GetInt("CropH") >> Settings->GetInt("Scaled");

  // If any value is outside the allowed range reset the inital crop
  // rectangle to the whole image.
  if(x < 0 || x >= m_8bitImageItem->pixmap().width() ||
     y < 0 || y >= m_8bitImageItem->pixmap().height() ||
     width <= 0 || width > m_8bitImageItem->pixmap().width() ||
     height <= 0 || height > m_8bitImageItem->pixmap().height() )
  {
    x = 0;
    y = 0;
    width = m_8bitImageItem->pixmap().width();
    height = m_8bitImageItem->pixmap().height();
  }


  m_Crop = new ptRichRectInteraction(this, x, y, width, height,
                                     Settings->GetInt("FixedAspectRatio"),
                                     Settings->GetInt("AspectRatioW"),
                                     Settings->GetInt("AspectRatioH"),
                                     Settings->GetInt("CropGuidelines") );

  connect(m_Crop, SIGNAL(finished(ptStatus)), this, SLOT(finishInteraction(ptStatus)));
  connect(this, SIGNAL(mouseChanged(QMouseEvent*)), m_Crop, SLOT(mouseAction(QMouseEvent*)));
  connect(this, SIGNAL(keyChanged(QKeyEvent*)), m_Crop, SLOT(keyAction(QKeyEvent*)));
  m_Interaction = iaCrop;
}


////////////////////////////////////////////////////////////////////////////////
//
// finishInteraction()
//
////////////////////////////////////////////////////////////////////////////////

void RotateAngleDetermined(const ptStatus ExitStatus, double RotateAngle);
void CleanupAfterCrop(const ptStatus CropStatus, const QRect CropRect);

void ptViewWindow::finishInteraction(ptStatus ExitStatus) {
  switch (m_Interaction) {
    case iaDrawLine: {
      double Angle = m_DrawLine->angle();
      DelAndNull(m_DrawLine);   // also disconnects all signals/slots
      m_Interaction = iaNone;
      RotateAngleDetermined(ExitStatus, Angle);
      break;
    }

    case iaSelectRect: {
      QRect sr = m_SelectRect->rect();
      DelAndNull(m_SelectRect);   // also disconnects all signals/slots
      m_Interaction = iaNone;
      m_CB_SimpleRect(ExitStatus, sr);
      break;
    }

    case iaCrop: {
      QRect cr = m_Crop->rect();
      DelAndNull(m_Crop);   // also disconnects all signals/slots
      m_Interaction = iaNone;
      CleanupAfterCrop(ExitStatus, cr);
      break;
    }

    default:
      assert(!"Unknown m_Interaction");
      break;
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
  connect(ac_ZoomFit, SIGNAL(triggered()), this, SLOT(Menu_ZoomFit()));
  QIcon ZoomFitIcon;
  ZoomFitIcon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag_h.png")));
  ac_ZoomFit->setIcon(ZoomFitIcon);
  ac_ZoomFit->setIconVisibleInMenu(true);

  ac_Zoom100 = new QAction(tr("Zoom 100%"), this);
  connect(ac_Zoom100, SIGNAL(triggered()), this, SLOT(Menu_Zoom100()));
  QIcon Zoom100Icon;
  Zoom100Icon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag1.png")));
  ac_Zoom100->setIcon(Zoom100Icon);
  ac_Zoom100->setIconVisibleInMenu(true);


  ac_Mode_RGB = new QAction(tr("RGB"), this);
  ac_Mode_RGB->setCheckable(true);
  connect(ac_Mode_RGB, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_Structure = new QAction(tr("Structure"), this);
  ac_Mode_Structure->setCheckable(true);
  connect(ac_Mode_Structure, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_L = new QAction(tr("L*"), this);
  ac_Mode_L->setCheckable(true);
  connect(ac_Mode_L, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_A = new QAction(tr("a*"), this);
  ac_Mode_A->setCheckable(true);
  connect(ac_Mode_A, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_B = new QAction(tr("b*"), this);
  ac_Mode_B->setCheckable(true);
  connect(ac_Mode_B, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_Gradient = new QAction(tr("Gradient"), this);
  ac_Mode_Gradient->setCheckable(true);
  connect(ac_Mode_Gradient, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

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
  connect(ac_Clip_Indicate, SIGNAL(triggered()), this, SLOT(Menu_Clip_Indicate()));

  ac_Clip_Over = new QAction(tr("Over exposure"), this);
  ac_Clip_Over->setCheckable(true);
  ac_Clip_Over->setChecked(Settings->GetInt("ExposureIndicatorOver"));
  connect(ac_Clip_Over, SIGNAL(triggered()), this, SLOT(Menu_Clip_Over()));

  ac_Clip_Under = new QAction(tr("Under exposure"), this);
  ac_Clip_Under->setCheckable(true);
  ac_Clip_Under->setChecked(Settings->GetInt("ExposureIndicatorUnder"));
  connect(ac_Clip_Under, SIGNAL(triggered()), this, SLOT(Menu_Clip_Under()));

  ac_Clip_R = new QAction(tr("R"), this);
  ac_Clip_R->setCheckable(true);
  ac_Clip_R->setChecked(Settings->GetInt("ExposureIndicatorR"));
  connect(ac_Clip_R, SIGNAL(triggered()), this, SLOT(Menu_Clip_R()));

  ac_Clip_G = new QAction(tr("G"), this);
  ac_Clip_G->setCheckable(true);
  ac_Clip_G->setChecked(Settings->GetInt("ExposureIndicatorG"));
  connect(ac_Clip_G, SIGNAL(triggered()), this, SLOT(Menu_Clip_G()));

  ac_Clip_B = new QAction(tr("B"), this);
  ac_Clip_B->setCheckable(true);
  ac_Clip_B->setChecked(Settings->GetInt("ExposureIndicatorB"));
  connect(ac_Clip_B, SIGNAL(triggered()), this, SLOT(Menu_Clip_B()));

  ac_SensorClip = new QAction(tr("Sensor"), this);
  ac_SensorClip->setCheckable(true);
  ac_SensorClip->setChecked(Settings->GetInt("ExposureIndicatorSensor"));
  connect(ac_SensorClip, SIGNAL(triggered()), this, SLOT(Menu_SensorClip()));
  ac_SensorClipSep = new QAction(this);
  ac_SensorClipSep->setSeparator(true);

  ac_ShowZoomBar = new QAction(tr("Show zoom bar"), this);
  ac_ShowZoomBar->setCheckable(true);
  ac_ShowZoomBar->setChecked(Settings->GetInt("ShowBottomContainer"));
  connect(ac_ShowZoomBar, SIGNAL(triggered()), this, SLOT(Menu_ShowZoomBar()));

  ac_ShowTools = new QAction(tr("Show tools"), this);
  ac_ShowTools->setCheckable(true);
  ac_ShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  connect(ac_ShowTools, SIGNAL(triggered()), this, SLOT(Menu_ShowTools()));

  ac_Fullscreen = new QAction(tr("Full screen"), this);
  ac_Fullscreen->setCheckable(true);
  ac_Fullscreen->setChecked(0);
  connect(ac_Fullscreen, SIGNAL(triggered()), this, SLOT(Menu_Fullscreen()));
}


////////////////////////////////////////////////////////////////////////////////
//
// contextMenuEvent()
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::contextMenuEvent(QContextMenuEvent* event) {
  if (m_Interaction == iaSelectRect || m_Interaction == iaDrawLine) {
    return;
  }

  // Create the menus themselves
  // Note: Menus cannot be created with new. That breaks the theming.
  QMenu Menu_Mode(tr("Mode"), this);
  Menu_Mode.setPalette(Theme->ptMenuPalette);
  Menu_Mode.setStyle(Theme->ptStyle);
  Menu_Mode.addAction(ac_Mode_RGB);
  Menu_Mode.addAction(ac_Mode_Structure);
  Menu_Mode.addAction(ac_Mode_L);
  Menu_Mode.addAction(ac_Mode_A);
  Menu_Mode.addAction(ac_Mode_B);
  Menu_Mode.addAction(ac_Mode_Gradient);

  QMenu Menu_Clip(tr("Clipping"), this);
  Menu_Clip.setPalette(Theme->ptMenuPalette);
  Menu_Clip.setStyle(Theme->ptStyle);
  Menu_Clip.addAction(ac_Clip_Indicate);
  Menu_Clip.addSeparator();
  Menu_Clip.addAction(ac_Clip_Over);
  Menu_Clip.addAction(ac_Clip_Under);
  Menu_Clip.addSeparator();
  Menu_Clip.addAction(ac_Clip_R);
  Menu_Clip.addAction(ac_Clip_G);
  Menu_Clip.addAction(ac_Clip_B);

  QMenu Menu(this);
  Menu.setPalette(Theme->ptMenuPalette);
  Menu.setStyle(Theme->ptStyle);
  Menu.addAction(ac_ZoomFit);
  Menu.addAction(ac_Zoom100);
  Menu.addSeparator();
  if (m_Interaction == iaNone) {
    Menu.addMenu(&Menu_Mode);
    Menu.addMenu(&Menu_Clip);
    Menu.addSeparator();
    Menu.addAction(ac_SensorClip);
    Menu.addAction(ac_SensorClipSep);
  }
  Menu.addAction(ac_ShowTools);
  Menu.addAction(ac_ShowZoomBar);
  Menu.addSeparator();
  Menu.addAction(ac_Fullscreen);

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

  Menu.exec(((QMouseEvent*)event)->globalPos());
}


////////////////////////////////////////////////////////////////////////////////
//
// slots for the context menu
//
////////////////////////////////////////////////////////////////////////////////

void Update(short Phase, short SubPhase = -1, short WithIdentify  = 1, short ProcessorMode = ptProcessorMode_Preview);
void ptViewWindow::Menu_Clip_Indicate() {
  Settings->SetValue("ExposureIndicator",(int)ac_Clip_Indicate->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_Clip_Over() {
  Settings->SetValue("ExposureIndicatorOver",(int)ac_Clip_Over->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_Clip_Under() {
  Settings->SetValue("ExposureIndicatorUnder",(int)ac_Clip_Under->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_Clip_R() {
  Settings->SetValue("ExposureIndicatorR",(int)ac_Clip_R->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_Clip_G() {
  Settings->SetValue("ExposureIndicatorG",(int)ac_Clip_G->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_Clip_B() {
  Settings->SetValue("ExposureIndicatorB",(int)ac_Clip_B->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_SensorClip() {
  Settings->SetValue("ExposureIndicatorSensor",(int)ac_SensorClip->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::Menu_ShowZoomBar() {
  Settings->SetValue("ShowBottomContainer",(int)ac_ShowZoomBar->isChecked());
  MainWindow->UpdateSettings();
}

void ptViewWindow::Menu_ShowTools() {
  Settings->SetValue("ShowToolContainer",(int)ac_ShowTools->isChecked());
  MainWindow->UpdateSettings();
}

void CB_FullScreenButton(const int State);
void ptViewWindow::Menu_Fullscreen() {
  CB_FullScreenButton((int)ac_Fullscreen->isChecked());
}

void ptViewWindow::Menu_ZoomFit() {
  ZoomToFit();
}

void ptViewWindow::Menu_Zoom100() {
  ZoomTo(1.0);
}

void ptViewWindow::Menu_Mode() {
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
