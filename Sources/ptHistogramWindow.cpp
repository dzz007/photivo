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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptSettings.h"
#include "ptConstants.h"
#include "ptHistogramWindow.h"
#include "ptTheme.h"

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
  :QWidget(NULL) {

  m_RelatedImage = RelatedImage; // don't delete that at cleanup !
  // Some other dynamic members we want to have clean.
  m_QPixmap      = NULL;
  m_Image8       = NULL;

  m_LogoActive   = 1;

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
  m_AtnLnX = new QAction(tr("LnX"), this);
  m_AtnLnX->setStatusTip(tr("X axis logarithmic"));
  m_AtnLnX->setCheckable(true);
  m_AtnLnX->setChecked(Settings->GetInt("HistogramLogX"));
  connect(m_AtnLnX, SIGNAL(triggered()), this, SLOT(MenuLnX()));

  m_AtnLnY = new QAction(tr("LnY"), this);
  m_AtnLnY->setStatusTip(tr("Y axis logarithmic"));
  m_AtnLnY->setCheckable(true);
  m_AtnLnY->setChecked(Settings->GetInt("HistogramLogY"));
  connect(m_AtnLnY, SIGNAL(triggered()), this, SLOT(MenuLnY()));

  m_AtnCrop = new QAction(tr("Selection"), this);
  m_AtnCrop->setStatusTip(tr("Histogram only on a part of the image"));
  m_AtnCrop->setCheckable(true);
  if (Settings->GetInt("HistogramCrop"))
    m_AtnCrop->setChecked(1);
  else
    m_AtnCrop->setChecked(0);
  connect(m_AtnCrop, SIGNAL(triggered()), this, SLOT(MenuCrop()));

  m_AtnLinear = new QAction(tr("Linear"), this);
  m_AtnLinear->setStatusTip(tr("Use data from linear pipe"));
  m_AtnLinear->setCheckable(true);
  connect(m_AtnLinear, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnPreview = new QAction(tr("Preview"), this);
  m_AtnPreview->setStatusTip(tr("Use data with preview profile"));
  m_AtnPreview->setCheckable(true);
  connect(m_AtnPreview, SIGNAL(triggered()), this, SLOT(MenuMode()));

  m_AtnOutput = new QAction(tr("Output"), this);
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

  m_AtnR = new QAction(tr("R"), this);
  m_AtnR->setStatusTip(tr("R"));
  m_AtnR->setCheckable(true);
  connect(m_AtnR, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_AtnG = new QAction(tr("G"), this);
  m_AtnG->setStatusTip(tr("G"));
  m_AtnG->setCheckable(true);
  connect(m_AtnG, SIGNAL(triggered()), this, SLOT(MenuChannel()));

  m_AtnB = new QAction(tr("B"), this);
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

}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptHistogramWindow::~ptHistogramWindow() {
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

void ptHistogramWindow::resizeEvent(QResizeEvent*) {
  // Schedule the action 500ms from here to avoid multiple rescaling actions
  // during multiple resizeEvents from a window resized by the user.
  m_ResizeTimer->start(500); // 500 ms.
}

void ptHistogramWindow::ResizeTimerExpired() {
  // Create side effect for recalibrating the maximum
  m_PreviousHistogramGamma = -1;
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

  //QTime Timer;
  //Timer.start();

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
  double r = 0;
  uint16_t HistogramPoint = 0;
#pragma omp parallel default(shared) private (r, HistogramPoint)
    {
#ifdef _OPENMP
      // We need a thread-private copy.
      int TpHistogram[3][HistogramWidth];
      memset (TpHistogram, 0, sizeof Histogram);
#endif
#pragma omp for
    for (int32_t i=0; i<(int32_t) Size; i++) {
      for (short c=0;c<MaxColor;c++) {
        r = ToFloatTable[m_RelatedImage->m_Image[i][c]];
        HistogramPoint = (uint16_t)(r*HistogramWidth);
#ifdef _OPENMP
          TpHistogram[c][HistogramPoint]++;
#else
          Histogram[c][HistogramPoint]++;
#endif
      }
    }
#ifdef _OPENMP
#pragma omp critical
      for(int c=0; c<3*HistogramWidth; c++) {
        Histogram[0][c]+=TpHistogram[0][c];
      }
#endif
    } // End omp parallel zone.

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
          (4096* log10(1+2.16227766/HistoAverage*Histogram[c][k]));
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
  delete m_Image8;
  m_Image8 = new ptImage8(WidgetWidth,WidgetHeight,3);

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
      for (uint16_t k=Row;k<RowLimit;k++) {
        // 2- ! Image8[0]=B for QT !
        for (short z=0; z<3; z++) {
          m_Image8->m_Image[k*m_Image8->m_Width+i+HistogramMargin][z] +=
            ((z==(2-c)) || (MaxColor ==1))?0xff:0;
        }
      }
      // baselines. A grey colour.
      uint32_t Index = RowLimit*m_Image8->m_Width+i+HistogramMargin;
      m_Image8->m_Image[Index][0] = 0x80;
      m_Image8->m_Image[Index][1] = 0x80;
      m_Image8->m_Image[Index][2] = 0x80;
      // Average line.
      r = HistoAverage/(double)(m_HistoMax);
      if (r>=0.99) r=0.99; // Safety.
      Row = RowLimit-(uint16_t)(r*WidgetHeight);
      // if (Row<0) Row = 0;
      if (Row >= WidgetHeight) Row=WidgetHeight-1;
      Index = Row*m_Image8->m_Width+i+HistogramMargin;
      m_Image8->m_Image[Index][0] = 0xa0;
      m_Image8->m_Image[Index][1] = 0xa0;
      m_Image8->m_Image[Index][2] = 0xa0;
    }
  }

  // Grid
  int Sections = 5;
  int Step = (int) ((double)HistogramWidth/(double)Sections);
  for (short i=1; i<Sections; i++) {
    uint16_t Col=i*Step+HistogramMargin;
    for (uint16_t Row=0; Row<WidgetHeight; Row++) {
      int value = (int) MIN(MAX(Row-15,0), 0x60);
      uint32_t Index = Row*WidgetWidth+Col;
      if (!m_Image8->m_Image[Index][0] &&
          !m_Image8->m_Image[Index][1] &&
          !m_Image8->m_Image[Index][2]) {
        m_Image8->m_Image[Index][0] = value;
        m_Image8->m_Image[Index][1] = value;
        m_Image8->m_Image[Index][2] = value;
      }
    }
  }

  //~ // Replace now what's still black with the background color
  //~ // Changed to background color black, to be independent of the QT$ theme
  //~ QColor BGColor = Qt::black;//palette().color(QPalette::Window);
  //~ for (uint16_t Row=0; Row<WidgetHeight; Row++) {
    //~ for (uint16_t Col=1; Col<WidgetWidth-1; Col++) {
      //~ if (!m_Image8->m_Image[Row*WidgetWidth+Col][0] &&
          //~ !m_Image8->m_Image[Row*WidgetWidth+Col][1] &&
          //~ !m_Image8->m_Image[Row*WidgetWidth+Col][2]) {
      //~ m_Image8->m_Image[Row*WidgetWidth+Col][0] = BGColor.blue();
      //~ m_Image8->m_Image[Row*WidgetWidth+Col][1] = BGColor.green();
      //~ m_Image8->m_Image[Row*WidgetWidth+Col][2] = BGColor.red();
      //~ }
    //~ }
  //~ }

}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateView.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::UpdateView(const ptImage* NewRelatedImage) {

  if (NewRelatedImage) m_RelatedImage = NewRelatedImage;
  if (!m_RelatedImage) return;

  CalculateHistogram();

  // The detour QImage=>QPixmap is needed to enjoy
  // HW acceleration of QPixmap.
  delete m_QPixmap;
  m_QPixmap = new QPixmap(
   QPixmap::fromImage(QImage((const uchar*) m_Image8->m_Image,
                             m_Image8->m_Width,
                             m_Image8->m_Height,
                             QImage::Format_RGB32)));
  m_LogoActive = 0;
  repaint();

}



////////////////////////////////////////////////////////////////////////////////
//
// paintEvent handler.
// Just draw the previously constructed m_QPixmap.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::paintEvent(QPaintEvent*) {
  //printf("(%s,%d) %s - Size : (%d,%d)\n",
  //       __FILE__,__LINE__,__PRETTY_FUNCTION__,width(),height());
  QPainter Painter(this);
  Painter.save();
  if (!m_QPixmap) {
    QString FileName = Settings->GetString("UserDirectory") + "photivoLogo.png";
    m_QPixmap = new QPixmap(FileName);
    QPixmap* Scaled = new QPixmap(
      m_QPixmap->scaled(width()-16,
                        height()-16,
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation));
    delete m_QPixmap;
    m_QPixmap = Scaled;
  }

  if (m_LogoActive) {
    Painter.drawPixmap((width()-m_QPixmap->width())/2,8,*m_QPixmap);
    Painter.restore();
  } else {
    Painter.drawPixmap(0,0,*m_QPixmap);
    Painter.restore();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// contextMenuEvent handler.
//
////////////////////////////////////////////////////////////////////////////////

void ptHistogramWindow::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu Menu(this);
  Menu.setPalette(Theme->ptMenuPalette);
  Menu.setStyle(Theme->ptStyle);
  QMenu ChannelMenu(this);
  ChannelMenu.setPalette(Theme->ptMenuPalette);
  ChannelMenu.setStyle(Theme->ptStyle);
  ChannelMenu.addAction(m_AtnRGB);
  ChannelMenu.addAction(m_AtnR);
  ChannelMenu.addAction(m_AtnG);
  ChannelMenu.addAction(m_AtnB);
  ChannelMenu.setTitle(tr("Channel"));
  // TODO Check for ActiveTab == a LAB tab
  //~ if (Settings->GetInt("HistogramMode")==ptHistogramMode_Linear)
    //~ ChannelMenu.setEnabled(0);
  //~ else
    //~ ChannelMenu.setEnabled(1);
  QMenu ModeMenu(this);
  ModeMenu.setPalette(Theme->ptMenuPalette);
  ModeMenu.setStyle(Theme->ptStyle);
  ModeMenu.addAction(m_AtnLinear);
  ModeMenu.addAction(m_AtnPreview);
  ModeMenu.addAction(m_AtnOutput);
  ModeMenu.setTitle(tr("Mode"));
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
////////////////////////////////////////////////////////////////////////////////
