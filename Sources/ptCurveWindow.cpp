/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#include "ptCurveWindow.h"
#include "ptSettings.h"
#include "ptTheme.h"

#include <iostream>
using namespace std;

extern void CB_CurveWindowManuallyChanged(const short Channel);
extern void CB_CurveWindowRecalc(const short Channel, const short ForceUpdate = 0);

extern ptTheme* Theme;

//==============================================================================

// Instantiates a (also here defined) CurveWidget,
// which acts as a central widget where the operations are finally done upon.
ptCurveWindow::ptCurveWindow(ptCurve*      ARelatedCurve,
                             const short   AChannel,
                             QWidget*      AParent,
                             const QString &ACaption /*= ""*/)
: QWidget(AParent),
  FCaptionLabel(nullptr)
{
  RelatedCurve = ARelatedCurve;
  Channel      = AChannel;

  // set up caption in topleft corner
  if (!ACaption.isEmpty()) {
    FCaptionLabel = new QLabel("<b style='color:#ffffff'>" + ACaption + "</b>", this);
    FCaptionLabel->move(5,5);

    // Make label ignore mouse events and ensure transparent background
    FCaptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    FCaptionLabel->setAttribute(Qt::WA_NoSystemBackground, true);
    FCaptionLabel->setAttribute(Qt::WA_OpaquePaintEvent, false);
  }

  // Sizing and layout related.
  QSizePolicy Policy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  Policy.setHeightForWidth(1);
  setSizePolicy(Policy);

  QVBoxLayout *Layout = new QVBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(this);
  AParent->setLayout(Layout);
  AParent->setStyleSheet("");

  // Some other dynamic members we want to have clean.
  FOverlayAnchorX = 0;
  FOverlayAnchorY = 0;
  FQPixmap        = NULL;
  Image8         = NULL;
  // Nothing moving. (Event hanpting)
  FMovingAnchor   = -1;
  FActiveAnchor   = -1;
  FMousePosX      = 0;
  FMousePosY      = 0;
  FBlockEvents    = 0;
  FRecalcNeeded   = 0;

  // Timer to delay on resize operations.
  // (avoiding excessive calculations)
  ResizeTimer = new QTimer(this);
  ResizeTimer->setSingleShot(1);
  connect(ResizeTimer,
          SIGNAL(timeout()),
          this,
          SLOT(ResizeTimerExpired()));

  // Timer for the wheel interaction
  FWheelTimer = new QTimer(this);
  FWheelTimer->setSingleShot(1);
  connect(FWheelTimer,
          SIGNAL(timeout()),
          this,
          SLOT(WheelTimerExpired()));



  FAtnAdaptive = new QAction(tr("A&daptive"), this);
  FAtnAdaptive->setStatusTip(tr("Adaptive saturation"));
  FAtnAdaptive->setCheckable(true);
  connect(FAtnAdaptive, SIGNAL(triggered()), this, SLOT(SetSatMode()));
  FAtnAbsolute = new QAction(tr("A&bsolute"), this);
  FAtnAbsolute->setStatusTip(tr("Absolute saturation"));
  FAtnAbsolute->setCheckable(true);
  connect(FAtnAbsolute, SIGNAL(triggered()), this, SLOT(SetSatMode()));

  FSatModeGroup = new QActionGroup(this);
  FSatModeGroup->addAction(FAtnAdaptive);
  FSatModeGroup->addAction(FAtnAbsolute);
  FAtnAdaptive->setChecked(Settings->GetInt("SatCurveMode")>0?true:false);
  FAtnAbsolute->setChecked(Settings->GetInt("SatCurveMode")>0?false:true);

  FAtnByLuma = new QAction(tr("By l&uminance"), this);
  FAtnByLuma->setStatusTip(tr("Mask by luminance"));
  FAtnByLuma->setCheckable(true);
  connect(FAtnByLuma, SIGNAL(triggered()), this, SLOT(SetType()));
  FAtnByChroma = new QAction(tr("By c&olor"), this);
  FAtnByChroma->setStatusTip(tr("Mask by color"));
  FAtnByChroma->setCheckable(true);
  connect(FAtnByChroma, SIGNAL(triggered()), this, SLOT(SetType()));

  FTypeGroup = new QActionGroup(this);
  FTypeGroup->addAction(FAtnByLuma);
  FTypeGroup->addAction(FAtnByChroma);
  if (Channel == ptCurveChannel_Saturation) {
    FAtnByLuma->setChecked(Settings->GetInt("SatCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("SatCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Texture) {
    FAtnByLuma->setChecked(Settings->GetInt("TextureCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("TextureCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Denoise) {
    FAtnByLuma->setChecked(Settings->GetInt("DenoiseCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("DenoiseCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Denoise2) {
    FAtnByLuma->setChecked(Settings->GetInt("Denoise2CurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("Denoise2CurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Hue) {
    FAtnByLuma->setChecked(Settings->GetInt("HueCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("HueCurveType")>0?false:true);
  } else {
    FAtnByLuma->setChecked(true);
    FAtnByChroma->setChecked(false);
  }

  FAtnITLinear = new QAction(tr("&Linear"), this);
  FAtnITLinear->setStatusTip(tr("Linear interpolation"));
  FAtnITLinear->setCheckable(true);
  connect(FAtnITLinear, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));
  FAtnITSpline = new QAction(tr("&Spline"), this);
  FAtnITSpline->setStatusTip(tr("Spline interpolation"));
  FAtnITSpline->setCheckable(true);
  connect(FAtnITSpline, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));
  FAtnITCosine = new QAction(tr("&Cosine"), this);
  FAtnITCosine->setStatusTip(tr("Cosine interpolation"));
  FAtnITCosine->setCheckable(true);
  connect(FAtnITCosine, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));

  FITGroup = new QActionGroup(this);
  FITGroup->addAction(FAtnITLinear);
  FITGroup->addAction(FAtnITSpline);
  FITGroup->addAction(FAtnITCosine);
  FAtnITLinear->setChecked(
    RelatedCurve->m_IntType==ptCurveIT_Linear?true:false);
  FAtnITSpline->setChecked(
    RelatedCurve->m_IntType==ptCurveIT_Spline?true:false);
  FAtnITCosine->setChecked(
    RelatedCurve->m_IntType==ptCurveIT_Cosine?true:false);

  // Cyclic curve
  if (Channel == ptCurveChannel_Saturation ||
      Channel == ptCurveChannel_Texture ||
      Channel == ptCurveChannel_Denoise ||
      Channel == ptCurveChannel_Denoise2 ||
      Channel == ptCurveChannel_Hue) {
    FCyclicCurve = (int)FAtnByChroma->isChecked();
  } else if (Channel == ptCurveChannel_LByHue) {
    FCyclicCurve = 1;
  } else {
    FCyclicCurve = 0;
  }
}

//==============================================================================

ptCurveWindow::~ptCurveWindow() {
  //printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
  delete FQPixmap;
  delete Image8;
}

//==============================================================================

// resizeEvent.
// Delay a resize via a timer.
// ResizeTimerExpired upon expiration.
void ptCurveWindow::resizeEvent(QResizeEvent*) {
  // Schedule the action 500ms from here to avoid multiple rescaling actions
  // during multiple resizeEvents from a window resized by the user.
  ResizeTimer->start(200); // 500 ms.
}

//==============================================================================

void ptCurveWindow::ResizeTimerExpired() {
  // m_RelatedCurve enforces update, even if it is the same image.
  UpdateView(RelatedCurve);
}

//==============================================================================

void ptCurveWindow::SetCurveState(const short state) {
  switch (Channel) {
    case ptCurveChannel_RGB :
      Settings->SetValue("CurveRGB",state); break;
    case ptCurveChannel_R :
      Settings->SetValue("CurveR",state); break;
    case ptCurveChannel_G :
      Settings->SetValue("CurveG",state); break;
    case ptCurveChannel_B :
      Settings->SetValue("CurveB",state); break;
    case ptCurveChannel_L :
      Settings->SetValue("CurveL",state); break;
    case ptCurveChannel_a :
      Settings->SetValue("CurveLa",state); break;
    case ptCurveChannel_b :
      Settings->SetValue("CurveLb",state); break;
    case ptCurveChannel_Outline :
      Settings->SetValue("CurveOutline",state); break;
    case ptCurveChannel_LByHue :
      Settings->SetValue("CurveLByHue",state); break;
    case ptCurveChannel_Hue :
      Settings->SetValue("CurveHue",state); break;
    case ptCurveChannel_Texture :
      Settings->SetValue("CurveTexture",state); break;
    case ptCurveChannel_Saturation :
      Settings->SetValue("CurveSaturation",state); break;
    case ptCurveChannel_Base :
      Settings->SetValue("BaseCurve",state); break;
    case ptCurveChannel_Base2 :
      Settings->SetValue("BaseCurve2",state); break;
    case ptCurveChannel_ShadowsHighlights :
      Settings->SetValue("CurveShadowsHighlights",state); break;
    case ptCurveChannel_Denoise :
      Settings->SetValue("CurveDenoise",state); break;
    case ptCurveChannel_Denoise2 :
      Settings->SetValue("CurveDenoise2",state); break;
    case ptCurveChannel_SpotLuma :
      // Spot curve only has one single state.
      break;
    default :
      assert(!"Unknown curve in 'SetCurveState'.");
  }
}

//==============================================================================

short ptCurveWindow::GetCurveState() {
  short State = 0;
  switch (Channel) {
    case ptCurveChannel_RGB :
      State = Settings->GetInt("CurveRGB"); break;
    case ptCurveChannel_R :
      State = Settings->GetInt("CurveR"); break;
    case ptCurveChannel_G :
      State = Settings->GetInt("CurveG"); break;
    case ptCurveChannel_B :
      State = Settings->GetInt("CurveB"); break;
    case ptCurveChannel_L :
      State = Settings->GetInt("CurveL"); break;
    case ptCurveChannel_a :
      State = Settings->GetInt("CurveLa"); break;
    case ptCurveChannel_b :
      State = Settings->GetInt("CurveLb"); break;
    case ptCurveChannel_Outline :
      State = Settings->GetInt("CurveOutline"); break;
    case ptCurveChannel_LByHue :
      State = Settings->GetInt("CurveLByHue"); break;
    case ptCurveChannel_Hue :
      State = Settings->GetInt("CurveHue"); break;
    case ptCurveChannel_Texture :
      State = Settings->GetInt("CurveTexture"); break;
    case ptCurveChannel_Saturation :
      State = Settings->GetInt("CurveSaturation"); break;
    case ptCurveChannel_Base :
      State = Settings->GetInt("BaseCurve"); break;
    case ptCurveChannel_Base2 :
      State = Settings->GetInt("BaseCurve2"); break;
    case ptCurveChannel_ShadowsHighlights :
      State = Settings->GetInt("CurveShadowsHighlights"); break;
    case ptCurveChannel_Denoise :
      State = Settings->GetInt("CurveDenoise"); break;
    case ptCurveChannel_Denoise2 :
      State = Settings->GetInt("CurveDenoise2"); break;
    case ptCurveChannel_SpotLuma :
      // Spot curve only has one single state.
      break;
    default :
      assert(!"Unknown curve in 'GetCurveState'.");
  }
  return State;
}

//==============================================================================

void ptCurveWindow::ContextMenu(QMouseEvent* event) {
  short TempSetting = GetCurveState();

  if (Channel != ptCurveChannel_Saturation &&
      !(TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None) )
    return;

  QMenu Menu(NULL);
  Menu.setStyle(Theme->style());
  Menu.setPalette(Theme->menuPalette());
  if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None)) {
    FAtnITLinear->setChecked(
      RelatedCurve->m_IntType==ptCurveIT_Linear?true:false);
    FAtnITSpline->setChecked(
      RelatedCurve->m_IntType==ptCurveIT_Spline?true:false);
    FAtnITCosine->setChecked(
      RelatedCurve->m_IntType==ptCurveIT_Cosine?true:false);
    Menu.addAction(FAtnITLinear);
    Menu.addAction(FAtnITSpline);
    Menu.addAction(FAtnITCosine);
  }
  if (Channel == ptCurveChannel_Saturation) {
    FAtnAdaptive->setChecked(Settings->GetInt("SatCurveMode")>0?true:false);
    //~ m_AtnAbsolute->setChecked(Settings->GetInt("SatCurveMode")>0?false:true);
    if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None))
      Menu.addSeparator();
    Menu.addAction(FAtnAbsolute);
    Menu.addAction(FAtnAdaptive);
    Menu.addSeparator();
    Menu.addAction(FAtnByLuma);
    Menu.addAction(FAtnByChroma);
  } else if (Channel == ptCurveChannel_Texture ||
             Channel == ptCurveChannel_Denoise ||
             Channel == ptCurveChannel_Denoise2 ||
             Channel == ptCurveChannel_Hue) {
    if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None))
      Menu.addSeparator();
    Menu.addAction(FAtnByLuma);
    Menu.addAction(FAtnByChroma);
  }

  Menu.exec(event->globalPos());
}

//==============================================================================

void ptCurveWindow::SetSatMode() {
  if (Settings->GetInt("SatCurveMode") == (int)FAtnAdaptive->isChecked())
    return;
  Settings->SetValue("SatCurveMode",(int)FAtnAdaptive->isChecked());

  if (Settings->GetInt("CurveSaturation")==ptCurveChoice_None) return;
  if (FBlockEvents) return;

  FBlockEvents  = 1;
  CB_CurveWindowRecalc(Channel);
  FRecalcNeeded = 0;
  FBlockEvents  = 0;

  return;
}

//==============================================================================

void ptCurveWindow::SetType() {
  if (Channel == ptCurveChannel_Saturation) {
    if (Settings->GetInt("SatCurveType") == (int)FAtnByLuma->isChecked())
      return;
    Settings->SetValue("SatCurveType",(int)FAtnByLuma->isChecked());
  } else if (Channel == ptCurveChannel_Texture) {
    if (Settings->GetInt("TextureCurveType") == (int)FAtnByLuma->isChecked())
      return;
    Settings->SetValue("TextureCurveType",(int)FAtnByLuma->isChecked());
  } else if (Channel == ptCurveChannel_Denoise) {
    if (Settings->GetInt("DenoiseCurveType") == (int)FAtnByLuma->isChecked())
      return;
    Settings->SetValue("DenoiseCurveType",(int)FAtnByLuma->isChecked());
  } else if (Channel == ptCurveChannel_Denoise2) {
    if (Settings->GetInt("Denoise2CurveType") == (int)FAtnByLuma->isChecked())
      return;
    Settings->SetValue("Denoise2CurveType",(int)FAtnByLuma->isChecked());
  } else if (Channel == ptCurveChannel_Hue) {
    if (Settings->GetInt("HueCurveType") == (int)FAtnByLuma->isChecked())
      return;
    Settings->SetValue("HueCurveType",(int)FAtnByLuma->isChecked());
  }
  if (Channel == ptCurveChannel_Saturation ||
      Channel == ptCurveChannel_Texture ||
      Channel == ptCurveChannel_Denoise ||
      Channel == ptCurveChannel_Denoise2 ||
      Channel == ptCurveChannel_Hue) {
    FCyclicCurve = (int)FAtnByChroma->isChecked();
    if (FCyclicCurve == 1) {
      RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors-1] = RelatedCurve->m_YAnchor[0];
      RelatedCurve->SetCurveFromAnchors();
    }
  }
  CalculateCurve();
  UpdateView();

  if (Settings->GetInt("CurveSaturation")==ptCurveChoice_None &&
      Channel == ptCurveChannel_Saturation) return;
  if (Settings->GetInt("CurveTexture")==ptCurveChoice_None &&
      Channel == ptCurveChannel_Texture) return;
  if (Settings->GetInt("CurveDenoise")==ptCurveChoice_None &&
      Channel == ptCurveChannel_Denoise) return;
  if (Settings->GetInt("CurveDenoise2")==ptCurveChoice_None &&
      Channel == ptCurveChannel_Denoise2) return;
  if (Settings->GetInt("CurveHue")==ptCurveChoice_None &&
      Channel == ptCurveChannel_Hue) return;
  if (FBlockEvents) return;

  FBlockEvents  = 1;
  CB_CurveWindowRecalc(Channel);
  FRecalcNeeded = 0;
  FBlockEvents  = 0;

  return;
}

//==============================================================================

void ptCurveWindow::SetInterpolationType() {
  short Temp = 0;
  if ((int)FAtnITLinear->isChecked())
    Temp = ptCurveIT_Linear;
  if ((int)FAtnITSpline->isChecked())
    Temp = ptCurveIT_Spline;
  if ((int)FAtnITCosine->isChecked())
    Temp = ptCurveIT_Cosine;
  if (RelatedCurve->m_IntType==Temp) return;

  RelatedCurve->m_IntType = Temp;

  if (FBlockEvents) return;

  FBlockEvents  = 1;
  RelatedCurve->SetCurveFromAnchors();
  UpdateView();
  CB_CurveWindowRecalc(Channel);
  FRecalcNeeded = 0;
  FBlockEvents  = 0;

  return;
}

//==============================================================================

void ptCurveWindow::SetBWGradient(ptImage8* Image) {
  int Width  = width();
  int Height = height();

  for (uint16_t i=0;i<Width;i++) {
    int Value = (int)(i/(float)Width*255);
    for (uint16_t Row = Height-Height/20;
         Row <= Height-2;
         Row++) {
      Image->m_Image[Row*Width+i][0] = Value;
      Image->m_Image[Row*Width+i][1] = Value;
      Image->m_Image[Row*Width+i][2] = Value;
    }
  }
}

//==============================================================================

void ptCurveWindow::SetBWGammaGradient(ptImage8* Image) {
  int Width  = width();
  int Height = height();

  for (uint16_t i=0;i<Width;i++) {
    int Value = (int)(powf(i/(float)Width,0.45f)*255);
    for (uint16_t Row = Height-Height/20;
         Row <= Height-2;
         Row++) {
      Image->m_Image[Row*Width+i][0] = Value;
      Image->m_Image[Row*Width+i][1] = Value;
      Image->m_Image[Row*Width+i][2] = Value;
    }
  }
}

//==============================================================================

void ptCurveWindow::SetColorGradient(ptImage8* Image) {
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
      Image->m_Image[Row*Width+i][0] = ValueB;
      Image->m_Image[Row*Width+i][1] = ValueG;
      Image->m_Image[Row*Width+i][2] = ValueR;
    }
  }
}

//==============================================================================

void ptCurveWindow::CalculateCurve() {
  if (!RelatedCurve) return;

  if (Channel == ptCurveChannel_Saturation) {
    FAtnByLuma->setChecked(Settings->GetInt("SatCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("SatCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Texture) {
    FAtnByLuma->setChecked(Settings->GetInt("TextureCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("TextureCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Denoise) {
    FAtnByLuma->setChecked(Settings->GetInt("DenoiseCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("DenoiseCurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Denoise2) {
    FAtnByLuma->setChecked(Settings->GetInt("Denoise2CurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("Denoise2CurveType")>0?false:true);
  } else if (Channel == ptCurveChannel_Hue) {
    FAtnByLuma->setChecked(Settings->GetInt("HueCurveType")>0?true:false);
    FAtnByChroma->setChecked(Settings->GetInt("HueCurveType")>0?false:true);
  }

  int Width  = width();
  int Height = height();

  uint16_t LocalCurve[Width];

  // Zero the LocalCurve
  memset(LocalCurve,0,sizeof(LocalCurve));

  // Compute it. Take already the Y axis going down into account.
  for (uint16_t x=0; x<Width; x++) {
    uint16_t CurveX = (uint16_t)( 0.5 + (float)x/(Width-1) * 0xffff );
    LocalCurve[x] = Height-1- (uint16_t)
     ( 0.5 + (float) RelatedCurve->m_Curve[CurveX]/0xffff * (Height-1));
  }

  delete Image8;
  Image8 = new ptImage8(Width,Height,3);

  // Colors from the palette should give more consistent results.
  QColor FGColor = QColor(200,200,200);//palette().color(QPalette::WindowText);
//  QColor BGColor = QColor(0,0,0);//palette().color(QPalette::Window);
  QColor MColor  = QColor(53,53,53);//palette().color(QPalette::Mid);

  // Gradient for saturation curve
  if (Channel == ptCurveChannel_Saturation) {
    if ((int)FAtnByLuma->isChecked() == 1) {
      SetBWGradient(Image8);
    } else {
      SetColorGradient(Image8);
    }
  } else if (Channel == ptCurveChannel_Texture ||
             Channel == ptCurveChannel_Denoise ||
             Channel == ptCurveChannel_Denoise2 ||
             Channel == ptCurveChannel_Hue) {
    if ((int)FAtnByLuma->isChecked() == 1) {
      SetBWGradient(Image8);
    } else {
      SetColorGradient(Image8);
    }
  } else if (Channel == ptCurveChannel_LByHue) {
    SetColorGradient(Image8);
  } else if (Channel == ptCurveChannel_a) {
    for (uint16_t i=1;i < 3*(Height-1)/10;i++) {
      for (uint16_t Row = 1;
           Row < 2*(Width-1)/10+1;
           Row++) {
        Image8->m_Image[Row*Width+i][0] = 100;
        Image8->m_Image[Row*Width+i][1] = 50;
        Image8->m_Image[Row*Width+i][2] = 200;
      }
    }
    for (uint16_t i = 7*(Height-1)/10+1;i < Width;i++) {
      for (uint16_t Row = 8*(Width-1)/10+1;
           Row < Height;
           Row++) {
        Image8->m_Image[Row*Width+i][0] = 50;
        Image8->m_Image[Row*Width+i][1] = 150;
        Image8->m_Image[Row*Width+i][2] = 50;
      }
    }
  } else if (Channel == ptCurveChannel_b) {
    for (uint16_t i=1;i < 3*(Height-1)/10;i++) {
      for (uint16_t Row = 1;
           Row < 2*(Width-1)/10+1;
           Row++) {
        Image8->m_Image[Row*Width+i][0] = 75;
        Image8->m_Image[Row*Width+i][1] = 255;
        Image8->m_Image[Row*Width+i][2] = 255;
      }
    }
    for (uint16_t i = 7*(Height-1)/10+1;i < Width;i++) {
      for (uint16_t Row = 8*(Width-1)/10+1;
           Row < Height;
           Row++) {
        Image8->m_Image[Row*Width+i][0] = 200;
        Image8->m_Image[Row*Width+i][1] = 100;
        Image8->m_Image[Row*Width+i][2] = 50;
      }
    }
  } else if (Channel == ptCurveChannel_L ||
             Channel == ptCurveChannel_Outline ||
             Channel == ptCurveChannel_Base2 ||
             Channel == ptCurveChannel_SpotLuma) {
    SetBWGradient(Image8);
  } else if (Channel == ptCurveChannel_RGB ||
             Channel == ptCurveChannel_Base) {
    SetBWGammaGradient(Image8);
  }


  // Grid lines.
  for (uint16_t Count = 0, Row = Height-1;
       Count <= 10;
       Count++, Row = Height-1-Count*(Height-1)/10) {
    uint32_t Temp = Row*Width;
    for (uint16_t i=0;i<Width;i++) {
      Image8->m_Image[Temp][0] = MColor.blue();
      Image8->m_Image[Temp][1] = MColor.green();
      Image8->m_Image[Temp][2] = MColor.red();
      ++Temp;
    }
  }
  for (uint16_t Count = 0, Column = 0;
       Count <= 10;
       Count++, Column = Count*(Width-1)/10) {
    uint32_t Temp = Column;
    for (uint16_t i=0;i<Height;i++) {
      Image8->m_Image[Temp][0] = MColor.blue();
      Image8->m_Image[Temp][1] = MColor.green();
      Image8->m_Image[Temp][2] = MColor.red();
      Temp += Width;
    }
  }

  // The curve
  for (uint16_t i=0; i<Width; i++) {
    int32_t Row      = LocalCurve[i];
    int32_t NextRow  = LocalCurve[(i<(Width-1))?i+1:i];
    uint16_t kStart = MIN(Row,NextRow);
    uint16_t kEnd   = MAX(Row,NextRow);
    uint32_t Temp = i+kStart*Width;
    for(uint16_t k=kStart;k<=kEnd;k++) {
      Image8->m_Image[Temp][0] = FGColor.blue();
      Image8->m_Image[Temp][1] = FGColor.green();
      Image8->m_Image[Temp][2] = FGColor.red();
      Temp += Width;
    }
  }

  // Anchors in case of anchored curve.
  if (RelatedCurve->m_Type == ptCurveType_Anchor) {
    for (short Anchor=0;Anchor<RelatedCurve->m_NrAnchors;Anchor++) {
      int32_t XSpot =
        (uint16_t) (.5 + RelatedCurve->m_XAnchor[Anchor]*(Width-1));
      int32_t YSpot =
        (uint16_t) (.5 + Height-1-RelatedCurve->m_YAnchor[Anchor]*(Height-1));
      // Remember it for faster event detection.
      FXSpot[Anchor] = XSpot;
      FYSpot[Anchor] = YSpot;
      for (int32_t Row=YSpot-3; Row<YSpot+4 ; Row++) {
        if (Row>=Height) continue;
        if (Row<0) continue;
        for (int32_t Column=XSpot-3; Column<XSpot+4; Column++) {
           if (Column>=Width) continue;
           if (Column<0) continue;
           Image8->m_Image[Row*Width+Column][0] = FGColor.blue();
           Image8->m_Image[Row*Width+Column][1] = FGColor.green();
           Image8->m_Image[Row*Width+Column][2] = FGColor.red();
        }
      }
    }
  }

  // If we have an anchor moving around, show it too.
  if (FMovingAnchor != -1) {
    for (int32_t Row=FOverlayAnchorY-3; Row<FOverlayAnchorY+4 ; Row++) {
      if (Row>=Height) continue;
      if (Row<0) continue;
      for (int32_t Column=FOverlayAnchorX-3; Column<FOverlayAnchorX+4;
           Column++) {
        if (Column>=Width) continue;
        if (Column<0) continue;
        Image8->m_Image[Row*Width+Column][0] = FGColor.blue();
        Image8->m_Image[Row*Width+Column][1] = FGColor.green();
        Image8->m_Image[Row*Width+Column][2] = FGColor.red();
      }
    }
  }

}

//==============================================================================

void ptCurveWindow::UpdateView(ptCurve* NewRelatedCurve) {
  if (NewRelatedCurve) RelatedCurve = NewRelatedCurve;
  if (!RelatedCurve) return;

  CalculateCurve();

  // The detour QImage=>QPixmap is needed to enjoy
  // HW acceleration of QPixmap.
  if (!Image8) return;
  if (isEnabled()==0) {
    for (uint16_t j = 0; j < Image8->m_Width; j++) {
      for (uint16_t i = 0; i < Image8->m_Height; i++) {
        Image8->m_Image[i*Image8->m_Width+j][0] >>= 1;
        Image8->m_Image[i*Image8->m_Width+j][1] >>= 1;
        Image8->m_Image[i*Image8->m_Width+j][2] >>= 1;
      }
    }
  }

  delete FQPixmap;
  FQPixmap = new QPixmap(
   QPixmap::fromImage(QImage((const uchar*) Image8->m_Image,
                             Image8->m_Width,
                             Image8->m_Height,
                             QImage::Format_RGB32)));
  repaint();
}

//==============================================================================


void ptCurveWindow::changeEvent(QEvent* Event) {
  if (Event->type() == QEvent::EnabledChange)
    UpdateView(RelatedCurve);
}

//==============================================================================

// Just draw the previously constructed m_QPixmap.
void ptCurveWindow::paintEvent(QPaintEvent*) {
  QPainter Painter(this);
  Painter.save();
  if (FQPixmap) Painter.drawPixmap(0,0,*FQPixmap);
  Painter.restore();
}

//==============================================================================

// How many pixels will be considered as 'bingo' for having the anchor ?
const short SnapDelta = 6;
// Percentage to be close to a curve to get a new Anchor
const double CurveDelta = 0.12;
// Distance to the next anchor
const float Delta = 0.005;

// Implements part of the anchors creation/deletion/moving.
void ptCurveWindow::mousePressEvent(QMouseEvent *Event) {
  // Reset the wheel status
  if (FActiveAnchor != -1 &&
      (abs((int)FMousePosX-Event->x())>2 || abs((int)FMousePosY-Event->y())>2)) {
    FActiveAnchor = -1;
    setMouseTracking(false);
    FMousePosX    = 0;
    FMousePosY    = 0;
  }

  if (FBlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (RelatedCurve->m_NrAnchors < 2) return;

  short Xd;
  short Yd;
  short i;
  short Snapped  = 0; // got an anchor?
  short XSnapped = 0; // in the same x range of an anchor?

  // Did we snap one of the anchors ?
  for (i=0; i<RelatedCurve->m_NrAnchors; i++) {
    Xd =  abs(FXSpot[i]-Event->x());
    Yd =  abs(FYSpot[i]-Event->y());
    if (Xd<SnapDelta) XSnapped = 1;
    if (XSnapped && (Yd<SnapDelta)) {
      Snapped = 1;
      if (Event->buttons() == 1) { // Left mouse. Start moving.
        FMovingAnchor = i;
        FRecalcNeeded = 1;
      }
      if (Event->buttons() == 2) { // Right mouse. Delete.
        // Delete indeed if still more than 2 anchors.
        if (RelatedCurve->m_NrAnchors > 2) {
          short j;
          for (j=i;j<RelatedCurve->m_NrAnchors-1;j++) {
            RelatedCurve->m_XAnchor[j]=RelatedCurve->m_XAnchor[j+1];
            RelatedCurve->m_YAnchor[j]=RelatedCurve->m_YAnchor[j+1];
          }
          RelatedCurve->m_NrAnchors--;
          if (i == 0 && FCyclicCurve == 1) {
            RelatedCurve->m_YAnchor[0] = RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors-1];
          } else if (i == RelatedCurve->m_NrAnchors && FCyclicCurve == 1)  {
            RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors-1] = RelatedCurve->m_YAnchor[0];
          }
          RelatedCurve->SetCurveFromAnchors();
          // Notify we have a manual curve now ...
          SetCurveState(ptCurveChoice_Manual);
          UpdateView();
          FRecalcNeeded = 1;
        }
      }
      break;
    }
  }

  // Insert a new anchor ? (Right mouse but not on an anchor).
  if (RelatedCurve->m_NrAnchors < ptMaxAnchors &&
      !XSnapped && Event->buttons() == 2 &&
      // Close to the curve or far away?
      fabs(((double)RelatedCurve->
            m_Curve[(int32_t)((double)Event->x()/(double)width()*0xffff)]
             /(double)0xffff) -
            (((double)height()-(double)Event->y())/(double)height())) < CurveDelta) {
    // Find out where to insert. (Initially the average of the
    // neighbouring anchors).
    if (Event->x() < FXSpot[0]) {
      for (short j=RelatedCurve->m_NrAnchors; j>0 ; j--) {
        RelatedCurve->m_XAnchor[j] = RelatedCurve->m_XAnchor[j-1];
        RelatedCurve->m_YAnchor[j] = RelatedCurve->m_YAnchor[j-1];
      }
      RelatedCurve->m_XAnchor[0] = (double)Event->x()/(double)width();
      RelatedCurve->m_YAnchor[0] =
        RelatedCurve->m_Curve[
          (int32_t)(RelatedCurve->m_XAnchor[0]*0xffff)]/(double)0xffff;
      FRecalcNeeded = 1;
    } else if (Event->x() > FXSpot[RelatedCurve->m_NrAnchors-1]) {
      RelatedCurve->m_XAnchor[RelatedCurve->m_NrAnchors] =
        (double)Event->x()/(double)width();
      RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors] =
        RelatedCurve->m_Curve[(int32_t)(RelatedCurve->m_XAnchor[
          RelatedCurve->m_NrAnchors]*0xffff)]/(double)0xffff;
      FRecalcNeeded = 1;
    } else {
      for (i=0; i<RelatedCurve->m_NrAnchors-1; i++) {
        if (Event->x()>FXSpot[i] && Event->x()<FXSpot[i+1]) {
          for (short j=RelatedCurve->m_NrAnchors; j>i+1 ; j--) {
            RelatedCurve->m_XAnchor[j] = RelatedCurve->m_XAnchor[j-1];
            RelatedCurve->m_YAnchor[j] = RelatedCurve->m_YAnchor[j-1];
          }
          RelatedCurve->m_XAnchor[i+1] =
              (double)Event->x()/(double)width();
          RelatedCurve->m_YAnchor[i+1] =
            RelatedCurve->m_Curve[
              (int32_t)(RelatedCurve->m_XAnchor[i+1]*0xffff)]/(double)0xffff;
          break;
        }
      }
      if ((int)FAtnITCosine->isChecked()) FRecalcNeeded = 1;
    }
    RelatedCurve->m_NrAnchors++;
    RelatedCurve->SetCurveFromAnchors();
    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);
    UpdateView();
  } else if (!Snapped && Event->buttons() == 2) {
    // no other interaction needed -> show ContextMenu
    ContextMenu((QMouseEvent*)Event);
  }
  return;
}

//==============================================================================

void ptCurveWindow::wheelEvent(QWheelEvent *Event) {
  if (FBlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (RelatedCurve->m_NrAnchors < 2) return;

  // anchor moving?
  if (FMovingAnchor != -1) return;

  // we use the mose tracking to disable the wheel event
  // with the next mouse move.
  setMouseTracking(true);

  int Width  = width();
  int Height = height();
  short Xd;
  short Yd;

  if (FActiveAnchor == -1) {
    // Did we snap one of the anchors ?
    for (short i = 0; i < RelatedCurve->m_NrAnchors; i++) {
      Xd = abs(FXSpot[i]-Event->x());
      Yd = abs(FYSpot[i]-Event->y());
      if ((Xd<SnapDelta) && (Yd<SnapDelta)) {
        FActiveAnchor = i;
        FMousePosX    = Event->x();
        FMousePosY    = Event->y();
        break;
      }
    }
  }

  if (FActiveAnchor != -1) {
    float Temp = 0;
    if (((QMouseEvent*)Event)->modifiers() & Qt::AltModifier) {
      Temp = RelatedCurve->m_XAnchor[FActiveAnchor];
      if (((QMouseEvent*)Event)->modifiers() & Qt::ControlModifier) {
        Temp = Temp + Event->delta()/(float) Width/30.0f;
      } else {
        Temp = Temp + Event->delta()/(float) Width/120.0f;
      }
      // compare with neighbours
      if (FActiveAnchor < RelatedCurve->m_NrAnchors - 1)
        Temp = MIN(Temp, (FXSpot[FActiveAnchor+1]/(float) (Width-1)) - Delta);
      if (FActiveAnchor > 0)
        Temp = MAX(Temp, (FXSpot[FActiveAnchor-1]/(float) (Width-1)) + Delta);
    } else {
      Temp = RelatedCurve->m_YAnchor[FActiveAnchor];
      if (((QMouseEvent*)Event)->modifiers() & Qt::ControlModifier) {
        Temp = Temp + Event->delta()/(float) Height/30.0f;
      } else {
        Temp = Temp + Event->delta()/(float) Height/120.0f;
      }
    }
    // out of range?
    Temp = MAX(0.0, Temp);
    Temp = MIN(1.0, Temp);

    if (((QMouseEvent*)Event)->modifiers() & Qt::AltModifier) {
      RelatedCurve->m_XAnchor[FActiveAnchor] = Temp;
    } else {
      RelatedCurve->m_YAnchor[FActiveAnchor] = Temp;
    }

    RelatedCurve->SetCurveFromAnchors();

    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);

    FOverlayAnchorX = (int32_t) (RelatedCurve->m_XAnchor[FActiveAnchor]*(Width-1));
    FOverlayAnchorY = (int32_t) ((1.0 - RelatedCurve->m_YAnchor[FActiveAnchor]) * (Height-1));
    UpdateView();
    FWheelTimer->start(200);
  }
}

void ptCurveWindow::WheelTimerExpired() {
  FBlockEvents  = 1;
  CB_CurveWindowManuallyChanged(Channel);
  FRecalcNeeded = 0;
  FBlockEvents  = 0;
}

//==============================================================================

void ptCurveWindow::mouseMoveEvent(QMouseEvent *Event) {
  // Reset the wheel status
  if (FActiveAnchor != -1 &&
      (abs((int)FMousePosX-Event->x())>2 || abs((int)FMousePosY-Event->y())>2)) {
    FActiveAnchor = -1;
    setMouseTracking(false);
    FMousePosX    = 0;
    FMousePosY    = 0;
  }

  if (FBlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (RelatedCurve->m_NrAnchors < 2) return;

  int Width  = width();
  int Height = height();

  if (FMovingAnchor != -1) {
    double X =  Event->x()/(double) (Width-1);
    double Y = 1.0 - Event->y()/(double)(Height-1);

    // Handle mouse out of range X coordinate
    if (FMovingAnchor == 0) {
      if (Event->x() >= FXSpot[1]) {
        X = (FXSpot[1]/(double) (Width-1)) - Delta;
      }
      X = MAX(0.0, X);
    } else if (FMovingAnchor == RelatedCurve->m_NrAnchors-1)  {
      if (Event->x()<=FXSpot[RelatedCurve->m_NrAnchors-2]) {
        X = (FXSpot[RelatedCurve->m_NrAnchors-2]/(double) (Width-1)) + Delta;
      }
      X=MIN(1.0,X);
    } else if (Event->x()>=FXSpot[FMovingAnchor+1]) {
      X = (FXSpot[FMovingAnchor+1]/(double) (Width-1)) - Delta;
    } else if (Event->x()<=FXSpot[FMovingAnchor-1]) {
      X = (FXSpot[FMovingAnchor-1]/(double) (Width-1)) + Delta;
    }
    Y = MAX(0.0, Y);  // Handle mouse out of range Y coordinate
    Y = MIN(1.0, Y);

    if (FMovingAnchor == 0 && FCyclicCurve == 1) {
      RelatedCurve->m_XAnchor[0] = X;
      RelatedCurve->m_YAnchor[0] = Y;
      RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors-1] = Y;
    } else if (FMovingAnchor == RelatedCurve->m_NrAnchors-1 && FCyclicCurve == 1)  {
      RelatedCurve->m_XAnchor[RelatedCurve->m_NrAnchors-1] = X;
      RelatedCurve->m_YAnchor[RelatedCurve->m_NrAnchors-1] = Y;
      RelatedCurve->m_YAnchor[0] = Y;
    } else {
      RelatedCurve->m_XAnchor[FMovingAnchor] = X;
      RelatedCurve->m_YAnchor[FMovingAnchor] = Y;
    }
    RelatedCurve->SetCurveFromAnchors();

    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);

    FOverlayAnchorX = (int32_t) (X*(Width-1));
    FOverlayAnchorY = (int32_t) ((1.0 - Y) * (Height-1));
    UpdateView();
  }
  return;
}

//==============================================================================

// Install the newly placed anchor and finalize.
void ptCurveWindow::mouseReleaseEvent(QMouseEvent*) {

  if (FBlockEvents) return;

  FMovingAnchor = -1;
  // This recalculates the image at release of the button.
  // As this takes time we block further events on this one
  // at least.
  if (FRecalcNeeded) {
    FBlockEvents  = 1;
    CB_CurveWindowManuallyChanged(Channel);
    FRecalcNeeded = 0;
    FBlockEvents  = 0;
  }
  return;
}

//==============================================================================

