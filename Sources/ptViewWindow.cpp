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

#include <cassert>

#include "ptDefines.h"
#include "ptSettings.h"
#include "ptTheme.h"
#include "ptViewWindow.h"
#include "ptConstants.h"

extern ptTheme* Theme;
extern ptSettings* Settings;

//==============================================================================

ptViewWindow::ptViewWindow(QWidget* Parent, ptMainWindow* mainWin)
: QGraphicsView(Parent),
  // constants
  MinZoom(0.05), MaxZoom(4.0),
  // member variables
  FCtrlIsPressed(0),
  FDrawLine(nullptr),
  FLocalAdjust(nullptr),
  FSelectRect(nullptr),
  FCrop(nullptr),
  FInteraction(iaNone),
  FLeftMousePressed(0),
  FZoomIsSaved(0),
  FZoomFactor(1.0),
  FZoomFactorSav(0.0),
  FZoomModeSav(ptZoomMode_Fit),
  FPixelReading(prNone),
  // always keep this at the end of the initializer list
  FMainWindow(mainWin)
{
  assert(FMainWindow != NULL);    // Main window must exist before view window
  assert(Theme != NULL);
  assert(Settings != NULL);

  ZoomFactors << MinZoom << 0.08 << 0.10 << 0.15 << 0.20 << 0.25 << 0.33 << 0.50 << 0.66 << 1.00
              << 1.50 << 2.00 << 3.00 << MaxZoom;

  FDragDelta = new QLine();
  FStatusOverlay = new ptReportOverlay(this, "", QColor(), QColor(), 0, Qt::AlignLeft, 20);
  FZoomSizeOverlay = new ptReportOverlay(this, "", QColor(75,150,255), QColor(190,220,255),
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
  FImageScene = new QGraphicsScene(0, 0, 0, 0, Parent);
  F8bitImageItem = FImageScene->addPixmap(QPixmap());
  F8bitImageItem->setPos(0, 0);
  this->setScene(FImageScene);

  FGrid = new ptGridInteraction(this);  // scene must already be alive
  FPixelReader = NULL;
  ConstructContextMenu();

  FPReadTimer = new QTimer();
  FPReadTimer->setSingleShot(true);
}

//==============================================================================

ptViewWindow::~ptViewWindow() {
  delete FDragDelta;
  delete FStatusOverlay;
  delete FZoomSizeOverlay;
  delete FDrawLine;
  delete FSelectRect;
  delete FGrid;

  delete ac_PRead_None;
  delete ac_PRead_Linear;
  delete ac_PRead_Preview;
  delete ac_PReadGroup;

  delete FPReadTimer;
}

//==============================================================================

// Convert a 16bit ptImage to an 8bit QPixmap. Mind R<->B. Also update the
// graphics scene and the viewport.
void ptViewWindow::UpdateImage(const ptImage* relatedImage) {
  if (relatedImage) {
    this->blockSignals(1);
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

    F8bitImageItem->setPixmap(QPixmap::fromImage(*Img8bit, Qt::ColorOnly));
    DelAndNull(Img8bit);
    FImageScene->setSceneRect(0, 0,
                              F8bitImageItem->pixmap().width(),
                              F8bitImageItem->pixmap().height());

    if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit) {
      ZoomToFit(0);
    }

    this->blockSignals(0);
  }
}

//==============================================================================

// ZoomTo() is also called by wheelEvent() for mouse wheel zoom.
void ptViewWindow::ZoomTo(float factor) {
  Settings->SetValue("ZoomMode",ptZoomMode_NonFit);
  factor = qBound(MinZoom, factor, MaxZoom);

  if(((uint)(factor * 10000) % 10000) < 1) {
    // nearest neighbour resize for 200%, 300%, 400% zoom
    F8bitImageItem->setTransformationMode(Qt::FastTransformation);
  } else {
    // bilinear resize for all others
    F8bitImageItem->setTransformationMode(Qt::SmoothTransformation);
  }
  setTransform(QTransform(factor, 0, 0, factor, 0, 0));

  FZoomFactor = transform().m11();
  int z = qRound(FZoomFactor * 100);
  Settings->SetValue("Zoom", z);
  FZoomSizeOverlay->exec(QString::number(z) + "%");
}

//==============================================================================

int ptViewWindow::ZoomToFit(const short withMsg /*= 1*/) {
  Settings->SetValue("ZoomMode",ptZoomMode_Fit);

  if (!F8bitImageItem->pixmap().isNull()) {
    // Always smooth scaling because we don't know the zoom factor in advance.
    F8bitImageItem->setTransformationMode(Qt::SmoothTransformation);
    fitInView(F8bitImageItem, Qt::KeepAspectRatio);
    FZoomFactor = transform().m11();

    if (withMsg) {
      FZoomSizeOverlay->exec(tr("Fit"));
    }
  }

  Settings->SetValue("Zoom",qRound(FZoomFactor * 100));
  return FZoomFactor;
}

//==============================================================================

void ptViewWindow::ZoomStep(int direction) {
  int ZoomIdx = -1;

  // zoom larger
  if (direction > 0) {
    for (int i = 0; i < ZoomFactors.size(); i++) {
      if (ZoomFactors[i] > FZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }

  // zoom smaller
  } else if (direction < 0) {
    for (int i = ZoomFactors.size() - 1; i >= 0; i--) {
      if (ZoomFactors[i] < FZoomFactor) {
        ZoomIdx = i;
        break;
      }
    }
  }

  if (ZoomIdx != -1) {
    ZoomTo(ZoomFactors[ZoomIdx]);
  }
}

//==============================================================================

void ptViewWindow::RestoreZoom() {
  if (FZoomIsSaved) {
    if (FZoomModeSav == ptZoomMode_Fit) {
      ZoomToFit();
    } else {
      ZoomTo(FZoomFactorSav);
    }

    FZoomIsSaved = 0;
  }
}

//==============================================================================

void ptViewWindow::SaveZoom() {
  FZoomFactorSav = FZoomFactor;
  FZoomModeSav = Settings->GetValue("ZoomMode").toInt();
  FZoomIsSaved = 1;
}

//==============================================================================

void ptViewWindow::setGrid(const short enabled, const uint linesX, const uint linesY) {
  if (enabled) {
    FGrid->show(linesX, linesY);
  } else {
    FGrid->hide();
  }
}

//==============================================================================

void ptViewWindow::paintEvent(QPaintEvent* event) {
  // Fill viewport with background colour
  QPainter Painter(viewport());
  Painter.fillRect(0, 0, viewport()->width(), viewport()->height(), palette().color(QPalette::Window));
  Painter.end();

  // takes care of updating the scene
  QGraphicsView::paintEvent(event);
}

//==============================================================================

void ptViewWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
    FLeftMousePressed = 1;
    FDragDelta->setPoints(event->pos(), event->pos());
  }

  // Broadcast event to possible interaction handlers
  if (FInteraction != iaNone) {
    emit mouseChanged(event);
  }
}

//==============================================================================

void ptViewWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && FLeftMousePressed) {
    FLeftMousePressed = 0;
    event->accept();
  } else {
    event->ignore();
  }

  // Broadcast event to possible interaction handlers
  if (FInteraction != iaNone) {
    emit mouseChanged(event);
  }
}

//==============================================================================

void ptViewWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  // Broadcast event to possible interaction handlers
  if (FInteraction != iaNone) {
    emit mouseChanged(event);
  } else {
    event->ignore();
  }
}

//==============================================================================

void ptViewWindow::mouseMoveEvent(QMouseEvent* event) {
  // We broadcast the pixel location
  if (FPixelReading != prNone && !FPReadTimer->isActive()) {
    FPReadTimer->start(10);
    QPointF Point = mapToScene(event->pos());
    if (Point.x() >= 0 && Point.x() < F8bitImageItem->pixmap().width() &&
        Point.y() >= 0 && Point.y() < F8bitImageItem->pixmap().height()) {
      if (FPixelReader) FPixelReader(Point, FPixelReading);
    } else {
      if (FPixelReader) FPixelReader(Point, prNone);
    }
  }

  // drag image with left mouse button to scroll
  // Also Ctrl needed in crop & spot repair modes
  short ImgDragging = FLeftMousePressed && FInteraction == iaNone;
  if (FInteraction == iaCrop || FInteraction == iaSpotRepair) {
    ImgDragging = FLeftMousePressed && FCtrlIsPressed;
  }

  if (ImgDragging) {
    FDragDelta->setP2(event->pos());
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                                    FDragDelta->x2() +
                                    FDragDelta->x1());
    verticalScrollBar()->setValue(verticalScrollBar()->value() -
                                  FDragDelta->y2() +
                                  FDragDelta->y1());
    FDragDelta->setP1(event->pos());
    event->accept();

  } else {
    event->ignore();
    // Broadcast event to possible interaction handlers
    if (FInteraction != iaNone) {
      emit mouseChanged(event);
    }
  }
}

//==============================================================================

void ptViewWindow::wheelEvent(QWheelEvent* event) {
  ZoomStep(event->delta());
}

void ptViewWindow::leaveEvent(QEvent*) {
  if (FPixelReader) FPixelReader(QPoint(), prNone);
}

//==============================================================================

void ptViewWindow::keyPressEvent(QKeyEvent* event) {
  // FCtrlIsPressed is not a simple bool flag to account for keyboards with
  // multiple ctrl keys. There's a lot of those. ;-) For each ctrl press the
  // variable is increased, for each release it's decreased. This is necessary
  // because in cases like press left ctrl - press right ctrl - release left ctrl
  // Photivo should still recognise the ctrl key as being held down.
  if (event->key() == Qt::Key_Control) {
    FCtrlIsPressed++;
    if (FInteraction == iaNone || FInteraction == iaCrop || FInteraction == iaSpotRepair) {
      setCursor(Qt::ClosedHandCursor);
    }
  } else {
    event->ignore();  // necessary to forward unhandled keys to main window
  }

  if (FInteraction != iaNone) {
    emit keyChanged(event);
  }
}

//==============================================================================

void ptViewWindow::keyReleaseEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Control) {
    FCtrlIsPressed--;
    if (FCtrlIsPressed == 0 &&
      (FInteraction == iaNone || FInteraction == iaCrop || FInteraction == iaSpotRepair))
    {
      setCursor(Qt::ArrowCursor);
    }
  } else {
    event->ignore();  // necessary to forward unhandled keys to main window
  }

  if (FInteraction != iaNone) {
    emit keyChanged(event);
  }
}

//==============================================================================

// The two functions are necessary to enable d&d over the view window.
// The actual d&d action is handled by the resp. main window events.

void ptViewWindow::dragEnterEvent(QDragEnterEvent* event) {
  event->ignore();
}

void ptViewWindow::dropEvent(QDropEvent* event) {
  event->ignore();
}

//==============================================================================

void ptViewWindow::resizeEvent(QResizeEvent* event) {
  if (Settings->GetInt("ZoomMode") == ptZoomMode_Fit) {
    event->accept();
    ZoomToFit(0);
  } else {
    // takes care of positioning the scene inside the viewport on non-fit zooms
    QGraphicsView::resizeEvent(event);
  }
}

//==============================================================================

// Top left corner overlay for the processing status
void ptViewWindow::ShowStatus(short mode) {
  switch (mode) {
    case ptStatus_Done:
      FStatusOverlay->setColors(QColor(0,130,0), QColor(120,170,120));   // green
      FStatusOverlay->setDuration(1500);
      FStatusOverlay->exec(QObject::tr("Done"));
      break;

    case ptStatus_Updating:
      FStatusOverlay->setColors(QColor(255,140,0), QColor(255,200,120));   // orange
      FStatusOverlay->setDuration(0);
      FStatusOverlay->exec(QObject::tr("Updating"));
      break;

    case ptStatus_Processing:
      FStatusOverlay->setColors(QColor(255,75,75), QColor(255,190,190));   // red
      FStatusOverlay->setDuration(0);
      FStatusOverlay->exec(QObject::tr("Processing"));
      break;

    default:    // should not happen
      FStatusOverlay->stop();
      break;
  }
}

//==============================================================================

void ptViewWindow::ShowStatus(const QString text) {
  FStatusOverlay->setColors(QColor(75,150,255), QColor(190,220,255));    // blue
  FStatusOverlay->setDuration(1500);
  FStatusOverlay->exec(text);
}

//==============================================================================

// Start draw line interaction to determine rotation angle.
void ptViewWindow::StartLine() {
  if (FInteraction == iaNone) {
    FDrawLine = new ptLineInteraction(this);
    connect(FDrawLine, SIGNAL(finished(ptStatus)), this, SLOT(finishInteraction(ptStatus)));
    connect(this, SIGNAL(mouseChanged(QMouseEvent*)), FDrawLine, SLOT(mouseAction(QMouseEvent*)));
    connect(this, SIGNAL(keyChanged(QKeyEvent*)), FDrawLine, SLOT(keyAction(QKeyEvent*)));
    FInteraction = iaDrawLine;
  }
}

//==============================================================================

// Start simple selection interaction for spot WB and histogram "crop".
void ptViewWindow::StartSimpleRect(void (*CB_SimpleRect)(const ptStatus, QRect)) {
  if (FInteraction == iaNone) {
    assert(CB_SimpleRect != NULL);
    FCB_SimpleRect = CB_SimpleRect;
    FSelectRect = new ptSimpleRectInteraction(this);

    connect(FSelectRect, SIGNAL(finished(ptStatus)),
            this,        SLOT  (finishInteraction(ptStatus)));
    connect(this,        SIGNAL(mouseChanged(QMouseEvent*)),
            FSelectRect, SLOT  (mouseAction(QMouseEvent*)));
    connect(this,        SIGNAL(keyChanged(QKeyEvent*)),
            FSelectRect, SLOT  (keyAction(QKeyEvent*)));

    FInteraction = iaSelectRect;
  }
}

//==============================================================================

void ptViewWindow::StartCrop()
{
  if (FInteraction != iaNone) {
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
  if(x < 0 || x >= F8bitImageItem->pixmap().width() ||
     y < 0 || y >= F8bitImageItem->pixmap().height() ||
     width <= 0 || width > F8bitImageItem->pixmap().width() ||
     height <= 0 || height > F8bitImageItem->pixmap().height() )
  {
    x = 0;
    y = 0;
    width = F8bitImageItem->pixmap().width();
    height = F8bitImageItem->pixmap().height();
  }


  FCrop = new ptRichRectInteraction(this, x, y, width, height,
                                     Settings->GetInt("FixedAspectRatio"),
                                     Settings->GetInt("AspectRatioW"),
                                     Settings->GetInt("AspectRatioH"),
                                     Settings->GetInt("CropGuidelines") );

  connect(FCrop, SIGNAL(finished(ptStatus)),
          this,  SLOT  (finishInteraction(ptStatus)));
  connect(this,  SIGNAL(mouseChanged(QMouseEvent*)),
          FCrop, SLOT  (mouseAction(QMouseEvent*)));
  connect(this,  SIGNAL(keyChanged(QKeyEvent*)),
          FCrop, SLOT  (keyAction(QKeyEvent*)));

  FInteraction = iaCrop;
}

//==============================================================================

void ptViewWindow::StartLocalAdjust() {
  if (FInteraction != iaNone) {
    return;
  }
  FLocalAdjust = new ptSpotInteraction(this);

  connect(FLocalAdjust, SIGNAL(finished(ptStatus)),
          this,         SLOT  (finishInteraction(ptStatus)));
//  connect(FLocalAdjust, SIGNAL(clicked(QPoint,bool)),
//          AListView,    SLOT  (processCoordinates(QPoint,bool)));
  connect(this,         SIGNAL(mouseChanged(QMouseEvent*)),
          FLocalAdjust, SLOT  (mouseAction(QMouseEvent*)));

  FInteraction = iaLocalAdjust;
}

//==============================================================================

void ptViewWindow::StartSpotRepair() {
  if (FInteraction != iaNone) {
    return;
  }
  //TODO: BJ ptRepairInteraction constructor should not have to know its ListView
  FSpotRepair = new ptRepairInteraction(this, nullptr);

  connect(FSpotRepair, SIGNAL(finished(ptStatus)),
          this,        SLOT  (finishInteraction(ptStatus)));
  connect(this,        SIGNAL(mouseChanged(QMouseEvent*)),
          FSpotRepair, SLOT  (mouseAction(QMouseEvent*)));
  connect(this,        SIGNAL(keyChanged(QKeyEvent*)),
          FSpotRepair, SLOT  (keyAction(QKeyEvent*)));

  FInteraction = iaSpotRepair;
}

//==============================================================================

void RotateAngleDetermined(const ptStatus ExitStatus, double RotateAngle);
void CleanupAfterCrop(const ptStatus CropStatus, const QRect CropRect);
void CleanupAfterSpotRepair();
void CleanupAfterLocalAdjust();

void ptViewWindow::finishInteraction(ptStatus ExitStatus) {
  switch (FInteraction) {
    case iaDrawLine: {
      double Angle = FDrawLine->angle();
      DelAndNull(FDrawLine);   // also disconnects all signals/slots
      FInteraction = iaNone;
      RotateAngleDetermined(ExitStatus, Angle);
      break;
    }

    case iaSelectRect: {
      QRect sr = FSelectRect->rect();
      DelAndNull(FSelectRect);   // also disconnects all signals/slots
      FInteraction = iaNone;
      FCB_SimpleRect(ExitStatus, sr);
      break;
    }

    case iaCrop: {
      QRect cr = FCrop->rect();
      DelAndNull(FCrop);   // also disconnects all signals/slots
      FInteraction = iaNone;
      CleanupAfterCrop(ExitStatus, cr);
      break;
    }

    case iaSpotRepair: {
      DelAndNull(FSpotRepair);   // also disconnects all signals/slots
      FInteraction = iaNone;
      CleanupAfterSpotRepair();
      break;
    }

    case iaLocalAdjust: {
      DelAndNull(FLocalAdjust);
      FInteraction = iaNone;
      CleanupAfterLocalAdjust();
      break;
    }

    default:
      assert(!"Unknown FInteraction");
      break;
  }
}

//==============================================================================

// Convenience function to keep constructor short. Is only called once from the
// constructor.
void ptViewWindow::ConstructContextMenu() {
  // Create actions for context menu
  ac_ZoomIn = new QAction(tr("Zoom &in") + "\t" + tr("1"), this);
  ac_ZoomIn->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-in.png")));
  connect(ac_ZoomIn, SIGNAL(triggered()), this, SLOT(Menu_ZoomIn()));

  ac_Zoom100 = new QAction(tr("Zoom &100%") + "\t" + tr("2"), this);
  connect(ac_Zoom100, SIGNAL(triggered()), this, SLOT(Menu_Zoom100()));
  ac_Zoom100->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-original.png")));

  ac_ZoomOut = new QAction(tr("Zoom &out") + "\t" + tr("3"), this);
  ac_ZoomOut->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-out.png")));
  connect(ac_ZoomOut, SIGNAL(triggered()), this, SLOT(Menu_ZoomOut()));

  ac_ZoomFit = new QAction(tr("Zoom &fit") + "\t" + tr("4"), this);
  connect(ac_ZoomFit, SIGNAL(triggered()), this, SLOT(Menu_ZoomFit()));
  ac_ZoomFit->setIcon(QIcon(QString::fromUtf8(":/dark/icons/zoom-fit.png")));


  ac_Mode_RGB = new QAction(tr("&RGB") + "\t" + tr("0"), this);
  ac_Mode_RGB->setCheckable(true);
  connect(ac_Mode_RGB, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_Structure = new QAction(tr("&Structure") + "\t" + tr("9"), this);
  ac_Mode_Structure->setCheckable(true);
  connect(ac_Mode_Structure, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_L = new QAction(tr("&L*") + "\t" + tr("8"), this);
  ac_Mode_L->setCheckable(true);
  connect(ac_Mode_L, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_A = new QAction(tr("&a*") + "\t" + tr("7"), this);
  ac_Mode_A->setCheckable(true);
  connect(ac_Mode_A, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_B = new QAction(tr("&b*") + "\t" + tr("6"), this);
  ac_Mode_B->setCheckable(true);
  connect(ac_Mode_B, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_Mode_Gradient = new QAction(tr("&Gradient") + "\t" + tr("5"), this);
  ac_Mode_Gradient->setCheckable(true);
  connect(ac_Mode_Gradient, SIGNAL(triggered()), this, SLOT(Menu_Mode()));

  ac_ModeGroup = new QActionGroup(this);
  ac_ModeGroup->addAction(ac_Mode_RGB);
  ac_ModeGroup->addAction(ac_Mode_L);
  ac_ModeGroup->addAction(ac_Mode_A);
  ac_ModeGroup->addAction(ac_Mode_B);
  ac_ModeGroup->addAction(ac_Mode_Gradient);
  ac_ModeGroup->addAction(ac_Mode_Structure);

  ac_PRead_None = new QAction(tr("&disabled"), this);
  ac_PRead_None->setCheckable(true);
  connect(ac_PRead_None, SIGNAL(triggered()), this, SLOT(Menu_PixelReading()));

  ac_PRead_Linear = new QAction(tr("&linear"), this);
  ac_PRead_Linear->setCheckable(true);
  connect(ac_PRead_Linear, SIGNAL(triggered()), this, SLOT(Menu_PixelReading()));

  ac_PRead_Preview = new QAction(tr("&preview"), this);
  ac_PRead_Preview->setCheckable(true);
  connect(ac_PRead_Preview, SIGNAL(triggered()), this, SLOT(Menu_PixelReading()));

  ac_PReadGroup = new QActionGroup(this);
  ac_PReadGroup->addAction(ac_PRead_None);
  ac_PReadGroup->addAction(ac_PRead_Linear);
  ac_PReadGroup->addAction(ac_PRead_Preview);
  switch (Settings->GetInt("PixelReader")) {
    case (int)prLinear:  ac_PRead_Linear->setChecked(true);  break;
    case (int)prPreview: ac_PRead_Preview->setChecked(true); break;
    default:             ac_PRead_None->setChecked(true);
  };
  Menu_PixelReading();

  ac_Clip_Indicate = new QAction(tr("Highlight &clipped pixels") + "\t" + tr("C"), this);
  ac_Clip_Indicate->setCheckable(true);
  ac_Clip_Indicate->setChecked(Settings->GetInt("ExposureIndicator"));
  connect(ac_Clip_Indicate, SIGNAL(triggered()), this, SLOT(Menu_Clip_Indicate()));

  ac_Clip_Over = new QAction(tr("&Over exposure"), this);
  ac_Clip_Over->setCheckable(true);
  ac_Clip_Over->setChecked(Settings->GetInt("ExposureIndicatorOver"));
  connect(ac_Clip_Over, SIGNAL(triggered()), this, SLOT(Menu_Clip_Over()));

  ac_Clip_Under = new QAction(tr("&Under exposure"), this);
  ac_Clip_Under->setCheckable(true);
  ac_Clip_Under->setChecked(Settings->GetInt("ExposureIndicatorUnder"));
  connect(ac_Clip_Under, SIGNAL(triggered()), this, SLOT(Menu_Clip_Under()));

  ac_Clip_R = new QAction(tr("&R"), this);
  ac_Clip_R->setCheckable(true);
  ac_Clip_R->setChecked(Settings->GetInt("ExposureIndicatorR"));
  connect(ac_Clip_R, SIGNAL(triggered()), this, SLOT(Menu_Clip_R()));

  ac_Clip_G = new QAction(tr("&G"), this);
  ac_Clip_G->setCheckable(true);
  ac_Clip_G->setChecked(Settings->GetInt("ExposureIndicatorG"));
  connect(ac_Clip_G, SIGNAL(triggered()), this, SLOT(Menu_Clip_G()));

  ac_Clip_B = new QAction(tr("&B"), this);
  ac_Clip_B->setCheckable(true);
  ac_Clip_B->setChecked(Settings->GetInt("ExposureIndicatorB"));
  connect(ac_Clip_B, SIGNAL(triggered()), this, SLOT(Menu_Clip_B()));

  ac_SensorClip = new QAction(tr("&Sensor"), this);
  ac_SensorClip->setCheckable(true);
  ac_SensorClip->setChecked(Settings->GetInt("ExposureIndicatorSensor"));
  connect(ac_SensorClip, SIGNAL(triggered()), this, SLOT(Menu_SensorClip()));
  ac_SensorClipSep = new QAction(this);
  ac_SensorClipSep->setSeparator(true);

  ac_ShowZoomBar = new QAction(tr("Show &bottom bar"), this);
  ac_ShowZoomBar->setCheckable(true);
  ac_ShowZoomBar->setChecked(Settings->GetInt("ShowBottomContainer"));
  connect(ac_ShowZoomBar, SIGNAL(triggered()), this, SLOT(Menu_ShowZoomBar()));

  ac_ShowTools = new QAction(tr("Show &tool pane") + "\t" + tr("Space"), this);
  ac_ShowTools->setCheckable(true);
  ac_ShowTools->setChecked(Settings->GetInt("ShowToolContainer"));
  connect(ac_ShowTools, SIGNAL(triggered()), this, SLOT(Menu_ShowTools()));

#ifndef PT_WITHOUT_FILEMGR
  ac_OpenFileMgr = new QAction(tr("Open file m&anager") + "\t" + tr("Ctrl+M"), this);
  connect(ac_OpenFileMgr, SIGNAL(triggered()), this, SLOT(Menu_OpenFileMgr()));
#endif

  ac_Fullscreen = new QAction(tr("Full&screen") + "\t" + tr("F11"), this);
  ac_Fullscreen->setCheckable(true);
  ac_Fullscreen->setChecked(0);
  connect(ac_Fullscreen, SIGNAL(triggered()), this, SLOT(Menu_Fullscreen()));
}

//==============================================================================

void ptViewWindow::contextMenuEvent(QContextMenuEvent* event) {
  if (FInteraction == iaSelectRect || FInteraction == iaDrawLine) {
    return;
  }

  // Create the menus themselves
  // Note: Menus cannot be created with new. That breaks the theming.
  QMenu Menu_Mode(tr("Display &mode"), this);
  Menu_Mode.setPalette(Theme->menuPalette());
  Menu_Mode.setStyle(Theme->style());
  Menu_Mode.addAction(ac_Mode_RGB);
  Menu_Mode.addAction(ac_Mode_Structure);
  Menu_Mode.addAction(ac_Mode_L);
  Menu_Mode.addAction(ac_Mode_A);
  Menu_Mode.addAction(ac_Mode_B);
  Menu_Mode.addAction(ac_Mode_Gradient);

  QMenu Menu_Clip(tr("Show &clipping"), this);
  Menu_Clip.setPalette(Theme->menuPalette());
  Menu_Clip.setStyle(Theme->style());
  Menu_Clip.addAction(ac_Clip_Indicate);
  Menu_Clip.addSeparator();
  Menu_Clip.addAction(ac_Clip_Over);
  Menu_Clip.addAction(ac_Clip_Under);
  Menu_Clip.addSeparator();
  Menu_Clip.addAction(ac_Clip_R);
  Menu_Clip.addAction(ac_Clip_G);
  Menu_Clip.addAction(ac_Clip_B);

  QMenu Menu_PRead(tr("Pixel values"), this);
  Menu_PRead.setPalette(Theme->menuPalette());
  Menu_PRead.setStyle(Theme->style());
  Menu_PRead.addAction(ac_PRead_None);
  Menu_PRead.addAction(ac_PRead_Linear);
  Menu_PRead.addAction(ac_PRead_Preview);

  QMenu Menu(this);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  Menu.addAction(ac_ZoomIn);
  Menu.addAction(ac_Zoom100);
  Menu.addAction(ac_ZoomOut);
  Menu.addAction(ac_ZoomFit);
  Menu.addSeparator();
  if (FInteraction == iaNone) {
    Menu.addMenu(&Menu_Mode);
    Menu.addMenu(&Menu_Clip);
    Menu.addMenu(&Menu_PRead);
    Menu.addSeparator();
    Menu.addAction(ac_SensorClip);
    Menu.addAction(ac_SensorClipSep);
  }
  Menu.addAction(ac_ShowTools);
  Menu.addAction(ac_ShowZoomBar);
#ifndef PT_WITHOUT_FILEMGR
  Menu.addAction(ac_OpenFileMgr);
#endif
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

//==============================================================================
// slots for the context menu

void Update(short Phase, short SubPhase = -1, short WithIdentify  = 1, short ProcessorMode = ptProcessorMode_Preview);
void ptViewWindow::Menu_Clip_Indicate() {
  Settings->SetValue("ExposureIndicator",(int)ac_Clip_Indicate->isChecked());
  Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_Clip_Over() {
  Settings->SetValue("ExposureIndicatorOver",(int)ac_Clip_Over->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_Clip_Under() {
  Settings->SetValue("ExposureIndicatorUnder",(int)ac_Clip_Under->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_Clip_R() {
  Settings->SetValue("ExposureIndicatorR",(int)ac_Clip_R->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_Clip_G() {
  Settings->SetValue("ExposureIndicatorG",(int)ac_Clip_G->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_Clip_B() {
  Settings->SetValue("ExposureIndicatorB",(int)ac_Clip_B->isChecked());
  if (Settings->GetInt("ExposureIndicator"))
    Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_SensorClip() {
  Settings->SetValue("ExposureIndicatorSensor",(int)ac_SensorClip->isChecked());
  Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_ShowZoomBar() {
  Settings->SetValue("ShowBottomContainer",(int)ac_ShowZoomBar->isChecked());
  FMainWindow->UpdateSettings();
}

void ptViewWindow::Menu_ShowTools() {
  Settings->SetValue("ShowToolContainer",(int)ac_ShowTools->isChecked());
  FMainWindow->UpdateSettings();
}

void ptViewWindow::Menu_OpenFileMgr() {
  emit openFileMgr();
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

void ptViewWindow::Menu_ZoomIn() {
  ZoomStep(1);
}

void ptViewWindow::Menu_ZoomOut() {
  ZoomStep(-1);
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

  Update(ptProcessorPhase_Preview);
}

void ptViewWindow::Menu_PixelReading() {
  if (ac_PRead_None->isChecked()) {
    if (FPixelReader) FPixelReader(QPoint(), prNone);
    FPixelReading = prNone;
  }
  else if (ac_PRead_Linear->isChecked())  FPixelReading = prLinear;
  else if (ac_PRead_Preview->isChecked()) FPixelReading = prPreview;

  Settings->SetValue("PixelReader", (int)FPixelReading);
}

//==============================================================================
