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

#include <QString>
#include <QObject>
#include <QMessageBox>

#ifdef _OPENMP
  #include <omp.h>
#endif

#define cimg_use_openmp 1

#include "ptCimg.h"
#include "ptImage.h"
#include "ptError.h"
#include "ptCalloc.h"
#include "ptCurve.h"

#include <cassert>

// Lut
extern float ToFloatTable[0x10000];

class ptGreycCount {
  public:
  ptImage* Image;
  uint32_t Counter;
  uint32_t MaxCounter;
  uint32_t NextReport;
  uint8_t  Iteration;
  float    da;
  void     CountFunction();
  void     (*ReportProgress)(const QString Message);
  ptGreycCount(ptImage* Image,float da,void (*ReportProgress)(const QString)) {
    this->Image          = Image;
    this->Counter        = 0;
    this->da             = da;
    this->ReportProgress = ReportProgress;
    Iteration  = 0;
    NextReport = 0;
    MaxCounter = (uint32_t) (Image->m_Width*Image->m_Height*(da+360.0)/da);
  }
  void NextIteration() {
    Iteration++;
    Counter    = 0;
    NextReport = 0;
  }
};

ptGreycCount *GreycCount;

#define cimg_plugin_greycstoration
#define cimg_plugin_greycstoration_count GreycCount->CountFunction()

#ifdef WIN32
  #define cimg_display_type 0
#else
  #define cimg_display 0
#endif

#ifdef SYSTEM_CIMG
  #include <CImg.h>
#else
  #include "greyc/CImg.h"
#endif


using namespace cimg_library;

void ptGreycCount::CountFunction() {
  Counter++;
  if (Counter>NextReport) {
    int Percent = (int)(Counter*100.0/MaxCounter);
    QString Message = QObject::tr("GreycStoration iteration ");
    QString Tmp;
    Tmp.setNum(Iteration);
    Message += Tmp;
    Message += " (";
    Tmp.setNum(Percent);
    Message += Tmp;
    Message += "%)";
    ReportProgress(Message);
    NextReport += MaxCounter/20; // 5% steps
    if (NextReport>=MaxCounter) NextReport = MaxCounter-1;
  }
};

void ptGreycStoration(ptImage* Image,
                      void     (*ReportProgress)(const QString Message),
                      short    NrIterations,
                      float    Amplitude,
                      float    Sharpness,
                      float    Anisotropy,
                      float    Alpha,
                      float    Sigma,
                      float    pt,
                      float    da,
                      float    GaussPrecision,
                      int      Interpolation,
                      short    Fast) {

  GreycCount = new ptGreycCount(Image,da,ReportProgress);

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short c=0; c<3; c++) {
        CImage(Col,Row,c) = Image->m_Image[Row*Width+Col][c];
      }
    }
  }

  static const CImg<unsigned char> EmptyMask;

  for (short Iter=0; Iter<NrIterations; Iter++) {
    GreycCount->NextIteration();
    CImage.blur_anisotropic(Amplitude,
                            Sharpness,
                            Anisotropy,
                            Alpha,
                            Sigma,
                            pt,
                            da,
                            GaussPrecision,
                            Interpolation,
                            Fast);
  }
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short c=0; c<3; c++) {
        Image->m_Image[Row*Width+Col][c] = CImage(Col,Row,c);
      }
    }
  }

  delete GreycCount;
}

void ptGreycStorationLab(ptImage* Image,
       void     (*ReportProgress)(const QString Message),
                         short    NrIterations,
                         float    Amplitude,
       float    Sharpness,
                         float    Anisotropy,
                         float    Alpha,
                         float    Sigma,
                         float    pt,
                         float    da,
                         float    GaussPrecision,
                         int      Interpolation,
                         short    Fast,
                         short    MaskType,
                         double   Opacity) {

  GreycCount = new ptGreycCount(Image,da,ReportProgress);

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,1,0);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      CImage(Col,Row) = Image->m_Image[Row*Width+Col][0];
    }
  }

  for (short Iter=0; Iter<NrIterations; Iter++) {
    GreycCount->NextIteration();
    CImage.blur_anisotropic(Amplitude,
                            Sharpness,
                            Anisotropy,
                            Alpha,
                            Sigma,
                            pt,
                            da,
                            GaussPrecision,
                            Interpolation,
                            Fast);
  }

  float (*Mask) = NULL;
  switch (MaskType) {
    case ptDenoiseMask_All:
#pragma omp parallel for default(shared) schedule(static)
      for (uint16_t Row=0; Row<Image->m_Height; Row++) {
  for (uint16_t Col=0; Col<Image->m_Width; Col++) {
    Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)(CImage(Col,Row)*Opacity+Image->m_Image[Row*Width+Col][0]*(1-Opacity)));
  }
      }
      break;

  case ptDenoiseMask_Shadows1:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.0, 0.5, 0.0);
    break;

  case ptDenoiseMask_Shadows2:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.1, 0.6, 0.0);
    break;

  case ptDenoiseMask_Shadows3:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.2, 0.7, 0.0);
    break;

  case ptDenoiseMask_Shadows4:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.3, 0.8, 0.0);
    break;

  case ptDenoiseMask_Shadows5:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.4, 0.9, 0.0);
    break;

  default:
    break;
  }

  if (MaskType != ptDenoiseMask_All) {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
  Image->m_Image[Row*Width+Col][0] =
    CLIP((int32_t) ((Mask[Row*Width+Col]*CImage(Col,Row) +
        (1-Mask[Row*Width+Col])*Image->m_Image[Row*Width+Col][0])*Opacity +
        (1-Opacity)*Image->m_Image[Row*Width+Col][0]));
      }
    }
    FREE2(Mask);
  }

  delete GreycCount;
}

void ptCimgEdgeTensors(ptImage* Image,
                       const double    Sharpness,
                       const double    Anisotropy,
                       const double    Alpha,
                       const double    Sigma,
                       const double    Blur,
                       const short     MaskType) {

  constexpr int NumberOfThreads = 1;

  uint16_t FullWidth  = Image->m_Width;
  uint16_t Width  = (int) ((double)Image->m_Width/(double)NumberOfThreads+0.5);
  uint16_t Height = Image->m_Height;

  float (*Mask) = NULL;
  switch (MaskType) {
  case ptDenoiseMask_All:
    break;

  case ptDenoiseMask_Shadows1:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.0, 0.5, 0.0);
    break;

  case ptDenoiseMask_Shadows2:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.1, 0.6, 0.0);
    break;

  case ptDenoiseMask_Shadows3:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.2, 0.7, 0.0);
    break;

  case ptDenoiseMask_Shadows4:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.3, 0.8, 0.0);
    break;

  case ptDenoiseMask_Shadows5:
      Mask=Image->GetMask(ptMaskType_Shadows, 0.4, 0.9, 0.0);
    break;

  default:
    break;
  }

  CImg <float> CImage[NumberOfThreads];
  uint16_t PrivateWidth[NumberOfThreads];

#pragma omp parallel for schedule(static)
  for (short Threads=0; Threads < NumberOfThreads; Threads++) {
    PrivateWidth[Threads] = 0;
    if (Threads<NumberOfThreads-1)
      PrivateWidth[Threads] = Width;
    else
      PrivateWidth[Threads] = Image->m_Width-(NumberOfThreads-1)*Width;

    CImage[Threads].assign(PrivateWidth[Threads],Height,1,1);

    for (uint16_t Col=0; Col<PrivateWidth[Threads]; Col++) {
      for (uint16_t Row=0; Row<Image->m_Height; Row++) {
        CImage[Threads](Col,Row) = ToFloatTable[Image->m_Image[Row*FullWidth+Col+Threads*Width][0]];
      }
    }
  }

  for (short Threads=0; Threads < NumberOfThreads; Threads++) {
    CImage[Threads].diffusion_tensors(Sharpness,Anisotropy,Alpha,Sigma);
    if (Blur) CImage[Threads].blur(Blur,true);

  }

#pragma omp parallel for schedule(static)
  for (short Threads=0; Threads < NumberOfThreads; Threads++) {

    if (MaskType != ptDenoiseMask_All) {
      for (uint16_t Col=0; Col<PrivateWidth[Threads]; Col++) {
        for (uint16_t Row=0; Row<Image->m_Height; Row++) {
          Image->m_Image[Row*FullWidth+Col+Threads*Width][0] =
            CLIP((int32_t) (Mask[Row*FullWidth+Col+Threads*Width]*0xffff*CImage[Threads](Col,Row)));
        }
      }
      FREE2(Mask);
    } else {
      for (uint16_t Col=0; Col<PrivateWidth[Threads]; Col++) {
        for (uint16_t Row=0; Row<Image->m_Height; Row++) {
          Image->m_Image[Row*FullWidth+Col+Threads*Width][0] = CLIP((int32_t)(0xffff*CImage[Threads](Col,Row)));
        }
      }
    }
  }
#pragma omp parallel for default(shared) schedule(static)
  for (uint32_t i=0; i<(uint32_t) Image->m_Height*Image->m_Width; i++) {
    Image->m_Image[i][1]=0x8080;
    Image->m_Image[i][2]=0x8080;
  }
}


void ptCimgBlurBilateralChannel(ptImage* Image, const float Sigma_s, const float Sigma_r) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <float> CImage(Width,Height,1,1,0);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      CImage(Col,Row) = ToFloatTable[Image->m_Image[Row*Width+Col][0]];
    }
  }

//~ for (int i=0;i<2;i++)
  CImage.blur_bilateral(Sigma_s,Sigma_r);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)(CImage(Col,Row,0)*0xffff));
    }
  }
}

void ptCimgBlur(ptImage* Image, const short ChannelMask, const float Sigma) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  CImage.blur(Sigma,true);

  for (short Channel=0;Channel<3;Channel++) {

    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        Image->m_Image[Row*Width+Col][Channel] = CImage(Col,Row,Channel);
      }
    }
  }
}

void ptCimgBlurLayer(uint16_t *Layer, const uint16_t Width, const uint16_t Height, const float Sigma) {

  CImg <uint16_t> CImage(Width,Height,1,1,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      CImage(Col,Row) = Layer[Row*Width+Col];
    }
  }

  CImage.blur(Sigma,true);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      Layer[Row*Width+Col] = CImage(Col,Row);
    }
  }

  ~CImage;
}

void ptCimgNoise(ptImage* Image, const double Sigma, const short NoiseType, const double Radius){

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;
  double Strength = Sigma;
  int Type = LIM((int)NoiseType,0,2);

  CImg <uint16_t> CImage(Width,Height,1,1,0);
  CImage.fill(0x7FFF);

  if (NoiseType == 1) Strength = Strength * 2;  // for the same visual impression
  CImage.noise(Strength,Type);
  CImage.blur(Radius,true);
  uint32_t Temp = 0;
#pragma omp parallel for default(shared) schedule(static) private(Temp)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    Temp = Row*Width;
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        Image->m_Image[Temp+Col][0] = CImage(Col,Row);
    }
  }

  ~CImage;
}

void ptCimgSharpen(ptImage* Image,
       const short ChannelMask,
       const float Amplitude,
       const short NrIterations,
       const short UseEdgeMask){

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;
assert (ChannelMask == 1); // TODO: mike
  CImg <float> CImage(Width,Height,1,1,0);
  for (short Channel=0;Channel<3;Channel++) {
      // Is it a channel we are supposed to handle ?
      if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        CImage(Col,Row,Channel) = ToFloatTable[Image->m_Image[Row*Width+Col][Channel]];
      }
    }
  }

  CImg <float> Mask(Width,Height,1,1,0);
  float NewAmplitude = Amplitude;
  if (UseEdgeMask) {

    CImgList<float> grad = CImage.get_gradient("xy",3);
    for (short Channel=0;Channel<3;Channel++) {
      // Is it a channel we are supposed to handle ?
      if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
      for (uint16_t Row=0; Row<Image->m_Height; Row++) {
        for (uint16_t Col=0; Col<Image->m_Width; Col++) {
          Mask(Col,Row,Channel) = sqrt(pow(grad[0](Col,Row,Channel),2.) +
                                       pow(grad[1](Col,Row,Channel),2.));
        }
      }
    }
    Mask.blur(5).normalize(0,1);

    NewAmplitude = 2 * NewAmplitude;
  }

  for (short Iter=0; Iter<NrIterations; Iter++) {
    CImage=CImage.sharpen(NewAmplitude);
  }

  for (short Channel=0;Channel<3;Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;

    if (UseEdgeMask) {
#pragma omp parallel for default(shared) schedule(static)
      for (uint16_t Row=0; Row<Image->m_Height; Row++) {
        for (uint16_t Col=0; Col<Image->m_Width; Col++) {
          Image->m_Image[Row*Width+Col][Channel] = CLIP((int32_t) (Mask(Col,Row,Channel)*0xffff*CImage(Col,Row,Channel)+
            Image->m_Image[Row*Width+Col][Channel]*(1.0-Mask(Col,Row,Channel))));
        }
      }
    } else {
#pragma omp parallel for default(shared) schedule(static)
      for (uint16_t Row=0; Row<Image->m_Height; Row++) {
        for (uint16_t Col=0; Col<Image->m_Width; Col++) {
          Image->m_Image[Row*Width+Col][Channel] = CLIP((int32_t) (0xffff*CImage(Col,Row,Channel)));
        }
      }
    }
  }

  ~CImage;
}

inline double midtones(double x)
  {return 3.8*x*(x-1)*log(x);} // For the midtones
//~ {return -4*x*(x-1);}

// works at the moment only for shadows!

float *ptGradientMask(const ptImage* Image, const double Radius, const double Threshold /* =0 */) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  float (*dMask) = (float (*)) CALLOC2(Width*Height,sizeof(*dMask));
  ptMemoryError(dMask,__FILE__,__LINE__);

  CImg <float> CImage(Width,Height,1,1,0);
  if (Image->m_ColorSpace == ptSpace_Lab) {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        CImage(Col,Row) = ToFloatTable[Image->m_Image[Row*Width+Col][0]];
      }
    }
  } else {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        CImage(Col,Row) = ((double)Image->m_Image[Row*Width+Col][0]*0.3+
                          (double)Image->m_Image[Row*Width+Col][1]*0.59+
                          (double)Image->m_Image[Row*Width+Col][2]*0.11)/(double)0xffff;
      }
    }
  }

  CImgList<float> grad = CImage.get_gradient("xy",3);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      CImage(Col,Row) = sqrt(pow(grad[0](Col,Row),2.) +
                             pow(grad[1](Col,Row),2.));
    }
  }
  CImage.blur(Radius).normalize(0,1);


  if (Threshold) {
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        CImage(Col,Row) = ptCurve::Sigmoidal(CImage(Col,Row),0.25, Threshold);
      }
    }
  }

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      if (CImage(Col,Row) > 0.8) CImage(Col,Row) = 0.8;
      dMask[Row*Width+Col] = 1.0 - CImage(Col,Row);
    }
  }

  ~CImage;

  return dMask;
}

// This edge detection sums over all channels
void ptCimgEdgeDetectionSum(ptImage* Image,
                            const double ColorWeight,
                            const short GradientMode /* = 4 */) {

  assert(Image->m_ColorSpace == ptSpace_Lab);

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  // We could not use negative GradientMode in Photivo, so GradientMode - 1 here
  CImgList<float> grad = CImage.get_gradient("xy",GradientMode - 1);
  ~CImage;

  CImg <float> Sum(Width, Height, 1, 1, 0);

  const double Denom = 1.0f + 2 * ColorWeight;

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      Sum(Col,Row) = ((powf(powf(grad[0](Col,Row,0),2.f)+powf(grad[1](Col,Row,0),2.f),.5f)) +
                      ColorWeight*(powf(powf(grad[0](Col,Row,1),2.f)+powf(grad[1](Col,Row,1),2.f),.5f)) +
                      ColorWeight*(powf(powf(grad[0](Col,Row,2),2.f)+powf(grad[1](Col,Row,2),2.f),.5f))   )/Denom;
    }
  }
  Sum.normalize(0,1);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)(powf(Sum(Col,Row),0.25f)*0xffff));
    }
  }
}

// This edge detection sums over all channels (alternaive version with int16 internally)
void ptCimgEdgeDetectionSumAlt(ptImage* Image,
                            const double ColorWeight) {

  assert(Image->m_ColorSpace == ptSpace_Lab);

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  CImgList<float> grad = CImage.get_gradient("xy",3);

  const double Denom = 1.0f + 2 * ColorWeight;

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      CImage(Col,Row,0) = CLIP((int32_t)(((powf(powf(grad[0](Col,Row,0),2.f)+powf(grad[1](Col,Row,0),2.f),.5f)) +
                                          ColorWeight*(powf(powf(grad[0](Col,Row,1),2.f)+powf(grad[1](Col,Row,1),2.f),.5f)) +
                                          ColorWeight*(powf(powf(grad[0](Col,Row,2),2.f)+powf(grad[1](Col,Row,2),2.f),.5f))   )/Denom));
    }
  }
  CImage.normalize(0,0xffff);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      Image->m_Image[Row*Width+Col][0] = CLIP((int32_t)(powf((float)CImage(Col,Row,0)/0xffff,0.25f)*0xffff));
    }
  }
}

// Edges per channel
void ptCimgEdgeDetection(ptImage* Image, const short ChannelMask){

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  CImgList<float> grad = CImage.get_gradient("xy",3);
  for (short Channel=0;Channel<3;Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        CImage(Col,Row,Channel) = CLIP((int32_t)
          (sqrt(pow(grad[0](Col,Row,Channel),2.)+pow(grad[1](Col,Row,Channel),2.))));
      }
    }
  }
  CImage.normalize(0,0xffff);

  for (short Channel=0;Channel<3;Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        Image->m_Image[Row*Width+Col][Channel] = CImage(Col,Row,Channel);
      }
    }
  }

  ~CImage;
}

void ptCimgEdgeDetectionLayer(uint16_t *Layer,
                              const uint16_t Width,
                              const uint16_t Height) {

  CImg <uint16_t> CImage(Width,Height,1,1,0);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
        CImage(Col,Row) = Layer[Row*Width+Col];
    }
  }

  CImgList<float> grad = CImage.get_gradient("xy",3);
#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      CImage(Col,Row) = CLIP((int32_t) (sqrt(pow(grad[0](Col,Row),2.)+pow(grad[1](Col,Row),2.))));
    }
  }

  CImage.normalize(0,0xffff);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      Layer[Row*Width+Col] = CImage(Col,Row);
    }
  }

  ~CImage;
}

void ptCimgEqualize(ptImage* Image, const double Opacity) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  CImage.normalize(0,0xffff);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        Image->m_Image[Row*Width+Col][Channel] =
          CLIP((int32_t) (Opacity*CImage(Col,Row,Channel)+(1-Opacity)*Image->m_Image[Row*Width+Col][Channel]));
      }
    }
  }
}

void ptCimgRotate(ptImage* Image, const double Angle, const short Interpolation) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  CImg <uint16_t> CImage(Width,Height,1,3,0);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Image->m_Height; Row++) {
    for (uint16_t Col=0; Col<Image->m_Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        CImage(Col,Row,Channel) = Image->m_Image[Row*Width+Col][Channel];
      }
    }
  }

  CImage.rotate(Angle, 0, Interpolation);

  Image->m_Width  = CImage.width();
  Image->m_Height = CImage.height();
  Width  = Image->m_Width;
  Height = Image->m_Height;
  Image->setSize((int32_t)Width*Height);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      for (short Channel=0; Channel<3; Channel++) {
        Image->m_Image[Row*Width+Col][Channel] = CImage(Col,Row,Channel);
      }
    }
  }
}
