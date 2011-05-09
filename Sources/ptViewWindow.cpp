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

#include "ptViewWindow.h"
#include "ptSettings.h"
#include "ptConstants.h"
#include "ptTheme.h"

#include <cassert>
#include <QPen>
#include <QMessageBox>
#include <QRect>
#include <QLine>
#include <QPoint>

// A prototype we need
void UpdateSettings();
void CB_InputChanged(const QString,const QVariant);
void CB_ZoomFitButton();

extern QString ImageFileToOpen;
extern ptTheme* Theme;

////////////////////////////////////////////////////////////////////////////////
//
// ptViewWindow constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptViewWindow::ptViewWindow(const ptImage* RelatedImage,
                                 QWidget* Parent)

  : QAbstractScrollArea(Parent), EdgeThickness(20), TinyRectThreshold(40)
{
  m_RelatedImage = RelatedImage; // don't delete that at cleanup !
  m_ZoomFactor   = 1.0;
  m_PreviousZoomFactor   = -1.0;

  // Some other dynamic members we want to have clean.
  m_QImage           = NULL;
  m_QImageZoomed     = NULL;
  m_QImageCut        = NULL;
  // With respect to event hanpting.
  m_HasGrid          = 0;
  m_GridX            = 0;
  m_GridY            = 0;
  m_CropGuidelines   = 0;
  m_CropLightsOut    = Settings->GetInt("LightsOut");
  m_FixedAspectRatio = 0;
  m_AspectRatio      = 0.0;
  m_AspectRatioW     = 0;
  m_AspectRatioH     = 0;

  m_InteractionMode = vaNone;
  m_ImageFrame      = new QRect(0,0,0,0);
  m_ViewSizeRect    = new QRect(0,0,0,0);
  m_PipeSizeRect    = new QRect(0,0,0,0);
  m_DragDelta       = new QLine(0,0,0,0);
  m_NowDragging     = 0;
  m_MovingEdge      = meNone;
  m_PrevMovingEdge  = meNone;

  //Avoiding tricky blacks at zoom fit.
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setMouseTracking(true);   // Move events without pressed button. Needed for cursor change.

  QVBoxLayout *Layout = new QVBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(this);
  Parent->setLayout(Layout);
  setStyleSheet("QAbstractScrollArea {Border: none;}");

  // OSD for the zoom size
  m_SizeReportText = "";
  QString SizeReportStyleSheet;
  SizeReportStyleSheet = "QLabel {border: 8px solid rgb(75,150,255);"
  "border-radius: 25px; padding: 8px; color: rgb(75,150,255);"
  "background: rgb(190,220,255);}";
  m_SizeReport = new QLabel();
  m_SizeReport->setText("<h1>"+m_SizeReportText+"%</h1>");
  m_SizeReport->setTextFormat(Qt::RichText);
  m_SizeReport->setAlignment(Qt::AlignCenter);
  m_SizeReport->setTextInteractionFlags(Qt::NoTextInteraction);
  m_SizeReport->setParent(this);
  m_SizeReport->setStyleSheet(SizeReportStyleSheet);
  m_SizeReport->setVisible(0);

  // A timer for report of the size
  m_SizeReportTimeOut = 1000;
  m_SizeReportTimer = new QTimer(m_SizeReport);
  m_SizeReportTimer->setSingleShot(1);

  connect(m_SizeReportTimer,SIGNAL(timeout()),
          this,SLOT(SizeReportTimerExpired()));

  // OSD for the pipe status
  m_StatusReport = new QLabel();
  m_StatusReport->setTextFormat(Qt::RichText);
  m_StatusReport->setAlignment(Qt::AlignCenter);
  m_StatusReport->setTextInteractionFlags(Qt::NoTextInteraction);
  m_StatusReport->setParent(this);
  m_StatusReport->setVisible(0);

  // A timer for report of the size
  m_StatusReportTimeOut = 1500;
  m_StatusReportTimer = new QTimer(m_StatusReport);
  m_StatusReportTimer->setSingleShot(1);

  connect(m_StatusReportTimer,SIGNAL(timeout()),
          this,SLOT(StatusReportTimerExpired()));

  // A timer for the resize with mousewheel
  m_ResizeTimeOut = 300;
  m_ResizeTimer = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);

  connect(m_ResizeTimer,SIGNAL(timeout()),
          this,SLOT(ResizeTimerExpired()));

  // for resize with mousewheel
  m_NewSize = 0;

  // Create actions for context menu
  m_AtnZoomFit = new QAction(tr("Zoom fit"), this);
  connect(m_AtnZoomFit, SIGNAL(triggered()), this, SLOT(MenuZoomFit()));
  QIcon ZoomFitIcon;
  ZoomFitIcon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag_h.png")));
  m_AtnZoomFit->setIcon(ZoomFitIcon);
  m_AtnZoomFit->setIconVisibleInMenu(true);

  m_AtnZoom100 = new QAction(tr("Zoom 100%"), this);
  connect(m_AtnZoom100, SIGNAL(triggered()), this, SLOT(MenuZoom100()));
  QIcon Zoom100Icon;
  Zoom100Icon.addPixmap(QPixmap(
    QString::fromUtf8(":/photivo/Icons/viewmag1.png")));
  m_AtnZoom100->setIcon(Zoom100Icon);
  m_AtnZoom100->setIconVisibleInMenu(true);

  m_AtnExpIndicate = new QAction(tr("Indicate"), this);
  m_AtnExpIndicate->setStatusTip(tr("Indicate clipping"));
  m_AtnExpIndicate->setCheckable(true);
  m_AtnExpIndicate->setChecked(Settings->GetInt("ExposureIndicator"));
  connect(m_AtnExpIndicate, SIGNAL(triggered()), this, SLOT(MenuExpIndicate()));

  m_AtnExpIndOver = new QAction(tr("Over exposure"), this);
  m_AtnExpIndOver->setCheckable(true);
  m_AtnExpIndOver->setChecked(Settings->GetInt("ExposureIndicatorOver"));
  connect(m_AtnExpIndOver, SIGNAL(triggered()), this, SLOT(MenuExpIndOver()));

  m_AtnExpIndUnder = new QAction(tr("Under exposure"), this);
  m_AtnExpIndUnder->setCheckable(true);
  m_AtnExpIndUnder->setChecked(Settings->GetInt("ExposureIndicatorUnder"));
  connect(m_AtnExpIndUnder, SIGNAL(triggered()), this, SLOT(MenuExpIndUnder()));

  m_AtnExpIndR = new QAction(tr("R"), this);
  m_AtnExpIndR->setCheckable(true);
  m_AtnExpIndR->setChecked(Settings->GetInt("ExposureIndicatorR"));
  connect(m_AtnExpIndR, SIGNAL(triggered()), this, SLOT(MenuExpIndR()));

  m_AtnExpIndG = new QAction(tr("G"), this);
  m_AtnExpIndG->setCheckable(true);
  m_AtnExpIndG->setChecked(Settings->GetInt("ExposureIndicatorG"));
  connect(m_AtnExpIndG, SIGNAL(triggered()), this, SLOT(MenuExpIndG()));

  m_AtnExpIndB = new QAction(tr("B"), this);
  m_AtnExpIndB->setCheckable(true);
  m_AtnExpIndB->setChecked(Settings->GetInt("ExposureIndicatorB"));
  connect(m_AtnExpIndB, SIGNAL(triggered()), this, SLOT(MenuExpIndB()));

  m_AtnExpIndSensor = new QAction(tr("Sensor"), this);
  m_AtnExpIndSensor->setCheckable(true);
  m_AtnExpIndSensor->setChecked(Settings->GetInt("ExposureIndicatorSensor"));
  connect(m_AtnExpIndSensor, SIGNAL(triggered()), this, SLOT(MenuExpIndSensor()));

  m_AtnShowBottom = new QAction(tr("Show zoom bar"), this);
  m_AtnShowBottom->setCheckable(true);
  m_AtnShowBottom->setChecked(Settings->GetInt("ShowBottomContainer"));
  connect(m_AtnShowBottom, SIGNAL(triggered()), this, SLOT(MenuShowBottom()));

  m_AtnShowTools = new QAction(tr("Show tools"), this);
  m_AtnShowTools->setCheckable(true);
  m_AtnShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  connect(m_AtnShowTools, SIGNAL(triggered()), this, SLOT(MenuShowTools()));

  m_AtnFullScreen = new QAction(tr("Full screen"), this);
  m_AtnFullScreen->setCheckable(true);
  m_AtnFullScreen->setChecked(0);
  connect(m_AtnFullScreen, SIGNAL(triggered()), this, SLOT(MenuFullScreen()));

  m_AtnModeRGB = new QAction(tr("RGB"), this);
  m_AtnModeRGB->setCheckable(true);
  connect(m_AtnModeRGB, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnModeL = new QAction(tr("L*"), this);
  m_AtnModeL->setCheckable(true);
  connect(m_AtnModeL, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnModeA = new QAction(tr("a*"), this);
  m_AtnModeA->setCheckable(true);
  connect(m_AtnModeA, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnModeB = new QAction(tr("b*"), this);
  m_AtnModeB->setCheckable(true);
  connect(m_AtnModeB, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnModeGradient = new QAction(tr("Gradient"), this);
  m_AtnModeGradient->setCheckable(true);
  connect(m_AtnModeGradient, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnModeStructure = new QAction(tr("Structure"), this);
  m_AtnModeStructure->setCheckable(true);
  connect(m_AtnModeStructure, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_ModeGroup = new QActionGroup(this);
  m_ModeGroup->addAction(m_AtnModeRGB);
  m_ModeGroup->addAction(m_AtnModeL);
  m_ModeGroup->addAction(m_AtnModeA);
  m_ModeGroup->addAction(m_AtnModeB);
  m_ModeGroup->addAction(m_AtnModeGradient);
  m_ModeGroup->addAction(m_AtnModeStructure);

  if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_Structure)
    m_AtnModeStructure->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_Gradient)
    m_AtnModeGradient->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_L)
    m_AtnModeL->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_A)
    m_AtnModeA->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_B)
    m_AtnModeB->setChecked(true);
  else
    m_AtnModeRGB->setChecked(true);
}

////////////////////////////////////////////////////////////////////////////////
//
// ptViewWindow destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptViewWindow::~ptViewWindow() {
  //printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
  delete m_QImage;
  m_QImage = NULL;      // invalidate pointer to be safe
  delete m_QImageZoomed;
  delete m_QImageCut;
  delete m_ImageFrame;
  delete m_ViewSizeRect;
  delete m_PipeSizeRect;
  delete m_DragDelta;
}


////////////////////////////////////////////////////////////////////////////////
//
// Start & stop user interaction
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::StartCrop(const int x, const int y, const int width, const int height,
                             const short FixedAspectRatio,
                             const uint AspectRatioW, const uint AspectRatioH,
                             const short CropGuidelines)
{
  assert(m_QImage != NULL);   // need image loaded to start crop

  if ((width <= 0) || (height <= 0)) {  // Catch invalid inital rect
    m_PipeSizeRect->setRect(0, 0, m_QImage->width(), m_QImage->height());

  } else {
    // Set up initial pipe sized crop rectangle.
    m_PipeSizeRect->setLeft(qBound(0, x, m_QImage->width()));
    m_PipeSizeRect->setTop(qBound(0, y, m_QImage->height()));
    m_PipeSizeRect->setWidth(qBound(0, width, m_QImage->width() - m_PipeSizeRect->left()));
    m_PipeSizeRect->setHeight(qBound(0, height, m_QImage->height() - m_PipeSizeRect->top()));
  }

  m_CropGuidelines = CropGuidelines;
  m_MovingEdge = meNone;
  m_PrevMovingEdge = meNone;
  m_DragDelta->setLine(0,0,0,0);
  m_InteractionMode = vaCrop;

  if (FixedAspectRatio) {
    setAspectRatio(FixedAspectRatio, AspectRatioW, AspectRatioH);
  } else {
    m_FixedAspectRatio = 0;
    viewport()->repaint();
  }
}

QRect ptViewWindow::StopCrop() {
  FinalizeAction();
  return QRect(m_PipeSizeRect->topLeft(), m_PipeSizeRect->bottomRight());
}

void ptViewWindow::StartSelection() {
  // (-1,-1) means rect is not set.
  m_PipeSizeRect->setRect(-1,-1,0,0);
  m_ViewSizeRect->setRect(-1,-1,0,0);
  m_FixedAspectRatio = 0;
  m_CropGuidelines = ptCropGuidelines_None;
  m_MovingEdge = meNone;
  m_PrevMovingEdge = meNone;
  m_InteractionMode = vaSelectRect;
}

void ptViewWindow::StartLine() {
  m_InteractionMode = vaDrawLine;
}

void ptViewWindow::FinalizeAction() {
  m_InteractionMode = vaNone;
  m_ViewSizeRect->setCoords(-1,-1,0,0);
  this->setCursor(Qt::ArrowCursor);
}


////////////////////////////////////////////////////////////////////////////////
//
// Getter methods to return status/result of user interaction
//
////////////////////////////////////////////////////////////////////////////////

ptViewportAction ptViewWindow::OngoingAction() {
  return m_InteractionMode;
}

double ptViewWindow::GetRotationAngle() {
  if (m_DragDelta->x1() == m_DragDelta->x2()) {
    return 90.0;
  }
  double m = -(double)(m_DragDelta->y1() - m_DragDelta->y2()) / (m_DragDelta->x1() - m_DragDelta->x2());
  return atan(m) * 180.0 / ptPI;
}

QRect ptViewWindow::GetRectangle() {
  return QRect(m_PipeSizeRect->topLeft(), m_PipeSizeRect->bottomRight());
}


////////////////////////////////////////////////////////////////////////////////
//
// Setter methods for interaction with main window UI widgets
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::setCropGuidelines(const short CropGuidelines) {
  if (m_InteractionMode == vaCrop) {
    m_CropGuidelines = CropGuidelines;
    viewport()->repaint();
  }
}


void ptViewWindow::setAspectRatio(const short FixedAspectRatio, uint AspectRatioW,
                                  uint AspectRatioH, const short ImmediateUpdate)
{
  m_FixedAspectRatio = FixedAspectRatio;
  if (m_FixedAspectRatio) {
    assert((AspectRatioW != 0) && (AspectRatioH != 0));
    m_AspectRatioW = AspectRatioW;
    m_AspectRatioH = AspectRatioH;
    m_AspectRatio = (double) AspectRatioW / (double) AspectRatioH;

    if (ImmediateUpdate) {
      m_MovingEdge = meNone;
      EnforceRectAspectRatio();
      viewport()->repaint();
    }
  }
}


void ptViewWindow::FlipAspectRatio() {
  QPoint CenterBak = m_PipeSizeRect->center();
  int HeightBak = m_PipeSizeRect->height();
  m_PipeSizeRect->setHeight(m_PipeSizeRect->width());
  m_PipeSizeRect->setWidth(HeightBak);
  m_PipeSizeRect->moveCenter(CenterBak);

  double NewAR = (double)m_PipeSizeRect->width() / m_PipeSizeRect->height();
  short WidthAdjusted = 0;
  short HeightAdjusted = 0;

  if (m_PipeSizeRect->width() > m_QImage->width()) {
    m_PipeSizeRect->setWidth(m_QImage->width());
    m_PipeSizeRect->moveLeft(0);
    WidthAdjusted = 1;
  }
  if (m_PipeSizeRect->left() < 0) {
    m_PipeSizeRect->moveLeft(0);
  } else if (m_PipeSizeRect->right() >= m_QImage->width()) {
    m_PipeSizeRect->moveRight(m_QImage->width() - 1);
  }
  if (WidthAdjusted) {
    CenterBak = m_PipeSizeRect->center();
    m_PipeSizeRect->setHeight((int)(m_PipeSizeRect->width() / NewAR));
    m_PipeSizeRect->moveCenter(CenterBak);
  }

  if (m_PipeSizeRect->height() > m_QImage->height()) {
    m_PipeSizeRect->setHeight(m_QImage->height());
    m_PipeSizeRect->moveTop(0);
    HeightAdjusted = 1;
  }
  if (m_PipeSizeRect->top() < 0) {
    m_PipeSizeRect->moveTop(0);
  } else if (m_PipeSizeRect->bottom() >= m_QImage->height()) {
    m_PipeSizeRect->moveBottom(m_QImage->height() - 1);
  }
  if (HeightAdjusted) {
    CenterBak = m_PipeSizeRect->center();
    m_PipeSizeRect->setWidth((int)(m_PipeSizeRect->height() * NewAR));
    m_PipeSizeRect->moveCenter(CenterBak);
  }

  if (m_FixedAspectRatio) {
    setAspectRatio(m_FixedAspectRatio, m_AspectRatioH, m_AspectRatioW, 0);
  }

  viewport()->repaint();
}


void ptViewWindow::CenterCropRectHor() {
  if (m_InteractionMode == vaCrop) {
    int center = (int)(m_QImage->width() / 2);
    if (m_PipeSizeRect->center().x() != center) {
      m_PipeSizeRect->moveCenter(QPoint(center, m_PipeSizeRect->center().y()));
      viewport()->repaint();
    }
  }
}


void ptViewWindow::CenterCropRectVert() {
  if (m_InteractionMode == vaCrop) {
    int center = (int)(m_QImage->height() / 2);
    if (m_PipeSizeRect->center().y() != center) {
      m_PipeSizeRect->moveCenter(QPoint(m_PipeSizeRect->center().x(), center));
      viewport()->repaint();
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// LightsOut
//
////////////////////////////////////////////////////////////////////////////////

void CB_LightsOutChoice(const QVariant Choice);

void ptViewWindow::ToggleLightsOut() {
  m_CropLightsOut = (m_CropLightsOut+1)%3;
  ::CB_LightsOutChoice(m_CropLightsOut);
}


////////////////////////////////////////////////////////////////////////////////
//
// Grid
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::setGrid(const short Enabled, const short GridX, const short GridY) {
  if (Enabled) {
    m_HasGrid = 1;
    m_GridX = GridX;
    m_GridY = GridY;
  } else {
    m_HasGrid = 0;
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// ZoomFit/Zoom
//
////////////////////////////////////////////////////////////////////////////////

short ptViewWindow::ZoomFitFactor(const uint16_t Width, const uint16_t Height) {
  float Factor1 =(float)
    (viewport()->size().width())/Width;
  float Factor2 =(float)
    (viewport()->size().height())/Height;
  m_ZoomFactor = MIN(Factor1,Factor2);
  return (short)(m_ZoomFactor*100.0f+0.5f);
}

short ptViewWindow::ZoomFit() {
  // Startup condition.
  if (!m_RelatedImage) return 100;
  // Normal condition.
  m_SizeReport->setVisible(0);
  UpdateView();
  return ZoomFitFactor(m_RelatedImage->m_Width,m_RelatedImage->m_Height);
}

void ptViewWindow::Zoom(const short Factor, const short Update) {
  m_ZoomFactor = Factor/100.0;
  Settings->SetValue("Zoom",Factor); // for convenience
  Settings->SetValue("ZoomMode",ptZoomMode_NonFit);
  if (Update == 1) {
    m_SizeReportTimer->start(m_SizeReportTimeOut);
    m_SizeReportText = QString::number(Factor);
    m_SizeReport->setGeometry(width()-170,20,150,70);
    m_SizeReport->setText("<h1>"+m_SizeReportText+"%</h1>");
    m_SizeReport->update();
    m_SizeReport->setVisible(1);
    UpdateView();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateView.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::UpdateView(const ptImage* NewRelatedImage) {

  if (NewRelatedImage) {
    m_RelatedImage = NewRelatedImage;
  }
  if (!m_RelatedImage) {
    return;
  }

  if (Settings->GetInt("ZoomMode")==ptZoomMode_Fit) {
    Settings->SetValue("Zoom",ZoomFitFactor(m_RelatedImage->m_Width,m_RelatedImage->m_Height));
  }

  // Convert the ptImage to a QImage. Mind R<->B and 16->8
  if (NewRelatedImage) {
    delete m_QImage;
    m_QImage = new QImage(m_RelatedImage->m_Width,
                          m_RelatedImage->m_Height,
                          QImage::Format_RGB32);
    uint32_t Size = m_RelatedImage->m_Height * m_RelatedImage->m_Width;
    uint32_t* ImagePtr = (QRgb*) m_QImage->scanLine(0);
#pragma omp parallel for schedule(static)
    for (uint32_t i = 0; i < Size; i++) {
      uint8_t* Pixel = (uint8_t*) ImagePtr + (i<<2);
      for (short c=0; c<3; c++) {
        // Mind the R<->B swap !
        Pixel[2-c] = m_RelatedImage->m_Image[i][c]>>8;
      }
      Pixel[3] = 0xff;
    }
  }

  // Size of zoomed image.
  m_ZoomWidth  = (uint16_t)(m_RelatedImage->m_Width * m_ZoomFactor + 0.5);
  m_ZoomHeight = (uint16_t)(m_RelatedImage->m_Height * m_ZoomFactor + 0.5);

  // Size of viewport.
  uint16_t VP_Width  = viewport()->size().width();
  uint16_t VP_Height = viewport()->size().height();

  // Set the scrollBars acccordingly.
  verticalScrollBar()->setPageStep(VP_Height);
  verticalScrollBar()->setRange(0,m_ZoomHeight-VP_Height);
  horizontalScrollBar()->setPageStep(VP_Width);
  horizontalScrollBar()->setRange(0,m_ZoomWidth-VP_Width);

  // This correspond to the current (X,Y) offset of the cut image
  // versus the zoomed image.
  int32_t CurrentStartX = horizontalScrollBar()->value();
  int32_t CurrentStartY = verticalScrollBar()->value();

  if (NewRelatedImage || m_PreviousZoomFactor != m_ZoomFactor) {

    // Recalculate CurrentStartX/Y such that centrum stays centrum.
    CurrentStartX = (int32_t)
      ( (CurrentStartX+VP_Width/2)*m_ZoomFactor/m_PreviousZoomFactor -
        VP_Width/2 );
    CurrentStartY = (int32_t)
      ( (CurrentStartY+VP_Height/2)*m_ZoomFactor/m_PreviousZoomFactor -
        VP_Height/2 );
    CurrentStartX = MAX(0,CurrentStartX);
    CurrentStartY = MAX(0,CurrentStartY);

    m_PreviousZoomFactor = m_ZoomFactor;

    // Recalculate the zoomed image.
    delete m_QImageZoomed;
    m_QImageZoomed = new QImage(m_QImage->scaled(m_ZoomWidth,
                                                 m_ZoomHeight,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation));

  }

  // Maybe move scrollbars such that centrum stays centrum during zoom.
  // block signals as to avoid this programmatic update generates events.
  // TODO Correct way of doing things ?

  horizontalScrollBar()->blockSignals(1);
  horizontalScrollBar()->setValue(CurrentStartX);
  horizontalScrollBar()->blockSignals(0);

  verticalScrollBar()->blockSignals(1);
  verticalScrollBar()->setValue(CurrentStartY);
  verticalScrollBar()->blockSignals(0);

  // Recalculate the image cut out of m_QImageZoomed.
  RecalcCut();

  // Update view.
  viewport()->update();
}

////////////////////////////////////////////////////////////////////////////////
//
// RecalcCut()
//
// Recalculates m_QImageCut when the selection
// has been changed, i.e. for scrollbar movements or a resizing.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::RecalcCut() {
  if (!m_QImageZoomed) {
    return;
  }

  delete m_QImageCut;
  m_QImageCut = new QImage(m_QImageZoomed->copy(
      horizontalScrollBar()->value(),
      verticalScrollBar()->value(),
      MIN(horizontalScrollBar()->pageStep(), m_QImageZoomed->width()),
      MIN(verticalScrollBar()->pageStep(), m_QImageZoomed->height())
      ));
}


////////////////////////////////////////////////////////////////////////
//
// RecalcRect
// Calc new corner point, change moving edge/corner when rect is
// dragged into another "quadrant", and update rectangle.
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::RecalcRect() {
  QRect NewPos;
  int ImageRight = m_QImage->width() - 1;
  int ImageBottom = m_QImage->height() - 1;
  int dx = (int)(m_DragDelta->dx() / m_ZoomFactor);
  int dy = (int)(m_DragDelta->dy() / m_ZoomFactor);
  int dxCeil = dx;
  int dyCeil = dy;
  int dxFloor = (int)(m_DragDelta->dx() / m_ZoomFactor);
  int dyFloor = (int)(m_DragDelta->dy() / m_ZoomFactor);

  switch (m_MovingEdge) {
    case meTopLeft:
      // Calc preliminary new position of moved corner point. Don't allow it to move
      // beyond the actual image.
      NewPos.setX(qBound(0, m_PipeSizeRect->left() + dxFloor, ImageRight));
      NewPos.setY(qBound(0, m_PipeSizeRect->top() + dyFloor, ImageBottom));
      // Determine if moving corner changed
      m_PrevMovingEdge = m_MovingEdge;
      if ((NewPos.x() > m_PipeSizeRect->right()) && (NewPos.y() <= m_PipeSizeRect->bottom())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() > m_PipeSizeRect->right()) && (NewPos.y() >= m_PipeSizeRect->bottom())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() <= m_PipeSizeRect->right()) && (NewPos.y() > m_PipeSizeRect->bottom())) {
        m_MovingEdge = meBottomLeft;
      }
      // Update crop rect
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->right()),
                                MIN(NewPos.y(), m_PipeSizeRect->bottom()),
                                MAX(NewPos.x(), m_PipeSizeRect->right()),
                                MAX(NewPos.y(), m_PipeSizeRect->bottom()) );
      break;


    case meTop:
      dx = 0;
      NewPos.setY(qBound(0, m_PipeSizeRect->top() + dyFloor, ImageBottom));
      m_PrevMovingEdge = m_MovingEdge;
      if (NewPos.y() > m_PipeSizeRect->bottom()) {
        m_MovingEdge = meBottom;
      }
      m_PipeSizeRect->setCoords(m_PipeSizeRect->left(),
                                MIN(NewPos.y(), m_PipeSizeRect->bottom()),
                                m_PipeSizeRect->right(),
                                MAX(NewPos.y(), m_PipeSizeRect->bottom()) );
      break;

    case meTopRight:
      NewPos.setX(qBound(0, m_PipeSizeRect->right() + dxCeil, ImageRight));
      NewPos.setY(qBound(0, m_PipeSizeRect->top() + dyFloor, ImageBottom));
      m_PrevMovingEdge = m_MovingEdge;
      if ((NewPos.x() < m_PipeSizeRect->left()) && (NewPos.y() <= m_PipeSizeRect->bottom())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() < m_PipeSizeRect->left()) && (NewPos.y() > m_PipeSizeRect->bottom())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() >= m_PipeSizeRect->left()) && (NewPos.y() > m_PipeSizeRect->bottom())) {
        m_MovingEdge = meBottomRight;
      }
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->left()),
                                MIN(NewPos.y(), m_PipeSizeRect->bottom()),
                                MAX(NewPos.x(), m_PipeSizeRect->left()),
                                MAX(NewPos.y(), m_PipeSizeRect->bottom()) );
      break;

    case meRight:
      dy = 0;
      m_PrevMovingEdge = m_MovingEdge;
      NewPos.setX(qBound(0, m_PipeSizeRect->right() + dxCeil, ImageRight));
      if ((NewPos.x() < m_PipeSizeRect->left())) {
        m_MovingEdge = meLeft;
      }
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->left()),
                                m_PipeSizeRect->top(),
                                MAX(NewPos.x(), m_PipeSizeRect->left()),
                                m_PipeSizeRect->bottom() );
      break;

    case meBottomRight:
      NewPos.setX(qBound(0, m_PipeSizeRect->right() + dxCeil, ImageRight));
      NewPos.setY(qBound(0, m_PipeSizeRect->bottom() + dyCeil, ImageBottom));
      m_PrevMovingEdge = m_MovingEdge;
      if ((NewPos.x() < m_PipeSizeRect->left()) && (NewPos.y() >= m_PipeSizeRect->top())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() < m_PipeSizeRect->left()) && (NewPos.y() < m_PipeSizeRect->top())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() >= m_PipeSizeRect->left()) && (NewPos.y() < m_PipeSizeRect->top())) {
        m_MovingEdge = meTopRight;
      }
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->left()),
                                MIN(NewPos.y(), m_PipeSizeRect->top()),
                                MAX(NewPos.x(), m_PipeSizeRect->left()),
                                MAX(NewPos.y(), m_PipeSizeRect->top()) );
      break;

    case meBottom:
      dx = 0;
      NewPos.setY(qBound(0, m_PipeSizeRect->bottom() + dy, ImageBottom));
      m_PrevMovingEdge = m_MovingEdge;
      if (NewPos.y() < m_PipeSizeRect->top()) {
        m_MovingEdge = meTop;
      }
      m_PipeSizeRect->setCoords(m_PipeSizeRect->left(),
                                MIN(NewPos.y(), m_PipeSizeRect->top()),
                                m_PipeSizeRect->right(),
                                MAX(NewPos.y(), m_PipeSizeRect->top()) );
      break;

    case meBottomLeft:
      NewPos.setX(qBound(0, m_PipeSizeRect->left() + dx, ImageRight));
      NewPos.setY(qBound(0, m_PipeSizeRect->bottom() + dy, ImageBottom));
      m_PrevMovingEdge = m_MovingEdge;
      if ((NewPos.x() > m_PipeSizeRect->right()) && (NewPos.y() >= m_PipeSizeRect->top())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() > m_PipeSizeRect->right()) && (NewPos.y() < m_PipeSizeRect->top())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() <= m_PipeSizeRect->right()) && (NewPos.y() < m_PipeSizeRect->top())) {
        m_MovingEdge = meTopLeft;
      }
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->right()),
                                MIN(NewPos.y(), m_PipeSizeRect->top()),
                                MAX(NewPos.x(), m_PipeSizeRect->right()),
                                MAX(NewPos.y(), m_PipeSizeRect->top()) );
      break;

    case meLeft:
      dy = 0;
      NewPos.setX(qBound(0, m_PipeSizeRect->left() + dx, ImageRight));
      m_PrevMovingEdge = m_MovingEdge;
      if (NewPos.x() > m_PipeSizeRect->right()) {
        m_MovingEdge = meRight;
      }
      m_PipeSizeRect->setCoords(MIN(NewPos.x(), m_PipeSizeRect->right()),
                                m_PipeSizeRect->top(),
                                MAX(NewPos.x(), m_PipeSizeRect->right()),
                                m_PipeSizeRect->bottom() );
      break;


    case meNone:
      break;

    default:
      assert(0);
      break;
  }

  if (m_FixedAspectRatio) {
    EnforceRectAspectRatio(dx, dy);
  }
}


////////////////////////////////////////////////////////////////////////
//
// UpdateViewportRect()
// Transform pipe sized crop rect to viewport scale rect and coordinates.
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::UpdateViewportRects() {
  if (m_QImageCut == NULL) {
    return;
  }

  m_ViewSizeRect->moveTo(m_ViewSizeRect->left() - m_ImageFrame->left(),
                         m_ViewSizeRect->top() - m_ImageFrame->top() );

  // Calc position/size of the image frame in viewport
  int VPWidth  = viewport()->size().width();
  int VPHeight = viewport()->size().height();

  if (VPHeight > m_QImageCut->height()) {
    m_ImageFrame->setTop((VPHeight - m_QImageCut->height()) / 2);
  } else {
    m_ImageFrame->setTop(0);
  }
  if (VPWidth > m_QImageCut->width()) {
    m_ImageFrame->setLeft((VPWidth - m_QImageCut->width()) / 2);
  } else {
    m_ImageFrame->setLeft(0);
  }

  m_ImageFrame->setWidth(m_QImageCut->width());
  m_ImageFrame->setHeight(m_QImageCut->height());


  // Make sure opposite edge/corner stays where it is. Without doing this explicitely
  // egde/corner tends to jump back and forth one pixel.
  int ScaledX1 = m_ImageFrame->left() + (int)(m_PipeSizeRect->left() * m_ZoomFactor);
  int ScaledY1 = m_ImageFrame->top() + (int)(m_PipeSizeRect->top() * m_ZoomFactor);
  int ScaledW = (int)(m_PipeSizeRect->width() * m_ZoomFactor);
  int ScaledH = (int)(m_PipeSizeRect->height() * m_ZoomFactor);

  if ((m_PipeSizeRect->left() == -1) && (m_PipeSizeRect->top() == -1)) {
    m_ViewSizeRect->setRect(-1,-1,0,0);

  } else {
    int OldRight = m_ViewSizeRect->right() + m_ImageFrame->left();
    int OldBottom = m_ViewSizeRect->bottom() + m_ImageFrame->top();
    m_ViewSizeRect->setRect(ScaledX1, ScaledY1, ScaledW, ScaledH);

    if (m_MovingEdge == m_PrevMovingEdge) {
      switch (m_MovingEdge) {
        case meTopLeft:
          m_ViewSizeRect->moveRight(OldRight);
          m_ViewSizeRect->moveBottom(OldBottom);
          if (m_ViewSizeRect->left() < m_ImageFrame->left())
            m_ViewSizeRect->moveLeft(m_ImageFrame->left());
          if (m_ViewSizeRect->top() < m_ImageFrame->top())
            m_ViewSizeRect->moveTop(m_ImageFrame->top());
          break;

        case meTop:
        case meTopRight:
          m_ViewSizeRect->moveBottom(OldBottom);
          if (m_ViewSizeRect->top() < m_ImageFrame->top())
            m_ViewSizeRect->moveTop(m_ImageFrame->top());
          break;

        case meBottomLeft:
        case meLeft:
          m_ViewSizeRect->moveRight(OldRight);
          if (m_ViewSizeRect->left() < m_ImageFrame->left())
            m_ViewSizeRect->moveLeft(m_ImageFrame->left());
          break;

        default:
          break;
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////
//
// EnforceRectAspectRatio
//
// Make sure rectangle has the proper AR. Wenn adjusted the opposite
// corner/edge to the one that was moved remains fixed.
// When there was no movement (e.g. changed AR values in MainWindow)
// the center of the rectangle remains fixed.
//
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::EnforceRectAspectRatio(int dx, int dy) {
  dx = qAbs(dx);
  dy = qAbs(dy);
  int ImageRight = m_QImage->width() - 1;
  int ImageBottom = m_QImage->height() - 1;
  int NewWidth = qRound(m_PipeSizeRect->height() * m_AspectRatio);
  int NewHeight = qRound(m_PipeSizeRect->width() / m_AspectRatio);
  int EdgeCenter = 0;

  switch (m_MovingEdge){
    case meTopLeft:
      if (dx > dy) {  // primarily horizontal mouse movement: new width takes precedence
        m_PipeSizeRect->setTop(m_PipeSizeRect->top() + m_PipeSizeRect->height() - NewHeight);
        if (m_PipeSizeRect->top() < 0) {
          m_PipeSizeRect->setTop(0);
          m_PipeSizeRect->setLeft(m_PipeSizeRect->right() - qRound(m_PipeSizeRect->height() * m_AspectRatio));
        }
      } else {  // primarily vertical mouse movement: new height takes precedence
        m_PipeSizeRect->setLeft(m_PipeSizeRect->left() + m_PipeSizeRect->width() - NewWidth);
        if (m_PipeSizeRect->left() < 0) {
          m_PipeSizeRect->setLeft(0);
          m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
        }
      }
      break;

    case meTop:
      EdgeCenter = m_PipeSizeRect->left() + qRound(m_PipeSizeRect->width() / 2);
      m_PipeSizeRect->setWidth(NewWidth);
      m_PipeSizeRect->moveLeft(EdgeCenter - qRound(NewWidth / 2));
      if (m_PipeSizeRect->right() > ImageRight) {
        m_PipeSizeRect->setRight(ImageRight);
        m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
      }
      if (m_PipeSizeRect->left() < 0) {
        m_PipeSizeRect->setLeft(0);
        m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
      }
      break;

    case meTopRight:
      if (dx > dy) {
        m_PipeSizeRect->setTop(m_PipeSizeRect->top() + m_PipeSizeRect->height() - NewHeight);
        if (m_PipeSizeRect->top() < 0) {
          m_PipeSizeRect->setTop(0);
          m_PipeSizeRect->setRight(m_PipeSizeRect->left() + qRound(m_PipeSizeRect->height() * m_AspectRatio));
        }
      } else {
        m_PipeSizeRect->setWidth(NewWidth);
        if (m_PipeSizeRect->right() > ImageRight) {
          m_PipeSizeRect->setRight(ImageRight);
          m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
        }
      }
      break;

    case meRight:
      EdgeCenter = m_PipeSizeRect->top() + qRound(m_PipeSizeRect->height() / 2);
      m_PipeSizeRect->setHeight(NewHeight);
      m_PipeSizeRect->moveTop(EdgeCenter - qRound(NewHeight / 2));
      if (m_PipeSizeRect->bottom() > ImageBottom) {
        m_PipeSizeRect->setBottom(ImageBottom);
        m_PipeSizeRect->setRight(m_PipeSizeRect->left() + qRound(m_PipeSizeRect->height() * m_AspectRatio));
      }
      if (m_PipeSizeRect->top() < 0) {
        m_PipeSizeRect->setTop(0);
        m_PipeSizeRect->setRight(m_PipeSizeRect->left() + qRound(m_PipeSizeRect->height() * m_AspectRatio));
      }
      break;

    case meBottomRight:
      if (dx > dy) {
        m_PipeSizeRect->setBottom(m_PipeSizeRect->bottom() + NewHeight - m_PipeSizeRect->height());
        if (m_PipeSizeRect->bottom() > ImageBottom) {
          m_PipeSizeRect->setBottom(ImageBottom);
          m_PipeSizeRect->setRight(m_PipeSizeRect->left() + qRound(m_PipeSizeRect->height() * m_AspectRatio));
        }
      } else {
        m_PipeSizeRect->setWidth(NewWidth);
        if (m_PipeSizeRect->right() > ImageRight) {
          m_PipeSizeRect->setRight(ImageRight);
          m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
        }
      }
      break;

    case meBottom:
      EdgeCenter = m_PipeSizeRect->left() + qRound(m_PipeSizeRect->width() / 2);
      m_PipeSizeRect->setWidth(NewWidth);
      m_PipeSizeRect->moveLeft(EdgeCenter - qRound(NewWidth / 2));
      if (m_PipeSizeRect->right() > ImageRight) {
        m_PipeSizeRect->setRight(ImageRight);
        m_PipeSizeRect->setBottom(m_PipeSizeRect->top() + qRound(m_PipeSizeRect->width() / m_AspectRatio));
      }
      if (m_PipeSizeRect->left() < 0) {
        m_PipeSizeRect->setLeft(0);
        m_PipeSizeRect->setBottom(m_PipeSizeRect->top() + qRound(m_PipeSizeRect->width() / m_AspectRatio));
      }
      break;

    case meBottomLeft:
      if (dx > dy) {
        m_PipeSizeRect->setBottom(m_PipeSizeRect->bottom() + NewHeight - m_PipeSizeRect->height());
        if (m_PipeSizeRect->bottom() > ImageBottom) {
          m_PipeSizeRect->setBottom(ImageBottom);
          m_PipeSizeRect->setLeft(m_PipeSizeRect->right() - qRound(m_PipeSizeRect->height() * m_AspectRatio));
        }
      } else {
        m_PipeSizeRect->setLeft(m_PipeSizeRect->left() + m_PipeSizeRect->width() - NewWidth);
        if (m_PipeSizeRect->left() < 0) {
          m_PipeSizeRect->setLeft(0);
          m_PipeSizeRect->setTop(m_PipeSizeRect->bottom() - qRound(m_PipeSizeRect->width() / m_AspectRatio));
        }
      }
      break;

    case meLeft:
      EdgeCenter = m_PipeSizeRect->top() + qRound(m_PipeSizeRect->height() / 2);
      m_PipeSizeRect->setHeight(NewHeight);
      m_PipeSizeRect->moveTop(EdgeCenter - qRound(NewHeight / 2));
      if (m_PipeSizeRect->bottom() > ImageBottom) {
        m_PipeSizeRect->setBottom(ImageBottom);
        m_PipeSizeRect->setLeft(m_PipeSizeRect->right() - qRound(m_PipeSizeRect->height() * m_AspectRatio));
      }
      if (m_PipeSizeRect->top() < 0) {
        m_PipeSizeRect->setTop(0);
        m_PipeSizeRect->setLeft(m_PipeSizeRect->right() - qRound(m_PipeSizeRect->height() * m_AspectRatio));
      }
      break;

    case meNone: {
      QPoint center = m_PipeSizeRect->center();
      m_PipeSizeRect->setWidth(NewWidth);
      m_PipeSizeRect->setHeight(qRound(m_PipeSizeRect->width() / m_AspectRatio));
      m_PipeSizeRect->moveCenter(center);

      if (m_PipeSizeRect->left() < 0) {
        m_PipeSizeRect->moveLeft(0);
      }
      if (m_PipeSizeRect->top() < 0) {
        m_PipeSizeRect->moveTop(0);
      }
      if (m_PipeSizeRect->right() >= m_QImage->width()) {
        m_PipeSizeRect->setRight(m_QImage->width() - 1);
        m_PipeSizeRect->setHeight(qRound(m_PipeSizeRect->width() / m_AspectRatio));
      }
      if (m_PipeSizeRect->bottom() >= m_QImage->height()) {
        double ShrinkFactor = (m_QImage->height() - m_PipeSizeRect->top()) / m_QImage->height();
        m_PipeSizeRect->setBottom(m_QImage->height() - 1);
        m_PipeSizeRect->setWidth((int)(m_PipeSizeRect->width() * ShrinkFactor));
      }
      break;
    }

    default:
      assert(0);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// scrollContentsBy()
//
// Overloaded virtual from QAbstractScrollArea to handle scrolling.
// Only recalculates the image cut in our case.
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::scrollContentsBy(int,int) {
  // Make a new cut out of our zoomed image.
  RecalcCut();
  // And update the view.
  viewport()->repaint();
}


////////////////////////////////////////////////////////////////////////////////
//
// paintEvent
//
// Overloaded virtual from QAbstractScrollArea.
// Handles all painting into the viewport:
// - window background (theme or user defined colour
// - (visible part of the) image
// - overlays: crop rect incl. guidelines, lights out, grid, rotate line
//
////////////////////////////////////////////////////////////////////////////////

void CB_MenuFileOpen(const short HaveFile);
void CB_OpenSettingsFile(QString SettingsFileName);

void ptViewWindow::paintEvent(QPaintEvent*) {
  UpdateViewportRects();
  short PaintRect = ((m_PipeSizeRect->left() > -1) && (m_PipeSizeRect->top() > -1));

  // Fill viewport with background colour and draw image
  QPainter Painter(viewport());
  Painter.save();
  Painter.fillRect(0, 0, viewport()->size().width(), viewport()->size().height(),
                   palette().color(QPalette::Window));
  if (m_QImageCut) {
    Painter.drawImage(m_ImageFrame->left(), m_ImageFrame->top(), *m_QImageCut);
  }


  // Grid overlay
  if (m_HasGrid) {
    QPen Pen(QColor(150, 150, 150),1);
    Painter.setPen(Pen);
    uint16_t XNrLines = m_GridX;
    uint16_t XStep = (int) ((double)m_QImageCut->width() / (double) (XNrLines+1));
    uint16_t YNrLines = m_GridY;
    uint16_t YStep = (int) ((double)m_QImageCut->height() / (double) (YNrLines+1));
    if (XNrLines) {
      for (int i = 1; i <= XNrLines; i++) { //vertical lines
        Painter.drawLine(m_ImageFrame->left()+i*XStep,
             m_ImageFrame->top(),
             m_ImageFrame->left()+i*XStep,
             m_ImageFrame->top()+m_QImageCut->height()-1);
      }
    }
    if (YNrLines) {
      for (int i = 1; i <= YNrLines; i++) { //horizontal lines
        Painter.drawLine(m_ImageFrame->left(),
             m_ImageFrame->top()+i*YStep,
             m_ImageFrame->left()+m_QImageCut->width()-1,
             m_ImageFrame->top()+i*YStep);
      }
    }
  }


  switch (m_InteractionMode) {
    // Draw rectangle for crop/selection tools
    case vaSelectRect:
    case vaCrop: {
      // Lights out: paint area outside the crop rectangle for
      // lights dimmed and lights off modes
      QBrush LightsOutBrush(QColor(20, 20, 20, 200));
      if (m_CropLightsOut == 2) {   // lights off
        if (Settings->GetInt("BackgroundColor")) {
          LightsOutBrush.setColor(QColor(Settings->GetInt("BackgroundRed"),
                                Settings->GetInt("BackgroundGreen"),
                                Settings->GetInt("BackgroundBlue")));
        } else {
          LightsOutBrush.setColor(Theme->ptBackGround);
        }
      }

      // Paint outside areas for lights dimmed/black.
      // Up to four rectangles are drawn according to the following figure.
      // tttttttttttttttttt
      // lll   crop     rrr
      // lll rectangle  rrr
      // bbbbbbbbbbbbbbbbbb
      if ((m_CropLightsOut > 0) && PaintRect) {
        if (m_ViewSizeRect->top() > m_ImageFrame->top()) { // Top
          Painter.fillRect(m_ImageFrame->left(), m_ImageFrame->top(),
                           m_ImageFrame->width(), m_ViewSizeRect->top() - m_ImageFrame->top(),
                           LightsOutBrush);
        }
        if (m_ViewSizeRect->bottom() < m_ImageFrame->bottom()) { // Bottom
          Painter.fillRect(m_ImageFrame->left(), m_ViewSizeRect->bottom() + 1,
                           m_ImageFrame->width(), m_ImageFrame->bottom() - m_ViewSizeRect->bottom(),
                           LightsOutBrush);
        }
        if (m_ViewSizeRect->left() > m_ImageFrame->left()) {   // left
          Painter.fillRect(m_ImageFrame->left(), m_ViewSizeRect->top(),
                           m_ViewSizeRect->left() - m_ImageFrame->left(), m_ViewSizeRect->height(),
                           LightsOutBrush);
        }
        if (m_ViewSizeRect->right() < m_ImageFrame->right()) {   // right
          Painter.fillRect(m_ViewSizeRect->right() + 1, m_ViewSizeRect->top(),
                           m_ImageFrame->right() - m_ViewSizeRect->right(), m_ViewSizeRect->height(),
                           LightsOutBrush);
        }
      }


      // Draw outline/guidelines for crop rectangle (not for "lights off")
      if ((m_CropLightsOut != 2) && PaintRect) {
        QPen Pen(QColor(150, 150, 150),1);
        Painter.setPen(Pen);
        Painter.drawRect(*m_ViewSizeRect);

        switch (m_CropGuidelines) {
          case ptCropGuidelines_RuleThirds: {
            int HeightThird = (int)(m_ViewSizeRect->height() / 3);
            int WidthThird = (int)(m_ViewSizeRect->width() / 3);
            Painter.drawRect(m_ViewSizeRect->left() + WidthThird, m_ViewSizeRect->top(),
                             WidthThird, m_ViewSizeRect->height());
            Painter.drawRect(m_ViewSizeRect->left(), m_ViewSizeRect->top() + HeightThird,
                             m_ViewSizeRect->width(), HeightThird);
            break;
          }

          case ptCropGuidelines_GoldenRatio: {
            int ShortWidth = (int)(m_ViewSizeRect->width() * 5/13);
            int ShortHeight = (int)(m_ViewSizeRect->height() * 5/13);
            Painter.drawRect(m_ViewSizeRect->left() + ShortWidth, m_ViewSizeRect->top(),
                             m_ViewSizeRect->width() - (2 * ShortWidth), m_ViewSizeRect->height());

            Painter.drawRect(m_ViewSizeRect->left(), m_ViewSizeRect->top() + ShortHeight,
                             m_ViewSizeRect->width(), m_ViewSizeRect->height() - (2 * ShortHeight));
            break;
          }

          case ptCropGuidelines_Diagonals: {
            int length = m_ViewSizeRect->width() > m_ViewSizeRect->height() ? m_ViewSizeRect->height() : m_ViewSizeRect->width();
            Painter.drawLine(m_ViewSizeRect->left(), m_ViewSizeRect->top(),
                             m_ViewSizeRect->left() + length, m_ViewSizeRect->top() + length);
            Painter.drawLine(m_ViewSizeRect->right(), m_ViewSizeRect->bottom(),
                             m_ViewSizeRect->right() - length + 1, m_ViewSizeRect->bottom() - length + 1);
            Painter.drawLine(m_ViewSizeRect->left(), m_ViewSizeRect->bottom(),
                             m_ViewSizeRect->left() + length, m_ViewSizeRect->bottom() - length + 1);
            Painter.drawLine(m_ViewSizeRect->right(), m_ViewSizeRect->top(),
                             m_ViewSizeRect->right() - length + 1, m_ViewSizeRect->top() + length);
            break;
          }

          case ptCropGuidelines_Centerlines: {
            Painter.drawLine(m_ViewSizeRect->center().x(), m_ViewSizeRect->top(),
                             m_ViewSizeRect->center().x(), m_ViewSizeRect->bottom());
            Painter.drawLine(m_ViewSizeRect->left(), m_ViewSizeRect->center().y(),
                             m_ViewSizeRect->right(), m_ViewSizeRect->center().y());
          }

          default:
            break;
        }
      }

      break;
    }


    // draw angle line for the rotate tool
    case vaDrawLine: {
      QPen Pen(QColor(255, 0, 0),1);
      Painter.setPen(Pen);
      Painter.drawLine(*m_DragDelta);
    }

    default:
      break;
  }

  Painter.restore();
}


////////////////////////////////////////////////////////////////////////
//
// resizeEvent
//
////////////////////////////////////////////////////////////////////////

// ResizeEvent : Make a new cut on the appropriate place.
void ptViewWindow::resizeEvent(QResizeEvent*) {
  int VPWidth  = viewport()->size().width();
  int VPHeight = viewport()->size().height();

  // Adapt scrollbars to new viewport size
  verticalScrollBar()->setPageStep(VPHeight);
  verticalScrollBar()->setRange(0, m_ZoomHeight - VPHeight);
  horizontalScrollBar()->setPageStep(VPWidth);
  horizontalScrollBar()->setRange(0, m_ZoomWidth - VPWidth);

  RecalcCut();

  if ((m_InteractionMode == vaCrop) || (Settings->GetInt("ZoomMode") == ptZoomMode_Fit)) {
    ::CB_ZoomFitButton();
  }
}


////////////////////////////////////////////////////////////////////////
//
// MouseDragPos
//
// Returns the area of the crop/selection rectangle the mouse cursor hovers over.
// The mouse position inside the crop rectangle determines which action is performed
// on drag. There are nine areas: 55511111111666
//                                444        222
//                                77733333333888
// - Dragging the corners changes both adjacent edges.
// - Dragging the edges changes only that edge. Dragging in the middle area moves
//   the rectangle without changing its size.
// - Dragging beyond actual image borders is not possible.
// - Edge areas are usually EdgeThickness pixels thick. On hovering mouse cursor
//   changes shape to indicate the move/resize mode.
// - For rectangle edges of TinyRectThreshold pixels or shorter only the corner modes
//   apply, one for each half of the edge. This avoids too tiny interaction areas.
// - Because of the press/drag/release nature of the simple selection, only the
//   corner areas are relevant there.
//
////////////////////////////////////////////////////////////////////////

ptMovingEdge ptViewWindow::MouseDragPos(QMouseEvent* Event) {
  // Catch mouse outside current crop rect
  if (!m_ViewSizeRect->contains(Event->pos())) {
    return meNone;
  }

  ptMovingEdge HoverOver = meNone;
  int TBthick = 0;
  int LRthick = 0;

  // Determine edge area thickness
  if (m_ViewSizeRect->height() <= TinyRectThreshold) {
    TBthick = (int)(m_ViewSizeRect->height() / 2);
  } else {
    TBthick = EdgeThickness;
  }
  if (m_ViewSizeRect->width() <= TinyRectThreshold) {
    LRthick = (int)(m_ViewSizeRect->width() / 2);
  } else {
    LRthick = EdgeThickness;
  }

  // Determine in which area the mouse is
  if (m_ViewSizeRect->bottom() - Event->y() <= TBthick) {
    HoverOver = meBottom;
  } else if (Event->y() - m_ViewSizeRect->top() <= TBthick) {
    HoverOver = meTop;
  } else {
    HoverOver = meCenter;
  }

  if (m_ViewSizeRect->right() - Event->x() <= LRthick) {
    if (HoverOver == meBottom) {
      HoverOver = meBottomRight;
    } else if (HoverOver == meTop) {
      HoverOver = meTopRight;
    } else {
      HoverOver = meRight;
    }

  } else if (Event->x() - m_ViewSizeRect->left() <= LRthick) {
    if (HoverOver == meBottom) {
      HoverOver = meBottomLeft;
    } else if (HoverOver == meTop) {
      HoverOver = meTopLeft;
    } else {
      HoverOver = meLeft;
    }
  }

  return HoverOver;
}


////////////////////////////////////////////////////////////////////////
//
// mousePressEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::mousePressEvent(QMouseEvent* Event) {
  // left click actions
  if (Event->button() == Qt::LeftButton) {
    switch (m_InteractionMode) {
      case vaSelectRect:  // Start a simple selection rectangle
      case vaCrop:        // Start/modify a crop rectangle.
        if (m_ImageFrame->contains(Event->pos())) {
          m_NowDragging = 1;
          m_DragDelta->setPoints(Event->pos(), Event->pos());

          // Start new rect when none is present or clicked outside current one.
          // Always the case for vaSelectRect.
          if (!m_ViewSizeRect->contains(Event->pos())) {
            m_ViewSizeRect->setRect(Event->x(), Event->y(), 0, 0);
            m_PipeSizeRect->setRect(
                  (int)((Event->x() - m_ImageFrame->left()) / m_ZoomFactor),
                  (int)((Event->y() - m_ImageFrame->top()) / m_ZoomFactor),
                  0, 0);
            m_MovingEdge = meNone;
            viewport()->repaint();
          }
        }
        break;

      // scroll the image
      case vaNone:
        if (m_ImageFrame->contains(Event->pos())) {
          m_NowDragging = 1;
          m_DragDelta->setPoints(Event->pos(), Event->pos());
        }
        break;

      // Start a rotation line. May be started outside the actual image display frame
      case vaDrawLine:
        m_NowDragging = 1;
        m_DragDelta->setPoints(Event->pos(), Event->pos());
        break;
    }
  }
}


////////////////////////////////////////////////////////////////////////
//
// mouseDoubleClickEvent
//
////////////////////////////////////////////////////////////////////////

void StopCrop(short CropConfirmed);

void ptViewWindow::mouseDoubleClickEvent(QMouseEvent* Event) {
  if ((Event->type() == QEvent::MouseButtonDblClick) && (m_InteractionMode == vaCrop)) {
    if (m_ViewSizeRect->contains(Event->pos())) {
      ::StopCrop(1);
    }
  }
}


////////////////////////////////////////////////////////////////////////
//
// mouseMoveEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseMoveEvent(QMouseEvent* Event) {
  // dragging rectangle or image
  if (m_NowDragging) {
    m_DragDelta->setP2(Event->pos());               // viewport scale!

    // Transform delta to pipe size scale. To avoid choppy behaviour we need to ensure the
    // final change/move of the viewport scale rect is at least 1px --> we add 0.5 towards
    // +inf or -inf depending on the sign of dx/dy.
    int dx = (int)(m_DragDelta->dx() / m_ZoomFactor + 0.5 * SIGN(m_DragDelta->dx()));
    int dy = (int)(m_DragDelta->dy() / m_ZoomFactor + 0.5 * SIGN(m_DragDelta->dy()));

    switch (m_InteractionMode) {
      case vaSelectRect:
      case vaCrop:
        // Move current rectangle. The qBounds make sure it stops at image boundaries.
        if ((m_MovingEdge == meCenter) /*|| (Event->modifiers() == Qt::ControlModifier)*/) {
          //m_MovingEdge = meCenter;
          m_PipeSizeRect->moveTo(
              qBound(0, m_PipeSizeRect->left() + dx, m_QImage->width() - m_PipeSizeRect->width()),
              qBound(0, m_PipeSizeRect->top() + dy, m_QImage->height() - m_PipeSizeRect->height())
          );
        } else {
          // initialize movement direction when rectangle was just started
          if (m_MovingEdge == meNone) {
            if ((dx >= 0) && (dy >= 0)) {
              m_MovingEdge = meBottomRight;
            } else if ((dx < 0) && (dy <= 0)) {
              m_MovingEdge = meBottomLeft;
            } else if ((dx > 0) && (dy < 0)) {
              m_MovingEdge = meTopLeft;
            } else if ((dx > 0) && (dy > 0)) {
              m_MovingEdge = meTopRight;
            }
          }

          RecalcRect();
        }
        m_DragDelta->setP1(m_DragDelta->p2());
        break;


      case vaNone: {
        int CurrentStartX = horizontalScrollBar()->value();
        int CurrentStartY = verticalScrollBar()->value();
        horizontalScrollBar()->setValue(CurrentStartX - m_DragDelta->x2() + m_DragDelta->x1());
        verticalScrollBar()->setValue(CurrentStartY - m_DragDelta->y2() + m_DragDelta->y1());
        m_DragDelta->setP1(m_DragDelta->p2());
        break;
      }


      case vaDrawLine:
        break;
    }

    viewport()->repaint();


  } else {
    // no dragging: mouse cursor might change when in image crop mode
    if (m_InteractionMode == vaCrop) {
      m_MovingEdge = MouseDragPos(Event);
      switch (m_MovingEdge) {
      case meNone:
        this->setCursor(Qt::ArrowCursor);
        break;
      case meTop:
      case meBottom:
        this->setCursor(Qt::SizeVerCursor);
        break;
      case meLeft:
      case meRight:
        this->setCursor(Qt::SizeHorCursor);
        break;
      case meTopLeft:
      case meBottomRight:
        this->setCursor(Qt::SizeFDiagCursor);
        break;
      case meTopRight:
      case meBottomLeft:
        this->setCursor(Qt::SizeBDiagCursor);
        break;
      case meCenter:
        this->setCursor(Qt::SizeAllCursor);
        break;
      default:
        assert(0);
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////
//
// leaveEvent
//
// Triggered when mouse leaves the ViewWindow. mouseMoveEvent doesn't
// catch this, so we need this function to do all necessary cleanup.
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::leaveEvent(QEvent*) {
  // Unset moving crop rectangle edge to make window resizing work properly
  // when in crop mode.
  m_PrevMovingEdge = meNone;
  m_MovingEdge = meNone;
}


////////////////////////////////////////////////////////////////////////
//
// mouseReleaseEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseReleaseEvent(QMouseEvent* Event) {
  // left click actions
  if (Event->button() == Qt::LeftButton) {
    if (m_NowDragging) {
      m_NowDragging = 0;

      switch (m_InteractionMode) {
        case vaNone:    // Scroll image: Nothing to do here.
        case vaCrop:    // Crop: Also nothing to do.
          break;

        case vaSelectRect:  // Selection: finalize rectangle and exit selection mode
        case vaDrawLine:    // Rotation line: finalize line and exit
          FinalizeAction();
          break;
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////
//
// keyPressEvent, keyReleaseEvent
//
// Handle Ctrl to move the crop rectangle.
// All other key events are forwarded to MainWindow.
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::keyPressEvent(QKeyEvent* Event) {
  if ((Event->key() == Qt::Key_Control) &&
      ((m_InteractionMode == vaCrop) || (m_InteractionMode == vaSelectRect)))
  {
    m_PrevMovingEdge = m_MovingEdge;
    m_MovingEdge = meCenter;

  } else {
    this->parent()->event(Event);
  }
}

void ptViewWindow::keyReleaseEvent(QKeyEvent* Event) {
  if ((Event->key() == Qt::Key_Control) &&
      ((m_InteractionMode == vaCrop) || (m_InteractionMode == vaSelectRect)))
  {
    m_MovingEdge = m_PrevMovingEdge;

  } else {
    this->parent()->event(Event);
  }
}


////////////////////////////////////////////////////////////////////////
//
// wheelEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::wheelEvent(QWheelEvent* Event) {
  if ((m_InteractionMode == vaNone) && (((QMouseEvent*)Event)->modifiers() == Qt::NoModifier)) {
    m_SizeReportTimer->start(m_SizeReportTimeOut);

    QList<int> Scales;
    Scales << 5 << 8 << 10 << 15 << 20 << 25 << 33 << 50 << 66 << 100 << 150 << 200 << 300 << 400;

    Settings->SetValue("ZoomMode",ptZoomMode_NonFit);
    int TempZoom = m_NewSize?m_NewSize:Settings->GetInt("Zoom");
    int Choice = 0;
    if (Event->delta() < 0) {
      for (int i=0; i<Scales.size(); i++) {
        if (Scales.at(i) < TempZoom) Choice = i;
      }
    } else {
      for (int i=0; i<Scales.size(); i++) {
        if (Scales.at(Scales.size()-1-i) > TempZoom) Choice = Scales.size()-1-i;
      }
      if (TempZoom == Scales.at(Scales.size()-1)) Choice = Scales.size()-1;
    }
    m_NewSize = Scales.at(Choice);
    m_ResizeTimer->start(m_ResizeTimeOut);
    m_SizeReportText = QString::number(m_NewSize);
    m_SizeReport->setGeometry(width()-170,20,150,70);
    m_SizeReport->setText("<h1>"+m_SizeReportText+"%</h1>");
    m_SizeReport->update();
    m_SizeReport->setVisible(1);
  }
}


////////////////////////////////////////////////////////////////////////
//
// contextMenuEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::contextMenuEvent(QContextMenuEvent* Event) {
  if (m_InteractionMode == vaNone) {    // avoid c-menu interfering with crop etc.
    ContextMenu(Event);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// Call for status report
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::StatusReport(short State) {
  if (State == 0) { // Done
    QString StatusReportStyleSheet;
    StatusReportStyleSheet = "QLabel {border: 8px solid rgb(0,130,0);"
    "border-radius: 25px; padding: 8px; color: rgb(0,130,0);"
    "background: rgb(120,170,120);}";
    m_StatusReport->setStyleSheet(StatusReportStyleSheet);
    m_StatusReport->setText("<h1> Done </h1>");
    m_StatusReportTimer->start(m_StatusReportTimeOut);
    m_StatusReport->setGeometry(20,20,150,70);
    m_StatusReport->update();
    m_StatusReport->setVisible(1);
  } else if (State == 1) { // Updating
    m_StatusReportTimer->stop();
    QString StatusReportStyleSheet;
    StatusReportStyleSheet = "QLabel {border: 8px solid rgb(255,140,0);"
    "border-radius: 25px; padding: 8px; color: rgb(255,140,0);"
    "background: rgb(255,200,120);}";
    m_StatusReport->setStyleSheet(StatusReportStyleSheet);
    m_StatusReport->setText("<h1> Updating </h1>");
    m_StatusReport->setGeometry(20,20,200,70);
    m_StatusReport->update();
    m_StatusReport->setVisible(1);
  } else if (State == 2) { // Processing
    m_StatusReportTimer->stop();
    QString StatusReportStyleSheet;
    StatusReportStyleSheet = "QLabel {border: 8px solid rgb(255,75,75);"
    "border-radius: 25px; padding: 8px; color: rgb(255,75,75);"
    "background: rgb(255,190,190);}";
    m_StatusReport->setStyleSheet(StatusReportStyleSheet);
    m_StatusReport->setText("<h1> Processing </h1>");
    m_StatusReport->setGeometry(20,20,220,70);
    m_StatusReport->update();
    m_StatusReport->setVisible(1);
  } else { // should not happen, clean up
    m_StatusReportTimer->stop();
    m_StatusReport->setVisible(0);
  }
}

void ptViewWindow::StatusReport(const QString Text) {
  QString StatusReportStyleSheet;
  StatusReportStyleSheet = "QLabel {border: 8px solid rgb(75,150,255);"
  "border-radius: 25px; padding: 8px; color: rgb(75,150,255);"
  "background: rgb(190,220,255);}";
  m_StatusReport->setStyleSheet(StatusReportStyleSheet);
  m_StatusReport->setText("<h1> "+ Text + " </h1>");
  m_StatusReportTimer->start(m_StatusReportTimeOut);
  m_StatusReport->setGeometry(20,20,90+15*Text.length(),70);
  m_StatusReport->update();
  m_StatusReport->setVisible(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// slots for the timers
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::SizeReportTimerExpired() {
  m_SizeReport->setVisible(0);
}

void ptViewWindow::StatusReportTimerExpired() {
  m_StatusReport->setVisible(0);
}

void ptViewWindow::ResizeTimerExpired() {
  if (m_NewSize != Settings->GetInt("Zoom"))
    CB_InputChanged("ZoomInput",m_NewSize);
  m_NewSize = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// generate context menu
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::ContextMenu(QEvent* Event) {
  if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_Structure)
    m_AtnModeStructure->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_Gradient)
    m_AtnModeGradient->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_L)
    m_AtnModeL->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_A)
    m_AtnModeA->setChecked(true);
  else if (Settings->GetInt("SpecialPreview")==ptSpecialPreview_B)
    m_AtnModeB->setChecked(true);
  else
    m_AtnModeRGB->setChecked(true);

  m_AtnShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  m_AtnShowBottom->setChecked(Settings->GetInt("ShowBottomContainer"));
  m_AtnExpIndicate->setChecked(Settings->GetInt("ExposureIndicator"));

  QMenu Menu(this);
  Menu.setPalette(Theme->ptMenuPalette);
  Menu.setStyle(Theme->ptStyle);
  QMenu IndicateMenu(this);
  IndicateMenu.setPalette(Theme->ptMenuPalette);
  IndicateMenu.setStyle(Theme->ptStyle);
  IndicateMenu.addAction(m_AtnExpIndicate);
  IndicateMenu.addSeparator();
  IndicateMenu.addAction(m_AtnExpIndOver);
  IndicateMenu.addAction(m_AtnExpIndUnder);
  IndicateMenu.addSeparator();
  IndicateMenu.addAction(m_AtnExpIndR);
  IndicateMenu.addAction(m_AtnExpIndG);
  IndicateMenu.addAction(m_AtnExpIndB);
  IndicateMenu.setTitle(tr("Clipping"));
  QMenu ModeMenu(this);
  ModeMenu.setPalette(Theme->ptMenuPalette);
  ModeMenu.setStyle(Theme->ptStyle);
  ModeMenu.addAction(m_AtnModeRGB);
  ModeMenu.addAction(m_AtnModeStructure);
  ModeMenu.addAction(m_AtnModeL);
  ModeMenu.addAction(m_AtnModeA);
  ModeMenu.addAction(m_AtnModeB);
  ModeMenu.addAction(m_AtnModeGradient);
  ModeMenu.setTitle(tr("Mode"));
  Menu.addAction(m_AtnZoomFit);
  Menu.addAction(m_AtnZoom100);
  Menu.addSeparator();
  Menu.addMenu(&ModeMenu);
  Menu.addMenu(&IndicateMenu);
  Menu.addSeparator();
  if (Settings->GetInt("ShowExposureIndicatorSensor")==1) {
    m_AtnExpIndSensor->setEnabled(Settings->GetInt("ShowExposureIndicatorSensor"));
    Menu.addAction(m_AtnExpIndSensor);
    Menu.addSeparator();
  }
  Menu.addAction(m_AtnShowTools);
  Menu.addAction(m_AtnShowBottom);
  Menu.addSeparator();
  Menu.addAction(m_AtnFullScreen);
  Menu.exec(((QMouseEvent*)Event)->globalPos());
}

////////////////////////////////////////////////////////////////////////////////
//
// slots for the context menu
//
////////////////////////////////////////////////////////////////////////////////

void Update(short Phase,
            short SubPhase      = -1,
            short WithIdentify  = 1,
            short ProcessorMode = ptProcessorMode_Preview);

void ptViewWindow::MenuExpIndicate() {
  Settings->SetValue("ExposureIndicator",(int)m_AtnExpIndicate->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndOver() {
  Settings->SetValue("ExposureIndicatorOver",(int)m_AtnExpIndOver->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndUnder() {
  Settings->SetValue("ExposureIndicatorUnder",(int)m_AtnExpIndUnder->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndR() {
  Settings->SetValue("ExposureIndicatorR",(int)m_AtnExpIndR->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndG() {
  Settings->SetValue("ExposureIndicatorG",(int)m_AtnExpIndG->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndB() {
  Settings->SetValue("ExposureIndicatorB",(int)m_AtnExpIndB->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuExpIndSensor() {
  Settings->SetValue("ExposureIndicatorSensor",(int)m_AtnExpIndSensor->isChecked());
  Update(ptProcessorPhase_NULL);
}

void ptViewWindow::MenuShowBottom() {
  Settings->SetValue("ShowBottomContainer",(int)m_AtnShowBottom->isChecked());
  UpdateSettings();
}

void ptViewWindow::MenuShowTools() {
  Settings->SetValue("ShowToolContainer",(int)m_AtnShowTools->isChecked());
  UpdateSettings();
}

void CB_FullScreenButton(const int State);
void ptViewWindow::MenuFullScreen() {
  CB_FullScreenButton((int)m_AtnFullScreen->isChecked());
}

void ptViewWindow::MenuZoomFit() {
  CB_ZoomFitButton();
}

void CB_ZoomFullButton();
void ptViewWindow::MenuZoom100() {
  CB_ZoomFullButton();
}

void ptViewWindow::MenuMode() {
  if (m_AtnModeRGB->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_RGB);
  else if (m_AtnModeL->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_L);
  else if (m_AtnModeA->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_A);
  else if (m_AtnModeB->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_B);
  else if (m_AtnModeGradient->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_Gradient);
  else if (m_AtnModeStructure->isChecked())
    Settings->SetValue("SpecialPreview", ptSpecialPreview_Structure);

  Update(ptProcessorPhase_NULL);
}
