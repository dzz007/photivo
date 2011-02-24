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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
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
  m_CropLightsOut    = Settings->m_IniSettings->value("CropLightsOut",0).toInt();
  m_FixedAspectRatio = 0;
  m_AspectRatio      = 0.0;
  m_AspectRatioW     = 0;
  m_AspectRatioH     = 0;

  m_InteractionMode = vaNone;
  m_Frame           = new QRect(0,0,0,0);
  m_Rect            = new QRect(0,0,0,0);
  m_RealSizeRect    = new QRect(0,0,0,0);
  m_DragDelta       = new QLine(0,0,0,0);
  m_NowDragging     = 0;
  m_MovingEdge      = meNone;

  //Avoiding tricky blacks at zoom fit.
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setAcceptDrops(true);     // Drag and drop
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
  delete m_QImageZoomed;
  delete m_QImageCut;
  delete m_Frame;
  delete m_Rect;
  delete m_RealSizeRect;
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
  assert(m_RelatedImage != NULL);

  // Catch invalid inital rect
  if ((width <= 0) || (height <= 0)) {
    m_RealSizeRect->setRect(0, 0, m_RelatedImage->m_Width, m_RelatedImage->m_Height);
  } else {
    m_RealSizeRect->setRect(x, y, width, height);
  }

  // transform to viewport scale and coordinates
  int ScaledX = (int)(m_Frame->left() + (m_RealSizeRect->left() * m_ZoomFactor + 0.5));
  int ScaledY = (int)(m_Frame->top() + (m_RealSizeRect->top() * m_ZoomFactor + 0.5));
  int ScaledW = (int)(m_RealSizeRect->width() * m_ZoomFactor + 0.5);
  int ScaledH = (int)(m_RealSizeRect->height() * m_ZoomFactor + 0.5);

  // Setup m_Rect and catch invalid values with the qBounds.
  m_Rect->setRect(qBound(m_Frame->left(), ScaledX, m_Frame->right()),
                  qBound(m_Frame->top(), ScaledY, m_Frame->bottom()),
                  qBound(0, ScaledW, m_Frame->width()),
                  qBound(0, ScaledH, m_Frame->height()) );

  m_CropGuidelines = CropGuidelines;
  m_MovingEdge = meNone;
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
  return QRect(m_RealSizeRect->topLeft(), m_RealSizeRect->bottomRight());
}

void ptViewWindow::StartSelection() {
  m_RealSizeRect->setCoords(0,0,0,0);
  m_Rect->setCoords(0,0,0,0);
  m_FixedAspectRatio = 0;
  m_CropGuidelines = ptCropGuidelines_None;
  m_MovingEdge = meNone;
  m_InteractionMode = vaSelectRect;
}

void ptViewWindow::StartLine() {
  m_InteractionMode = vaDrawLine;
}

void ptViewWindow::FinalizeAction() {
  switch (m_InteractionMode) {
    case vaSelectRect:
    case vaCrop:
      m_RealSizeRect->setRect((int)((m_Rect->left() - m_Frame->left()) / m_ZoomFactor + 0.5),
                              (int)((m_Rect->top() - m_Frame->top()) / m_ZoomFactor + 0.5),
                              (int)(m_Rect->width() / m_ZoomFactor + 0.5),
                              (int)(m_Rect->height() / m_ZoomFactor + 0.5));
      break;

    default:
      break;
  }

  m_InteractionMode = vaNone;
  m_Rect->setCoords(0,0,0,0);
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
  m_RealSizeRect->setRect((int)((m_Rect->left() - m_Frame->left()) / m_ZoomFactor + 0.5),
                          (int)((m_Rect->top() - m_Frame->top()) / m_ZoomFactor + 0.5),
                          (int)(m_Rect->width() / m_ZoomFactor + 0.5),
                          (int)(m_Rect->height() / m_ZoomFactor + 0.5) );
  return QRect(m_RealSizeRect->topLeft(), m_RealSizeRect->bottomRight());
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


////////////////////////////////////////////////////////////////////////////////
//
// LightsOut
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::ToggleLightsOut() {
  m_CropLightsOut = (m_CropLightsOut+1)%3;
  Settings->m_IniSettings->setValue("CropLightsOut",m_CropLightsOut);
  viewport()->repaint();
}


////////////////////////////////////////////////////////////////////////////////
//
// Grid
//
////////////////////////////////////////////////////////////////////////////////

void ptViewWindow::Grid(const short Enabled, const short GridX, const short GridY) {
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
  m_ZoomWidth  = (uint16_t)(m_RelatedImage->m_Width*m_ZoomFactor+.5);
  m_ZoomHeight = (uint16_t)(m_RelatedImage->m_Height*m_ZoomFactor+.5);

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

  // TODOBJ: not sure yet, but the new version should be sufficient ’cause members m_StartX
  // and m_StartY are not used anywhere else. --> Test, then delete comment. Also delete
  // from .h!
  // Following are coordinates in a zoomed image.
//  m_StartX = horizontalScrollBar()->value();
//  uint16_t Width  = MIN(horizontalScrollBar()->pageStep(),
//                        m_QImageZoomed->width());
//  m_StartY = verticalScrollBar()->value();
//  uint16_t Height = MIN(verticalScrollBar()->pageStep(),
//                        m_QImageZoomed->height());

//  // Make a new cut out of our zoomed image
//  delete m_QImageCut;
//  m_QImageCut = new QImage(m_QImageZoomed->copy(m_StartX,
//                                                m_StartY,
//                                                Width,
//                                                Height));
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
  int dx = m_DragDelta->dx();
  int dy = m_DragDelta->dy();
  QRect NewPos;
  switch (m_MovingEdge) {
    case meTopLeft:
      // Calc preliminary new position of moved corner point. Don’t allow it to move
      // becond the actual image (m_Frame).
      NewPos.setX(qBound(m_Frame->left(), m_Rect->left() + dx, m_Frame->right()));
      NewPos.setY(qBound(m_Frame->top(), m_Rect->top() + dy, m_Frame->bottom()));
      // Determine if moving corner changed
      if ((NewPos.x() > m_Rect->right()) && (NewPos.y() <= m_Rect->bottom())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() > m_Rect->right()) && (NewPos.y() >= m_Rect->bottom())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() <= m_Rect->right()) && (NewPos.y() > m_Rect->bottom())) {
        m_MovingEdge = meBottomLeft;
      }
      // Update crop rect
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->right()),
                        MIN(NewPos.y(), m_Rect->bottom()),
                        MAX(NewPos.x(), m_Rect->right()),
                        MAX(NewPos.y(), m_Rect->bottom()));
      break;


    case meTop:
      dx = 0;
      NewPos.setY(qBound(m_Frame->top(), m_Rect->top() + dy, m_Frame->bottom()));
      if (NewPos.y() > m_Rect->bottom()) {
        m_MovingEdge = meBottom;
      }
      m_Rect->setCoords(m_Rect->left(), MIN(NewPos.y(), m_Rect->bottom()),
                        m_Rect->right(), MAX(NewPos.y(), m_Rect->bottom()));
      break;

    case meTopRight:
      NewPos.setX(qBound(m_Frame->left(), m_Rect->right() + dx, m_Frame->right()));
      NewPos.setY(qBound(m_Frame->top(), m_Rect->top() + dy, m_Frame->bottom()));
      if ((NewPos.x() < m_Rect->left()) && (NewPos.y() <= m_Rect->bottom())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() < m_Rect->left()) && (NewPos.y() > m_Rect->bottom())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() >= m_Rect->left()) && (NewPos.y() > m_Rect->bottom())) {
        m_MovingEdge = meBottomRight;
      }
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->left()),
                        MIN(NewPos.y(), m_Rect->bottom()),
                        MAX(NewPos.x(), m_Rect->left()),
                        MAX(NewPos.y(), m_Rect->bottom()));
      break;

    case meRight:
      dy = 0;
      NewPos.setX(qBound(m_Frame->left(), m_Rect->right() + dx, m_Frame->right()));
      if ((NewPos.x() < m_Rect->left())) {
        m_MovingEdge = meLeft;
      }
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->left()), m_Rect->top(),
                        MAX(NewPos.x(), m_Rect->left()), m_Rect->bottom());
      break;

    case meBottomRight:
      NewPos.setX(qBound(m_Frame->left(), m_Rect->right() + dx, m_Frame->right()));
      NewPos.setY(qBound(m_Frame->top(), m_Rect->bottom() + dy, m_Frame->bottom()));
      if ((NewPos.x() < m_Rect->left()) && (NewPos.y() >= m_Rect->top())) {
        m_MovingEdge = meBottomLeft;
      } else if ((NewPos.x() < m_Rect->left()) && (NewPos.y() < m_Rect->top())) {
        m_MovingEdge = meTopLeft;
      } else if ((NewPos.x() >= m_Rect->left()) && (NewPos.y() < m_Rect->top())) {
        m_MovingEdge = meTopRight;
      }
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->left()),
                        MIN(NewPos.y(), m_Rect->top()),
                        MAX(NewPos.x(), m_Rect->left()),
                        MAX(NewPos.y(), m_Rect->top()));
      break;

    case meBottom:
      dx = 0;
      NewPos.setY(qBound(m_Frame->top(), m_Rect->bottom() + dy, m_Frame->bottom()));
      if (NewPos.y() < m_Rect->top()) {
        m_MovingEdge = meTop;
      }
      m_Rect->setCoords(m_Rect->left(), MIN(NewPos.y(), m_Rect->top()),
                        m_Rect->right(), MAX(NewPos.y(), m_Rect->top()));
      break;

    case meBottomLeft:
      NewPos.setX(qBound(m_Frame->left(), m_Rect->left() + dx, m_Frame->right()));
      NewPos.setY(qBound(m_Frame->top(), m_Rect->bottom() + dy, m_Frame->bottom()));
      if ((NewPos.x() > m_Rect->right()) && (NewPos.y() >= m_Rect->top())) {
        m_MovingEdge = meBottomRight;
      } else if ((NewPos.x() > m_Rect->right()) && (NewPos.y() < m_Rect->top())) {
        m_MovingEdge = meTopRight;
      } else if ((NewPos.x() <= m_Rect->right()) && (NewPos.y() < m_Rect->top())) {
        m_MovingEdge = meTopLeft;
      }
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->right()),
                        MIN(NewPos.y(), m_Rect->top()),
                        MAX(NewPos.x(), m_Rect->right()),
                        MAX(NewPos.y(), m_Rect->top()));
      break;

    case meLeft:
      dy = 0;
      NewPos.setX(qBound(m_Frame->left(), m_Rect->left() + dx, m_Frame->right()));
      if (NewPos.x() > m_Rect->right()) {
        m_MovingEdge = meRight;
      }
      m_Rect->setCoords(MIN(NewPos.x(), m_Rect->right()), m_Rect->top(),
                        MAX(NewPos.x(), m_Rect->right()), m_Rect->bottom());
      break;


    case meNone:
      break;

    default:
      assert(0);
      break;
  }

  if (m_FixedAspectRatio) {
    EnforceRectAspectRatio();
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

void ptViewWindow::EnforceRectAspectRatio() {
  int NewWidth = qRound(m_Rect->height() * m_AspectRatio);
  int NewHeight = qRound(m_Rect->width() / m_AspectRatio);

  switch (m_MovingEdge){
    case meTopLeft:
      m_Rect->setTop(m_Rect->top() + m_Rect->height() - NewHeight);
      if (m_Rect->top() < m_Frame->top()) {
        m_Rect->setTop(m_Frame->top());
        m_Rect->setLeft(m_Rect->right() - qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meTop:
      m_Rect->setWidth(NewWidth);
      if (m_Rect->right() > m_Frame->right()) {
        m_Rect->setRight(m_Frame->right());
        m_Rect->setTop(m_Rect->bottom() - qRound(m_Rect->width() / m_AspectRatio));
      }
      break;

    case meTopRight:
      m_Rect->setTop(m_Rect->top() + m_Rect->height() - NewHeight);
      if (m_Rect->top() < m_Frame->top()) {
        m_Rect->setTop(m_Frame->top());
        m_Rect->setRight(m_Rect->left() + qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meRight:
      m_Rect->setHeight(NewHeight);
      if (m_Rect->bottom() > m_Frame->bottom()) {
        m_Rect->setBottom(m_Frame->bottom());
        m_Rect->setRight(m_Rect->left() + qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meBottomRight:
      m_Rect->setBottom(m_Rect->bottom() + NewHeight - m_Rect->height());
      if (m_Rect->bottom() > m_Frame->bottom()) {
        m_Rect->setBottom(m_Frame->bottom());
        m_Rect->setRight(m_Rect->left() + qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meBottom:
      m_Rect->setWidth(NewWidth);
      if (m_Rect->right() > m_Frame->right()) {
        m_Rect->setRight(m_Frame->right());
        m_Rect->setBottom(m_Rect->top() + qRound(m_Rect->width() / m_AspectRatio));
      }
      break;

    case meBottomLeft:
      m_Rect->setBottom(m_Rect->bottom() + NewHeight - m_Rect->height());
      if (m_Rect->bottom() > m_Frame->bottom()) {
        m_Rect->setBottom(m_Frame->bottom());
        m_Rect->setLeft(m_Rect->right() - qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meLeft:
      m_Rect->setHeight(NewHeight);
      if (m_Rect->bottom() > m_Frame->bottom()) {
        m_Rect->setBottom(m_Frame->bottom());
        m_Rect->setLeft(m_Rect->right() - qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    case meNone:
      if (ABS(((double)m_Rect->width() / (double)m_Rect->height()) - m_AspectRatio) < 0.001) {
        m_Rect->setWidth(NewWidth);
        m_Rect->setHeight(qRound(m_Rect->width() / m_AspectRatio));
      }

      if (m_Rect->left() < m_Frame->left()) {
        m_Rect->setLeft(m_Frame->left());
        m_Rect->setBottom(m_Rect->top() + qRound(m_Rect->width() / m_AspectRatio));
      }
      if (m_Rect->right() > m_Frame->right()) {
        m_Rect->setRight(m_Frame->right());
        m_Rect->setTop(m_Rect->bottom() - qRound(m_Rect->width() / m_AspectRatio));
      }
      if (m_Rect->top() < m_Frame->top()) {
        m_Rect->setTop(m_Frame->top());
        m_Rect->setLeft(m_Rect->right() - qRound(m_Rect->height() * m_AspectRatio));
      }
      if (m_Rect->bottom() > m_Frame->bottom()) {
        m_Rect->setBottom(m_Frame->bottom());
        m_Rect->setLeft(m_Rect->right() - qRound(m_Rect->height() * m_AspectRatio));
      }
      break;

    default:
      assert(0);
  }


/*  // Correct AR
  if (m_FixedAspectRatio && (ABS(dx) >= ABS(dy))) {
    // dominant change horizontally: we’ll adjust height    
    m_Rect->setHeight(qRound(m_Rect->width() / m_AspectRatio));
    if (m_Rect->bottom() > m_Frame->bottom()) {
      m_Rect->moveBottom(m_Frame->bottom());
    }
    if (m_Rect->height() > m_Frame->height()) {
      m_Rect->setWidth(qRound(m_Frame->height() * m_AspectRatio));
      m_Rect->setHeight(m_Frame->height());
    }

  } else if (m_FixedAspectRatio && (ABS(dx) < ABS(dy))) {
    // dominant change is vertically: we’ll adjust width
    m_Rect->setWidth(qRound(m_Rect->height() * m_AspectRatio));
    if (m_Rect->right() > m_Frame->right()) {
      m_Rect->moveRight(m_Frame->right());
    }
    if (m_Rect->width() > m_Frame->width()) {
      m_Rect->setHeight(qRound(m_Frame->width() / m_AspectRatio));
      m_Rect->setWidth(m_Frame->width());
    }
  }
  */
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

void ptViewWindow::paintEvent(QPaintEvent* Event) {
  if (m_QImageCut == NULL) {
    return;
  }

  int VPWidth  = viewport()->size().width();
  int VPHeight = viewport()->size().height();

  // Calc position of the image frame in viewport
  m_Frame->setWidth(m_QImageCut->width());
  m_Frame->setHeight(m_QImageCut->height());
  if (VPHeight > m_QImageCut->height()) {
    m_Frame->setTop((VPHeight - m_QImageCut->height()) / 2);
  } else {
    m_Frame->setTop(0);
  }
  if (VPWidth > m_QImageCut->width()) {
    m_Frame->setLeft((VPWidth - m_QImageCut->width()) / 2);
  } else {
    m_Frame->setLeft(0);
  }
// TODOBJ: Remove printfs before final version!
//printf("Frame: %d %d %d %d | ", m_Frame->left(), m_Frame->top(), m_Frame->width(), m_Frame->height());
//printf("Rect: %d %d %d %d | ", m_Rect->left(), m_Rect->top(), m_Rect->width(), m_Rect->height());
//printf("AR %d x %d y %d fr %.3f\n", m_FixedAspectRatio, m_AspectRatioW, m_AspectRatioH, m_AspectRatio);

  // Fill viewport with background colour and draw image
  QPainter Painter(viewport());
  Painter.save();
  Painter.fillRect(0, 0, VPWidth, VPHeight, palette().color(QPalette::Window));
  if (m_QImageCut) {
    Painter.drawImage(m_Frame->left(), m_Frame->top(), *m_QImageCut);
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
        Painter.drawLine(m_Frame->left()+i*XStep,
             m_Frame->top(),
             m_Frame->left()+i*XStep,
             m_Frame->top()+m_QImageCut->height()-1);
      }
    }
    if (YNrLines) {
      for (int i = 1; i <= YNrLines; i++) { //horizontal lines
        Painter.drawLine(m_Frame->left(),
             m_Frame->top()+i*YStep,
             m_Frame->left()+m_QImageCut->width()-1,
             m_Frame->top()+i*YStep);
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
      if (m_CropLightsOut > 0) {
        if (m_Rect->top() > m_Frame->top()) { // Top
          Painter.fillRect(m_Frame->left(), m_Frame->top(),
                           m_Frame->width(), m_Rect->top() - m_Frame->top(),
                           LightsOutBrush);
        }
        if (m_Rect->bottom() < m_Frame->bottom()) { // Bottom
          Painter.fillRect(m_Frame->left(), m_Rect->bottom() + 1,
                           m_Frame->width(), m_Frame->bottom() - m_Rect->bottom(),
                           LightsOutBrush);
        }
        if (m_Rect->left() > m_Frame->left()) {   // left
          Painter.fillRect(m_Frame->left(), m_Rect->top(),
                           m_Rect->left() - m_Frame->left(), m_Rect->height(),
                           LightsOutBrush);
        }
        if (m_Rect->right() < m_Frame->right()) {   // right
          Painter.fillRect(m_Rect->right() + 1, m_Rect->top(),
                           m_Frame->right() - m_Rect->right(), m_Rect->height(),
                           LightsOutBrush);
        }
      }


      // Draw outline/guidelines for crop rectangle (not for "lights off")
      if (m_CropLightsOut != 2) {
        QPen Pen(QColor(150, 150, 150),1);
        Painter.setPen(Pen);
        Painter.drawRect(*m_Rect);

        switch (m_CropGuidelines) {
          case ptCropGuidelines_RuleThirds: {
            int HeightThird = (int)(m_Rect->height() / 3);
            int WidthThird = (int)(m_Rect->width() / 3);
            Painter.drawRect(m_Rect->left() + WidthThird, m_Rect->top(),
                             WidthThird, m_Rect->height());
            Painter.drawRect(m_Rect->left(), m_Rect->top() + HeightThird,
                             m_Rect->width(), HeightThird);
            break;
          }

          case ptCropGuidelines_GoldenRatio: {
            int ShortWidth = (int)(m_Rect->width() * 5/13);
            int ShortHeight = (int)(m_Rect->height() * 5/13);
            Painter.drawRect(m_Rect->left() + ShortWidth, m_Rect->top(),
                             m_Rect->width() - (2 * ShortWidth), m_Rect->height());

            Painter.drawRect(m_Rect->left(), m_Rect->top() + ShortHeight,
                             m_Rect->width(), m_Rect->height() - (2 * ShortHeight));
            break;
          }

          case ptCropGuidelines_Diagonals: {
            int length = m_Rect->width() > m_Rect->height() ? m_Rect->height() : m_Rect->width();
            Painter.drawLine(m_Rect->left(), m_Rect->top(),
                             m_Rect->left() + length, m_Rect->top() + length);
            Painter.drawLine(m_Rect->right(), m_Rect->bottom(),
                             m_Rect->right() - length + 1, m_Rect->bottom() - length + 1);
            Painter.drawLine(m_Rect->left(), m_Rect->bottom(),
                             m_Rect->left() + length, m_Rect->bottom() - length + 1);
            Painter.drawLine(m_Rect->right(), m_Rect->top(),
                             m_Rect->right() - length + 1, m_Rect->top() + length);
            break;
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
void ptViewWindow::resizeEvent(QResizeEvent* Event) {
  uint16_t VP_Width  = viewport()->size().width();
  uint16_t VP_Height = viewport()->size().height();

  // Adapt scrollbars to new viewport size
  verticalScrollBar()->setPageStep(VP_Height);
  verticalScrollBar()->setRange(0,m_ZoomHeight-VP_Height);
  horizontalScrollBar()->setPageStep(VP_Width);
  horizontalScrollBar()->setRange(0,m_ZoomWidth-VP_Width);

  // Recalculat the cut.
  RecalcCut();
  if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit)
    ::CB_ZoomFitButton();
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
  if (!m_Rect->contains(Event->pos())) {
    return meNone;
  }

  ptMovingEdge HoverOver = meNone;
  int TBthick = 0;
  int LRthick = 0;

  // Determine edge area thickness
  if (m_Rect->height() <= TinyRectThreshold) {
    TBthick = (int)(m_Rect->height() / 2);
  } else {
    TBthick = EdgeThickness;
  }
  if (m_Rect->width() <= TinyRectThreshold) {
    LRthick = (int)(m_Rect->width() / 2);
  } else {
    LRthick = EdgeThickness;
  }

  // Determine in which area the mouse is
  if (m_Rect->bottom() - Event->y() <= TBthick) {
    HoverOver = meBottom;
  } else if (Event->y() - m_Rect->top() <= TBthick) {
    HoverOver = meTop;
  } else {
    HoverOver = meCenter;
  }

  if (m_Rect->right() - Event->x() <= LRthick) {
    if (HoverOver == meBottom) {
      HoverOver = meBottomRight;
    } else if (HoverOver == meTop) {
      HoverOver = meTopRight;
    } else {
      HoverOver = meRight;
    }

  } else if (Event->x() - m_Rect->left() <= LRthick) {
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
        if (m_Frame->contains(Event->pos())) {
          m_NowDragging = 1;
          m_DragDelta->setPoints(Event->pos(), Event->pos());

          // Start new rect when none is present or clicked outside current one.
          // Always the case for vaSelectRect.
          if (!m_Rect->contains(Event->pos())) {
            m_Rect->setRect(Event->x(), Event->y(), 0, 0);
            m_MovingEdge = meNone;
            viewport()->repaint();
          }
        }
        break;

      // scroll the image
      case vaNone:
        if (m_Frame->contains(Event->pos())) {
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
// mouseMoveEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::mouseMoveEvent(QMouseEvent* Event) {
  // dragging rectangle or image
  if (m_NowDragging) {
    m_DragDelta->setP2(Event->pos());

    switch (m_InteractionMode) {
      case vaSelectRect:
      case vaCrop:
        // Move current rectangle. The qBounds make sure it stops at image boundaries.
        if ((m_MovingEdge == meCenter) || (Event->modifiers() == Qt::ControlModifier)) {
          m_Rect->moveTo(
              qBound(m_Frame->left(),
                     m_Rect->left() + m_DragDelta->dx(),
                     m_Frame->right() - m_Rect->width()),
              qBound(m_Frame->top(),
                     m_Rect->top() + m_DragDelta->dy(),
                     m_Frame->bottom() - m_Rect->height())
          );
        } else {
          // initialize movement direction when rectangle was just started
          int dx = m_DragDelta->dx();
          int dy = m_DragDelta->dy();
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
// dragEnterEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::dragEnterEvent(QDragEnterEvent* Event) {
  // accept just text/uri-list mime format
  if (Event->mimeData()->hasFormat("text/uri-list")) {
    Event->acceptProposedAction();
  }
}


////////////////////////////////////////////////////////////////////////
//
// dropEvent
//
////////////////////////////////////////////////////////////////////////

void ptViewWindow::dropEvent(QDropEvent* Event) {
  QList<QUrl> UrlList;
  QString DropName;
  QFileInfo DropInfo;

  if (Event->mimeData()->hasUrls())
  {
    UrlList = Event->mimeData()->urls(); // returns list of QUrls

    // if just text was dropped, urlList is empty (size == 0)
    if ( UrlList.size() > 0) // if at least one QUrl is present in list
    {
      DropName = UrlList[0].toLocalFile(); // convert first QUrl to local path
      DropInfo.setFile( DropName ); // information about file
      if ( DropInfo.isFile() ) { // if is file
        if (DropInfo.completeSuffix()!="pts" && DropInfo.completeSuffix()!="ptj" &&
            DropInfo.completeSuffix()!="dls" && DropInfo.completeSuffix()!="dlj") {
          ImageFileToOpen = DropName;
          CB_MenuFileOpen(1);
        } else {
          if ( Settings->GetInt("ResetSettingsConfirmation") == 0 ) {
            CB_OpenSettingsFile(DropName);
          } else {
            #ifdef Q_OS_WIN32
              DropName = DropName.replace(QString("/"), QString("\\"));
            #endif
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setWindowTitle(tr("Settings file dropped!"));
            msgBox.setText(tr("Do you really want to open\n")+DropName);
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);
            if (msgBox.exec()==QMessageBox::Ok){
              CB_OpenSettingsFile(DropName);
            }
          }
        }
      }
    }
  }
  Event->acceptProposedAction();
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
