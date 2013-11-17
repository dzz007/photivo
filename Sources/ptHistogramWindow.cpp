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

#include "ptSettings.h"
#include "ptConstants.h"
#include "ptHistogramWindow.h"
#include "ptTheme.h"
#include "ptImage.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QContextMenuEvent>
#include <QPainter>

#include <iostream>

#ifdef _OPENMP
  #include <omp.h>
#endif

extern ptTheme* Theme;

// Lut
extern float ToFloatTable[0x10000];

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
// Instantiates a (also here defined) HistogramWidget,
// which acts as a central widget where the operations are finally done upon.
//
////////////////////////////////////////////////////////////////////////////////

ptHistogramWindow::ptHistogramWindow(const ptImage* RelatedImage,
                                           QWidget* Parent)
: QWidget(nullptr)
{
  m_RelatedImage = RelatedImage; // don't delete that at cleanup !
  // Some other dynamic members we want to have clean.
  m_QPixmap      = NULL;

  m_PreviousHistogramGamma = -1;
  m_PreviousHistogramLogX  = -1;
  m_PreviousHistogramLogY  = -1;
  m_PreviousFileName       = "";

  // Sizing and layout related.
  QSizePolicy Policy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  Policy.setHeightForWidth(1);
  setSizePolicy(Policy);

  QVBoxLayout *Layout = new QVBoxLayout;
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->addWidget(this);
  Parent->setLayout(Layout);

  // Timer to delay on resize operations.
  // (avoiding excessive calculations)
  m_ResizeTimer = new QTimer(this);
  m_ResizeTimer->setSingleShot(1);
  connect(m_ResizeTimer,
          SIGNAL(timeout()),
          this,
          SLOT(ResizeTimerExpired()));

  // Create actions for context menu
  m_AtnLnX = new QAction(tr("Logarithmic &X axis"), this);
  m_AtnLnX->setCheckable(true);
  m_AtnLnX->setChecked(Settings->GetInt("HistogramLogX"));
  connect(m_AtnLnX, SIGNAL(triggered()), this, SLOT(MenuLnX()));

  m_AtnLnY = new QAction(tr("Logarithmic &Y axis"), this);
  m_AtnLnY->setCheckable(true);
  m_AtnLnY->setChecked(Settings->GetInt("HistogramLogY"));
  connect(m_AtnLnY, SIGNAL(triggered()), this, SLOT(MenuLnY()));

  m_AtnCrop = new QAction(tr("&Selection"), this);
  m_AtnCrop->setStatusTip(tr("Histogram only on a part of the image"));
  m_AtnCrop->setCheckable(true);
  if (Settings->GetInt("HistogramCrop"))
    m_AtnCrop->setChecked(1);
  else
    m_AtnCrop->setChecked(0);
  connect(m_AtnCrop, SIGNAL(triggered()), this, SLOT(MenuCrop()));

  m_AtnLinear = new QAction(tr("&Linear"), this);
  m_AtnLinear->setStatusTip(tr("Use data from linear pipe"));
  m_AtnLinear->setCheckable(true);
  connect(m_AtnLinear, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnPreview = new QAction(tr("&Preview"), this);
  m_AtnPreview->setStatusTip(tr("Use data with preview profile"));
  m_AtnPreview->setCheckable(true);
  connect(m_AtnPreview, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnOutput = new QAction(tr("&Output"), this);
  m_AtnOutput->setStatusTip(tr("Use data with output profile"));
  m_AtnOutput->setCheckable(true);
  connect(m_AtnOutput, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_ModeGroup = new QActionGroup(this);
  m_ModeGroup->addAction(m_AtnLinear);
  m_ModeGroup->addAction(m_AtnPreview);
  m_ModeGroup->addAction(m_AtnOutput);

  if (Settings->GetInt("HistogramMode")==ptHistogramMode_Linear)
    m_AtnLinear->setChecked(true);
  else if (Settings->GetInt("HistogramMode")==ptHistogramMode_Output)
    m_AtnOutput->setChecked(true);
  else
    m_AtnPreview->setChecked(true);

  m_AtnRGB = new QAction(tr("RGB"), this);
  m_AtnRGB->setStatusTip(tr("RGB"));
  m_AtnRGB->setCheckable(true);
  connect(m_AtnRGB, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_AtnR = new QAction(tr("&R"), this);
  m_AtnR->setStatusTip(tr("R"));
  m_AtnR->setCheckable(true);
  connect(m_AtnR, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_AtnG = new QAction(tr("&G"), this);
  m_AtnG->setStatusTip(tr("G"));
  m_AtnG->setCheckable(true);
  connect(m_AtnG, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_AtnB = new QAction(tr("&B"), this);
  m_AtnB->setStatusTip(tr("B"));
  m_AtnB->setCheckable(true);
  connect(m_AtnB, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_ChannelGroup = new QActionGroup(this);
  m_ChannelGroup->addAction(m_AtnRGB);
  m_ChannelGroup->addAction(m_AtnR);
  m_ChannelGroup->addAction(m_AtnG);
  m_ChannelGroup->addAction(m_AtnB);

  if (Settings->GetInt("HistogramChannel")==ptHistogramChannel_RGB)
    m_AtnRGB->setChecked(true);
  else if (Settings->GetInt("HistogramChannel")==ptHistogramChannel_R)
    m_AtnR->setChecked(true);
  else if (Settings->GetInt("HistogramChannel")==ptHistogramChannel_G)
    m_AtnG->setChecked(true);
  else
    m_AtnB->setChecked(true);

  m_LookUp = NULL;
  FillLookUp();

  InitOverlay();
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptHistogramWindow::~ptHistogramWindow() {
  delete m_QPixmap;
  delete m_LookUp;
  delete m_PixelInfoR;
  delete m_PixelInfoG;
  delete m_PixelInfoB;
  delete m_PixelInfoTimer;
  delete m_OverlayPalette;
  delete m_InfoIcon;
}


////////////////////////////////////////////////////////////////////////////////
//
// public Init
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::Init() {
  ResizeTimerExpired();
}

//==============================================================================

void ptHistogramWindow::PixelInfo(const QString R, const QString G, const QString B) {
  m_PixelInfoR->setText("R: " + R);
  m_PixelInfoR->adjustSize();
  m_PixelInfoR->show();

  m_PixelInfoG->setText("G: " + G);
  m_PixelInfoG->adjustSize();
  m_PixelInfoG->show();

  m_PixelInfoB->setText("B: " + B);
  m_PixelInfoB->adjustSize();
  m_PixelInfoB->show();
}

//==============================================================================

void ptHistogramWindow::setInfoIconState(bool AIsActive)
{
  m_InfoIcon->setVisible(AIsActive);
}

////////////////////////////////////////////////////////////////////////////////
//
// resizeEvent.
// Delay a resize via a timer.
// ResizeTimerExpired upon expiration.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::resizeEvent(QResizeEvent*) {
  // Schedule the action 500ms from here to avoid multiple rescaling actions
  // during multiple resizeEvents from a window resized by the user.
  m_ResizeTimer->start(25); // 25 ms.
}

void ptHistogramWindow::ResizeTimerExpired() {
  m_InfoIcon->move(width() - 20, 4);

  // Create side effect for recalibrating the maximum
  m_PreviousHistogramGamma = -1;

  FillLookUp();
  // m_RelatedImage enforces update, even if it is the same image.
  UpdateView(m_RelatedImage);
}

////////////////////////////////////////////////////////////////////////////////
//
// CalculateHistogram.
//
// Calculates the histogram into an m_Image8.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::CalculateHistogram() {

  // QTime Timer;
  // Timer.start();

  int WidgetWidth  = width();
  int WidgetHeight = height();

  const uint16_t HistogramMargin = 5;
  const uint16_t HistogramWidth  = WidgetWidth-2*HistogramMargin;

  uint32_t Histogram[3][HistogramWidth];
  // Zero the Histogram
  memset(Histogram,0,sizeof(Histogram));

  // MaxColor (We want only the luminance in LAB).
  short MaxColor = (m_RelatedImage->m_ColorSpace==ptSpace_Lab)?1:3;

  // Average of ideal linear histogram.
  uint32_t HistoAverage =
    m_RelatedImage->m_Width*m_RelatedImage->m_Height/HistogramWidth;

  //printf("(%s,%d) %d\n",__FILE__,__LINE__,Timer.elapsed());
  // Calculate Histogram
  const uint16_t Width  = m_RelatedImage->m_Width;
  const uint16_t Height = m_RelatedImage->m_Height;
  const int32_t Size   = Width*Height;
  const short HistogramGamma = Settings->GetInt("HistogramMode");

  for (int32_t i=0; i<(int32_t) Size; i++) {
    for (short c=0;c<MaxColor;c++) {
      Histogram[c][m_LookUp[m_RelatedImage->m_Image[i][c]]]++;
    }
  }

  // Logaritmic variants.
  const short HistogramLogX = Settings->GetInt("HistogramLogX");
  if (HistogramLogX) {
    uint32_t LogHistogram[3][HistogramWidth];
    // Zero the Histogram
    memset(LogHistogram,0,sizeof(LogHistogram));
    for (uint16_t k=1;k<HistogramWidth;k++) {
      for (short c=0; c<MaxColor; c++ ) {
        short Index = (short)(log(k)/log(HistogramWidth)*HistogramWidth);
        LogHistogram[c][Index] += Histogram[c][k];
      }
    }
    memcpy(Histogram,LogHistogram,sizeof(LogHistogram));
  }

  // Logaritmic variants.
  const short HistogramLogY = Settings->GetInt("HistogramLogY");
  if (HistogramLogY) {
    for (uint16_t k=0;k<HistogramWidth;k++) {
      for (short c=0; c<MaxColor; c++ ) {
        // sqrt(10)-1 such that average arrives on 50%
        Histogram[c][k] = (uint32_t)
          //4096 is rather random scaler. Later rescaled.
          (4096* log10f(1+2.16227766/HistoAverage*Histogram[c][k]));
      }
    }
    // On 50% per construction.
    HistoAverage = 2048;
  }

  // Maximum : only recalculated in a few cases : changed setting or file.
  QString FileName = (Settings->GetStringList("InputFileNameList"))[0];
  if ((HistogramGamma != m_PreviousHistogramGamma)  ||
      (HistogramLogX  != m_PreviousHistogramLogX)   ||
      (HistogramLogY  != m_PreviousHistogramLogY)   ||
      (FileName       != m_PreviousFileName)) {

    m_PreviousHistogramGamma = HistogramGamma;
    m_PreviousHistogramLogX  = HistogramLogX;
    m_PreviousHistogramLogY  = HistogramLogY;
    m_PreviousFileName       = FileName;

    m_HistoMax = 0;
    // Remark that we don't take the extremes of the x-axis
    // into account. Typical clipping region.
    uint16_t LeftEnd = -1;
    uint16_t Value = 0;
    while (Value == 0) {
      LeftEnd += 1;
      for (short c=0; c<MaxColor; c++)
        Value += Histogram[c][LeftEnd];
      if (LeftEnd==HistogramWidth-1) Value = 1;
    }
    uint16_t RightEnd = HistogramWidth;
    Value = 0;
    while (Value == 0) {
      RightEnd -= 1;
      for (short c=0; c<MaxColor; c++)
        Value += Histogram[c][RightEnd];
      if (RightEnd==LeftEnd) Value = 1;
    }

    for (uint16_t k=LeftEnd+1;k<RightEnd;k++) {
      for (short c=0; c<MaxColor; c++ ) {
        if (Histogram[c][k] > m_HistoMax) {
          m_HistoMax = Histogram[c][k];
        }
      }
    }
    // Create some headroom for changes.
    m_HistoMax = m_HistoMax*4/3;
  }

  // Instantiate an Image8 and put the histogram in it.
  m_Image8.setSize(WidgetWidth, WidgetHeight, 3);
  m_Image8.fillColor(0, 0, 0, 0);

  uint16_t RowLimit = WidgetHeight-1;

  for (short c=0; c<MaxColor; c++ ) {
    if (!(MaxColor==1) && !((1<<c) & Settings->GetInt("HistogramChannel"))) {
      continue;
    }
    for (uint16_t i=0; i<HistogramWidth; i++) {
      double r = Histogram[c][i]/(double)(m_HistoMax);
      if (r>=0.99) r=0.99; // Safety.
      uint16_t Row = RowLimit-(uint16_t)(r*WidgetHeight);
      // if (Row<0) Row = 0;
      if (Row >= WidgetHeight) Row=WidgetHeight-1;
      uint32_t Index = 0;
      for (uint16_t k=Row;k<RowLimit;k++) {
        Index = k*WidgetWidth+i+HistogramMargin;
        // 2- ! Image8[0]=B for QT !
        for (short z=0; z<3; z++) {
          m_Image8.image()[Index][z] +=
            ((z==(2-c)) || (MaxColor ==1))?0xff:0;
        }
      }
      // baselines. A grey colour.
      Index = RowLimit*m_Image8.width()+i+HistogramMargin;
      m_Image8.image()[Index][0] = 0x80;
      m_Image8.image()[Index][1] = 0x80;
      m_Image8.image()[Index][2] = 0x80;
      // Average line.
      r = HistoAverage/(double)(m_HistoMax);
      if (r>=0.99) r=0.99; // Safety.
      Row = RowLimit-(uint16_t)(r*WidgetHeight);
      // if (Row<0) Row = 0;
      if (Row >= WidgetHeight) Row=WidgetHeight-1;
      Index = Row*m_Image8.width()+i+HistogramMargin;
      m_Image8.image()[Index][0] = 0xa0;
      m_Image8.image()[Index][1] = 0xa0;
      m_Image8.image()[Index][2] = 0xa0;
    }
  }

  // Grid
  int Sections = 5;
  int Step = (int) ((double)HistogramWidth/(double)Sections);
  for (uint16_t Row=0; Row<WidgetHeight; Row++) {
    int value      = (int) MIN(MAX(Row-15,0), 0x60);
    uint32_t Index = Row*WidgetWidth+HistogramMargin;
    for (short i=1; i<Sections; i++) {
      Index += Step;
      if (m_Image8.image()[Index][0] == 0 &&
          m_Image8.image()[Index][1] == 0 &&
          m_Image8.image()[Index][2] == 0) {
        m_Image8.image()[Index][0] =
        m_Image8.image()[Index][1] =
        m_Image8.image()[Index][2] = value;
      }
    }
  }

  // printf("\n(%s,%d) %d\n\n",__FILE__,__LINE__,Timer.elapsed());
}
////////////////////////////////////////////////////////////////////////////////
//
// Fill the look up table for fast histogram creation
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::FillLookUp() {
  if (m_LookUp == NULL) {
    m_LookUp = new uint16_t[0x10000];
  }

  int WidgetWidth  = width();

  const uint16_t HistogramMargin = 5;
  const uint16_t HistogramWidth  = WidgetWidth-2*HistogramMargin;

#pragma omp parallel for
  for (uint32_t i=0; i<0x10000; i++) {
    m_LookUp[i] = (uint16_t)(ToFloatTable[i]*HistogramWidth);
  }
}

//==============================================================================

void ptHistogramWindow::InitOverlay() {
  m_OverlayPalette = new QPalette();
  m_OverlayPalette->setColor(QPalette::Foreground, QColor(0xa0, 0xa0, 0xa0));

  m_PixelInfoR = new QLabel(this);
  m_PixelInfoR->setTextFormat(Qt::PlainText);
  m_PixelInfoR->setTextInteractionFlags(Qt::NoTextInteraction);
  m_PixelInfoR->show();
  m_PixelInfoR->setPalette(*m_OverlayPalette);
  m_PixelInfoR->move(10, 4);
  m_PixelInfoR->hide();

  m_PixelInfoG = new QLabel(this);
  m_PixelInfoG->setTextFormat(Qt::PlainText);
  m_PixelInfoG->setTextInteractionFlags(Qt::NoTextInteraction);
  m_PixelInfoG->show();
  m_PixelInfoG->setPalette(*m_OverlayPalette);
  m_PixelInfoG->move(60, 4);
  m_PixelInfoG->hide();

  m_PixelInfoB = new QLabel(this);
  m_PixelInfoB->setTextFormat(Qt::PlainText);
  m_PixelInfoB->setTextInteractionFlags(Qt::NoTextInteraction);
  m_PixelInfoB->show();
  m_PixelInfoB->setPalette(*m_OverlayPalette);
  m_PixelInfoB->move(110, 4);
  m_PixelInfoB->hide();

  m_PixelInfoTimer = new QTimer(this);
  m_PixelInfoTimer->setSingleShot(true);
  connect(m_PixelInfoTimer, SIGNAL(timeout()), this, SLOT(PixelInfoHide()));

  m_InfoIcon = new QLabel(this);
  m_InfoIcon->move(width() - 20, 4);
  m_InfoIcon->setPixmap(QPixmap(QString::fromUtf8(":/dark/ui-graphics/bubble-attention.png")));
  m_InfoIcon->setToolTip(tr("RAW thumbnail is used"));
  m_InfoIcon->setPalette(Theme->menuPalette());
  m_InfoIcon->setStyle(Theme->style());
  m_InfoIcon->setStyleSheet(Theme->stylesheet());
  m_InfoIcon->hide();
}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateView.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::UpdateView(const ptImage* NewRelatedImage) {

  if (NewRelatedImage) m_RelatedImage = NewRelatedImage;
  if (!m_RelatedImage) return;

  setInfoIconState((Settings->GetInt("IsRAW") == 1) && !Settings->useRAWHandling());

  CalculateHistogram();

  // The detour QImage=>QPixmap is needed to enjoy
  // HW acceleration of QPixmap.
  delete m_QPixmap;
  m_QPixmap = new QPixmap(
   QPixmap::fromImage(QImage((const uchar*) m_Image8.image().data(),
                             m_Image8.width(),
                             m_Image8.height(),
                             QImage::Format_RGB32)));
  repaint();
}



////////////////////////////////////////////////////////////////////////////////
//
// paintEvent handler.
// Just draw the previously constructed m_QPixmap.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::paintEvent(QPaintEvent*) {
  QPainter Painter(this);
  if (m_QPixmap)
    Painter.drawPixmap(0,0,*m_QPixmap);
}

////////////////////////////////////////////////////////////////////////////////
//
// contextMenuEvent handler.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu Menu(this);
  Menu.setPalette(Theme->menuPalette());
  Menu.setStyle(Theme->style());
  QMenu ChannelMenu(this);
  ChannelMenu.setPalette(Theme->menuPalette());
  ChannelMenu.setStyle(Theme->style());
  ChannelMenu.addAction(m_AtnRGB);
  ChannelMenu.addAction(m_AtnR);
  ChannelMenu.addAction(m_AtnG);
  ChannelMenu.addAction(m_AtnB);
  ChannelMenu.setTitle(tr("Display &channels"));
  QMenu ModeMenu(this);
  ModeMenu.setPalette(Theme->menuPalette());
  ModeMenu.setStyle(Theme->style());
  ModeMenu.addAction(m_AtnLinear);
  ModeMenu.addAction(m_AtnPreview);
  ModeMenu.addAction(m_AtnOutput);
  ModeMenu.setTitle(tr("Display &mode"));
  Menu.addMenu(&ModeMenu);
  Menu.addMenu(&ChannelMenu);
  Menu.addSeparator();
  Menu.addAction(m_AtnLnX);
  Menu.addAction(m_AtnLnY);
  Menu.addSeparator();
  if (Settings->GetInt("HistogramCrop"))
    m_AtnCrop->setChecked(1);
  else
    m_AtnCrop->setChecked(0);
  Menu.addAction(m_AtnCrop);

  Menu.exec(event->globalPos());
}

////////////////////////////////////////////////////////////////////////////////
//
// slots for the context menu
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::MenuLnX() {
  Settings->SetValue("HistogramLogX",(int)m_AtnLnX->isChecked());
  UpdateView();
}

void ptHistogramWindow::MenuLnY() {
  Settings->SetValue("HistogramLogY",(int)m_AtnLnY->isChecked());
  UpdateView();
}

void HistogramGetCrop();
void ptHistogramWindow::MenuCrop() {
  if (m_AtnCrop->isChecked())
    Settings->SetValue("HistogramCrop", 1);
  else
    Settings->SetValue("HistogramCrop", 0);

  ::HistogramGetCrop();
}

void ptHistogramWindow::MenuChannel() {
  if (m_AtnRGB->isChecked())
    Settings->SetValue("HistogramChannel", ptHistogramChannel_RGB);
  else if (m_AtnR->isChecked())
    Settings->SetValue("HistogramChannel", ptHistogramChannel_R);
  else if (m_AtnG->isChecked())
    Settings->SetValue("HistogramChannel", ptHistogramChannel_G);
  else if (m_AtnB->isChecked())
    Settings->SetValue("HistogramChannel", ptHistogramChannel_B);

  UpdateView();
}

void Update(short Phase,
            short SubPhase      = -1,
            short WithIdentify  = 1,
            short ProcessorMode = ptProcessorMode_Preview);
void ptHistogramWindow::MenuMode() {
  if (m_AtnLinear->isChecked())
    Settings->SetValue("HistogramMode", ptHistogramMode_Linear);
  else if (m_AtnPreview->isChecked())
    Settings->SetValue("HistogramMode", ptHistogramMode_Preview);
  else if (m_AtnOutput->isChecked())
    Settings->SetValue("HistogramMode", ptHistogramMode_Output);

  Update(ptProcessorPhase_OnlyHistogram);
}

//==============================================================================

void ptHistogramWindow::PixelInfoHide() {
  m_PixelInfoR->hide();
  m_PixelInfoG->hide();
  m_PixelInfoB->hide();
}

////////////////////////////////////////////////////////////////////////////////
