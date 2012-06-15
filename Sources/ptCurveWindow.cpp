/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008-2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#include <vector>

#include <QPainter>
#include <QActionGroup>
#include <QMenu>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>

#include "ptCurveWindow.h"
#include "ptCurve.h"
#include "ptTheme.h"
#include "ptInfo.h"
#include <filters/ptCfgItem.h>

//==============================================================================

// How many pixels will be considered as 'bingo' for having the anchor ?
const int CSnapDelta = 6;
// Percentage to be close to a curve to get a new Anchor
const double CCurveDelta = 0.12;
// Distance to the next anchor
const float CAnchorDelta = 0.005f;
// Delays in ms before certain actions are triggered
const int CPipeDelay   = 300;

//==============================================================================

// NOTE: ptCurveWindow would be a good place to use C++11’s ctor delegation.
// Unfortunately it’s only available in GCC 4.7.
ptCurveWindow::ptCurveWindow(QWidget *AParent)
: ptWidget(AParent),
  FCaptionLabel(nullptr),
  FWheelTimer(new QTimer(this)),
  FMouseAction(NoAction),
  FMovingAnchor(-1),
  FLinearIpolAction(nullptr),
  FSplineIpolAction(nullptr),
  FCosineIpolAction(nullptr),
  FIpolGroup(nullptr),
  FByLumaAction(nullptr),
  FByChromaAction(nullptr),
  FMaskGroup(nullptr)
{
  // Timer for the wheel interaction
  FWheelTimer->setSingleShot(1);
  FWheelTimer->setInterval(CPipeDelay);
  connect(FWheelTimer, SIGNAL(timeout()), this, SLOT(wheelTimerExpired()));

  QSizePolicy hPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  hPolicy.setHeightForWidth(true);
  this->setSizePolicy(hPolicy);
}

//==============================================================================

ptCurveWindow::ptCurveWindow(const ptCfgItem &ACfgItem, QWidget *AParent)
: ptWidget(AParent)
{
  this->init(ACfgItem);
}

//==============================================================================

ptCurveWindow::~ptCurveWindow() {
/*  Resources managed by Qt parent or external objects. Do not delete manually.
      all QAction and QActionGroup
      FCaptionLabel
      FWheelTimer
*/
}

//==============================================================================

void ptCurveWindow::init(const ptCfgItem &ACfgItem) {
  this->setObjectName(ACfgItem.Id);  // Do not touch! Filter commonDispatch relies on this.
  FCurve = ACfgItem.Curve;

  // set up caption in topleft corner
  this->setCaption(ACfgItem.Caption);
}

//==============================================================================

void ptCurveWindow::setValue(const QVariant &AValue) {
  GInfo->Assert(AValue.type() == QVariant::Map,
                QString("%1: Value must be of type QVariant::Map (8), but is (%2).")
                    .arg(this->objectName()).arg(AValue.type()), AT);

  auto hTempMap = AValue.toMap();
  FCurve->loadConfig(hTempMap);
  updateView();
  requestPipeRun();
}

//==============================================================================

void ptCurveWindow::setCaption(const QString &ACaption) {
  if (ACaption.isEmpty()) {
    DelAndNull(FCaptionLabel);

  } else {
    if (!FCaptionLabel) {
      FCaptionLabel = new QLabel(this);
      FCaptionLabel->move(5,5);
      FCaptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
      FCaptionLabel->setAttribute(Qt::WA_NoSystemBackground, true);
      FCaptionLabel->setAttribute(Qt::WA_OpaquePaintEvent, false);
    }
    FCaptionLabel->setText("<b style='color:#ffffff'>" + ACaption + "</b>");
  }
}

//==============================================================================

void ptCurveWindow::setMaskType() {
  auto hOldMask = FCurve->mask();

  if (FByLumaAction->isChecked())
    FCurve->setMask(ptCurve::LumaMask);
  else if (FByChromaAction->isChecked())
    FCurve->setMask(ptCurve::ChromaMask);

  if (hOldMask == FCurve->mask()) return;

  if (isCyclicCurve()) {
    // wrap-around/cyclic curve, i.e. left edge = right edge
    FCurve->anchors()->back().second = FCurve->anchors()->front().second;
    FCurve->calcCurve();
  }

  updateView();
  requestPipeRun();
}

//==============================================================================

void ptCurveWindow::setInterpolationType() {
  auto hOldIPol = FCurve->interpolType();

  if (FLinearIpolAction->isChecked())
    FCurve->setInterpolType(ptCurve::LinearInterpol);
  else if (FSplineIpolAction->isChecked())
    FCurve->setInterpolType(ptCurve::SplineInterpol);
  else if (FCosineIpolAction->isChecked())
    FCurve->setInterpolType(ptCurve::CosineInterpol);

  if (hOldIPol == FCurve->interpolType()) return;

  updateView();
  requestPipeRun();
}

//==============================================================================

void ptCurveWindow::setBWGradient(ptImage8* AImage) {
  int Width  = width();
  int Height = height();

  for (uint16_t i=0;i<Width;i++) {
    int Value = (int)(i/(float)Width*255);
    for (uint16_t Row = Height-Height/20;
         Row <= Height-2;
         Row++) {
      AImage->m_Image[Row*Width+i][0] = Value;
      AImage->m_Image[Row*Width+i][1] = Value;
      AImage->m_Image[Row*Width+i][2] = Value;
    }
  }
}

//==============================================================================

void ptCurveWindow::setBWGammaGradient(ptImage8* AImage) {
  int Width  = width();
  int Height = height();

  for (uint16_t i=0;i<Width;i++) {
    int Value = (int)(powf(i/(float)Width,0.45f)*255);
    for (uint16_t Row = Height-Height/20;
         Row <= Height-2;
         Row++) {
      AImage->m_Image[Row*Width+i][0] = Value;
      AImage->m_Image[Row*Width+i][1] = Value;
      AImage->m_Image[Row*Width+i][2] = Value;
    }
  }
}

//==============================================================================

void ptCurveWindow::setColorGradient(ptImage8* AImage) {
  int Width  = width();
  int Height = height();

  for (uint16_t i=0;i<Width;i++) {
    int ValueR = 0;
    int ValueG = 0;
    int ValueB = 0;
    if (i < Width/4) {
      ValueR = 255;
      ValueG = (int) (255*i/(Width/4));
    } else if (i < Width/2) {
      ValueR = 255-(int) (255*(i-Width/4)/(Width/4));
      ValueG = 255;
    } else if (i < 3*Width/4) {
      ValueG = 255-(int) (255*(i-Width/2)/(Width/4));
      ValueB = (int) (255*(i-Width/2)/(Width/4));
    } else if (i < Width) {
      ValueR = (int) (255*(i-3*Width/4)/(Width/4));
      ValueB = 255-(int) (255*(i-3*Width/4)/(Width/4));
    }
    for (uint16_t Row = Height-Height/20;
         Row <= Height-2;
         Row++) {
      AImage->m_Image[Row*Width+i][0] = ValueB;
      AImage->m_Image[Row*Width+i][1] = ValueG;
      AImage->m_Image[Row*Width+i][2] = ValueR;
    }
  }
}

//==============================================================================

void ptCurveWindow::setColorBlocks(const QColor &ATopLeftColor, const QColor &ABottomRightColor) {
  int hWidth  = this->width();
  int hHeight = this->height();

  // topleft color block
  for (uint16_t i=1;  i < 3*(hHeight-1)/10;  ++i) {
    for (uint16_t Row = 1;  Row < 2*(hWidth-1)/10+1;  ++Row) {
      int hImgIdx = Row*hWidth+i;
      FCanvas.m_Image[hImgIdx][0] = ATopLeftColor.blue();
      FCanvas.m_Image[hImgIdx][1] = ATopLeftColor.green();
      FCanvas.m_Image[hImgIdx][2] = ATopLeftColor.red();
    }
  }

  // bottom right color block
  for (uint16_t i = 7*(hHeight-1)/10+1;  i < hWidth; ++i) {
    for (uint16_t Row = 8*(hWidth-1)/10+1;  Row < hHeight;  ++Row) {
      int hImgIdx = Row*hWidth+i;
      FCanvas.m_Image[hImgIdx][0] = ABottomRightColor.blue();
      FCanvas.m_Image[hImgIdx][1] = ABottomRightColor.green();
      FCanvas.m_Image[hImgIdx][2] = ABottomRightColor.red();
    }
  }
}

//==============================================================================

void ptCurveWindow::calcCurveImage() {
  // Viewport dimensions are needed very often -> local variables for quicker access.
  int hWidth  = this->width();
  int hHeight = this->height();

  if (hHeight == 0 || hWidth == 0)
    return;

  FCanvas.setSize(hWidth, hHeight, 3);  // image is completely black afterwards

  QColor hCurveColor = QColor(200,200,200);
  QColor hGridColor  = QColor(53,53,53);

  // paint visual aids: gradient bar, colour blocks
  switch (FCurve->mask()) {
    case ptCurve::LumaMask:     setBWGradient(&FCanvas); break;
    case ptCurve::ChromaMask:   setColorGradient(&FCanvas); break;
    case ptCurve::GammaMask:    setBWGammaGradient(&FCanvas); break;
    case ptCurve::AChannelMask: setColorBlocks(QColor(200,50,100), QColor(50,150,50)); break;
    case ptCurve::BChannelMask: setColorBlocks(QColor(255,255,75), QColor(50,100,200)); break;
    case ptCurve::NoMask:       // fall through
    default:                    break; // nothing to do
  }

  // paint grid lines producing a 10×10 grid
  for (uint16_t Count = 0, Row = hHeight-1;
       Count <= 10;
       Count++, Row = hHeight-1-Count*(hHeight-1)/10)
  {
    uint32_t Temp = Row*hWidth;
    for (uint16_t i=0;i<hWidth;i++) {
      FCanvas.m_Image[Temp][0] = hGridColor.blue();
      FCanvas.m_Image[Temp][1] = hGridColor.green();
      FCanvas.m_Image[Temp][2] = hGridColor.red();
      ++Temp;
    }
  }
  for (uint16_t Count = 0, Column = 0;
       Count <= 10;
       Count++, Column = Count*(hWidth-1)/10)
  {
    uint32_t Temp = Column;
    for (uint16_t i=0;i<hHeight;i++) {
      FCanvas.m_Image[Temp][0] = hGridColor.blue();
      FCanvas.m_Image[Temp][1] = hGridColor.green();
      FCanvas.m_Image[Temp][2] = hGridColor.red();
      Temp += hWidth;
    }
  }

  if (!FCurve) return;

  // Compute curve points. The vector stores the position of the display curve (y value)
  // for each display x value. Note that coordinates origin is topleft.
  std::vector<uint16_t> hLocalCurve(hWidth);
  for (uint16_t LocalX = 0; LocalX < hWidth; ++LocalX) {
    uint16_t CurveX = (uint16_t)(0.5f + (float)LocalX / (hWidth-1) * 0xffff);
    hLocalCurve[LocalX] = hHeight-1 - (uint16_t)(0.5f + (float) FCurve->Curve[CurveX]/0xffff * (hHeight-1));
  }

  // paint the curve itself
  for (uint16_t i=0; i<hWidth; i++) {
    int32_t  Row      = hLocalCurve[i];
    int32_t  NextRow  = hLocalCurve[(i<(hWidth-1))?i+1:i];
    uint16_t kStart   = ptMin(Row,NextRow);
    uint16_t kEnd     = ptMax(Row,NextRow);
    uint32_t Temp     = i+kStart*hWidth;

    for(uint16_t k=kStart;k<=kEnd;k++) {
      FCanvas.m_Image[Temp][0] = hCurveColor.blue();
      FCanvas.m_Image[Temp][1] = hCurveColor.green();
      FCanvas.m_Image[Temp][2] = hCurveColor.red();
      Temp += hWidth;
    }
  }

  // paint anchors if we have an anchored curve
  FDisplayAnchors.clear();
  if (FCurve->curveType() == ptCurve::AnchorType) {
    for (auto hAnchor: *FCurve->anchors()) {
      int XSpot = 0.5 + hAnchor.first*(hWidth-1);
      int YSpot = 0.5 + hHeight-1 - hAnchor.second*(hHeight-1);
      FDisplayAnchors.push_back(TScreenAnchor(XSpot, YSpot));  // Remember anchors for fast UI access later

      for (int32_t Row = YSpot-3; Row < YSpot+4; ++Row) {
        if (Row >= hHeight) continue;
        if (Row <  0)       continue;
        for (int32_t Column = XSpot-3; Column < XSpot+4; ++Column) {
           if (Column >= hWidth) continue;
           if (Column <  0)      continue;
           int hImgIdx = Row*hWidth+Column;
           FCanvas.m_Image[hImgIdx][0] = hCurveColor.blue();
           FCanvas.m_Image[hImgIdx][1] = hCurveColor.green();
           FCanvas.m_Image[hImgIdx][2] = hCurveColor.red();
        }
      }
    }
  }
}

//==============================================================================

void ptCurveWindow::updateView(const std::shared_ptr<ptCurve> ANewCurve) {
  FCurve = ANewCurve;
  updateView();
}

//==============================================================================

void ptCurveWindow::updateView() {
  calcCurveImage();

  // grey out when curve windows is disabled
  if (!this->isEnabled()) {
    for (uint16_t j = 0; j < FCanvas.m_Width; j++) {
      for (uint16_t i = 0; i < FCanvas.m_Height; i++) {
        int hImgIdx = i*FCanvas.m_Width+j;
        FCanvas.m_Image[hImgIdx][0] >>= 1;
        FCanvas.m_Image[hImgIdx][1] >>= 1;
        FCanvas.m_Image[hImgIdx][2] >>= 1;
      }
    }
  }

  // Prepare the curve image for display. We take the detour QImage=>QPixmap instead of drawing
  // the QImage directly because QPixmap has HW accelerated drawing. Unfortunately the internal
  // layout of ptImage8 is not suited to be loaded into a QPixmap directly.
  FDisplayImage = QPixmap::fromImage(QImage((const uchar*) FCanvas.m_Image,
                                            FCanvas.m_Width,
                                            FCanvas.m_Height,
                                            QImage::Format_RGB32));
  repaint();
}

//==============================================================================

void ptCurveWindow::resizeEvent(QResizeEvent *) {
  updateView();
}

//==============================================================================

void ptCurveWindow::changeEvent(QEvent* Event) {
  // react on enable/disable
  if (Event->type() == QEvent::EnabledChange)
    updateView();   // No pipe request!
}

//==============================================================================

void ptCurveWindow::paintEvent(QPaintEvent*) {
  QPainter Painter(this);
  Painter.drawPixmap(0, 0, FDisplayImage);
}

//==============================================================================

void ptCurveWindow::mousePressEvent(QMouseEvent *AEvent) {
  if (!FCurve) return;
  FMouseAction  = NoAction;
  FMovingAnchor = -1;

  // only handle curves with minimum 2 anchors.
  if ((FCurve->anchorCount() < 2) || (FCurve->curveType() == ptCurve::FullPrecalcType))
    return;

  int hCaughtIdx = hasCaughtAnchor(AEvent->pos());
  if (hCaughtIdx > -1) {
    // mouse caught one of the anchors
    if (AEvent->buttons() == Qt::LeftButton) {
      // Initialize anchor dragging
      FMouseAction  = DragAction;
      FMovingAnchor = hCaughtIdx;
      return;
    }

    if ((AEvent->buttons() == Qt::RightButton) && (FCurve->anchorCount() > 2)) {
      // Delete if still more than 2 anchors
      FMouseAction = DeleteAction;
      // std::vector can only erase element via an iterator :(
      FCurve->anchors()->erase(FCurve->anchors()->begin() + hCaughtIdx);

      // handle wrap-around curves
      if (isCyclicCurve()) {
        if (hCaughtIdx == 0) {
          FCurve->anchors()->front().second = FCurve->anchors()->back().second;
        } else if (hCaughtIdx == FCurve->anchorCount()) {
          FCurve->anchors()->back().second = FCurve->anchors()->front().second;
        }
      }

      FCurve->calcCurve();
      return;
    }
  }

  if ((fabs(((double)FCurve->Curve[(uint16_t)((double)AEvent->x()/(double)width()*0xffff)]
            / (double)0xffff) - (((double)height()-(double)AEvent->y())/(double)height()))
       < CCurveDelta) && (AEvent->button() == Qt::RightButton))
  {
    // Insert a new anchor (Right mouse not on an anchor but close to the curve).
    FMouseAction   = InsertAction;
    int hInsertIdx = FDisplayAnchors.size();
    for (size_t i = 0; i < FDisplayAnchors.size(); ++i) {
      if (AEvent->x() < FDisplayAnchors.at(i).first) {
        hInsertIdx = i;
        break;
      }
    }

    double hNewX = (double)AEvent->x() / (double)this->width();
    double hNewY = (double)FCurve->Curve[(uint16_t)(hNewX*0xffff)] / (double)0xffff;
    FCurve->anchors()->insert(FCurve->anchors()->begin()+hInsertIdx, TAnchor(hNewX, hNewY));
    FCurve->calcCurve();

  } else if (AEvent->button() == Qt::RightButton) {
    // right click outside curve/anchor => context menu
    execContextMenu(AEvent->globalPos());
  }
}

//==============================================================================

void ptCurveWindow::mouseReleaseEvent(QMouseEvent*) {
  if (!FCurve) return;
  if (FMouseAction == NoAction) return;

  if (FMouseAction != DragAction)  // for drag updating is done in move event
    updateView();

  FMouseAction  = NoAction;
  FMovingAnchor = -1;

  requestPipeRun();
}

//==============================================================================

TAnchor ptCurveWindow::clampMovingAnchor(const TAnchor &APoint,
                                         const QPoint &AMousePos)
{
  auto   hMaxX = (double)(this->width()-1);
  double hNewX = APoint.first;

  if (FMovingAnchor == 0) {
    if (AMousePos.x() >= FDisplayAnchors[1].first) {
      hNewX = (FDisplayAnchors[1].first / hMaxX) - CAnchorDelta;
    }

  } else if (FMovingAnchor == FCurve->anchorCount()-1)  {
    if (AMousePos.x() <= FDisplayAnchors[FCurve->anchorCount()-2].first) {
      hNewX = (FDisplayAnchors[FCurve->anchorCount()-1].first / hMaxX) + CAnchorDelta;
    }

  } else if (AMousePos.x() >= FDisplayAnchors[FMovingAnchor+1].first) {
    hNewX = (FDisplayAnchors[FMovingAnchor+1].first / hMaxX) - CAnchorDelta;

  } else if (AMousePos.x() <= FDisplayAnchors[FMovingAnchor-1].first) {
    hNewX = (FDisplayAnchors[FMovingAnchor-1].first / hMaxX) + CAnchorDelta;
  }

  return TAnchor(ptBound(0.0, hNewX,         1.0),
                 ptBound(0.0, APoint.second, 1.0));
}

//==============================================================================

void ptCurveWindow::mouseMoveEvent(QMouseEvent *AEvent) {
  if (!FCurve) return;
  if (FMouseAction == DragAction && FMovingAnchor > -1) {
    // mouse position normalised and clamped to (0.0-1.0) and inverted y axis = curve coordinates
    TAnchor hNormPos(AEvent->x()/(double)(this->width()-1),
                              1.0 - AEvent->y()/(double)(this->height()-1) );

    // Handle mouse out of range X and Y coordinates
    hNormPos = clampMovingAnchor(hNormPos, AEvent->pos());

    (*FCurve->anchors())[FMovingAnchor] = hNormPos;

    // handle wrap-around curves
    if (isCyclicCurve()) {
      if (FMovingAnchor == 0)
        FCurve->setAnchorY(FCurve->anchorCount()-1, hNormPos.second);
      else if (FMovingAnchor == FCurve->anchorCount())
        FCurve->setAnchorY(0, hNormPos.second);
    }

    FCurve->calcCurve();
    updateView();
  }
}

//==============================================================================

void ptCurveWindow::wheelTimerExpired() {
  FMouseAction  = NoAction;
  FMovingAnchor = -1;
  requestPipeRun();
}

//==============================================================================

int ptCurveWindow::hasCaughtAnchor(const QPoint APos) {
  int hResult = -1;

  int i = 0;
  for (TScreenAnchor hAnchor: FDisplayAnchors) {
    if ((abs(hAnchor.first  - APos.x()) < CSnapDelta) &&   // snap on x axis
        (abs(hAnchor.second - APos.y()) < CSnapDelta))     // snap on y axis
    {
      hResult = i;
      break;
    }
    ++i;
  }

  return hResult;
}

//==============================================================================

bool ptCurveWindow::isCyclicCurve() {
  return FCurve->mask() == ptCurve::ChromaMask;
}

//==============================================================================

void ptCurveWindow::requestPipeRun() {
  emit valueChanged(this->objectName(), FCurve->storeConfig());
}

//==============================================================================

void ptCurveWindow::execContextMenu(const QPoint APos) {
  createMenuActions();

  // build the menu entries
  QMenu hMenu(nullptr);
  hMenu.setPalette(Theme->menuPalette());
  hMenu.setStyle(Theme->style());

  hMenu.addActions(FIpolGroup->actions());
  switch (FCurve->interpolType()) {
    case ptCurve::LinearInterpol: FLinearIpolAction->setChecked(true); break;
    case ptCurve::SplineInterpol: FSplineIpolAction->setChecked(true); break;
    case ptCurve::CosineInterpol: FCosineIpolAction->setChecked(true); break;
    default: GInfo->Raise("Unhandled curve interpolation type: " + (int)FCurve->interpolType(), AT);
  }

  if (FCurve->supportedMasks() == (ptCurve::LumaMask | ptCurve::ChromaMask)) {
    hMenu.addSeparator();
    hMenu.addActions(FMaskGroup->actions());
    switch (FCurve->mask()) {
      case ptCurve::LumaMask: FByLumaAction->setChecked(true); break;
      case ptCurve::ChromaMask: FByChromaAction->setChecked(true); break;
      default: GInfo->Raise("Unhandled curve mask type: " + (int)FCurve->mask(), AT);
    }
  }

  hMenu.exec(APos);
}

//==============================================================================

void ptCurveWindow::createMenuActions() {
  if (FByLumaAction) return;  // actions already created

  // Mask type group
  FByLumaAction = new QAction(tr("L&uminance mask"), this);
  FByLumaAction->setCheckable(true);
  connect(FByLumaAction, SIGNAL(triggered()), this, SLOT(setMaskType()));

  FByChromaAction = new QAction(tr("C&olor mask"), this);
  FByChromaAction->setCheckable(true);
  connect(FByChromaAction, SIGNAL(triggered()), this, SLOT(setMaskType()));

  FMaskGroup = new QActionGroup(this);
  FMaskGroup->addAction(FByLumaAction);
  FMaskGroup->addAction(FByChromaAction);

  // Interpolation type group
  FLinearIpolAction = new QAction(tr("&Linear"), this);
  FLinearIpolAction->setStatusTip(tr("Linear interpolation"));
  FLinearIpolAction->setCheckable(true);
  connect(FLinearIpolAction, SIGNAL(triggered()), this, SLOT(setInterpolationType()));

  FSplineIpolAction = new QAction(tr("&Spline"), this);
  FSplineIpolAction->setStatusTip(tr("Spline interpolation"));
  FSplineIpolAction->setCheckable(true);

  connect(FSplineIpolAction, SIGNAL(triggered()), this, SLOT(setInterpolationType()));
  FCosineIpolAction = new QAction(tr("&Cosine"), this);
  FCosineIpolAction->setStatusTip(tr("Cosine interpolation"));
  FCosineIpolAction->setCheckable(true);
  connect(FCosineIpolAction, SIGNAL(triggered()), this, SLOT(setInterpolationType()));

  FIpolGroup = new QActionGroup(this);
  FIpolGroup->addAction(FLinearIpolAction);
  FIpolGroup->addAction(FSplineIpolAction);
  FIpolGroup->addAction(FCosineIpolAction);
}

//==============================================================================

