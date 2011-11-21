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

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
// Instantiates a (also here defined) CurveWidget,
// which acts as a central widget where the operations are finally done upon.
//
////////////////////////////////////////////////////////////////////////////////

ptCurveWindow::ptCurveWindow(ptCurve*    RelatedCurve,
                             const short Channel,
                             QWidget*    Parent)

  :QWidget(NULL) {

  m_RelatedCurve = RelatedCurve;
  m_Channel      = Channel;

  // Sizing and layout related.

  QSizePolicy Policy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  Policy.setHeightForWidth(1);
  setSizePolicy(Policy);

  QVBoxLayout *Layout = new QVBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(this);
  Parent->setLayout(Layout);
  m_Parent = Parent;
  m_Parent->setStyleSheet("");

  // Some other dynamic members we want to have clean.
  m_OverlayAnchorX = 0;
  m_OverlayAnchorY = 0;
  m_QPixmap        = NULL;
  m_Image8         = NULL;
  // Nothing moving. (Event hanpting)
  m_MovingAnchor   = -1;
  m_ActiveAnchor   = -1;
  m_MousePosX      = 0;
  m_MousePosY      = 0;
  m_BlockEvents    = 0;
  m_RecalcNeeded   = 0;

  // Timer to delay on resize operations.
  // (avoiding excessive calculations)
  m_ResizeTimer = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);
  connect(m_ResizeTimer,
          SIGNAL(timeout()),
          this,
          SLOT(ResizeTimerExpired()));

  // Timer for the wheel interaction
  m_WheelTimer = new QTimer(this);
  m_WheelTimer->setSingleShot(1);
  connect(m_WheelTimer,
          SIGNAL(timeout()),
          this,
          SLOT(WheelTimerExpired()));



  m_AtnAdaptive = new QAction(tr("A&daptive"), this);
  m_AtnAdaptive->setStatusTip(tr("Adaptive saturation"));
  m_AtnAdaptive->setCheckable(true);
  connect(m_AtnAdaptive, SIGNAL(triggered()), this, SLOT(SetSatMode()));
  m_AtnAbsolute = new QAction(tr("A&bsolute"), this);
  m_AtnAbsolute->setStatusTip(tr("Absolute saturation"));
  m_AtnAbsolute->setCheckable(true);
  connect(m_AtnAbsolute, SIGNAL(triggered()), this, SLOT(SetSatMode()));

  m_SatModeGroup = new QActionGroup(this);
  m_SatModeGroup->addAction(m_AtnAdaptive);
  m_SatModeGroup->addAction(m_AtnAbsolute);
  m_AtnAdaptive->setChecked(Settings->GetInt("SatCurveMode")>0?true:false);
  m_AtnAbsolute->setChecked(Settings->GetInt("SatCurveMode")>0?false:true);

  m_AtnByLuma = new QAction(tr("By l&uminance"), this);
  m_AtnByLuma->setStatusTip(tr("Mask by luminance"));
  m_AtnByLuma->setCheckable(true);
  connect(m_AtnByLuma, SIGNAL(triggered()), this, SLOT(SetType()));
  m_AtnByChroma = new QAction(tr("By c&olor"), this);
  m_AtnByChroma->setStatusTip(tr("Mask by color"));
  m_AtnByChroma->setCheckable(true);
  connect(m_AtnByChroma, SIGNAL(triggered()), this, SLOT(SetType()));

  m_TypeGroup = new QActionGroup(this);
  m_TypeGroup->addAction(m_AtnByLuma);
  m_TypeGroup->addAction(m_AtnByChroma);
  if (m_Channel == ptCurveChannel_Saturation) {
    m_AtnByLuma->setChecked(Settings->GetInt("SatCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("SatCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Texture) {
    m_AtnByLuma->setChecked(Settings->GetInt("TextureCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("TextureCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Denoise) {
    m_AtnByLuma->setChecked(Settings->GetInt("DenoiseCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("DenoiseCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Denoise2) {
    m_AtnByLuma->setChecked(Settings->GetInt("Denoise2CurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("Denoise2CurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Hue) {
    m_AtnByLuma->setChecked(Settings->GetInt("HueCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("HueCurveType")>0?false:true);
  } else {
    m_AtnByLuma->setChecked(true);
    m_AtnByChroma->setChecked(false);
  }

  m_AtnITLinear = new QAction(tr("&Linear"), this);
  m_AtnITLinear->setStatusTip(tr("Linear interpolation"));
  m_AtnITLinear->setCheckable(true);
  connect(m_AtnITLinear, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));
  m_AtnITSpline = new QAction(tr("&Spline"), this);
  m_AtnITSpline->setStatusTip(tr("Spline interpolation"));
  m_AtnITSpline->setCheckable(true);
  connect(m_AtnITSpline, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));
  m_AtnITCosine = new QAction(tr("&Cosine"), this);
  m_AtnITCosine->setStatusTip(tr("Cosine interpolation"));
  m_AtnITCosine->setCheckable(true);
  connect(m_AtnITCosine, SIGNAL(triggered()), this, SLOT(SetInterpolationType()));

  m_ITGroup = new QActionGroup(this);
  m_ITGroup->addAction(m_AtnITLinear);
  m_ITGroup->addAction(m_AtnITSpline);
  m_ITGroup->addAction(m_AtnITCosine);
  m_AtnITLinear->setChecked(
    m_RelatedCurve->m_IntType==ptCurveIT_Linear?true:false);
  m_AtnITSpline->setChecked(
    m_RelatedCurve->m_IntType==ptCurveIT_Spline?true:false);
  m_AtnITCosine->setChecked(
    m_RelatedCurve->m_IntType==ptCurveIT_Cosine?true:false);

  // Cyclic curve
  if (m_Channel == ptCurveChannel_Saturation ||
      m_Channel == ptCurveChannel_Texture ||
      m_Channel == ptCurveChannel_Denoise ||
      m_Channel == ptCurveChannel_Denoise2 ||
      m_Channel == ptCurveChannel_Hue) {
    m_CyclicCurve = (int)m_AtnByChroma->isChecked();
  } else if (m_Channel == ptCurveChannel_LByHue) {
    m_CyclicCurve = 1;
  } else {
    m_CyclicCurve = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptCurveWindow::~ptCurveWindow() {
  //printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
  delete m_QPixmap;
  delete m_Image8;
}

////////////////////////////////////////////////////////////////////////////////
//
// resizeEvent.
// Delay a resize via a timer.
// ResizeTimerExpired upon expiration.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::resizeEvent(QResizeEvent*) {
  // Schedule the action 500ms from here to avoid multiple rescaling actions
  // during multiple resizeEvents from a window resized by the user.
  m_ResizeTimer->start(200); // 500 ms.
}

void ptCurveWindow::ResizeTimerExpired() {
  // m_RelatedCurve enforces update, even if it is the same image.
  UpdateView(m_RelatedCurve);
}

////////////////////////////////////////////////////////////////////////////////
//
// Set Curve State
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::SetCurveState(const short state) {
  switch (m_Channel) {
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
    default :
      assert(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Get Curve State
//
////////////////////////////////////////////////////////////////////////////////

short ptCurveWindow::GetCurveState() {
  short State = 0;
  switch (m_Channel) {
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
    default :
      assert(0);
  }
  return State;
}

////////////////////////////////////////////////////////////////////////////////
//
// Context menu and related
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::ContextMenu(QMouseEvent* event) {
  short TempSetting = GetCurveState();

  if (m_Channel != ptCurveChannel_Saturation &&
      !(TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None) )
    return;

  QMenu Menu(NULL);
  Menu.setStyle(Theme->style());
  Menu.setPalette(Theme->menuPalette());
  if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None)) {
    m_AtnITLinear->setChecked(
      m_RelatedCurve->m_IntType==ptCurveIT_Linear?true:false);
    m_AtnITSpline->setChecked(
      m_RelatedCurve->m_IntType==ptCurveIT_Spline?true:false);
    m_AtnITCosine->setChecked(
      m_RelatedCurve->m_IntType==ptCurveIT_Cosine?true:false);
    Menu.addAction(m_AtnITLinear);
    Menu.addAction(m_AtnITSpline);
    Menu.addAction(m_AtnITCosine);
  }
  if (m_Channel == ptCurveChannel_Saturation) {
    m_AtnAdaptive->setChecked(Settings->GetInt("SatCurveMode")>0?true:false);
    //~ m_AtnAbsolute->setChecked(Settings->GetInt("SatCurveMode")>0?false:true);
    if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None))
      Menu.addSeparator();
    Menu.addAction(m_AtnAbsolute);
    Menu.addAction(m_AtnAdaptive);
    Menu.addSeparator();
    Menu.addAction(m_AtnByLuma);
    Menu.addAction(m_AtnByChroma);
  } else if (m_Channel == ptCurveChannel_Texture ||
             m_Channel == ptCurveChannel_Denoise ||
             m_Channel == ptCurveChannel_Denoise2 ||
             m_Channel == ptCurveChannel_Hue) {
    if ((TempSetting==ptCurveChoice_Manual ||
        TempSetting==ptCurveChoice_None))
      Menu.addSeparator();
    Menu.addAction(m_AtnByLuma);
    Menu.addAction(m_AtnByChroma);
  }

  Menu.exec(event->globalPos());
}

void ptCurveWindow::SetSatMode() {
  if (Settings->GetInt("SatCurveMode") == (int)m_AtnAdaptive->isChecked())
    return;
  Settings->SetValue("SatCurveMode",(int)m_AtnAdaptive->isChecked());

  if (Settings->GetInt("CurveSaturation")==ptCurveChoice_None) return;
  if (m_BlockEvents) return;

  m_BlockEvents  = 1;
  CB_CurveWindowRecalc(m_Channel);
  m_RecalcNeeded = 0;
  m_BlockEvents  = 0;

  return;
}

void ptCurveWindow::SetType() {
  if (m_Channel == ptCurveChannel_Saturation) {
    if (Settings->GetInt("SatCurveType") == (int)m_AtnByLuma->isChecked())
      return;
    Settings->SetValue("SatCurveType",(int)m_AtnByLuma->isChecked());
  } else if (m_Channel == ptCurveChannel_Texture) {
    if (Settings->GetInt("TextureCurveType") == (int)m_AtnByLuma->isChecked())
      return;
    Settings->SetValue("TextureCurveType",(int)m_AtnByLuma->isChecked());
  } else if (m_Channel == ptCurveChannel_Denoise) {
    if (Settings->GetInt("DenoiseCurveType") == (int)m_AtnByLuma->isChecked())
      return;
    Settings->SetValue("DenoiseCurveType",(int)m_AtnByLuma->isChecked());
  } else if (m_Channel == ptCurveChannel_Denoise2) {
    if (Settings->GetInt("Denoise2CurveType") == (int)m_AtnByLuma->isChecked())
      return;
    Settings->SetValue("Denoise2CurveType",(int)m_AtnByLuma->isChecked());
  } else if (m_Channel == ptCurveChannel_Hue) {
    if (Settings->GetInt("HueCurveType") == (int)m_AtnByLuma->isChecked())
      return;
    Settings->SetValue("HueCurveType",(int)m_AtnByLuma->isChecked());
  }
  if (m_Channel == ptCurveChannel_Saturation ||
      m_Channel == ptCurveChannel_Texture ||
      m_Channel == ptCurveChannel_Denoise ||
      m_Channel == ptCurveChannel_Denoise2 ||
      m_Channel == ptCurveChannel_Hue) {
    m_CyclicCurve = (int)m_AtnByChroma->isChecked();
    if (m_CyclicCurve == 1) {
      m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors-1] = m_RelatedCurve->m_YAnchor[0];
      m_RelatedCurve->SetCurveFromAnchors();
    }
  }
  CalculateCurve();
  UpdateView();

  if (Settings->GetInt("CurveSaturation")==ptCurveChoice_None &&
      m_Channel == ptCurveChannel_Saturation) return;
  if (Settings->GetInt("CurveTexture")==ptCurveChoice_None &&
      m_Channel == ptCurveChannel_Texture) return;
  if (Settings->GetInt("CurveDenoise")==ptCurveChoice_None &&
      m_Channel == ptCurveChannel_Denoise) return;
  if (Settings->GetInt("CurveDenoise2")==ptCurveChoice_None &&
      m_Channel == ptCurveChannel_Denoise2) return;
  if (Settings->GetInt("CurveHue")==ptCurveChoice_None &&
      m_Channel == ptCurveChannel_Hue) return;
  if (m_BlockEvents) return;

  m_BlockEvents  = 1;
  CB_CurveWindowRecalc(m_Channel);
  m_RecalcNeeded = 0;
  m_BlockEvents  = 0;

  return;
}

void ptCurveWindow::SetInterpolationType() {
  short Temp = 0;
  if ((int)m_AtnITLinear->isChecked())
    Temp = ptCurveIT_Linear;
  if ((int)m_AtnITSpline->isChecked())
    Temp = ptCurveIT_Spline;
  if ((int)m_AtnITCosine->isChecked())
    Temp = ptCurveIT_Cosine;
  if (m_RelatedCurve->m_IntType==Temp) return;

  m_RelatedCurve->m_IntType = Temp;

  if (m_BlockEvents) return;

  m_BlockEvents  = 1;
  m_RelatedCurve->SetCurveFromAnchors();
  UpdateView();
  CB_CurveWindowRecalc(m_Channel);
  m_RecalcNeeded = 0;
  m_BlockEvents  = 0;

  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// CalculateCurve.
//
// Calculates the curve into an m_Image8.
//
////////////////////////////////////////////////////////////////////////////////

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


void ptCurveWindow::CalculateCurve() {

  if (!m_RelatedCurve) return;

  if (m_Channel == ptCurveChannel_Saturation) {
    m_AtnByLuma->setChecked(Settings->GetInt("SatCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("SatCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Texture) {
    m_AtnByLuma->setChecked(Settings->GetInt("TextureCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("TextureCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Denoise) {
    m_AtnByLuma->setChecked(Settings->GetInt("DenoiseCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("DenoiseCurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Denoise2) {
    m_AtnByLuma->setChecked(Settings->GetInt("Denoise2CurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("Denoise2CurveType")>0?false:true);
  } else if (m_Channel == ptCurveChannel_Hue) {
    m_AtnByLuma->setChecked(Settings->GetInt("HueCurveType")>0?true:false);
    m_AtnByChroma->setChecked(Settings->GetInt("HueCurveType")>0?false:true);
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
     ( 0.5 + (float) m_RelatedCurve->m_Curve[CurveX]/0xffff * (Height-1));
  }

  delete m_Image8;
  m_Image8 = new ptImage8(Width,Height,3);

  // Colors from the palette should give more consistent results.
  QColor FGColor = QColor(200,200,200);//palette().color(QPalette::WindowText);
  QColor BGColor = QColor(0,0,0);//palette().color(QPalette::Window);
  QColor MColor  = QColor(53,53,53);//palette().color(QPalette::Mid);

  // Gradient for saturation curve
  if (m_Channel == ptCurveChannel_Saturation) {
    if ((int)m_AtnByLuma->isChecked() == 1) {
      SetBWGradient(m_Image8);
    } else {
      SetColorGradient(m_Image8);
    }
  } else if (m_Channel == ptCurveChannel_Texture ||
             m_Channel == ptCurveChannel_Denoise ||
             m_Channel == ptCurveChannel_Denoise2 ||
             m_Channel == ptCurveChannel_Hue) {
    if ((int)m_AtnByLuma->isChecked() == 1) {
      SetBWGradient(m_Image8);
    } else {
      SetColorGradient(m_Image8);
    }
  } else if (m_Channel == ptCurveChannel_LByHue) {
    SetColorGradient(m_Image8);
  } else if (m_Channel == ptCurveChannel_a) {
    for (uint16_t i=1;i < 3*(Height-1)/10;i++) {
      for (uint16_t Row = 1;
           Row < 2*(Width-1)/10+1;
           Row++) {
        m_Image8->m_Image[Row*Width+i][0] = 100;
        m_Image8->m_Image[Row*Width+i][1] = 50;
        m_Image8->m_Image[Row*Width+i][2] = 200;
      }
    }
    for (uint16_t i = 7*(Height-1)/10+1;i < Width;i++) {
      for (uint16_t Row = 8*(Width-1)/10+1;
           Row < Height;
           Row++) {
        m_Image8->m_Image[Row*Width+i][0] = 50;
        m_Image8->m_Image[Row*Width+i][1] = 150;
        m_Image8->m_Image[Row*Width+i][2] = 50;
      }
    }
  } else if (m_Channel == ptCurveChannel_b) {
    for (uint16_t i=1;i < 3*(Height-1)/10;i++) {
      for (uint16_t Row = 1;
           Row < 2*(Width-1)/10+1;
           Row++) {
        m_Image8->m_Image[Row*Width+i][0] = 75;
        m_Image8->m_Image[Row*Width+i][1] = 255;
        m_Image8->m_Image[Row*Width+i][2] = 255;
      }
    }
    for (uint16_t i = 7*(Height-1)/10+1;i < Width;i++) {
      for (uint16_t Row = 8*(Width-1)/10+1;
           Row < Height;
           Row++) {
        m_Image8->m_Image[Row*Width+i][0] = 200;
        m_Image8->m_Image[Row*Width+i][1] = 100;
        m_Image8->m_Image[Row*Width+i][2] = 50;
      }
    }
  } else if (m_Channel == ptCurveChannel_L ||
             m_Channel == ptCurveChannel_Outline ||
             m_Channel == ptCurveChannel_Base2) {
    SetBWGradient(m_Image8);
  } else if (m_Channel == ptCurveChannel_RGB ||
             m_Channel == ptCurveChannel_Base) {
    SetBWGammaGradient(m_Image8);
  }


  // Grid lines.
  for (uint16_t Count = 0, Row = Height-1;
       Count <= 10;
       Count++, Row = Height-1-Count*(Height-1)/10) {
    uint32_t Temp = Row*Width;
    for (uint16_t i=0;i<Width;i++) {
      m_Image8->m_Image[Temp][0] = MColor.blue();
      m_Image8->m_Image[Temp][1] = MColor.green();
      m_Image8->m_Image[Temp][2] = MColor.red();
      ++Temp;
    }
  }
  for (uint16_t Count = 0, Column = 0;
       Count <= 10;
       Count++, Column = Count*(Width-1)/10) {
    uint32_t Temp = Column;
    for (uint16_t i=0;i<Height;i++) {
      m_Image8->m_Image[Temp][0] = MColor.blue();
      m_Image8->m_Image[Temp][1] = MColor.green();
      m_Image8->m_Image[Temp][2] = MColor.red();
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
      m_Image8->m_Image[Temp][0] = FGColor.blue();
      m_Image8->m_Image[Temp][1] = FGColor.green();
      m_Image8->m_Image[Temp][2] = FGColor.red();
      Temp += Width;
    }
  }

  // Anchors in case of anchored curve.
  if (m_RelatedCurve->m_Type == ptCurveType_Anchor) {
    for (short Anchor=0;Anchor<m_RelatedCurve->m_NrAnchors;Anchor++) {
      int32_t XSpot =
        (uint16_t) (.5 + m_RelatedCurve->m_XAnchor[Anchor]*(Width-1));
      int32_t YSpot =
        (uint16_t) (.5 + Height-1-m_RelatedCurve->m_YAnchor[Anchor]*(Height-1));
      // Remember it for faster event detection.
      m_XSpot[Anchor] = XSpot;
      m_YSpot[Anchor] = YSpot;
      for (int32_t Row=YSpot-3; Row<YSpot+4 ; Row++) {
        if (Row>=Height) continue;
        if (Row<0) continue;
        for (int32_t Column=XSpot-3; Column<XSpot+4; Column++) {
           if (Column>=Width) continue;
           if (Column<0) continue;
           m_Image8->m_Image[Row*Width+Column][0] = FGColor.blue();
           m_Image8->m_Image[Row*Width+Column][1] = FGColor.green();
           m_Image8->m_Image[Row*Width+Column][2] = FGColor.red();
        }
      }
    }
  }

  // If we have an anchor moving around, show it too.
  if (m_MovingAnchor != -1) {
    for (int32_t Row=m_OverlayAnchorY-3; Row<m_OverlayAnchorY+4 ; Row++) {
      if (Row>=Height) continue;
      if (Row<0) continue;
      for (int32_t Column=m_OverlayAnchorX-3; Column<m_OverlayAnchorX+4;
           Column++) {
        if (Column>=Width) continue;
        if (Column<0) continue;
        m_Image8->m_Image[Row*Width+Column][0] = FGColor.blue();
        m_Image8->m_Image[Row*Width+Column][1] = FGColor.green();
        m_Image8->m_Image[Row*Width+Column][2] = FGColor.red();
      }
    }
  }

}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateView.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::UpdateView(ptCurve* NewRelatedCurve) {

  if (NewRelatedCurve) m_RelatedCurve = NewRelatedCurve;
  if (!m_RelatedCurve) return;

  CalculateCurve();

  // The detour QImage=>QPixmap is needed to enjoy
  // HW acceleration of QPixmap.
  if (!m_Image8) return;
  if (isEnabled()==0) {
    for (uint16_t j = 0; j < m_Image8->m_Width; j++) {
      for (uint16_t i = 0; i < m_Image8->m_Height; i++) {
        m_Image8->m_Image[i*m_Image8->m_Width+j][0] >>= 1;
        m_Image8->m_Image[i*m_Image8->m_Width+j][1] >>= 1;
        m_Image8->m_Image[i*m_Image8->m_Width+j][2] >>= 1;
      }
    }
  }

  delete m_QPixmap;
  m_QPixmap = new QPixmap(
   QPixmap::fromImage(QImage((const uchar*) m_Image8->m_Image,
                             m_Image8->m_Width,
                             m_Image8->m_Height,
                             QImage::Format_RGB32)));
  repaint();
}

////////////////////////////////////////////////////////////////////////////////
//
// changeEvent handler.
// To react on enable/disable
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::changeEvent(QEvent* Event) {
  if (Event->type() == QEvent::EnabledChange)
    UpdateView(m_RelatedCurve);
}

////////////////////////////////////////////////////////////////////////////////
//
// paintEvent handler.
// Just draw the previously constructed m_QPixmap.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::paintEvent(QPaintEvent*) {
  QPainter Painter(this);
  Painter.save();
  if (m_QPixmap) Painter.drawPixmap(0,0,*m_QPixmap);
  Painter.restore();
}

// How many pixels will be considered as 'bingo' for having the anchor ?
const short SnapDelta = 6;
// Percentage to be close to a curve to get a new Anchor
const double CurveDelta = 0.12;
// Distance to the next anchor
const float Delta = 0.005;

////////////////////////////////////////////////////////////////////////////////
//
// mousePressEvent handler.
// Implements part of the anchors creation/deletion/moving.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::mousePressEvent(QMouseEvent *Event) {

  // Reset the wheel status
  if (m_ActiveAnchor != -1 &&
      (abs((int)m_MousePosX-Event->x())>2 || abs((int)m_MousePosY-Event->y())>2)) {
    m_ActiveAnchor = -1;
    setMouseTracking(false);
    m_MousePosX    = 0;
    m_MousePosY    = 0;
  }

  if (m_BlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (m_RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (m_RelatedCurve->m_NrAnchors < 2) return;

  short Xd;
  short Yd;
  short i;
  short Snapped  = 0; // got an anchor?
  short XSnapped = 0; // in the same x range of an anchor?

  // Did we snap one of the anchors ?
  for (i=0; i<m_RelatedCurve->m_NrAnchors; i++) {
    Xd =  abs(m_XSpot[i]-Event->x());
    Yd =  abs(m_YSpot[i]-Event->y());
    if (Xd<SnapDelta) XSnapped = 1;
    if (XSnapped && (Yd<SnapDelta)) {
      Snapped = 1;
      if (Event->buttons() == 1) { // Left mouse. Start moving.
        m_MovingAnchor = i;
        m_RecalcNeeded = 1;
      }
      if (Event->buttons() == 2) { // Right mouse. Delete.
        // Delete indeed if still more than 2 anchors.
        if (m_RelatedCurve->m_NrAnchors > 2) {
          short j;
          for (j=i;j<m_RelatedCurve->m_NrAnchors-1;j++) {
            m_RelatedCurve->m_XAnchor[j]=m_RelatedCurve->m_XAnchor[j+1];
            m_RelatedCurve->m_YAnchor[j]=m_RelatedCurve->m_YAnchor[j+1];
          }
          m_RelatedCurve->m_NrAnchors--;
          if (i == 0 && m_CyclicCurve == 1) {
            m_RelatedCurve->m_YAnchor[0] = m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors-1];
          } else if (i == m_RelatedCurve->m_NrAnchors && m_CyclicCurve == 1)  {
            m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors-1] = m_RelatedCurve->m_YAnchor[0];
          }
          m_RelatedCurve->SetCurveFromAnchors();
          // Notify we have a manual curve now ...
          SetCurveState(ptCurveChoice_Manual);
          UpdateView();
          m_RecalcNeeded = 1;
        }
      }
      break;
    }
  }

  // Insert a new anchor ? (Right mouse but not on an anchor).
  if (m_RelatedCurve->m_NrAnchors < ptMaxAnchors &&
      !XSnapped && Event->buttons() == 2 &&
      // Close to the curve or far away?
      fabs(((double)m_RelatedCurve->
            m_Curve[(int32_t)((double)Event->x()/(double)width()*0xffff)]
             /(double)0xffff) -
            (((double)height()-(double)Event->y())/(double)height())) < CurveDelta) {
    // Find out where to insert. (Initially the average of the
    // neighbouring anchors).
    if (Event->x() < m_XSpot[0]) {
      for (short j=m_RelatedCurve->m_NrAnchors; j>0 ; j--) {
        m_RelatedCurve->m_XAnchor[j] = m_RelatedCurve->m_XAnchor[j-1];
        m_RelatedCurve->m_YAnchor[j] = m_RelatedCurve->m_YAnchor[j-1];
      }
      m_RelatedCurve->m_XAnchor[0] = (double)Event->x()/(double)width();
      m_RelatedCurve->m_YAnchor[0] =
        m_RelatedCurve->m_Curve[
          (int32_t)(m_RelatedCurve->m_XAnchor[0]*0xffff)]/(double)0xffff;
      m_RecalcNeeded = 1;
    } else if (Event->x() > m_XSpot[m_RelatedCurve->m_NrAnchors-1]) {
      m_RelatedCurve->m_XAnchor[m_RelatedCurve->m_NrAnchors] =
        (double)Event->x()/(double)width();
      m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors] =
        m_RelatedCurve->m_Curve[(int32_t)(m_RelatedCurve->m_XAnchor[
          m_RelatedCurve->m_NrAnchors]*0xffff)]/(double)0xffff;
      m_RecalcNeeded = 1;
    } else {
      for (i=0; i<m_RelatedCurve->m_NrAnchors-1; i++) {
        if (Event->x()>m_XSpot[i] && Event->x()<m_XSpot[i+1]) {
          for (short j=m_RelatedCurve->m_NrAnchors; j>i+1 ; j--) {
            m_RelatedCurve->m_XAnchor[j] = m_RelatedCurve->m_XAnchor[j-1];
            m_RelatedCurve->m_YAnchor[j] = m_RelatedCurve->m_YAnchor[j-1];
          }
          m_RelatedCurve->m_XAnchor[i+1] =
              (double)Event->x()/(double)width();
          m_RelatedCurve->m_YAnchor[i+1] =
            m_RelatedCurve->m_Curve[
              (int32_t)(m_RelatedCurve->m_XAnchor[i+1]*0xffff)]/(double)0xffff;
          break;
        }
      }
      if ((int)m_AtnITCosine->isChecked()) m_RecalcNeeded = 1;
    }
    m_RelatedCurve->m_NrAnchors++;
    m_RelatedCurve->SetCurveFromAnchors();
    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);
    UpdateView();
  } else if (!Snapped && Event->buttons() == 2) {
    // no other interaction needed -> show ContextMenu
    ContextMenu((QMouseEvent*)Event);
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// mouseWheelEvent handler
// together with the expired timer
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::wheelEvent(QWheelEvent *Event) {
  if (m_BlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (m_RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (m_RelatedCurve->m_NrAnchors < 2) return;

  // anchor moving?
  if (m_MovingAnchor != -1) return;

  // we use the mose tracking to disable the wheel event
  // with the next mouse move.
  setMouseTracking(true);

  int Width  = width();
  int Height = height();
  short Xd;
  short Yd;

  if (m_ActiveAnchor == -1) {
    // Did we snap one of the anchors ?
    for (short i = 0; i < m_RelatedCurve->m_NrAnchors; i++) {
      Xd = abs(m_XSpot[i]-Event->x());
      Yd = abs(m_YSpot[i]-Event->y());
      if ((Xd<SnapDelta) && (Yd<SnapDelta)) {
        m_ActiveAnchor = i;
        m_MousePosX    = Event->x();
        m_MousePosY    = Event->y();
        break;
      }
    }
  }

  if (m_ActiveAnchor != -1) {
    float Temp = 0;
    if (((QMouseEvent*)Event)->modifiers() & Qt::AltModifier) {
      Temp = m_RelatedCurve->m_XAnchor[m_ActiveAnchor];
      if (((QMouseEvent*)Event)->modifiers() & Qt::ControlModifier) {
        Temp = Temp + Event->delta()/(float) Width/30.0f;
      } else {
        Temp = Temp + Event->delta()/(float) Width/120.0f;
      }
      // compare with neighbours
      if (m_ActiveAnchor < m_RelatedCurve->m_NrAnchors - 1)
        Temp = MIN(Temp, (m_XSpot[m_ActiveAnchor+1]/(float) (Width-1)) - Delta);
      if (m_ActiveAnchor > 0)
        Temp = MAX(Temp, (m_XSpot[m_ActiveAnchor-1]/(float) (Width-1)) + Delta);
    } else {
      Temp = m_RelatedCurve->m_YAnchor[m_ActiveAnchor];
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
      m_RelatedCurve->m_XAnchor[m_ActiveAnchor] = Temp;
    } else {
      m_RelatedCurve->m_YAnchor[m_ActiveAnchor] = Temp;
    }

    m_RelatedCurve->SetCurveFromAnchors();

    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);

    m_OverlayAnchorX = (int32_t) (m_RelatedCurve->m_XAnchor[m_ActiveAnchor]*(Width-1));
    m_OverlayAnchorY = (int32_t) ((1.0 - m_RelatedCurve->m_YAnchor[m_ActiveAnchor]) * (Height-1));
    UpdateView();
    m_WheelTimer->start(200);
  }
}

void ptCurveWindow::WheelTimerExpired() {
  m_BlockEvents  = 1;
  CB_CurveWindowManuallyChanged(m_Channel);
  m_RecalcNeeded = 0;
  m_BlockEvents  = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// mouseMoveEvent handler.
// Move anchor around.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::mouseMoveEvent(QMouseEvent *Event) {

  // Reset the wheel status
  if (m_ActiveAnchor != -1 &&
      (abs((int)m_MousePosX-Event->x())>2 || abs((int)m_MousePosY-Event->y())>2)) {
    m_ActiveAnchor = -1;
    setMouseTracking(false);
    m_MousePosX    = 0;
    m_MousePosY    = 0;
  }

  if (m_BlockEvents) return;

  // Do only hanlde curves with minimum 2 anchors.
  if (m_RelatedCurve->m_Type != ptCurveType_Anchor) return;
  if (m_RelatedCurve->m_NrAnchors < 2) return;

  int Width  = width();
  int Height = height();

  if (m_MovingAnchor != -1) {
    double X =  Event->x()/(double) (Width-1);
    double Y = 1.0 - Event->y()/(double)(Height-1);

    // Handle mouse out of range X coordinate
    if (m_MovingAnchor == 0) {
      if (Event->x() >= m_XSpot[1]) {
        X = (m_XSpot[1]/(double) (Width-1)) - Delta;
      }
      X = MAX(0.0, X);
    } else if (m_MovingAnchor == m_RelatedCurve->m_NrAnchors-1)  {
      if (Event->x()<=m_XSpot[m_RelatedCurve->m_NrAnchors-2]) {
        X = (m_XSpot[m_RelatedCurve->m_NrAnchors-2]/(double) (Width-1)) + Delta;
      }
      X=MIN(1.0,X);
    } else if (Event->x()>=m_XSpot[m_MovingAnchor+1]) {
      X = (m_XSpot[m_MovingAnchor+1]/(double) (Width-1)) - Delta;
    } else if (Event->x()<=m_XSpot[m_MovingAnchor-1]) {
      X = (m_XSpot[m_MovingAnchor-1]/(double) (Width-1)) + Delta;
    }
    Y = MAX(0.0, Y);  // Handle mouse out of range Y coordinate
    Y = MIN(1.0, Y);

    if (m_MovingAnchor == 0 && m_CyclicCurve == 1) {
      m_RelatedCurve->m_XAnchor[0] = X;
      m_RelatedCurve->m_YAnchor[0] = Y;
      m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors-1] = Y;
    } else if (m_MovingAnchor == m_RelatedCurve->m_NrAnchors-1 && m_CyclicCurve == 1)  {
      m_RelatedCurve->m_XAnchor[m_RelatedCurve->m_NrAnchors-1] = X;
      m_RelatedCurve->m_YAnchor[m_RelatedCurve->m_NrAnchors-1] = Y;
      m_RelatedCurve->m_YAnchor[0] = Y;
    } else {
      m_RelatedCurve->m_XAnchor[m_MovingAnchor] = X;
      m_RelatedCurve->m_YAnchor[m_MovingAnchor] = Y;
    }
    m_RelatedCurve->SetCurveFromAnchors();

    // Notify we have a manual curve now ...
    SetCurveState(ptCurveChoice_Manual);

    m_OverlayAnchorX = (int32_t) (X*(Width-1));
    m_OverlayAnchorY = (int32_t) ((1.0 - Y) * (Height-1));
    UpdateView();
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// mouseReleaseEvent handler.
// Install the newly placed anchor and finalize.
//
////////////////////////////////////////////////////////////////////////////////

void ptCurveWindow::mouseReleaseEvent(QMouseEvent*) {

  if (m_BlockEvents) return;

  m_MovingAnchor = -1;
  // This recalculates the image at release of the button.
  // As this takes time we block further events on this one
  // at least.
  if (m_RecalcNeeded) {
    m_BlockEvents  = 1;
    CB_CurveWindowManuallyChanged(m_Channel);
    m_RecalcNeeded = 0;
    m_BlockEvents  = 0;
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////
