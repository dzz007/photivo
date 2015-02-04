/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2011 Michael Munzert <mail@mm-log.com>
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

#include "ptDcRaw.h"

#include "ptCalloc.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptInfo.h"
#include "ptImage.h"
#include "ptMessageBox.h"
#include "ptResizeFilters.h"
#include "ptCurve.h"
#include "ptKernel.h"
#include "ptConstants.h"
#include "ptRefocusMatrix.h"
#include "ptCimg.h"
#include "fastbilateral/fast_lbf.h"

#include <QString>
#include <QTime>

#include <algorithm>
#include <parallel/algorithm>
#include <stack>

// std stuff needs to be declared apparently for jpeglib
// which seems a bug in the jpeglib header ?
#include <cstdlib>
#include <cstdio>

#include <functional>

#ifdef _OPENMP
  #include <omp.h>
#endif

#ifdef WIN32
#define NO_JPEG
#endif

#ifndef NO_JPEG
#ifdef __cplusplus
  // This hack copes with jpeglib.h that does or doesnt provide the
  // extern internally.
  #pragma push_macro("__cplusplus")
  #undef __cplusplus
  extern "C" {
  #include <jpeglib.h>
  }
  #pragma pop_macro("__cplusplus")
#else
  #include <jpeglib.h>
#endif
#endif

extern cmsCIExyY       D65;
extern cmsCIExyY       D50;

extern cmsHPROFILE PreviewColorProfile;
extern cmsHTRANSFORM ToPreviewTransform;

// Lut
extern float    ToFloatTable[0x10000];
extern float    ToFloatABNeutral[0x10000];
extern uint16_t ToInvertTable[0x10000];
extern uint16_t ToSRGBTable[0x10000];

short ptImage::CurrentRGBMode = 0;

//==============================================================================

//Helping functions

//==============================================================================

// Convert an RGB pixel to fake luminance
inline uint16_t RGB_2_L(const uint16_t APixel[3]) {
  return CLIP((int32_t) (0.30f*APixel[0] +
                         0.59f*APixel[1] +
                         0.11f*APixel[2]));
}

//==============================================================================

// calculate the hue for A and B differences
inline float ToHue(const float ADiffA, const float ADiffB) {
  if (ADiffA == 0.0f && ADiffB == 0.0f) {
    return 0.0f;   // value for grey pixel
  } else {
    float hHue = atan2f(ADiffB,ADiffA);
    while (hHue < 0.0f) hHue += pt2PI;
    return hHue;
  }
}

//==============================================================================

inline uint16_t Sigmoidal_4_Value(const uint16_t AValue, const float APosContrast) {
  float hContrastHalfExp = exp(0.5f*APosContrast);
  float hOffset          = -1.0f/(1.0f + hContrastHalfExp);
  float hScaling         =  1.0f/(1.0f + 1.0f/hContrastHalfExp) + hOffset;
  return CLIP((int32_t)((((1.0f/(1.0f + exp(APosContrast*(0.5f - ToFloatTable[AValue])))) + hOffset)/hScaling)*ptWPf));
}

//==============================================================================

void SigmoidalTable(uint16_t (&ATable)[0x10000], const float AContrast, const float AThreshold) {
  float hInvContrast     = 1.0f/AContrast;
  float hContrastHalfExp = exp(0.5f*AContrast);
  float hOffset          = -1.0f/(1.0f + hContrastHalfExp);
  float hScaling         =  1.0f/(1.0f + 1.0f/hContrastHalfExp) + hOffset;
  float hInvScaling      =  1.0f/hScaling;
  float logtf            = -logf(AThreshold)/logf(2.0f);
  float logft            = -logf(2.0f)/logf(AThreshold);

  ATable[0] = 0;
  if (AContrast > 0) {
#pragma omp parallel for
    for (uint32_t i = 1; i < 0x10000; i++) {
      ATable[i] = CLIP((int32_t)(powf((((1.0f/(1.0f +
        expf(AContrast*(0.5f - powf(ToFloatTable[i],logft))))) + hOffset)*hInvScaling),logtf)*ptWPf));
    }
  } else {
#pragma omp parallel for
    for (uint32_t i = 1; i < 0x10000; i++) {
      ATable[i] = CLIP((int32_t)(powf(0.5f - hInvContrast*
        logf(1.0f/(hScaling*powf(ToFloatTable[i],logft) - hOffset) - 1.0f),logtf)*ptWPf));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// RGBToRGB.
// Conversion from RGB space to RGB space.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::RGBToRGB(const short To,
                           const short EvenIfEqual) {

  assert ((m_ColorSpace>0) && (m_ColorSpace<5));
  assert ((To>0) && (To<5));
  assert (3 == m_Colors);

  if ((m_ColorSpace == To) && !EvenIfEqual) return this;

  // Matrix for conversion is a multiplication via XYZ
  double Matrix[3][3];

  for(short i=0;i<3;i++) {
    for (short j=0;j<3;j++) {
      Matrix[i][j] = 0.0;
      for (short k=0;k<3;k++) {
        Matrix[i][j] +=
          MatrixXYZToRGB[To][i][k] * MatrixRGBToXYZ[m_ColorSpace][k][j];
      }
    }
  }

  // Convert the image.
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    int32_t Value[3];
    for (short c=0; c<3; c++) {
      Value[c] = 0;
      for (short k=0; k<3; k++) {
        Value[c] += (uint32_t) (Matrix[c][k]*m_Image[i][k]);
      }
    }
    for (short c=0; c<3; c++) {
      m_Image[i][c] = (uint16_t) CLIP(Value[c]);
    }
  }

  m_ColorSpace = To;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// lcmsRGBToRGB.
// Conversion from RGB space to RGB space using lcms.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsRGBToRGB(const short To,
                               const short EvenIfEqual,
                               const int   Intent) {

  // assert ((m_ColorSpace>0) && (m_ColorSpace<5));
  if (!((m_ColorSpace>0) && (m_ColorSpace<5))) {
    ptMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
    return this;
  }
  assert ((To>0) && (To<5));
  assert (3 == m_Colors);

  if ((m_ColorSpace == To) && !EvenIfEqual) return this;

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  cmsCIExyY       DFromReference;

  switch (m_ColorSpace) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DFromReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DFromReference = D50;
      break;
    default:
      assert(0);
  }

  InProfile = cmsCreateRGBProfile(&DFromReference,
                                  (cmsCIExyYTRIPLE*)&RGBPrimaries[m_ColorSpace],
                                  Gamma3);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsHPROFILE OutProfile = 0;

  cmsCIExyY       DToReference;

  switch (To) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DToReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DToReference = D50;
      break;
    default:
      assert(0);
  }

  OutProfile = cmsCreateRGBProfile(&DToReference,
                                   (cmsCIExyYTRIPLE*)&RGBPrimaries[To],
                                   Gamma3);

  if (!OutProfile) {
    ptLogError(ptError_Profile,"Could not open OutProfile profile.");
    cmsCloseProfile (InProfile);
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InProfile,
                                 TYPE_RGB_16,
                                 OutProfile,
                                 TYPE_RGB_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = m_Width*m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  m_ColorSpace = To;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// lcmsRGBToRGB.
// Conversion from RGB space to RGB space using lcms.
// This variant uses an external profile for its output transformation.
// It would be used for instance to map onto an physical display.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsRGBToRGB(cmsHPROFILE OutProfile,
                               const int   Intent,
                               const short Quality){

  // assert ((m_ColorSpace>0) && (m_ColorSpace<5));
  if (!((m_ColorSpace>0) && (m_ColorSpace<5))) {
    ptMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
    return this;
  }
  assert (3 == m_Colors);
  assert (OutProfile);

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  cmsCIExyY DFromReference;

  switch (m_ColorSpace) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DFromReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DFromReference = D50;
      break;
    default:
      assert(0);
  }

  InProfile = cmsCreateRGBProfile(&DFromReference,
                                  (cmsCIExyYTRIPLE*)&RGBPrimaries[m_ColorSpace],
                                  Gamma3);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  if (Quality == ptCMQuality_HighResPreCalc) {
    Transform =
      cmsCreateTransform(InProfile,
                         TYPE_RGB_16,
                         OutProfile,
                         TYPE_RGB_16,
                         Intent,
                         cmsFLAGS_HIGHRESPRECALC | cmsFLAGS_BLACKPOINTCOMPENSATION);
  } else { // fast sRGB preview also uses the not optimized profile for output
    Transform =
      cmsCreateTransform(InProfile,
                         TYPE_RGB_16,
                         OutProfile,
                         TYPE_RGB_16,
                         Intent,
                         cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);
  }

  int32_t Size = m_Width*m_Height;
  int32_t Step = 10000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Image = &m_Image[i][0];
    cmsDoTransform(Transform,Image,Image,Length);
  }

  cmsDeleteTransform(Transform);
  cmsCloseProfile(InProfile);

  m_ColorSpace = ptSpace_Profiled;

  return this;
}

ptImage* ptImage::lcmsRGBToPreviewRGB(const bool Fast /* = false */){
  if (Fast) {
    // Fast transform to sRGB by lookup
    RGBToRGB(ptSpace_sRGB_D65);
#pragma omp parallel for schedule(static)
    for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
      m_Image[i][0] = ToSRGBTable[m_Image[i][0]];
      m_Image[i][1] = ToSRGBTable[m_Image[i][1]];
      m_Image[i][2] = ToSRGBTable[m_Image[i][2]];
    }
  } else {
    // assert ((m_ColorSpace>0) && (m_ColorSpace<5));
    if (!((m_ColorSpace>0) && (m_ColorSpace<5))) {
      ptMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
      return this;
    }
    assert (3 == m_Colors);

    int32_t Size = m_Width*m_Height;
    int32_t Step = 10000;
  #pragma omp parallel for schedule(static)
    for (int32_t i = 0; i < Size; i+=Step) {
      int32_t Length = (i+Step)<Size ? Step : Size - i;
      uint16_t* Image = &m_Image[i][0];
      cmsDoTransform(ToPreviewTransform,Image,Image,Length);
    }
  }
  m_ColorSpace = ptSpace_Profiled;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// RGBToXYZ.
// Conversion from RGB space to XYZ space.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::RGBToXYZ() {

  assert ((m_ColorSpace>0) && (m_ColorSpace<5));

  // Convert the image.
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    int32_t Value[3];
    for (short c=0; c<3; c++) {
      Value[c] = 0;
      for (short k=0; k<3; k++) {
        Value[c] += (int32_t)(MatrixRGBToXYZ[m_ColorSpace][c][k]*m_Image[i][k]);
      }
    }
    for (short c=0; c<3; c++) {
      m_Image[i][c] = (uint16_t) CLIP(Value[c]>>1); // XYZ 1.0 maps on 8000H
    }
  }

  m_ColorSpace = ptSpace_XYZ;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// lcmsRGBToXYZ.
// Conversion from RGB space to XYZ space using lcms.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsRGBToXYZ(const int Intent) {

  assert ((m_ColorSpace>0) && (m_ColorSpace<5));

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  cmsCIExyY       DFromReference;

  switch (m_ColorSpace) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DFromReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DFromReference = D50;
      break;
    default:
      assert(0);
  }

  InProfile = cmsCreateRGBProfile(&DFromReference,
                                  (cmsCIExyYTRIPLE*)&RGBPrimaries[m_ColorSpace],
                                  Gamma3);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsHPROFILE OutProfile = 0;

  OutProfile = cmsCreateXYZProfile();

  if (!OutProfile) {
    ptLogError(ptError_Profile,"Could not open OutProfile profile.");
    cmsCloseProfile (InProfile);
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InProfile,
                                 TYPE_RGB_16,
                           OutProfile,
                                 TYPE_XYZ_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = m_Width*m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  m_ColorSpace = ptSpace_XYZ;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// XYZToRGB.
// Conversion from XYZ space to RGB space.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::XYZToRGB(const short To) {

  assert ((To>0) && (To<5));
  assert (m_ColorSpace = ptSpace_XYZ);

  // Convert the image.
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    int32_t Value[3];
    for (short c=0; c<3; c++) {
      Value[c] = 0;
      for (short k=0; k<3; k++) {
        Value[c] += (int32_t) (MatrixXYZToRGB[To][c][k]*m_Image[i][k]);
      }
    }
    for (short c=0; c<3; c++) {
      m_Image[i][c] = (uint16_t) CLIP(Value[c]<<1); // XYZ 1.0 maps on 8000H
    }
  }

  m_ColorSpace = To;

  return this;
}


////////////////////////////////////////////////////////////////////////////////
//
// lcmsXYZToRGB.
// Conversion from XYZ space to RGB space using lcms.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsXYZToRGB(const short To,
                               const int   Intent) {

  assert ((To>0) && (To<5));
  assert (m_ColorSpace = ptSpace_XYZ);

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  InProfile = cmsCreateXYZProfile();

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsHPROFILE OutProfile = 0;

  cmsCIExyY       DToReference;

  switch (To) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DToReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DToReference = D50;
      break;
    default:
      assert(0);
  }

  OutProfile = cmsCreateRGBProfile(&DToReference,
                                   (cmsCIExyYTRIPLE*)&RGBPrimaries[To],
                                   Gamma3);
  if (!OutProfile) {
    ptLogError(ptError_Profile,"Could not open OutProfile profile.");
    cmsCloseProfile (InProfile);
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InProfile,
                                 TYPE_XYZ_16,
                           OutProfile,
                                 TYPE_RGB_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = m_Width*m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  m_ColorSpace = To;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// RGBToLab
// Conversion from RGB space to Lab space.
//
////////////////////////////////////////////////////////////////////////////////

// A lookup table used for this purpose.
// Initialized at the first call.
static double ToLABFunctionTable[0x20000];
static short  ToLABFunctionInited = 0;

ptImage* ptImage::RGBToLab() {

  if (m_ColorSpace == ptSpace_Lab) return this;

  assert (3 == m_Colors);

  double DReference[3];
  switch (m_ColorSpace) {
    case ptSpace_sRGB_D65:
    case ptSpace_AdobeRGB_D65:
      DReference[0] = D65Reference[0];
      DReference[1] = D65Reference[1];
      DReference[2] = D65Reference[2];
      break;
    case ptSpace_WideGamutRGB_D50:
    case ptSpace_ProPhotoRGB_D50:
      DReference[0] = D50Reference[0];
      DReference[1] = D50Reference[1];
      DReference[2] = D50Reference[2];
      break;
    default:
      assert(0);
  }

  // First go to XYZ
  double xyz[3];
#pragma omp parallel for private(xyz)
  for (uint32_t i=0; i < (uint32_t)m_Width*m_Height; i++) {
    xyz[0] = MatrixRGBToXYZ[m_ColorSpace][0][0] * m_Image[i][0] +
             MatrixRGBToXYZ[m_ColorSpace][0][1] * m_Image[i][1] +
             MatrixRGBToXYZ[m_ColorSpace][0][2] * m_Image[i][2] ;
    xyz[1] = MatrixRGBToXYZ[m_ColorSpace][1][0] * m_Image[i][0] +
             MatrixRGBToXYZ[m_ColorSpace][1][1] * m_Image[i][1] +
             MatrixRGBToXYZ[m_ColorSpace][1][2] * m_Image[i][2] ;
    xyz[2] = MatrixRGBToXYZ[m_ColorSpace][2][0] * m_Image[i][0] +
             MatrixRGBToXYZ[m_ColorSpace][2][1] * m_Image[i][1] +
             MatrixRGBToXYZ[m_ColorSpace][2][2] * m_Image[i][2] ;

    // Reference White
    xyz[0] /= DReference[0];
    xyz[1] /= DReference[1];
    xyz[2] /= DReference[2];

    // Then to Lab
    xyz[0] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[0]+0.5) ];
    xyz[1] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[1]+0.5) ];
    xyz[2] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[2]+0.5) ];

    // L in 0 , a in 1, b in 2
    m_Image[i][0] = CLIP((int32_t) (0xffff*(116.0/100.0 * xyz[1] - 16.0/100.0)));
    m_Image[i][1] = CLIP((int32_t) (0x101*(128.0+500.0*(xyz[0]-xyz[1]))));
    m_Image[i][2] = CLIP((int32_t) (0x101*(128.0+200.0*(xyz[1]-xyz[2]))));

  }

  // And that's it.
  m_ColorSpace = ptSpace_Lab;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// lcmsRGBToLab
// Conversion from RGB space to Lab space using lcms.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsRGBToLab(const int Intent) {

  assert (3 == m_Colors);
  assert ((m_ColorSpace>0) && (m_ColorSpace<5));

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  cmsCIExyY       DFromReference;

  switch (m_ColorSpace) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DFromReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DFromReference = D50;
      break;
    default:
      assert(0);
  }

  InProfile = cmsCreateRGBProfile(&DFromReference,
                                  (cmsCIExyYTRIPLE*)&RGBPrimaries[m_ColorSpace],
                                  Gamma3);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsHPROFILE OutProfile = 0;

  OutProfile = cmsCreateLab4Profile(NULL);

  if (!OutProfile) {
    ptLogError(ptError_Profile,"Could not open OutProfile profile.");
    cmsCloseProfile (InProfile);
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InProfile,
                                 TYPE_RGB_16,
                                 OutProfile,
                                 TYPE_Lab_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = m_Width*m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  // And that's it.
  m_ColorSpace = ptSpace_Lab;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LabToRGB
// Conversion from Lab space to RGB space.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LabToRGB(const short To) {

  if (m_ColorSpace == To) return this;

  if (!(m_ColorSpace == ptSpace_Lab)) {
    ptMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
    return this;
  }

  assert (3 == m_Colors);
  assert ((To>0) && (To<5));

  double DReference[3];
  switch (To) {
    case ptSpace_sRGB_D65:
    case ptSpace_AdobeRGB_D65:
      DReference[0] = D65Reference[0];
      DReference[1] = D65Reference[1];
      DReference[2] = D65Reference[2];
      break;
    case ptSpace_WideGamutRGB_D50:
    case ptSpace_ProPhotoRGB_D50:
      DReference[0] = D50Reference[0];
      DReference[1] = D50Reference[1];
      DReference[2] = D50Reference[2];
      break;
    default:
      assert(0);
  }

  // This code has been tweaked with performance in mind.
  // Probably some of it could be detected by compiler, but overall I
  // still gain something like going from 167 to 35 clockticks.
  // (especially avoiding the stupid pow(x,3.0) -> x*x*x !

  double xyz[3];
  double fx,fy,fz;
  double xr,yr,zr;
  const double epsilon = 216.0/24389.0;
  const double kappa   = 24389.0/27.0;

  const uint32_t NrPixels = m_Width*m_Height;
#pragma omp parallel for private(xyz,fx,fy,fz,xr,yr,zr)
  for (uint32_t i=0; i < NrPixels ; i++) {

    const double L = ToFloatTable[m_Image[i][0]]*100.0;
    const double a = (m_Image[i][1]-0x8080) /((double)0x8080/128.0) ;
    const double b = (m_Image[i][2]-0x8080) /((double)0x8080/128.0) ;

    const double Tmp1 = (L+16.0)/116.0;

    yr = (L<=kappa*epsilon)?
       (L/kappa):(Tmp1*Tmp1*Tmp1);
    fy = (yr<=epsilon) ? ((kappa*yr+16.0)/116.0) : Tmp1;
    fz = fy - b/200.0;
    fx = a/500.0 + fy;
    const double fz3 = fz*fz*fz;
    const double fx3 = fx*fx*fx;
    zr = (fz3<=epsilon) ? ((116.0*fz-16.0)/kappa) : fz3;
    xr = (fx3<=epsilon) ? ((116.0*fx-16.0)/kappa) : fx3;

    xyz[0] = xr*DReference[0]*65535.0 - 0.5;
    xyz[1] = yr*DReference[1]*65535.0 - 0.5;
    xyz[2] = zr*DReference[2]*65535.0 - 0.5;

    // And finally to RGB via the matrix.
    for (short c=0; c<3; c++) {
      double Value = 0;
      Value += MatrixXYZToRGB[To][c][0] * xyz[0];
      Value += MatrixXYZToRGB[To][c][1] * xyz[1];
      Value += MatrixXYZToRGB[To][c][2] * xyz[2];
      m_Image[i][c] = (uint16_t) CLIP((int32_t)(Value));
    }
  }

  // And that's it.

  m_ColorSpace = To;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// lcmsLabToRGB
// Conversion from Lab space to RGB space using lcms.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::lcmsLabToRGB(const short To,
                               const int   Intent) {

  assert (3 == m_Colors);
  assert ((To>0) && (To<5));
  assert (m_ColorSpace == ptSpace_Lab);

  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE InProfile = 0;

  InProfile = cmsCreateLab4Profile(NULL);

  if (!InProfile) {
    ptLogError(ptError_Profile,"Could not open InProfile profile.");
    return NULL;
  }

  cmsHPROFILE OutProfile = 0;

  cmsCIExyY       DToReference;

  switch (To) {
    case ptSpace_sRGB_D65 :
    case ptSpace_AdobeRGB_D65 :
      DToReference = D65;
      break;
    case ptSpace_WideGamutRGB_D50 :
    case ptSpace_ProPhotoRGB_D50 :
      DToReference = D50;
      break;
    default:
      assert(0);
  }

  OutProfile = cmsCreateRGBProfile(&DToReference,
                                   (cmsCIExyYTRIPLE*)&RGBPrimaries[To],
                                   Gamma3);

  if (!OutProfile) {
    ptLogError(ptError_Profile,"Could not open OutProfile profile.");
    cmsCloseProfile (InProfile);
    return NULL;
  }

  cmsFreeToneCurve(Gamma);

  cmsHTRANSFORM Transform;
  Transform = cmsCreateTransform(InProfile,
                                 TYPE_Lab_16,
                                 OutProfile,
                                 TYPE_RGB_16,
                                 Intent,
                                 cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  int32_t Size = m_Width*m_Height;
  int32_t Step = 100000;
#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < Size; i+=Step) {
    int32_t Length = (i+Step)<Size ? Step : Size - i;
    uint16_t* Tile = &(m_Image[i][0]);
    cmsDoTransform(Transform,Tile,Tile,Length);
  }
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  // And that's it.

  m_ColorSpace = To;

  return this;
}

//==============================================================================

ptImage *ptImage::RGBToLch() {
  if (m_ColorSpace == ptSpace_LCH) return this;

  this->RGBToLab();
  this->LabToLch();

  return this;
}

//==============================================================================

ptImage *ptImage::LchToRGB(const short To) {
  if (m_ColorSpace == To) return this;

  LchToLab();
  LabToRGB(To);
  return this;
}

//==============================================================================

void ptImage::ResizeLCH(size_t ASize) {
  m_ImageL.resize(ASize);
  m_ImageL.shrink_to_fit();
  m_ImageC.resize(ASize);
  m_ImageC.shrink_to_fit();
  m_ImageH.resize(ASize);
  m_ImageH.shrink_to_fit();
}

//==============================================================================

ptImage *ptImage::LabToLch()
{
  if (m_ColorSpace == ptSpace_LCH) return this;

  assert (m_ColorSpace == ptSpace_Lab);

  uint32_t hSize = (uint32_t)m_Width*m_Height;

  ResizeLCH(hSize);

  float hValueA = 0.0f;
  float hValueB = 0.0f;
#pragma omp parallel for schedule(static) private(hValueA, hValueB)
  for (uint32_t i = 0; i < hSize; i++) {
    hValueA        = ToFloatABNeutral[m_Image[i][1]];
    hValueB        = ToFloatABNeutral[m_Image[i][2]];
    m_ImageL.at(i) = m_Image[i][0];
    m_ImageH.at(i) = ToHue(hValueA, hValueB);
    m_ImageC.at(i) = powf(hValueA*hValueA + hValueB*hValueB, 0.5f);
  }

  setSize(0);
  m_ColorSpace = ptSpace_LCH;
  return this;
}

//==============================================================================

ptImage *ptImage::toRGB()
{
  if      (m_ColorSpace == ptSpace_Lab)      LabToRGB(getCurrentRGB());
  else if (m_ColorSpace == ptSpace_XYZ)      XYZToRGB(getCurrentRGB());
  else if (m_ColorSpace == ptSpace_Profiled) GInfo->Raise("Cannot transform color space!");

  return this;
}

//==============================================================================

ptImage *ptImage::toLab()
{
  if      (m_ColorSpace == ptSpace_Lab)      return this;
  else if (m_ColorSpace == ptSpace_XYZ)      XYZToRGB(getCurrentRGB());
  else if (m_ColorSpace == ptSpace_Profiled) GInfo->Raise("Cannot transform color space!");

  RGBToLab();

  return this;
}

//==============================================================================

ptImage *ptImage::LchToLab() {
  if (m_ColorSpace == ptSpace_Lab) return this;

  assert (m_ColorSpace == ptSpace_LCH);
  assert (m_Image      == 0);

  uint32_t hSize = (uint32_t)m_Width*m_Height;
  setSize((size_t)hSize);

#pragma omp parallel for schedule(static)
  for (uint32_t i = 0; i < hSize; i++) {
    m_Image[i][0] = m_ImageL.at(i);
    m_Image[i][1] = CLIP((int32_t)(cosf(m_ImageH.at(i))*m_ImageC.at(i) + ptWPHLab));
    m_Image[i][2] = CLIP((int32_t)(sinf(m_ImageH.at(i))*m_ImageC.at(i) + ptWPHLab));
  }

  ResizeLCH(0);

  m_ColorSpace = ptSpace_Lab;
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage::ptImage() {
  m_Width              = 0;
  m_Height             = 0;
  m_Image              = nullptr;
  m_Colors             = 0;
  m_ColorSpace         = ptSpace_sRGB_D65;
  m_Data.clear();
  ResizeLCH(0);

  // Initialize the lookup table for the RGB->LAB function
  // if this would be the first time.
  if (!ToLABFunctionInited) {
    // Remark : we extend the table well beyond r>1.0 for numerical
    // stability purposes. XYZ>1.0 occurs sometimes and this way
    // it stays stable (srgb->lab->srgb correct within 0.02%)
#pragma omp parallel for
    for (uint32_t i=0; i<0x20000; i++) {
      double r = (double)(i) / ptWP;
      ToLABFunctionTable[i] = r > 216.0/24389.0 ? pow(r,1/3.0) : (24389.0/27.0*r + 16.0)/116.0;
    }
  ToLABFunctionInited = 1;
  }

  // Some lcsm initialization.
  //cmsErrorAction (LCMS_ERROR_SHOW);
  cmsWhitePointFromTemp(&D65, 6503);
  cmsWhitePointFromTemp(&D50, 5003);
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage::~ptImage() {
  // nothing to free :-)
}

//==============================================================================

void ptImage::setSize(size_t Size) {
  m_Data.resize(Size);
  m_Data.shrink_to_fit();
  m_Image = (uint16_t (*)[3]) m_Data.data();
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const ptDcRaw*  DcRawObject,
                      const short   TargetSpace,
                      const char*   ProfileName,
                      const int     Intent,
                      const int     ProfileGamma) {

  assert(NULL != DcRawObject);
  assert ((TargetSpace>0) && (TargetSpace<5));

  m_Width  = DcRawObject->m_Width;
  m_Height = DcRawObject->m_Height;

  // Temp image for later flip
  TImage16Data PreFlip;
  PreFlip.resize((size_t)m_Width*m_Height);

  if (!ProfileName) {
    // Matrix for conversion RGB to RGB : is a multiplication via XYZ
    double MatrixRGBToRGB[3][3];
    for(short i=0;i<3;i++) {
      for (short j=0;j<3;j++) {
        MatrixRGBToRGB[i][j] = 0.0;
        for (short k=0;k<3;k++) {
          MatrixRGBToRGB[i][j] +=
            MatrixXYZToRGB[TargetSpace][i][k] *
            // Yes dcraw assumes that the result of rgb_cam is in sRGB !
            MatrixRGBToXYZ[ptSpace_sRGB_D65][k][j];
        }
      }
    }

    // We just need to add the correct matrix calculation with rgb_cam
    // Be attentive : we still can have 4 channels at this moment !
    double Matrix[3][4];
    for(short i=0;i<3;i++) {
      for (short j=0;j<DcRawObject->m_Colors;j++) {
        Matrix[i][j] = 0.0;
        for (short k=0;k<3;k++) {
          Matrix[i][j] +=
            MatrixRGBToRGB[i][k] * DcRawObject->m_MatrixCamRGBToSRGB[k][j];
        }
      }
    }

    // Convert the image.
#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
      int32_t Value[3];
      for (short c=0; c<3; c++) {
        Value[c] = 0;
        for (short k=0; k<DcRawObject->m_Colors; k++) {
          Value[c] += (int32_t) (Matrix[c][k]*DcRawObject->m_Image[i][k]);
        }
      }
      for (short c=0; c<3; c++) {
        PreFlip[i][c] = (uint16_t) CLIP(Value[c]);
      }
    }

    m_Colors = MIN((int)(DcRawObject->m_Colors),3);
    m_ColorSpace = TargetSpace;

    if (m_Colors != 3) {
      fprintf(stderr,
              "At this moment anything with m_Colors != 3 was not yet "
              "tested sufficiently. You can remove the assertion related "
              "to this, but unexpected issues can pop up. Consider submitting "
              "a picture from this camera to the author in order to test "
              "further.\n");
      assert(0);
    }
  }// (if !ProfileName)
  else if (ProfileName) {

    m_Colors = MIN((int)(DcRawObject->m_Colors),3);
    m_ColorSpace = TargetSpace;

    if (m_Colors != 3) {
      fprintf(stderr,
              "At this moment anything with m_Colors != 3 was not yet "
              "tested sufficiently. You can remove the assertion related "
              "to this, but unexpected issues can pop up. Consider submitting "
              "a picture from this camera to the author in order to test "
              "further. Moreover , is this profile correct ??\n");
      assert(0);
    }

    short NrProfiles = (ProfileGamma) ? 3:2;
    cmsHPROFILE Profiles[NrProfiles];
    short ProfileIdx = 0;
    if (ProfileGamma) {
      cmsHPROFILE  PreProfile = 0;
      double Params[6];
      switch(ProfileGamma) {
        case ptCameraColorGamma_sRGB :
          // Parameters for standard (inverse) sRGB in lcms
          Params[0] = 1.0/2.4;
          Params[1] = 1.1371189;
          Params[2] = 0.0;
          Params[3] = 12.92;
          Params[4] = 0.0031308;
          Params[5] = -0.055;
          break;
        case ptCameraColorGamma_BT709 :
          // Parameters for standard (inverse) BT709 in lcms
          Params[0] = 0.45;
          Params[1] = 1.233405791;
          Params[2] = 0.0;
          Params[3] = 4.5;
          Params[4] = 0.018;
          Params[5] = -0.099;
          break;
        case ptCameraColorGamma_Pure22 :
          // Parameters for standard (inverse) 2.2 gamma in lcms
          Params[0] = 1.0/2.2;
          Params[1] = 1.0;
          Params[2] = 0.0;
          Params[3] = 0.0;
          Params[4] = 0.0;
          Params[5] = 0.0;
          break;
        default:
          assert(0);
      }
      cmsToneCurve* Gamma = cmsBuildParametricToneCurve(0,4,Params);
      cmsToneCurve* Gamma4[4];
      Gamma4[0] = Gamma4[1] = Gamma4[2] = Gamma4[3]= Gamma;
      PreProfile = cmsCreateLinearizationDeviceLink(cmsSigRgbData,Gamma4);
      Profiles[ProfileIdx] = PreProfile;
      if (!Profiles[ProfileIdx]) {
        ptLogError(ptError_Profile,"Could not open sRGB preprofile.");
        return NULL;
      }
      ProfileIdx++;
      cmsFreeToneCurve(Gamma);
    }

    Profiles[ProfileIdx] = cmsOpenProfileFromFile(ProfileName,"r");
    if (!Profiles[ProfileIdx]) {
      ptLogError(ptError_Profile,"Could not open profile %s.",ProfileName);
      for (; ProfileIdx>=0; ProfileIdx--) {
        cmsCloseProfile(Profiles[ProfileIdx]);
      }
      return NULL;
    }
    ProfileIdx++;

    // Calculate the output profile (with gamma 1)

    // Linear gamma.
    cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
    cmsToneCurve* Gamma3[3];
    Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

    cmsCIExyY  DToReference;

    switch (TargetSpace) {
      case ptSpace_sRGB_D65 :
      case ptSpace_AdobeRGB_D65 :
        DToReference = D65;
        break;
      case ptSpace_WideGamutRGB_D50 :
      case ptSpace_ProPhotoRGB_D50 :
        DToReference = D50;
        break;
      default:
        assert(0);
    }

    Profiles[ProfileIdx] =
      cmsCreateRGBProfile(&DToReference,
                          (cmsCIExyYTRIPLE*)
                          &RGBPrimaries[TargetSpace],
                          Gamma3);
    if (!Profiles[ProfileIdx]) {
      ptLogError(ptError_Profile,"Could not open OutProfile profile.");
      for (; ProfileIdx>=0; ProfileIdx--) {
        cmsCloseProfile(Profiles[ProfileIdx]);
      }
      return NULL;
    }

    cmsFreeToneCurve(Gamma);

    cmsHTRANSFORM Transform;
    Transform = cmsCreateMultiprofileTransform(Profiles,
                                               NrProfiles,
                                               TYPE_RGBA_16,
                                               TYPE_RGB_16,
                                               Intent,
                                               0);
    int32_t Size = m_Width*m_Height;
    int32_t Step = 100000;
#pragma omp parallel for schedule(static)
    for (int32_t i = 0; i < Size; i+=Step) {
      int32_t Length = (i+Step)<Size ? Step : Size - i;
      uint16_t* Tile1 = &(DcRawObject->m_Image[i][0]);
      uint16_t* Tile2 = &(PreFlip[i][0]);
      cmsDoTransform(Transform,Tile1,Tile2,Length);
    }
    cmsDeleteTransform(Transform);
    for (; ProfileIdx>=0; ProfileIdx--) {
      cmsCloseProfile(Profiles[ProfileIdx]);
    }
  }

  uint16_t TargetWidth  = m_Width;
  uint16_t TargetHeight = m_Height;
  // Free and allocate
  setSize((int32_t)TargetWidth*TargetHeight);

  // Flip image. With m_Flip as in dcraw.
  // (see also flip_index() function in dcraw)
  if (DcRawObject->m_Flip & 4) {
    SWAP(TargetWidth,TargetHeight);
  }
#pragma omp parallel for
  for (uint16_t TargetRow=0; TargetRow<TargetHeight; TargetRow++) {
    for (uint16_t TargetCol=0; TargetCol<TargetWidth; TargetCol++) {
      uint16_t OriginRow = TargetRow;
      uint16_t OriginCol = TargetCol;
      if (DcRawObject->m_Flip & 4) SWAP(OriginRow,OriginCol);
      if (DcRawObject->m_Flip & 2) OriginRow = m_Height-1-OriginRow;
      if (DcRawObject->m_Flip & 1) OriginCol = m_Width-1-OriginCol;
      for (short c=0; c<3; c++) {
        m_Data[TargetRow*TargetWidth+TargetCol][c] =
          PreFlip[OriginRow*m_Width+OriginCol][c];
      }
    }
  }

  m_Height = TargetHeight;
  m_Width  = TargetWidth;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
// To DcRaw's ImageAfterPhase2. (ad hoc, look at 'sensor' values)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const ptDcRaw*  DcRawObject,
                      const short   TargetSpace) {

  assert(NULL != DcRawObject);
  assert ((TargetSpace>0) && (TargetSpace<5));

  m_Width  = DcRawObject->m_Width;
  m_Height = DcRawObject->m_Height;
  m_ColorSpace = TargetSpace;

  // Temp image for later flip
  TImage16Data PreFlip;
  PreFlip.resize((size_t)m_Width*m_Height);

  // Convert the image.
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    for (short c=0; c<3; c++) {
      PreFlip[i][c] = DcRawObject->m_Image_AfterPhase2[i][c];
    }
  }

  m_Colors = MIN((int)(DcRawObject->m_Colors),3);

  if (m_Colors != 3) {
    fprintf(stderr,
            "At this moment anything with m_Colors != 3 was not yet "
            "tested sufficiently. You can remove the assertion related "
            "to this, but unexpected issues can pop up. Consider submitting "
            "a picture from this camera to the author in order to test "
            "further.\n");
    assert(0);
  }

  uint16_t TargetWidth  = m_Width;
  uint16_t TargetHeight = m_Height;

  // Free a maybe preexisting and allocate space.
  setSize((size_t)TargetWidth*TargetHeight);

  if (DcRawObject->m_Flip & 4) {
    SWAP(TargetWidth,TargetHeight);
  }
#pragma omp parallel for schedule(static)
  for (uint16_t TargetRow=0; TargetRow<TargetHeight; TargetRow++) {
    for (uint16_t TargetCol=0; TargetCol<TargetWidth; TargetCol++) {
      uint16_t OriginRow = TargetRow;
      uint16_t OriginCol = TargetCol;
      if (DcRawObject->m_Flip & 4) SWAP(OriginRow,OriginCol);
      if (DcRawObject->m_Flip & 2) OriginRow = m_Height-1-OriginRow;
      if (DcRawObject->m_Flip & 1) OriginCol = m_Width-1-OriginCol;
      for (short c=0; c<3; c++) {
        m_Data[TargetRow*TargetWidth+TargetCol][c] =
          PreFlip[OriginRow*m_Width+OriginCol][c];
      }
    }
  }

  m_Height = TargetHeight;
  m_Width  = TargetWidth;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set, just allocation
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const uint16_t Width,
                      const uint16_t Height) {

  m_Width  = Width;
  m_Height = Height;

  // Free a maybe preexisting and allocate space.
  setSize((size_t)m_Width*m_Height);

  m_Colors = 3;
  m_ColorSpace = ptSpace_sRGB_D65;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const ptImage *Origin) { // Always deep

  assert(NULL != Origin);

  m_Width      = Origin->m_Width;
  m_Height     = Origin->m_Height;
  m_Colors     = Origin->m_Colors;
  m_ColorSpace = Origin->m_ColorSpace;
  setSize((size_t)m_Width*m_Height);
  m_Data       = Origin->m_Data;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set scaled
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::SetScaled(const ptImage *Origin,
                            const short ScaleFactor) {

  assert(NULL != Origin);

  m_Width      = Origin->m_Width;
  m_Height     = Origin->m_Height;
  m_Colors     = Origin->m_Colors;
  m_ColorSpace = Origin->m_ColorSpace;

  if (ScaleFactor == 0) {
    setSize((int32_t)m_Width*m_Height);
    m_Data     = Origin->m_Data;
  } else {
    m_Width  >>= ScaleFactor;
    m_Height >>= ScaleFactor;

    short Step = 1 << ScaleFactor;
    float InvAverage = 1.0/powf(2.0,2.0 * ScaleFactor);

    // Allocate new.
    setSize((int32_t)m_Width*m_Height);

#pragma omp parallel for schedule(static)
    for (uint16_t Row=0; Row < m_Height; Row++) {
      for (uint16_t Col=0; Col < m_Width; Col++) {
        float PixelValue[3] = {0.0,0.0,0.0};
        for (uint8_t sRow=0; sRow < Step; sRow++) {
          for (uint8_t sCol=0; sCol < Step; sCol++) {
            int32_t index = (Row*Step+sRow)*Origin->m_Width+Col*Step+sCol;
            for (short c=0; c < 3; c++) {
              PixelValue[c] += Origin->m_Data[index][c];
            }
          }
        }
        for (short c=0; c < 3; c++) {
          m_Data[Row*m_Width+Col][c]
            = (int32_t) (PixelValue[c] * InvAverage);
        }
      }
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Get the RGB value at a given point
//
////////////////////////////////////////////////////////////////////////////////

RGBValue ptImage::GetRGB(const uint16_t x, const uint16_t y) {
  RGBValue RGB;

  if (m_ColorSpace != ptSpace_Lab) {
    RGB.R = m_Image[y*m_Width+x][0];
    RGB.G = m_Image[y*m_Width+x][1];
    RGB.B = m_Image[y*m_Width+x][2];
  } else {
    RGB.R = RGB.G = RGB.B = 0;
  }

  return RGB;
}

////////////////////////////////////////////////////////////////////////////////
//
// IndicateOverExposure
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::IndicateExposure(const short    Over,
                                   const short    Under,
                                   const uint8_t  ChannelMask,
                                   const uint16_t OverExposureLevel[3],
                                   const uint16_t UnderExposureLevel[3]) {

  if (!Over && !Under) return this;
  if (!ChannelMask) return this;
#pragma omp parallel for
  for (uint32_t i=0; i< (uint32_t)m_Height*m_Width; i++) {
    for (short Color=0; Color<m_Colors; Color++) {
      if (! (ChannelMask & (1<<Color) )) continue;
      if (m_Image[i][Color] >= OverExposureLevel[Color]) {
        if (Over) m_Image[i][Color] = 0;
      } else if (m_Image[i][Color] <= UnderExposureLevel[Color]) {
        if (Under) m_Image[i][Color] = 0xffff;
      }
    }
  }
  return this;
}

ptImage* ptImage::IndicateExposure(ptImage* ValueImage,
                                   const short    Over,
                                   const short    Under,
                                   const uint8_t  ChannelMask,
                                   const uint16_t OverExposureLevel[3],
                                   const uint16_t UnderExposureLevel[3]) {

  if (!Over && !Under) return this;
  if (!ChannelMask) return this;
#pragma omp parallel for
  for (uint32_t i=0; i< (uint32_t)m_Height*m_Width; i++) {
    for (short Color=0; Color<m_Colors; Color++) {
      if (! (ChannelMask & (1<<Color) )) continue;
      if (ValueImage->m_Image[i][Color] >= OverExposureLevel[Color]) {
        if (Over) m_Image[i][Color] = 0;
      } else if (ValueImage->m_Image[i][Color] <= UnderExposureLevel[Color]) {
        if (Under) m_Image[i][Color] = 0xffff;
      }
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Expose
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Expose(const double Exposure,
                         const TExposureClipMode ExposureClipMode) {

  assert (m_Colors == 3);
  assert (m_ColorSpace != ptSpace_XYZ);
  assert (Exposure < (2<<15));
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

# pragma omp parallel for schedule(static)
  for (uint32_t i=0; i< (uint32_t)m_Height*m_Width; i++) {
    uint32_t Pixel[3];
    uint32_t Highest = 0;
    for (short Color=0; Color<NrChannels; Color++) {
      Pixel[Color] = (uint32_t)(m_Image[i][Color]*Exposure);
      if (Pixel[Color] > Highest) Highest = Pixel[Color];
    }
    if (Highest<= 0XFFFF) {
      // No clipping.
      for (short Color=0; Color<NrChannels; Color++) {
        m_Image[i][Color] = Pixel[Color];
      }
    } else {
      if (ExposureClipMode == TExposureClipMode::Hard) {
        for (short Color=0; Color<NrChannels; Color++) {
          m_Image[i][Color] = CLIP((int32_t)Pixel[Color]);
        }
      } else if (ExposureClipMode == TExposureClipMode::Ratio) {
        for (short Color=0; Color<NrChannels; Color++) {
          m_Image[i][Color] = CLIP((int32_t)(Pixel[Color]*(float)0xFFFF/Highest));
        }
      } else {
        assert("Unexpected clip mode.");
      }
    }
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// FractionLevel
//
// Calculates the level in the image where Fraction of pixels is above.
//
////////////////////////////////////////////////////////////////////////////////

uint16_t ptImage::CalculateFractionLevel(const double  Fraction,
                                         const uint8_t ChannelMask) {
  // Build histogram that is valid at this point.
  // (we might not have one yet or it might be on altered data)
  uint32_t Histogram[0x10000][4];
  memset (Histogram, 0, sizeof Histogram);
#pragma omp parallel for schedule(static)
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    for (short c=0; c<m_Colors; c++) {
      Histogram[m_Image[i][c]][c]++;
    }
  }
  uint32_t ExpectedPixels = (uint32_t) (m_Width*m_Height*Fraction);
  uint32_t Total;
  uint16_t Level = 0;
  for (short c=0;c<3;c++){
    if (! (ChannelMask & (1<<c) )) continue;
    Total = 0;
    uint16_t Value = 0;
    for (Value=0xFFFF; Value > 0 ; Value--) {
      if ((Total+= Histogram[Value][c]) > ExpectedPixels)
        break;
    }
    if (Level < Value) Level = Value;
  }

  return Level;
}


////////////////////////////////////////////////////////////////////////////////
//
// ApplyCurve
//
////////////////////////////////////////////////////////////////////////////////


ptImage* ptImage::ApplyCurve(const ptCurve *Curve,
                             const uint8_t ChannelMask) {

  assert (NULL != Curve);
  assert (m_Colors == 3);
  assert (m_ColorSpace != ptSpace_XYZ);

  std::vector<short> Channel;
  if (ChannelMask & 1) Channel.push_back(0);
  if (ChannelMask & 2) Channel.push_back(1);
  if (ChannelMask & 4) Channel.push_back(2);
  __gnu_parallel::for_each (m_Data.begin(), m_Data.end(), [&](TPixel16 &Pixel) {
    std::for_each (Channel.begin(), Channel.end(), [&](const short &Value){
      Pixel[Value] = Curve->Curve[ Pixel[Value] ];
    });
  });

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply L by Hue Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplyLByHueCurve(const ptCurve *Curve) {

// Best solution would be to use the Lab <-> Lch conversion from lcms.
// This should be faster without sacrificing much quality.

  assert (m_ColorSpace == ptSpace_Lab);
  // neutral value for a* and b* channel
  const float WPH = 0x8080;

  __gnu_parallel::for_each (m_Data.begin(), m_Data.end(), [&](TPixel16 &Pixel) {
    // Factor by hue
    float ValueA = (float)Pixel[1]-WPH;
    float ValueB = (float)Pixel[2]-WPH;
    float Hue = 0;
    if (ValueA == 0.0 && ValueB == 0.0) {
      Hue = 0;   // value for grey pixel
    } else {
      Hue = atan2f(ValueB,ValueA);
    }
    while (Hue < 0) Hue += 2.*ptPI;

    float Factor = Curve->Curve[CLIP((int32_t)(Hue/ptPI*WPH))]/(float)0x7fff - 1.0f;
    if (Factor != 0.0) {
      float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.25) / (float) 0xb5;
      Factor = powf(2.0f, 3.0f*Factor*Col);

      Pixel[0] = CLIP((int32_t)(Pixel[0] * Factor));
    }
  });

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply Hue Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplyHueCurve(const ptCurve *Curve) {

  assert (m_ColorSpace == ptSpace_Lab);
  // neutral value for a* and b* channel
  const float WPH = 0x8080;
  const float ScalePi = ptPI / 0x7fff;
  const float InvScalePi = 0x7fff / ptPI;

  float ValueA = 0.0;
  float ValueB = 0.0;
  float Col = 0.0;
  float Hue = 0.0;

  if (Curve->mask() == ptCurve::ChromaMask) { // by chroma
#pragma omp parallel for schedule(static) private(ValueA, ValueB, Col, Hue)
      for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {

        ValueA = (float)m_Image[i][1]-WPH;
        ValueB = (float)m_Image[i][2]-WPH;

        if (ValueA == 0.0 && ValueB == 0.0) {
          Hue = 0;   // value for grey pixel
        } else {
          Hue = atan2f(ValueB,ValueA);
        }
        while (Hue < 0) Hue += 2.*ptPI;
        Col = powf(ValueA * ValueA + ValueB * ValueB, 0.5);

        Hue += ((float)Curve->Curve[CLIP((int32_t)(Hue*InvScalePi))]-(float)0x7fff)*ScalePi;

        m_Image[i][1] = CLIP((int32_t)((cosf(Hue)*Col)+WPH));
        m_Image[i][2] = CLIP((int32_t)((sinf(Hue)*Col)+WPH));
      }

  } else { // by luma
#pragma omp parallel for schedule(static) private(ValueA, ValueB, Col, Hue)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {

      ValueA = (float)m_Image[i][1]-WPH;
      ValueB = (float)m_Image[i][2]-WPH;

      if (ValueA == 0.0 && ValueB == 0.0) {
        Hue = 0;   // value for grey pixel
      } else {
        Hue = atan2f(ValueB,ValueA);
      }
      Col = powf(ValueA * ValueA + ValueB * ValueB, 0.5);

      Hue += ((float)Curve->Curve[m_Image[i][0]]-(float)0x7fff)*ScalePi;

      m_Image[i][1] = CLIP((int32_t)((cosf(Hue)*Col)+WPH));
      m_Image[i][2] = CLIP((int32_t)((sinf(Hue)*Col)+WPH));
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply Saturation Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplySaturationCurve(const ptCurve *Curve,
                                       const short Mode) {

// Best solution would be to use the Lab <-> Lch conversion from lcms.
// This should be faster without sacrificing much quality.

  assert (m_ColorSpace == ptSpace_Lab);
  // neutral value for a* and b* channel
  const float WPH = 0x8080;
  const float InvScalePi = 0x7fff / ptPI;

  float ValueA = 0.0;
  float ValueB = 0.0;

  if (Curve->mask() == ptCurve::ChromaMask) { // by chroma
#pragma omp parallel for schedule(static) private(ValueA, ValueB)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by hue
      ValueA = (float)m_Image[i][1]-WPH;
      ValueB = (float)m_Image[i][2]-WPH;
      float Hue = 0;
      if (ValueA == 0.0 && ValueB == 0.0) {
        Hue = 0;   // value for grey pixel
      } else {
        Hue = atan2f(ValueB,ValueA);
      }
      while (Hue < 0) Hue += 2.*ptPI;

      float Factor = Curve->Curve[CLIP((int32_t)(Hue*InvScalePi))]/(float)0x7fff;
      if (Factor == 1.0) continue;
      Factor *= Factor;
      float m = 0;
      if (Mode == 1) {
        float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.125);
        Col /= 0xd; // normalizing to 0..1

        if (Factor > 1)
          // work more on desaturated pixels
          m = Factor*(1-Col)+Col;
        else
          // work more on saturated pixels
          m = Factor*Col+(1-Col);
      } else {
        m = Factor;
      }
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1.0 - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1.0 - m)));
    }

  } else { // by luma
#pragma omp parallel for schedule(static) private(ValueA, ValueB)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by luminance
      float Factor = Curve->Curve[m_Image[i][0]]/(float)0x7fff;
      if (Factor == 1.0) continue;
      Factor *= Factor;
      float m = 0;
      if (Mode == 1) {
        ValueA = (float)m_Image[i][1]-WPH;
        ValueB = (float)m_Image[i][2]-WPH;
        float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.125);
        Col /= 0xd; // normalizing to 0..1

        if (Factor > 1)
          // work more on desaturated pixels
          m = Factor*(1-Col)+Col;
        else
          // work more on saturated pixels
          m = Factor*Col+(1-Col);
      } else {
        m = Factor;
      }
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1.0 - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1.0 - m)));
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply Texture Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplyTextureCurve(const ptCurve *Curve, const short Scaling) {
  assert (m_ColorSpace == ptSpace_Lab);

  const TChannelMask ChannelMask = ChMask_L;

  const float Threshold   = 10.0/pow(2,Scaling);
  const float Softness    = 0.01;
  const float Opacity     = 1.0;
  const float EdgeControl = 1.0;

  float hValueA = 0.0;
  float hValueB = 0.0;
  float m       = 0.0;

  ptImage *ContrastLayer = new ptImage;
  ContrastLayer->Set(this);

  ContrastLayer->fastBilateralChannel(Threshold, Softness, 2, ChannelMask);

  if (Curve->mask() == ptCurve::ChromaMask) {
#pragma omp parallel for schedule(static) private(hValueA, hValueB, m)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by hue
      hValueA    = ToFloatABNeutral[m_Image[i][1]];
      hValueB    = ToFloatABNeutral[m_Image[i][2]];

      float hHue = ToHue(hValueA, hValueB);
      float hCol = powf(ptSqr(hValueA) + ptSqr(hValueB), 0.125f);
      hCol /= 0x7; // normalizing to 0..2

      float Factor = Curve->Curve[CLIP((int32_t)(hHue/ptPI*ptWPHf))]/(float)0x3fff - 1.0f;
      m = 20.0f * Factor * hCol;

      ContrastLayer->m_Image[i][0] = CLIP((int32_t) ((ptWPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
      if (Factor < 0) ContrastLayer->m_Image[i][0] = ToInvertTable[ContrastLayer->m_Image[i][0]];
      if (fabsf(Factor*hCol)<0.1f) continue;
      ContrastLayer->m_Image[i][0] = Sigmoidal_4_Value(ContrastLayer->m_Image[i][0], m);
    }

  } else { // by luma
#pragma omp parallel for schedule(static) private(m)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by luminance
      float Factor = Curve->Curve[m_Image[i][0]]/(float)0x3fff - 1.0;
      m = 20.0f * Factor;

      ContrastLayer->m_Image[i][0] = CLIP((int32_t) ((ptWPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
      if (Factor < 0) ContrastLayer->m_Image[i][0] = ToInvertTable[ContrastLayer->m_Image[i][0]];
      if (fabsf(Factor)<0.1f) continue;
      ContrastLayer->m_Image[i][0] = Sigmoidal_4_Value(ContrastLayer->m_Image[i][0], m);
    }
  }

  if (EdgeControl)
    ContrastLayer->WaveletDenoise(ChannelMask, EdgeControl, 0.2, 0);

  Overlay(ContrastLayer->m_Image,Opacity,NULL);

  delete ContrastLayer;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Sigmoidal Contrast
// This can be done with a curve, but this should be a bit more effective.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::SigmoidalContrast(const double Contrast,
                                    const double Threshold,
                                    const short ChannelMask) {

  int Channels = 0;
  int Channel[3] = {0,1,2};
  if (ChannelMask & 1) {Channel[Channels] = 0; Channels++;}
  if (ChannelMask & 2) {Channel[Channels] = 1; Channels++;}
  if (ChannelMask & 4) {Channel[Channels] = 2; Channels++;}

  uint16_t ContrastTable[0x10000];
  SigmoidalTable(ContrastTable, Contrast, Threshold);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
    for (int c = 0; c<Channels; c++)
      m_Image[i][Channel[c]] = ContrastTable[ m_Image[i][Channel[c]] ];
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Crop
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Crop(const uint16_t X,
                       const uint16_t Y,
                       const uint16_t W,
                       const uint16_t H) {

  assert(m_Colors ==3);
  assert( (X+W) <= m_Width);
  assert( (Y+H) <= m_Height);

  ptImage hCroppedImage;
  hCroppedImage.setSize((size_t)W*H);

#pragma omp parallel for
  for (uint16_t Row=0;Row<H;Row++) {
    for (uint16_t Column=0;Column<W;Column++) {
      hCroppedImage.m_Data[Row*W+Column] = m_Data[(Y+Row)*m_Width+X+Column];
    }
  }

  setSize((size_t) W*H);

  m_Data   = hCroppedImage.m_Data;
  m_Width  = W;
  m_Height = H;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Overlay
//
////////////////////////////////////////////////////////////////////////////////

// macros are bad, but for lack of a better idea...
#define Value_4_Amount(AValue)      CLIP((int32_t)  ((AValue)*Amount  + Source*CompAmount))
#define Value_4_Amount_Mask(AValue) CLIP((int32_t) (((AValue)*Mask[i] + Source*(1.0f - Mask[i]))*Amount + Source*CompAmount))
#define LoopBody(ABlock, AResult) \
      if (!Mask) { \
        for (short Ch=0; Ch<3; Ch++) { \
          if  (! (ChannelMask & (1<<Ch))) continue; \
_Pragma("omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)") \
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) { \
            Source   = SourceImage[i][Ch]; \
            Blend    = BlendImage[i][Ch]; \
            { ABlock } \
            m_Image[i][Ch] = Value_4_Amount(AResult); \
          } \
        } \
      } else { \
        for (short Ch=0; Ch<3; Ch++) { \
          if  (! (ChannelMask & (1<<Ch))) continue; \
_Pragma("omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)") \
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) { \
            if (Mask[i] == 0.0f) continue; \
            Source   = SourceImage[i][Ch]; \
            Blend    = BlendImage[i][Ch]; \
            { ABlock } \
            m_Image[i][Ch] = Value_4_Amount_Mask(AResult); \
          } \
        } \
      }

ptImage* ptImage::Overlay(uint16_t    (*OverlayImage)[3],
                          const float  Amount,
                          const float  *Mask,
                          const short   Mode /* SoftLight */,
                          const short   Swap /* = 0 */) {

  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;
  float    Multiply   = 0;
  float    Screen     = 0;
  float    Overlay    = 0;
  uint16_t Source     = 0;
  uint16_t Blend      = 0;
  float    Temp       = 0;
  float    CompAmount = 1.0 - Amount;
  uint16_t (*SourceImage)[3];
  uint16_t (*BlendImage)[3];
  if (!Swap) {
    SourceImage   = m_Image;
    BlendImage    = OverlayImage;
  } else {
    BlendImage    = m_Image;
    SourceImage   = OverlayImage;
  }

  switch (Mode) {
    case ptOverlayMode_None: // just for completeness
      break;

    case ptOverlayMode_SoftLight:
      LoopBody({
        Multiply = (float)Source*Blend*ptInvWP;
        Screen   = ptWPf - (float)ToInvertTable[Source]*ToInvertTable[Blend]*ptInvWP;
        Overlay  = (ToInvertTable[Source]*Multiply + Source*Screen)*ptInvWP;
      }, Overlay)
      break;

    case ptOverlayMode_Multiply:
      LoopBody({
        Multiply = (float)Source*Blend*ptInvWP;
      }, Multiply)
      break;

    case ptOverlayMode_Screen:
      LoopBody({
        Screen = ptWPf - (float)ToInvertTable[Source]*ToInvertTable[Blend]*ptInvWP;
      }, Screen)
      break;

    case ptOverlayMode_GammaDark:
      LoopBody({
        if (Blend == 0) Multiply = 0;
        else            Multiply = ptWPf*powf(Source*ptInvWP,ptWPf/Blend);
      }, Multiply)
      break;

    case ptOverlayMode_GammaBright:
      LoopBody({
        if (Blend == ptWP) Multiply = ptWPf;
        else               Multiply = ptWPf-ptWPf*powf(ToInvertTable[Source]*ptInvWP,ptWPf/(float)ToInvertTable[Blend]);
      }, Multiply)
      break;

    case ptOverlayMode_Normal:
      LoopBody({
      }, Blend)
      break;

    case ptOverlayMode_Lighten:
      if (!Mask) {
        for (short Ch=0; Ch<3; Ch++) {
          // Is it a channel we are supposed to handle ?
          if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            Source   = SourceImage[i][Ch];
            Blend    = BlendImage[i][Ch];
            m_Image[i][Ch] = CLIP((int32_t) (ptMax((uint16_t)(Blend*Amount), Source)));
          }
        }
      } else {
        for (short Ch=0; Ch<3; Ch++) {
          // Is it a channel we are supposed to handle ?
          if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            if (Mask[i] == 0.0f) continue;
            Source   = SourceImage[i][Ch];
            Blend    = BlendImage[i][Ch];
            m_Image[i][Ch] = CLIP((int32_t) (ptMax((uint16_t)(Blend*Mask[i] + Source*(1.0f-Mask[i])*Amount),Source)));
          }
        }
      }
      break;

  case ptOverlayMode_Darken:
    if (!Mask) {
      for (short Ch=0; Ch<3; Ch++) {
        // Is it a channel we are supposed to handle ?
        if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
        for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          Source   = SourceImage[i][Ch];
          Blend    = BlendImage[i][Ch];
          m_Image[i][Ch] = CLIP((int32_t) (ptMin((uint16_t)(Blend*Amount), Source)));
        }
      }
    } else {
      for (short Ch=0; Ch<3; Ch++) {
        // Is it a channel we are supposed to handle ?
        if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
        for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          if (Mask[i] == 0.0f) continue;
          Source   = SourceImage[i][Ch];
          Blend    = BlendImage[i][Ch];
          m_Image[i][Ch] = CLIP((int32_t) (ptMin((uint16_t)(Blend*Mask[i]+Source*(1.0f-Mask[i])*Amount),Source)));
        }
      }
    }
    break;

    case ptOverlayMode_Overlay:
      LoopBody({
        if (Source <= ptWPH) Overlay = Source*Blend*ptInvWP;
        else                 Overlay = ptWPf - ToInvertTable[Source]*ToInvertTable[Blend]*ptInvWP;
      }, Overlay)
      break;

    case ptOverlayMode_GrainMerge:
      LoopBody({
      }, (float)Blend + Source - ptWPHf)
      break;

    case ptOverlayMode_ColorDodge: // a/(1-b)
      LoopBody({
        if (Source == 0)     Temp = 0;
        else {
          if (Blend == ptWP) Temp = ptWPf;
          else               Temp = (float)Source / (1.0f - (float)Blend*ptInvWP);
        }
      }, Temp)
      break;

    case ptOverlayMode_ColorBurn: // 1-(1-a)/b
      LoopBody({
        if (Source == ptWP) Temp = ptWPf;
        else {
          if (Blend == 0)   Temp = 0;
          else              Temp = ptWPf - ( ToInvertTable[Source] / (Blend*ptInvWP));
        }
      }, Temp)
      break;

    case ptOverlayMode_ShowMask:
      if (Mask) {
        for (short Ch=0; Ch<3; Ch++) {
          // Is it a channel we are supposed to handle ?
          if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            m_Image[i][Ch] = CLIP((int32_t) (Mask[i]*ptWPf));
          }
        }
      }
      break;

    case ptOverlayMode_Replace: // Replace, just for testing
      for (short Ch=0; Ch<3; Ch++) {
        // Is it a channel we are supposed to handle ?
        if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
        for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          Blend    = BlendImage[i][Ch];
          m_Image[i][Ch] = CLIP((int32_t) Blend);
        }
      }
      break;

  }
  return this;
}
#undef Value_4_Amount
#undef Value_4_Amount_Mask
#undef LoopBody

////////////////////////////////////////////////////////////////////////////////
//
// Flip
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Flip(const short FlipMode) {

  uint16_t Width = m_Width;
  uint16_t Height = m_Height;
  uint16_t Cache = 0;
  if (FlipMode == ptFlipMode_Vertical) {
#pragma omp parallel for default(shared) private(Cache)
    for (uint16_t Row=0;Row<Height/2;Row++) {
      for (uint16_t Column=0;Column<Width;Column++) {
        Cache = m_Image[Row*Width+Column][0];
        m_Image[Row*Width+Column][0] = m_Image[(Height-1-Row)*Width+Column][0];
        m_Image[(Height-1-Row)*Width+Column][0] = Cache;
        Cache = m_Image[Row*Width+Column][1];
        m_Image[Row*Width+Column][1] = m_Image[(Height-1-Row)*Width+Column][1];
        m_Image[(Height-1-Row)*Width+Column][1] = Cache;
        Cache = m_Image[Row*Width+Column][2];
        m_Image[Row*Width+Column][2] = m_Image[(Height-1-Row)*Width+Column][2];
        m_Image[(Height-1-Row)*Width+Column][2] = Cache;
      }
    }
  } else if (FlipMode == ptFlipMode_Horizontal) {
#pragma omp parallel for default(shared) private(Cache)
    for (uint16_t Row=0;Row<Height;Row++) {
      for (uint16_t Column=0;Column<Width/2;Column++) {
        Cache = m_Image[Row*Width+Column][0];
        m_Image[Row*Width+Column][0] = m_Image[Row*Width+(Width-1-Column)][0];
        m_Image[Row*Width+(Width-1-Column)][0] = Cache;
        Cache = m_Image[Row*Width+Column][1];
        m_Image[Row*Width+Column][1] = m_Image[Row*Width+(Width-1-Column)][1];
        m_Image[Row*Width+(Width-1-Column)][1] = Cache;
        Cache = m_Image[Row*Width+Column][2];
        m_Image[Row*Width+Column][2] = m_Image[Row*Width+(Width-1-Column)][2];
        m_Image[Row*Width+(Width-1-Column)][2] = Cache;
      }
    }
  }
  return this;
}


////////////////////////////////////////////////////////////////////////////////
//
// Levels
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Levels(const float BlackPoint,
                         const float WhitePoint) {

  const float WP = 0xffff;
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

  if (fabs(BlackPoint-WhitePoint)>0.001) {
    float m = 1.0/(WhitePoint-BlackPoint);
    float t = -BlackPoint/(WhitePoint-BlackPoint)*WP;
#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      for (short Ch=0; Ch<NrChannels; Ch++) {
        m_Image[i][Ch] = CLIP((int32_t)(m_Image[i][Ch]*m+t));
      }
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// DeFringe
// Original implementation by Emil Martinec for RawTherapee
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::DeFringe(const double Radius,
                           const short Threshold,
                           const int Flags,
                           const double Shift) {

  assert (m_ColorSpace == ptSpace_Lab);

  int Neighborhood = ceil(2*Radius)+1;

  float (*ChromaDiff) = (float(*)) CALLOC(m_Width*m_Height,sizeof(*ChromaDiff));
  ptMemoryError(ChromaDiff,__FILE__,__LINE__);

  ptImage *SaveLayer = new ptImage;
  SaveLayer->Set(this);

  ptCIBlur(Radius, 6);

  uint32_t Size = m_Width * m_Height;

  float Average = 0.0f;
#pragma omp parallel for schedule(static) reduction (+:Average)
  for (uint32_t i = 0; i < Size; i++) {
    ChromaDiff[i] = SQR((float)m_Image[i][1]-(float)SaveLayer->m_Image[i][1]) +
                    SQR((float)m_Image[i][2]-(float)SaveLayer->m_Image[i][2]);
    Average += ChromaDiff[i];
  }
  Average /= Size;

  float NewThreshold = Threshold*Average/33.0f;
  float HueShift = Shift * ptPI/6;
  float Val1 = MAX(0.f,HueShift);
  float Val2 = ptPI/3+HueShift;
  float Val3 = 2*ptPI/3+HueShift;
  float Val4 = 3*ptPI/3+HueShift;
  float Val5 = 4*ptPI/3+HueShift;
  float Val6 = 5*ptPI/3+HueShift;
  float Val7 = 6*ptPI/3+HueShift;

#pragma omp parallel for schedule(dynamic)
  for (uint16_t Row = 0; Row < m_Height; Row++) {
    for (uint16_t Col = 0; Col < m_Width; Col++) {
      short CorrectPixel = 0;
      uint32_t Index = Row*m_Width+Col;
      if (ChromaDiff[Index] > NewThreshold) {
        // Calculate hue.
        float ValueA = (float)SaveLayer->m_Image[Index][1]-0x8080;
        float ValueB = (float)SaveLayer->m_Image[Index][2]-0x8080;
        float Hue = 0;
        if (ValueA == 0.0 && ValueB == 0.0) {
          Hue = 0;   // value for grey pixel
        } else {
          Hue = atan2f(ValueB,ValueA);
        }
        while (Hue < 0) Hue += 2.*ptPI;
        // Check if we want to treat that hue
        if (Flags & 1) // red
          if ((Hue >= Val1 && Hue < Val2) || Hue >= Val7) CorrectPixel = 1;
        if (Flags & 2) // yellow
          if (Hue >= Val2 && Hue < Val3) CorrectPixel = 1;
        if (Flags & 4) // green
          if (Hue >= Val3 && Hue < Val4) CorrectPixel = 1;
        if (Flags & 8) // cyan
          if (Hue >= Val4 && Hue < Val5) CorrectPixel = 1;
        if (Flags & 16) // blue
          if (Hue >= Val5 && Hue < Val6) CorrectPixel = 1;
        if (Flags & 32) // purple
          if ((Hue >= Val6 && Hue < Val7) || Hue < Val1) CorrectPixel = 1;
      }
      if (CorrectPixel == 1 ) {
        float TotalA=0;
        float TotalB=0;
        float Total=0;
        float Weight;
        for (int i1 = MAX(0,Row-Neighborhood+1); i1 < MIN((int)m_Height,Row+Neighborhood); i1++)
          for (int j1 = MAX(0,Col-Neighborhood+1); j1 < MIN((int)m_Width,Col+Neighborhood); j1++) {
            // Neighborhood average of pixels weighted by chrominance
            uint32_t Index2 = i1*m_Width+j1;
            Weight = 1/(ChromaDiff[Index2]+Average);
            TotalA += Weight*SaveLayer->m_Image[Index2][1];
            TotalB += Weight*SaveLayer->m_Image[Index2][2];
            Total += Weight;
          }
        m_Image[Index][1] = CLIP((int32_t)(TotalA/Total));
        m_Image[Index][2] = CLIP((int32_t)(TotalB/Total));
      } else {
        m_Image[Index][1] = SaveLayer->m_Image[Index][1];
        m_Image[Index][2] = SaveLayer->m_Image[Index][2];
      }
    }
  }

  delete SaveLayer;
  FREE(ChromaDiff);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Impluse noise reduction
// Original implementation by Emil Martinec for RawTherapee
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::DenoiseImpulse(const double ThresholdL,
                                 const double ThresholdAB) {

  assert (m_ColorSpace == ptSpace_Lab);

  float hpfabs, hfnbrave;
  float hpfabs1, hfnbrave1, hpfabs2, hfnbrave2;

  ptImage *LowPass = new ptImage;
  LowPass->Set(this);

  short ChannelMask = 0;
  if (ThresholdL != 0.0) ChannelMask += 1;
  if (ThresholdAB != 0.0) ChannelMask += 6;
  LowPass->ptCIBlur(2.0f, ChannelMask);

  short (*Impulse)[3] = (short (*)[3]) CALLOC(m_Width*m_Height,sizeof(*Impulse));
  ptMemoryError(Impulse,__FILE__,__LINE__);

  static float eps = 1.0;
  float wtdsum, dirwt, norm;

  uint32_t Index = 0;
  uint32_t Index1 = 0;

  if (ThresholdL != 0.0) {
#pragma omp parallel for schedule(static) private(hpfabs, hfnbrave, Index, Index1)
    for (uint16_t Row = 0; Row < m_Height; Row++) {
      for (uint16_t Col = 0; Col < m_Width; Col++) {
        Index = Row*m_Width+Col;
        hpfabs = fabs((int32_t)m_Image[Index][0] - (int32_t)LowPass->m_Image[Index][0]);
        hfnbrave = 0;
        //block average of high pass data
        for (uint16_t i1 = MAX(0,Row-2); i1 <= MIN(Row+2,m_Height-1); i1++ ) {
          for (uint16_t j1 = MAX(0,Col-2); j1 <= MIN(Col+2,m_Width-1); j1++ ) {
            Index1 = i1*m_Width+j1;
            hfnbrave += fabs((int32_t)m_Image[Index1][0] - (int32_t)LowPass->m_Image[Index1][0]);
          }
        }
        hfnbrave = (hfnbrave - hpfabs) / 24.0f;
        hpfabs > (hfnbrave*(5.5-ThresholdL)) ? Impulse[Index][0]=1 : Impulse[Index][0]=0;
      }//now impulsive values have been identified
    }
  }

  if (ThresholdAB != 0.0) {
#pragma omp parallel for schedule(static) private(hpfabs1, hfnbrave1, hpfabs2, hfnbrave2, Index, Index1)
    for (uint16_t Row = 0; Row < m_Height; Row++) {
      for (uint16_t Col = 0; Col < m_Width; Col++) {
        Index = Row*m_Width+Col;
        hpfabs1 = fabs((int32_t)m_Image[Index][1] - (int32_t)LowPass->m_Image[Index][1]);
        hpfabs2 = fabs((int32_t)m_Image[Index][2] - (int32_t)LowPass->m_Image[Index][2]);
        hfnbrave1 = 0;
        hfnbrave2 = 0;
        //block average of high pass data
        for (uint16_t i1 = MAX(0,Row-2); i1 <= MIN(Row+2,m_Height-1); i1++ ) {
          for (uint16_t j1 = MAX(0,Col-2); j1 <= MIN(Col+2,m_Width-1); j1++ ) {
            Index1 = i1*m_Width+j1;
            hfnbrave1 += fabs((int32_t)m_Image[Index1][1] - (int32_t)LowPass->m_Image[Index1][1]);
            hfnbrave2 += fabs((int32_t)m_Image[Index1][2] - (int32_t)LowPass->m_Image[Index1][2]);
          }
        }
        hfnbrave1 = (hfnbrave1 - hpfabs1) / 24.0f;
        hfnbrave2 = (hfnbrave2 - hpfabs2) / 24.0f;
        hpfabs1 > (hfnbrave1*(5.5-ThresholdAB)) ? Impulse[Index][1]=1 : Impulse[Index][1]=0;
        hpfabs2 > (hfnbrave2*(5.5-ThresholdAB)) ? Impulse[Index][2]=1 : Impulse[Index][2]=0;
      }//now impulsive values have been identified
    }
  }

  for (short Channel=0;Channel<3;Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for schedule(static) private(norm, wtdsum, dirwt, Index, Index1)
    for (uint16_t Row = 0; Row < m_Height; Row++) {
      for (uint16_t Col = 0; Col < m_Width; Col++) {
        Index = Row*m_Width+Col;
        if (!Impulse[Index][Channel]) continue;
        norm=0.0;
        wtdsum=0.0;
        for (uint16_t i1 = MAX(0,Row-2); i1 <= MIN(Row+2,m_Height-1); i1++ ) {
          for (uint16_t j1 = MAX(0,Col-2); j1 <= MIN(Col+2,m_Width-1); j1++ ) {
            if (i1==Row && j1==Col) continue;
            Index1 = i1*m_Width+j1;
            if (Impulse[Index1][Channel]) continue;
            float Temp = (float)m_Image[Index1][Channel]-(float)m_Image[Index][Channel];
            dirwt = 1/(SQR(Temp)+eps);//use more sophisticated rangefn???
            wtdsum += dirwt*m_Image[Index1][Channel];
            norm += dirwt;
          }
        }
        //wtdsum /= norm;
        if (norm) {
          m_Image[Index][Channel] = CLIP((int32_t) (wtdsum/norm));//low pass filter
        }
      }
    }//now impulsive values have been corrected
  }


  FREE(Impulse);
  delete LowPass;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Reinhard 05 tone mapping operator
// Adapted from PFSTMO package, original Copyright (C) 2007 Grzegorz Krawczyk
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Reinhard05(const float Brightness,
                             const float Chromatic,
                             const float Light)
{
  assert (m_ColorSpace != ptSpace_Lab);
  // OpenMP is done for RGB
  // const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const short NrChannels = 3;

  const float DChromatic = 1 - Chromatic;
  const float DLight = 1 - Light;

  uint32_t m_Size = m_Width * m_Height;
  float (*Temp)[3] = (float (*)[3]) CALLOC(m_Size,sizeof(*Temp));
  ptMemoryError(Temp,__FILE__,__LINE__);

  // luminance
  float (*Y) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*Y));
  ptMemoryError(Y,__FILE__,__LINE__);

  if (NrChannels == 1)
#pragma omp parallel for schedule(static)
    for (uint32_t i=0; i < m_Size; i++) {
      Y[i] = ToFloatTable[m_Image[i][0]];
    }
  else
#pragma omp parallel for schedule(static)
    for (uint32_t i=0; i < m_Size; i++) {
      Y[i] = ToFloatTable[RGB_2_L(m_Image[i])];
    }

  float max_lum   = 0.0f;
  float min_lum   = 0.0f;
  float world_lum = 0.0f;
  float Cav[]     = {0.0f, 0.0f, 0.0f};
  float Cav1      = 0.0f;
  float Cav2      = 0.0f;
  float Cav3      = 0.0f;
  float Lav       = 0.0f;

#pragma omp parallel
{
  float thread_max = 0.0;
  float thread_min = 0.0;
#pragma omp for reduction(+:world_lum,Cav1,Cav2,Cav3,Lav)
  for (uint32_t i=0; i < m_Size; i++) {
    float lum = Y[i];
    thread_max = (thread_max > lum) ? thread_max : lum;
    thread_min = (thread_min < lum) ? thread_min : lum;
    world_lum += logf(2.3e-5+lum);
    Cav1 += m_Image[i][0];
    Cav2 += m_Image[i][1];
    Cav3 += m_Image[i][2];
    Lav += lum;
  }
#pragma omp critical
  if (thread_max > max_lum) {
    max_lum = thread_max;
  }
#pragma omp critical
  if (thread_min < min_lum) {
    min_lum = thread_min;
  }
}
  Cav[0] = Cav1;
  Cav[1] = Cav2;
  Cav[2] = Cav3;
  world_lum /= (float)m_Size;
  for (int c = 0; c < NrChannels; c++)
    Cav[c] = Cav[c] / ((float)m_Size * (float)0xffff);
  Lav /= (float)m_Size;

  //--- tone map image
  max_lum = logf( max_lum );
  min_lum = logf( min_lum );

  // image key
  float k = (max_lum - world_lum) / (max_lum - min_lum);
  // image contrast based on key value
  float m = 0.3f+0.7f*powf(k,1.4f);
  // image brightness
  float f = expf(-Brightness);

  float max_col = 0.0f;
  float min_col = 1.0f;

#pragma omp parallel
{
  float thread_max = 0.0f;
  float thread_min = 1.0f;
#pragma omp for
  for (uint32_t i=0; i < m_Size; i++) {
    float l = Y[i];
    float col;
    if( l != 0.0f ) {
      for (int c = 0; c < NrChannels; c++) {
        col = ToFloatTable[m_Image[i][c]];

        if(col != 0.0f) {
          // local light adaptation
          float Il = Chromatic * col + DChromatic * l;
          // global light adaptation
          float Ig = Chromatic * Cav[c] + DChromatic * Lav;
          // interpolated light adaptation
          float Ia = Light*Il + DLight * Ig;
          // photoreceptor equation
          col /= col + powf(f*Ia, m);
        }

        thread_max = (col>thread_max) ? col : thread_max;
        thread_min = (col<thread_min) ? col : thread_min;

        Temp[i][c]=col;
      }
    }
  }
#pragma omp critical
  if (thread_max > max_col) {
    max_col = thread_max;
  }
#pragma omp critical
  if (thread_min < min_col) {
    min_col = thread_min;
  }
}

  //--- normalize intensities
#pragma omp parallel for schedule(static)
  for (uint32_t i=0; i < m_Size; i++) {
    for (int c = 0; c < NrChannels; c++)
      m_Image[i][c] = CLIP((int32_t) ((Temp[i][c]-min_col)/(max_col-min_col)*0xffff));
  }

  FREE (Temp);
  FREE (Y);

  return this;
}

//------------------------------------------------------------------------------

ptImage* ptImage::ColorIntensity(int AVibrance, int ARed, int AGreen, int ABlue) {
  TChannelMatrix hMixer;

  if (AVibrance != 0) {
    hMixer[0][0] = 1.0 + (AVibrance/150.0);
    hMixer[0][1] = -(AVibrance/300.0);
    hMixer[0][2] = hMixer[0][1];
    hMixer[1][0] = hMixer[0][1];
    hMixer[1][1] = hMixer[0][0];
    hMixer[1][2] = hMixer[0][1];
    hMixer[2][0] = hMixer[0][1];
    hMixer[2][1] = hMixer[0][1];
    hMixer[2][2] = hMixer[0][0];

    this->mixChannels(hMixer);
  }

  if ((ARed != 0) || (AGreen != 0) || (ABlue != 0)) {
    hMixer[0][0] = 1.0 + (ARed/150.0);
    hMixer[0][1] = -(ARed/300.0);
    hMixer[0][2] = hMixer[0][1];
    hMixer[1][0] = -(AGreen/300.0);
    hMixer[1][1] = 1.0+(AGreen/150.0);;
    hMixer[1][2] = hMixer[1][0];
    hMixer[2][0] = -(ABlue/300.0);
    hMixer[2][1] = hMixer[2][0];
    hMixer[2][2] = 1.0+(ABlue/150.0);

    this->mixChannels(hMixer);
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Color Boost
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ColorBoost(const double ValueA,
                             const double ValueB) {

  assert ((m_ColorSpace == ptSpace_Lab));

  const double WPH = 0x8080; // Neutral in Lab A and B

  double t1 = (1-ValueA)*WPH;
  double t2 = (1-ValueB)*WPH;
  if (ValueA!=1.0) {
#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1]*ValueA+t1));
    }
  }
  if (ValueB!=1.0) {
#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2]*ValueB+t2));
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LCH conversion and L adjustment
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LumaAdjust(const double LC1, // 8 colors for L
                          const double LC2,
                          const double LC3,
                          const double LC4,
                          const double LC5,
                          const double LC6,
                          const double LC7,
                            const double LC8)
{
  assert (m_ColorSpace == ptSpace_Lab);
  float WPH = 0x7fff;
  float IQPI = 4/ptPI;

#pragma omp parallel for schedule(static)
  for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
    float Col = powf(((float)m_Image[i][1]-WPH)*((float)m_Image[i][1]-WPH) +
          ((float)m_Image[i][2]-WPH)*((float)m_Image[i][2]-WPH), 0.25);
    Col /= 0xb5; // normalizing to 0..1, sqrt(0x7fff)
    float Hue = 0;
    if (m_Image[i][1] == WPH && m_Image[i][2] == WPH) {
      Hue = 0;   // value for grey pixel
    } else {
      Hue = atan2f((float)m_Image[i][2]-WPH,
      (float)m_Image[i][1]-WPH);
    }
    while (Hue < 0) Hue += 2.*ptPI;

    if ( LC1 != 0 && Hue > -.1 && Hue < ptPI/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-0)*IQPI)*LC1*Col)));
    if ( LC2 != 0 && Hue > 0 && Hue < ptPI/2)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI/4)*IQPI)*LC2*Col)));
    if ( LC3 != 0 && Hue > ptPI/4 && Hue < ptPI*3/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI/2)*IQPI)*LC3*Col)));
    if ( LC4 != 0 && Hue > ptPI/2 && Hue < ptPI)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI*3/4)*IQPI)*LC4*Col)));
    if ( LC5 != 0 && Hue > ptPI*3/4 && Hue < ptPI*5/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI)*IQPI)*LC5*Col)));
    if ( LC6 != 0 && Hue > ptPI && Hue < ptPI*6/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI*5/4)*IQPI)*LC6*Col)));
    if ( LC7 != 0 && Hue > ptPI*5/4 && Hue < ptPI*7/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI*6/4)*IQPI)*LC7*Col)));
    if ( LC8 != 0 && Hue > ptPI*6/4 && Hue < ptPI*8/4)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI*7/4)*IQPI)*LC8*Col)));
    if ( LC1 != 0 && Hue > ptPI*7/4 && Hue < ptPI*2.1)
      m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * powf(2,(1.-fabsf(Hue-ptPI*2)*IQPI)*LC1*Col)));
  }

  return this;
}

//==============================================================================

ptImage* ptImage::SatAdjust(const double SC1, // 8 colors for saturation
                            const double SC2,
                            const double SC3,
                            const double SC4,
                            const double SC5,
                            const double SC6,
                            const double SC7,
                            const double SC8)
{
  assert (m_ColorSpace == ptSpace_Lab);
  float WPH = 0x7fff;
  float IQPI = 4/ptPI;

#pragma omp parallel for schedule(static)
  for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
    float Col = powf(((float)m_Image[i][1]-WPH)*((float)m_Image[i][1]-WPH) +
          ((float)m_Image[i][2]-WPH)*((float)m_Image[i][2]-WPH), 0.25);
    Col /= 0xb5; // normalizing to 0..1, sqrt(0x7fff)
    float Hue = 0;
    if (m_Image[i][1] == WPH && m_Image[i][2] == WPH) {
      Hue = 0;   // value for grey pixel
    } else {
      Hue = atan2f((float)m_Image[i][2]-WPH,
      (float)m_Image[i][1]-WPH);
    }
    while (Hue < 0) Hue += 2.*ptPI;

    float m = 0;
    if ( SC1 != 0 && Hue > -.1 && Hue < ptPI/4) {
      m = powf(8,(1.-fabsf(Hue-0)*IQPI)*SC1*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC2 != 0 && Hue > 0 && Hue < ptPI/2) {
      m = powf(8,(1.-fabsf(Hue-ptPI/4)*IQPI)*SC2*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC3 != 0 && Hue > ptPI/4 && Hue < ptPI*3/4) {
      m = powf(8,(1.-fabsf(Hue-ptPI/2)*IQPI)*SC3*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC4 != 0 && Hue > ptPI/2 && Hue < ptPI) {
      m = powf(8,(1.-fabsf(Hue-ptPI*3/4)*IQPI)*SC4*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC5 != 0 && Hue > ptPI*3/4 && Hue < ptPI*5/4) {
      m = powf(8,(1.-fabsf(Hue-ptPI)*IQPI)*SC5*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC6 != 0 && Hue > ptPI && Hue < ptPI*6/4) {
      m = powf(8,(1.-fabsf(Hue-ptPI*5/4)*IQPI)*SC6*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC7 != 0 && Hue > ptPI*5/4 && Hue < ptPI*7/4) {
      m = powf(8,(1.-fabsf(Hue-ptPI*6/4)*IQPI)*SC7*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC8 != 0 && Hue > ptPI*6/4 && Hue < ptPI*8/4) {
      m = powf(8,(1.-fabsf(Hue-ptPI*7/4)*IQPI)*SC8*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
    if ( SC1 != 0 && Hue > ptPI*7/4 && Hue < ptPI*2.1) {
      m = powf(8,(1.-fabsf(Hue-ptPI*2)*IQPI)*SC1*Col);
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Outline
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Outline(const short Mode,
                          const short GradientMode,
                          const ptCurve *Curve,
                          const double Weight,
                          const double Radius,
                          const short SwitchLayer) {

  assert (m_ColorSpace == ptSpace_Lab);

  if (Mode == ptOverlayMode_None) return this;

  ptImage *Gradient = new ptImage;

  Gradient->Set(this);

  ptCimgEdgeDetectionSum(Gradient, Weight, GradientMode);

  Gradient->ptCIBlur(Radius, 1);

  Gradient->ApplyCurve(Curve, 1);

  if (Mode != ptOverlayMode_Replace)
    Overlay(Gradient->m_Image, 1.0f, NULL, Mode, SwitchLayer);
  else
    Overlay(Gradient->m_Image, 1.0f, NULL, Mode);

  delete Gradient;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Color Enhance
// http://docs.google.com/View?id=dsgjq79_829f9wv8ncd
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ColorEnhance(const float AShadows,
                               const float AHighlights)
{
  assert (m_ColorSpace != ptSpace_Lab);

  if (AShadows) {
    ptImage *ShadowsLayer = new ptImage;
    ShadowsLayer->Set(this);

    // Invert and greyscale
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      ShadowsLayer->m_Image[i][0] = ToInvertTable[RGB_2_L(ShadowsLayer->m_Image[i])];
      ShadowsLayer->m_Image[i][1] = ShadowsLayer->m_Image[i][2] = ShadowsLayer->m_Image[i][0];
    }

    ShadowsLayer->Overlay(m_Image, 1.0, NULL, ptOverlayMode_ColorDodge, 1 /*Swap */);
    Overlay(ShadowsLayer->m_Image, AShadows, NULL, ptOverlayMode_ColorBurn);
    delete ShadowsLayer;
  }
  // I trade processing time for memory, so invert and greyscale will be
  // recalculated to save another parallel memory instance

  if (AHighlights) {
    ptImage *HighlightsLayer = new ptImage;
    HighlightsLayer->Set(this);

    // Invert and greyscale
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      HighlightsLayer->m_Image[i][0] = ToInvertTable[RGB_2_L(HighlightsLayer->m_Image[i])];
      HighlightsLayer->m_Image[i][1] = HighlightsLayer->m_Image[i][2] = HighlightsLayer->m_Image[i][0];
    }

    HighlightsLayer->Overlay(m_Image, 1.0, NULL, ptOverlayMode_ColorBurn, 1 /*Swap */);
    Overlay(HighlightsLayer->m_Image, AHighlights, NULL, ptOverlayMode_ColorDodge);
    delete HighlightsLayer;
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LMHLightRecovery
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LMHRecovery(const TMaskType MaskType,
                              const float  Amount,
                              const float  LowerLimit,
                              const float  UpperLimit,
                              const float  Softness)
{
  const float ExposureFactor = pow(2,Amount);
  const float InverseExposureFactor = 1/ExposureFactor;

  // Precalculated table for the transform of the original.
  // The transform is an exposure (>1) or a gamma driven darkening.
  // (Table generates less math except for images with < 20K pixels)
  uint16_t TransformTable[0x10000];
#pragma omp parallel for
  for (uint32_t i=0; i<0x10000; i++) {
    if (ExposureFactor<1.0) {
      TransformTable[i] = CLIP((int32_t)(powf(i*ptInvWP,InverseExposureFactor)*ptWPf));
    } else {
      TransformTable[i] = CLIP((int32_t)(i*ExposureFactor+0.5f));
    }
  }

  const double Soft = pow(2,Softness);

  // Precalculated table for softening the mask.
  double SoftTable[0x100]; // Assuming a 256 table is fine grained enough.
  for (int16_t i=0; i<0x100; i++) {
    if (Soft>1.0) {
      SoftTable[i] = ptBound((float)(pow(i/(float)0xff,Soft)), 0.0f, 1.0f);
    } else {
      SoftTable[i] = ptBound((float)(i/(float)0xff/Soft),0.0f,1.0f);
    }
  }

  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

  const float ReciprocalRange       = 1.0f/MAX(UpperLimit-LowerLimit,0.001f);
  const float ReciprocalLowerLimit  = 1.0f/MAX(LowerLimit,0.001f);
  const float ReciprocalUpperMargin = 1.0f/MAX(1.0f-UpperLimit,0.001f);
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    // Init Mask with luminance
    float Mask = ((m_ColorSpace == ptSpace_Lab) ?
      m_Image[i][0] :
      // Remark this classic 30/59/11 should be in fact colour space
      // dependent. TODO
      RGB_2_L(m_Image[i]))*ptInvWP;
    switch(MaskType) {
      case TMaskType::Shadows:
        // Mask is an inverted luminance mask, normalized 0..1 and
        // shifted over the limits such that it clips beyond the limits.
        // The Mask varies from 1 at LowerLimit to 0 at UpperLimit
        // Meaning that deep shadows will be pulled up a lot and
        // approximating the upperlimit we will take more of the original
        // image.
        Mask = 1.0f-LIM((Mask-LowerLimit)*ReciprocalRange,0.0f,1.0f);
        break;
      case TMaskType::Midtones:
        // Not fully understood but generates a useful and nice
        // midtone luminance mask.
        Mask = 1.0f -
               LIM((LowerLimit-Mask)*ReciprocalLowerLimit,0.0f,0.1f) -
               LIM((Mask-UpperLimit)*ReciprocalUpperMargin,0.0f,1.0f);
        Mask = LIM(Mask,0.0f,1.0f);
        break;
      case TMaskType::Highlights:
        // Mask is a luminance mask, normalized 0..1 and
        // shifted over the limits such that it clips beyond the limits.
        // The Mask varies from 0 at LowerLimit to 1 at UpperLimit
        // Meaning that as from the LowerLimit on , we will take more and
        // more of the darkened image.
        Mask = LIM((Mask-LowerLimit)*ReciprocalRange,0.0f,1.0f);
        break;
      case TMaskType::All:
        Mask = 1.0f;
        break;

      default:
        GInfo->Raise(QString("Unknown mask type: ") + QString::number(static_cast<int>(MaskType)), AT);
    }

    // Softening the mask
    Mask = SoftTable[(uint8_t)(Mask*0xff+0.5)];

    // Blend transformed and original according to mask.
    for (short Ch=0; Ch<NrChannels; Ch++) {
      uint16_t PixelValue = m_Image[i][Ch];
      m_Image[i][Ch] = CLIP((int32_t)
        (TransformTable[PixelValue]*Mask + PixelValue*(1.0f-Mask)));
      // Uncomment me to 'see' the mask.
      // m_Image[i][Ch] = Mask*WP;
    }
  }
  return this;
}


////////////////////////////////////////////////////////////////////////////////
//
// Highpass
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Highpass(const double Radius,
                           const double Amount,
                           const double HaloControl,
                           const double Denoise)
{
  double LowerLimit = 0.1;
  double UpperLimit = 1 - LowerLimit;
  double Softness = 0;

  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const TChannelMask ChannelMask = (m_ColorSpace == ptSpace_Lab) ? ChMask_L : ChMask_RGB;

  ptImage *HighpassLayer = new ptImage;
  HighpassLayer->Set(this);
  HighpassLayer->ptCIBlur(Radius, ChannelMask);

  // also calculates the curve
  auto AmpCurve = new ptCurve(createAmpAnchors(Amount, HaloControl));

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=0; Ch<NrChannels; Ch++) {
      HighpassLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)HighpassLayer->m_Image[i][Ch])+m_Image[i][Ch]));
    }
  }

  HighpassLayer->ApplyCurve(AmpCurve,ChannelMask);
  delete AmpCurve;

  if (Denoise) {
    HighpassLayer->fastBilateralChannel(4.0, Denoise/3.0, 1, ChMask_L);
  }

  float (*Mask);
  Mask = (m_ColorSpace == ptSpace_Lab)?
  GetMask(TMaskType::Midtones, LowerLimit, UpperLimit, Softness,1,0,0):
  GetMask(TMaskType::Midtones, LowerLimit, UpperLimit, Softness);

  Overlay(HighpassLayer->m_Image,0.5,Mask);
  delete HighpassLayer;
  FREE(Mask);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Gradient Sharpen
//
////////////////////////////////////////////////////////////////////////////////

// To the extent possible under law, Manuel Llorens <manuelllorens@gmail.com>
// has waived all copyright and related or neighboring rights to this work.
// This code is licensed under CC0 v1.0, see license information at
// http://creativecommons.org/publicdomain/zero/1.0/

ptImage* ptImage::GradientSharpen(const short Passes,
                                  const double Strength) {

  assert (m_ColorSpace == ptSpace_Lab);

  int32_t offset,c,i,j,p,width2;
  float lumH,lumV,lumD1,lumD2,v,contrast,s;
  float difL,difR,difT,difB,difLT,difRB,difLB,difRT,wH,wV,wD1,wD2,chmax[3];
  float f1,f2,f3,f4;

  uint16_t width = m_Width;
  uint16_t height = m_Height;

  width2=2*m_Width;

  float (*L) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*L));
  ptMemoryError(L,__FILE__,__LINE__);

  chmax[0]=0.08;
  chmax[1]=3.0;
  chmax[2]=3.0;
  c = 0;

#pragma omp parallel for private(offset) schedule(static)
  for(offset=0;offset<m_Width*m_Height;offset++)
    L[offset]=ToFloatTable[m_Image[offset][c]];

  //for(c=0;c<=channels;c++)
    for(p=0;p<Passes;p++){
      #pragma omp parallel for private(j,i,offset,wH,wV,wD1,wD2,s,lumH,lumV,lumD1,lumD2,v,contrast,f1,f2,f3,f4,difT,difB,difL,difR,difLT,difLB,difRT,difRB) schedule(static)
      for(j=2;j<height-2;j++)
        for(i=2,offset=j*width+i;i<width-2;i++,offset++){
          // weight functions
          wH=fabsf(L[offset+1]-L[offset-1]);
          wV=fabsf(L[offset+width]-L[offset-width]);

          s=1.0+fabs(wH-wV)/2.0;
          wD1=fabsf(L[offset+width+1]-L[offset-width-1])/s;
          wD2=fabsf(L[offset+width-1]-L[offset-width+1])/s;
          s=wD1;
          wD1/=wD2;
          wD2/=wD1;

          // initial values
          lumH=lumV=lumD1=lumD2=v=ToFloatTable[m_Image[offset][c]];

          // contrast detection
          contrast=sqrtf(fabsf(L[offset+1]-L[offset-1])*fabsf(L[offset+1]-L[offset-1])+fabsf(L[offset+width]-L[offset-width])*fabsf(L[offset+width]-L[offset-width]))/chmax[c];
          if(contrast>1.0) contrast=1.0;

          // new possible values
          if(((L[offset]<L[offset-1])&&(L[offset]>L[offset+1])) ||
             ((L[offset]>L[offset-1])&&(L[offset]<L[offset+1]))){
            f1=fabsf(L[offset-2]-L[offset-1]);
            f2=fabsf(L[offset-1]-L[offset]);
            f3=fabsf(L[offset-1]-L[offset-width])*fabsf(L[offset-1]-L[offset+width]);
            f4=sqrtf(fabsf(L[offset-1]-L[offset-width2])*fabsf(L[offset-1]-L[offset+width2]));
            difL=f1*f2*f2*f3*f3*f4;
            f1=fabsf(L[offset+2]-L[offset+1]);
            f2=fabsf(L[offset+1]-L[offset]);
            f3=fabsf(L[offset+1]-L[offset-width])*fabsf(L[offset+1]-L[offset+width]);
            f4=sqrtf(fabs(L[offset+1]-L[offset-width2])*fabsf(L[offset+1]-L[offset+width2]));
            difR=f1*f2*f2*f3*f3*f4;
            if((difR!=0)&&(difL!=0)){
              lumH=(L[offset-1]*difR+L[offset+1]*difL)/(difL+difR);
              lumH=v*(1-contrast)+lumH*contrast;
            }
          }

          if(((L[offset]<L[offset-width])&&(L[offset]>L[offset+width])) ||
             ((L[offset]>L[offset-width])&&(L[offset]<L[offset+width]))){
            f1=fabsf(L[offset-width2]-L[offset-width]);
            f2=fabsf(L[offset-width]-L[offset]);
            f3=fabsf(L[offset-width]-L[offset-1])*fabsf(L[offset-width]-L[offset+1]);
            f4=sqrtf(fabsf(L[offset-width]-L[offset-2])*fabsf(L[offset-width]-L[offset+2]));
            difT=f1*f2*f2*f3*f3*f4;
            f1=fabsf(L[offset+width2]-L[offset+width]);
            f2=fabsf(L[offset+width]-L[offset]);
            f3=fabsf(L[offset+width]-L[offset-1])*fabsf(L[offset+width]-L[offset+1]);
            f4=sqrtf(fabsf(L[offset+width]-L[offset-2])*fabsf(L[offset+width]-L[offset+2]));
            difB=f1*f2*f2*f3*f3*f4;
            if((difB!=0)&&(difT!=0)){
              lumV=(L[offset-width]*difB+L[offset+width]*difT)/(difT+difB);
              lumV=v*(1-contrast)+lumV*contrast;
            }
          }

          if(((L[offset]<L[offset-1-width])&&(L[offset]>L[offset+1+width])) ||
             ((L[offset]>L[offset-1-width])&&(L[offset]<L[offset+1+width]))){
            f1=fabsf(L[offset-2-width2]-L[offset-1-width]);
            f2=fabsf(L[offset-1-width]-L[offset]);
            f3=fabsf(L[offset-1-width]-L[offset-width+1])*fabsf(L[offset-1-width]-L[offset+width-1]);
            f4=sqrtf(fabsf(L[offset-1-width]-L[offset-width2+2])*fabsf(L[offset-1-width]-L[offset+width2-2]));
            difLT=f1*f2*f2*f3*f3*f4;
            f1=fabsf(L[offset+2+width2]-L[offset+1+width]);
            f2=fabsf(L[offset+1+width]-L[offset]);
            f3=fabsf(L[offset+1+width]-L[offset-width+1])*fabsf(L[offset+1+width]-L[offset+width-1]);
            f4=sqrtf(fabsf(L[offset+1+width]-L[offset-width2+2])*fabsf(L[offset+1+width]-L[offset+width2-2]));
            difRB=f1*f2*f2*f3*f3*f4;
            if((difLT!=0)&&(difRB!=0)){
              lumD1=(L[offset-1-width]*difRB+L[offset+1+width]*difLT)/(difLT+difRB);
              lumD1=v*(1-contrast)+lumD1*contrast;
            }
          }

          if(((L[offset]<L[offset+1-width])&&(L[offset]>L[offset-1+width])) ||
             ((L[offset]>L[offset+1-width])&&(L[offset]<L[offset-1+width]))){
            f1=fabsf(L[offset-2+width2]-L[offset-1+width]);
            f2=fabsf(L[offset-1+width]-L[offset]);
            f3=fabsf(L[offset-1+width]-L[offset-width-1])*fabsf(L[offset-1+width]-L[offset+width+1]);
            f4=sqrtf(fabsf(L[offset-1+width]-L[offset-width2-2])*fabsf(L[offset-1+width]-L[offset+width2+2]));
            difLB=f1*f2*f2*f3*f3*f4;
            f1=fabsf(L[offset+2-width2]-L[offset+1-width]);
            f2=fabsf(L[offset+1-width]-L[offset])*fabsf(L[offset+1-width]-L[offset]);
            f3=fabsf(L[offset+1-width]-L[offset+width+1])*fabsf(L[offset+1-width]-L[offset-width-1]);
            f4=sqrtf(fabsf(L[offset+1-width]-L[offset+width2+2])*fabsf(L[offset+1-width]-L[offset-width2-2]));
            difRT=f1*f2*f2*f3*f3*f4;
            if((difLB!=0)&&(difRT!=0)){
              lumD2=(L[offset+1-width]*difLB+L[offset-1+width]*difRT)/(difLB+difRT);
              lumD2=v*(1-contrast)+lumD2*contrast;
            }
          }

          s=Strength;

          // avoid sharpening diagonals too much
          if(((fabsf(wH/wV)<0.45)&&(fabsf(wH/wV)>0.05))||((fabsf(wV/wH)<0.45)&&(fabsf(wV/wH)>0.05)))
            s=Strength/3.0;

          // final mix
          if((wH!=0)&&(wV!=0)&&(wD1!=0)&&(wD2!=0))
            m_Image[offset][c]= CLIP((int32_t) ((v*(1-s)+(lumH*wH+lumV*wV+lumD1*wD1+lumD2*wD2)/(wH+wV+wD1+wD2)*s)*0xffff));
        }
    }

  FREE(L);
  return this;
}

// To the extent possible under law, Manuel Llorens <manuelllorens@gmail.com>
// has waived all copyright and related or neighboring rights to this work.
// This code is licensed under CC0 v1.0, see license information at
// http://creativecommons.org/publicdomain/zero/1.0/

ptImage* ptImage::MLMicroContrast(const double Strength,
                                  const double Scaling,
                                  const double Weight,
                                  const ptCurve *Curve)
{
  assert (m_ColorSpace == ptSpace_Lab);

  int32_t offset,offset2,c,i,j,col,row,n;
  float v,s,contrast,temp;
  float CompWeight = 1.0f - Weight;
  const float WPH = 0x8080;

  float ValueA = 0.0;
  float ValueB = 0.0;

  uint16_t width = m_Width;
  uint16_t height = m_Height;

  float (*L) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*L));
  ptMemoryError(L,__FILE__,__LINE__);

  int signs[9];

  float chmax = 8.0f/Scaling;

  c=0;

#pragma omp parallel for private(offset) schedule(static)
  for(offset=0;offset<m_Width*m_Height;offset++)
    L[offset]=ToFloatTable[m_Image[offset][c]];

#pragma omp parallel for private(j,i,offset,s,signs,v,n,row,col,offset2,contrast,temp, ValueA, ValueB) schedule(static)
  for(j=1;j<height-1;j++)
    for(i=1,offset=j*width+i;i<width-1;i++,offset++){
      if (Curve == NULL) s=Strength;
      else {
        // set s according to the curve
        if (Curve->mask() == ptCurve::ChromaMask) {
          ValueA = (float)m_Image[offset][1]-WPH;
          ValueB = (float)m_Image[offset][2]-WPH;
          float Hue = 0;
          if (ValueA == 0.0 && ValueB == 0.0) {
            Hue = 0;   // value for grey pixel
          } else {
            Hue = atan2f(ValueB,ValueA);
          }
          while (Hue < 0) Hue += 2.*ptPI;

          float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.125);
          Col /= 0x7; // normalizing to 0..2

          float Factor = Curve->Curve[CLIP((int32_t)(Hue/ptPI*WPH))]/(float)0x3333 - 1.0;
          s = Strength * Factor * Col;

        } else { //by luma
          float Factor = Curve->Curve[m_Image[offset][0]]/(float)0x3333 - 1.0;
          s = Strength * Factor;
        }
      }
      v=L[offset];

      n=0;
      for(row=j-1;row<=j+1;row++) {
        for(col=i-1,offset2=row*width+col;col<=i+1;col++,offset2++){
          signs[n]=0;
          if(v<L[offset2]) signs[n]=-1;
          if(v>L[offset2]) signs[n]=1;
          n++;
        }
      }

      contrast=sqrtf(fabsf(L[offset+1]-L[offset-1])*fabsf(L[offset+1]-L[offset-1])+fabsf(L[offset+width]-L[offset-width])*fabsf(L[offset+width]-L[offset-width]))/chmax;
      if(contrast>1.0) contrast=1.0;
      temp = ToFloatTable[m_Image[offset][c]];
      temp +=(v-L[offset-width-1])*sqrtf(2)*s;
      temp +=(v-L[offset-width])*s;
      temp +=(v-L[offset-width+1])*sqrtf(2)*s;

      temp +=(v-L[offset-1])*s;
      temp +=(v-L[offset+1])*s;

      temp +=(v-L[offset+width-1])*sqrtf(2)*s;
      temp +=(v-L[offset+width])*s;
      temp +=(v-L[offset+width+1])*sqrtf(2)*s;

      temp = MAX(0.0f,temp);

      // Reduce halo looking artifacs
      v=temp;
      n=0;
      for(row=j-1;row<=j+1;row++)
        for(col=i-1,offset2=row*width+col;col<=i+1;col++,offset2++){
          if(((v<L[offset2])&&(signs[n]>0))||((v>L[offset2])&&(signs[n]<0)))
            temp=v*Weight+L[offset2]*CompWeight;
          n++;
        }
      m_Image[offset][c]=CLIP((int32_t) ((temp*(1-contrast)+L[offset]*contrast)*0xffff));
    }

  FREE(L);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Hotpixel
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::HotpixelReduction(const double Threshold) {
  uint16_t Thres = (int32_t) ((1.0-Threshold)*0x2fff);
  uint16_t Width = m_Width;
  uint16_t Height = m_Height;

#pragma omp parallel for schedule(static) default(shared)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      uint16_t TempValue = 0;
      for (int c=0; c<3; c++) {
        // bright pixels
        if (Row > 1) {
          TempValue = MAX(m_Image[(Row-1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MAX(m_Image[(Row-1)*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MAX(m_Image[(Row-1)*Width+Col+1][c],TempValue);
        }
        if (Row < Height-1) {
          TempValue = MAX(m_Image[(Row+1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MAX(m_Image[(Row+1)*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MAX(m_Image[(Row+1)*Width+Col+1][c],TempValue);
        }
        if (Col > 1) TempValue = MAX(m_Image[Row*Width+Col-1][c],TempValue);
        if (Col < Width-1) TempValue = MAX(m_Image[Row*Width+Col+1][c],TempValue);
        if (TempValue+Thres<m_Image[Row*Width+Col][c])
          m_Image[Row*Width+Col][c] = TempValue;

        // dark pixels
        TempValue = 0xffff;
        if (Row > 1) {
          TempValue = MIN(m_Image[(Row-1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MIN(m_Image[(Row-1)*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MIN(m_Image[(Row-1)*Width+Col+1][c],TempValue);
        }
        if (Row < Height-1) {
          TempValue = MIN(m_Image[(Row+1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MIN(m_Image[(Row+1)*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MIN(m_Image[(Row+1)*Width+Col+1][c],TempValue);
        }
        if (Col > 1) TempValue = MIN(m_Image[Row*Width+Col-1][c],TempValue);
        if (Col < Width-1) TempValue = MIN(m_Image[Row*Width+Col+1][c],TempValue);
        if (TempValue-Thres>m_Image[Row*Width+Col][c])
          m_Image[Row*Width+Col][c] = TempValue;
      }
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Refined Shadows and Highlights
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ShadowsHighlights(const ptCurve *Curve,
                                    const double Radius,
                                    const double AmountCoarse,
                                    const double AmountFine) {

  assert (m_ColorSpace == ptSpace_Lab);

  uint16_t (*CoarseLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*CoarseLayer));
  ptMemoryError(CoarseLayer,__FILE__,__LINE__);
  uint16_t (*FineLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*FineLayer));
  ptMemoryError(FineLayer,__FILE__,__LINE__);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    CoarseLayer[i] = m_Image[i][0];
  }

  this->fastBilateralChannel(Radius, 0.14, 2, ChMask_L);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    FineLayer[i]   = CLIP((int32_t) ((ptWPH - (int32_t)m_Image[i][0]) + CoarseLayer[i]));
    CoarseLayer[i] = m_Image[i][0];
  }

  this->fastBilateralChannel(4*Radius, 0.14, 2, ChMask_L);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    CoarseLayer[i] = CLIP((int32_t) ((ptWPH - (int32_t)m_Image[i][0]) + CoarseLayer[i]));
  }

  //Curve on residual
  if (Curve != NULL) {
    ApplyCurve(Curve,1);
  }

  //Sigmoidal Contrast
  float Threshold = 0.5;
  float Contrast = AmountCoarse;
  uint16_t ContrastTable[0x10000];

  if (AmountCoarse != 0) {
    SigmoidalTable(ContrastTable, Contrast, Threshold);
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
      CoarseLayer[i] = ContrastTable[ CoarseLayer[i] ];
    }
  }

  Contrast = AmountFine;

  if (AmountFine != 0) {
    SigmoidalTable(ContrastTable, Contrast, Threshold);
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
      FineLayer[i]   = ContrastTable[ FineLayer[i] ];
    }
  }

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    m_Image[i][0] = CLIP((int32_t) (((int32_t)CoarseLayer[i] - ptWPH) + m_Image[i][0]));
    m_Image[i][0] = CLIP((int32_t) (((int32_t)FineLayer[i]   - ptWPH) + m_Image[i][0]));
  }

  FREE(CoarseLayer);
  FREE(FineLayer);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Microcontrast
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Microcontrast(const double Radius,
        const double Amount,
        const double Opacity,
        const double HaloControl,
        const TMaskType MaskType,
        const double LowerLimit,
        const double UpperLimit,
        const double Softness) {

  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;

  ptImage *MicrocontrastLayer = new ptImage;
  MicrocontrastLayer->Set(this);
  MicrocontrastLayer->ptCIBlur(Radius, ChannelMask);

  auto AmpCurve = new ptCurve(createAmpAnchors(Amount, HaloControl));

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=0; Ch<NrChannels; Ch++) {
      MicrocontrastLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)MicrocontrastLayer->m_Image[i][Ch])+m_Image[i][Ch]));
    }
  }

  MicrocontrastLayer->ApplyCurve(AmpCurve,ChannelMask);
  delete AmpCurve;

  float (*Mask);
  Mask = (m_ColorSpace == ptSpace_Lab)?
    GetMask( MaskType, LowerLimit, UpperLimit, Softness,1,0,0):
    GetMask( MaskType, LowerLimit, UpperLimit, Softness);

  Overlay(MicrocontrastLayer->m_Image,Opacity,Mask);
  delete MicrocontrastLayer;

  FREE(Mask);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Colorcontrast
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Colorcontrast(const double Radius,
        const double Amount,
        const double Opacity,
        const double HaloControl)
{
  const double WP = 0xffff;
  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = 3;
  const short ChannelMask = 6;

  ptImage *MicrocontrastLayer = new ptImage;
  MicrocontrastLayer->Set(this);
  MicrocontrastLayer->ptCIBlur(Radius, ChannelMask);

  auto AmpCurve = new ptCurve(createAmpAnchors(Amount, HaloControl));

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=1; Ch<NrChannels; Ch++) {
      MicrocontrastLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)MicrocontrastLayer->m_Image[i][Ch])+m_Image[i][Ch]));
    }
  }

  MicrocontrastLayer->ApplyCurve(AmpCurve,ChannelMask);
  delete AmpCurve;

  float Multiply = 0;
  float Screen = 0;
  float Overlay = 0;
  float Source = 0;
  float Blend = 0;

  for (short Ch=1; Ch<3; Ch++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      Source   = m_Image[i][Ch];
      Blend    = MicrocontrastLayer->m_Image[i][Ch];
      Multiply = CLIP((int32_t)(Source*Blend/WP));
      Screen   = CLIP((int32_t)(WP-(WP-Source)*(WP-Blend)/WP));
      Overlay  = CLIP((int32_t)((((WP-Source)*Multiply+Source*Screen)/WP)));
      m_Image[i][Ch] = CLIP((int32_t) (-WP*MIN(Opacity, 0.0)+Overlay*Opacity+Source*(1-fabs(Opacity))));
    }
  }

  delete MicrocontrastLayer;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Bilateral Denoise
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::BilateralDenoise(const double Threshold,
           const double Softness,
           const double Opacity,
           const double UseMask /* = 0*/) {

  ptImage *DenoiseLayer = new ptImage;
  DenoiseLayer->Set(this);
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const TChannelMask ChannelMask = (m_ColorSpace == ptSpace_Lab) ? ChMask_L : ChMask_RGB;
  const double WPH = 0x7fff;

  DenoiseLayer->fastBilateralChannel(Threshold, Softness, 2, ChannelMask);

  if (UseMask){

    double m = 10.0;
    double t = (1.0 - m)*WPH;

    uint16_t Table[0x10000];
#pragma omp parallel for schedule(static)
    for (uint32_t i=0; i<0x10000; i++) {
      Table[i] = CLIP((int32_t)(m*i+t));
    }

    ptImage *MaskLayer = new ptImage;
    MaskLayer->m_Colors = 3;
    MaskLayer->Set(DenoiseLayer);

#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      for (short Ch=0; Ch<NrChannels; Ch++) {
        MaskLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)DenoiseLayer->m_Image[i][Ch])+m_Image[i][Ch]));
        MaskLayer->m_Image[i][Ch] = Table[MaskLayer->m_Image[i][Ch]];
      }
    }

    if (ChannelMask == 7) {
#pragma omp parallel for default(shared) schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++)
        MaskLayer->m_Image[i][0] = RGB_2_L(MaskLayer->m_Image[i]);
    }

    ptCurve* Curve = new ptCurve({TAnchor(0.0, 1.0),
                                  TAnchor(0.4, 0.3),
                                  TAnchor(0.5, 0.0),
                                  TAnchor(0.6, 0.3),
                                  TAnchor(1.0, 1.0)});

    MaskLayer->ApplyCurve(Curve,1);
    MaskLayer->ptCIBlur(UseMask, 1);

    Curve->setFromAnchors({TAnchor(0.0, 0.0),
                           TAnchor(0.6, 0.4),
                           TAnchor(1.0, 0.8)});

    MaskLayer->ApplyCurve(Curve,1);
    delete Curve;

    float (*Mask);
    Mask = MaskLayer->GetMask(TMaskType::Shadows, 0.0, 1.0, 0.0, 1,0,0);
    Overlay(DenoiseLayer->m_Image,Opacity,Mask,ptOverlayMode_Normal);
    FREE(Mask);
    delete MaskLayer;
  } else {
    Overlay(DenoiseLayer->m_Image,Opacity,NULL,ptOverlayMode_Normal);
  }
  delete DenoiseLayer;
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Denoise curve
//
////////////////////////////////////////////////////////////////////////////////


ptImage* ptImage::ApplyDenoiseCurve(const double Threshold,
                                    const double Softness,
                                    const ptCurve *MaskCurve) {

  assert (m_ColorSpace == ptSpace_Lab);

  ptImage *DenoiseLayer = new ptImage;
  DenoiseLayer->Set(this);
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const TChannelMask ChannelMask = (m_ColorSpace == ptSpace_Lab) ? ChMask_L : ChMask_RGB;
  float WPH = 0x7fff;

  DenoiseLayer->fastBilateralChannel(Threshold, Softness, 2, ChannelMask);

  double UseMask = 50.0;

  float m = 10.0;
  float t = (1.0 - m)*WPH;

  uint16_t Table[0x10000];
#pragma omp parallel for schedule(static)
  for (uint32_t i=0; i<0x10000; i++) {
    Table[i] = CLIP((int32_t)(m*i+t));
  }

  ptImage *MaskLayer = new ptImage;
  MaskLayer->m_Colors = 3;
  MaskLayer->Set(DenoiseLayer);

  uint16_t (*Temp) = (uint16_t(*)) CALLOC(m_Width*m_Height,sizeof(*Temp));
  ptMemoryError(Temp,__FILE__,__LINE__);

#pragma omp parallel for default(shared) schedule(static)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=0; Ch<NrChannels; Ch++) {
      MaskLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)DenoiseLayer->m_Image[i][Ch])+m_Image[i][Ch]));
      MaskLayer->m_Image[i][Ch] = Table[MaskLayer->m_Image[i][Ch]];
    }
    Temp[i] = m_Image[i][0];
  }

  ptCurve* Curve = new ptCurve({TAnchor(0.0, 1.0),
                                TAnchor(0.4, 0.3),
                                TAnchor(0.5, 0.0),
                                TAnchor(0.6, 0.3),
                                TAnchor(1.0, 1.0)});  // also triggers curve calc
  MaskLayer->ApplyCurve(Curve, ChMask_L);

  MaskLayer->ptCIBlur(UseMask, ChMask_L);

  Curve->setFromAnchors({TAnchor(0.0, 0.0),
                         TAnchor(0.6, 0.4),
                         TAnchor(1.0, 0.8)});
  Curve->calcCurve();
  MaskLayer->ApplyCurve(Curve, ChMask_L);
  delete Curve;

  float (*Mask);
  Mask = MaskLayer->GetMask(TMaskType::Shadows, 0.0, 1.0, 0.0, 1,0,0);
  delete MaskLayer;
  Overlay(DenoiseLayer->m_Image,1.0f,Mask,ptOverlayMode_Normal);
  FREE(Mask);
  delete DenoiseLayer;

  // at this point Temp contains the unaltered L channel and m_Image
  // contains the denoised layer.

  // neutral value for a* and b* channel
  WPH = 0x8080;

  float ValueA = 0.0;
  float ValueB = 0.0;
  float Hue = 0.0;

  if (MaskCurve->mask() == ptCurve::ChromaMask) {
#pragma omp parallel for schedule(static) private(ValueA, ValueB, Hue)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by hue
      ValueA = (float)m_Image[i][1]-WPH;
      ValueB = (float)m_Image[i][2]-WPH;

      if (ValueA == 0.0 && ValueB == 0.0) {
        Hue = 0;   // value for grey pixel
      } else {
        Hue = atan2f(ValueB,ValueA);
      }
      while (Hue < 0) Hue += 2.*ptPI;

      float Factor = MaskCurve->Curve[CLIP((int32_t)(Hue/ptPI*WPH))]-(float)0x7fff;
      Factor /= (float)0x7fff;

      m_Image[i][0]=CLIP((int32_t)(Temp[i]*(1.0f - Factor)+m_Image[i][0]*Factor ));
    }

  } else { // by luma
#pragma omp parallel for schedule(static) private(ValueA, ValueB)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by luminance
      float Factor = MaskCurve->Curve[Temp[i]]-(float)0x7fff;
      Factor /= (float)0x7fff;

      m_Image[i][0]=CLIP((int32_t)(Temp[i]*(1.0f - Factor)+m_Image[i][0]*Factor ));
    }
  }

  FREE(Temp);

  return this;
}


////////////////////////////////////////////////////////////////////////////////
//
// Texturecontrast
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::TextureContrast(const double Threshold,
          const double Softness,
          const double Amount,
          const double Opacity,
          const double EdgeControl,
          const double Masking) {

  const int32_t WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const TChannelMask ChannelMask = (m_ColorSpace == ptSpace_Lab) ? ChMask_L : ChMask_RGB;

  ptImage *ContrastLayer = new ptImage;
  ContrastLayer->Set(this);

  ContrastLayer->fastBilateralChannel(Threshold, Softness, 2, ChannelMask);

#pragma omp parallel for default(shared) schedule(static)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=0; Ch<NrChannels; Ch++) {
      ContrastLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][Ch])+m_Image[i][Ch]));
      if (Amount < 0) ContrastLayer->m_Image[i][Ch] = 0xffff-ContrastLayer->m_Image[i][Ch];
    }
  }
  ContrastLayer->SigmoidalContrast(fabs(Amount), 0.5, ChannelMask);

  if (EdgeControl)
    ContrastLayer->WaveletDenoise(ChannelMask, EdgeControl, 0.2, 0);

  if (Masking) {
    ptImage *MaskLayer = new ptImage;
    MaskLayer->Set(ContrastLayer);

    if (ChannelMask == 7) {
#pragma omp parallel for default(shared) schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++)
        MaskLayer->m_Image[i][0] = RGB_2_L(MaskLayer->m_Image[i]);
    }

    ptCurve* Curve = new ptCurve({TAnchor(0.0, 1.0),
                                  TAnchor(0.4, 0.3),
                                  TAnchor(0.5, 0.0),
                                  TAnchor(0.6, 0.3),
                                  TAnchor(1.0, 1.0)});

    MaskLayer->ApplyCurve(Curve,1);
    MaskLayer->ptCIBlur(Masking, 1);

    Curve->setFromAnchors({TAnchor(0.0, 0.0),
                           TAnchor(0.4, 0.6),
                           TAnchor(0.8, 1.0)});

    MaskLayer->ApplyCurve(Curve,1);
    delete Curve;

    float (*Mask);
    Mask = MaskLayer->GetMask(TMaskType::Highlights, 0.0, 1.0, 0.0, 1,0,0);
    Overlay(ContrastLayer->m_Image,Opacity,Mask);
    FREE(Mask);
    delete MaskLayer;
  } else {
    Overlay(ContrastLayer->m_Image,Opacity,NULL);
  }

  delete ContrastLayer;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Localcontrast
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Localcontrast(const int Radius1, const double Opacity, const double m, const double Feather, const short Method) {

  uint16_t Width = m_Width;
  uint16_t Height = m_Height;
  int32_t Size = Width*Height;

  int MaxExponent=MAX((int)floor(log((float)Radius1)/log(2.))+1,1);
  uint16_t MaxRadius=(int)pow(2.,(float)(MaxExponent+1));
  int Step = 0;
  int Blocksize = 0;
  int SquareStep = 0;
  int Shift = 0;
  int32_t x1 = 0;
  int32_t x2 = 0;
  int32_t y1 = 0;
  int32_t y2 = 0;
  int32_t pos1x = 0;
  int32_t pos1y = 0;
  int32_t pos2x = 0;
  int32_t pos2y = 0;
  int32_t Index1 = 0;
  int32_t Index2 = 0;
  float value1 = 0;
  float value2 = 0;
  float ker = 0;

  float **Kernel;
  Kernel = (float **)CALLOC(MaxRadius,sizeof(float*));
  for(int i = 0; i < MaxRadius; i++) Kernel[i] = (float*)CALLOC(MaxRadius,sizeof(float));

  float r=0;
  for (uint16_t x=0; x<MaxRadius; x++) {
    for (uint16_t y=0; y<MaxRadius; y++) {
      r = powf((float)x*(float)x+(float)y*(float)y,0.5);
      if (r <= Radius1) {
        Kernel[x][y] = 1;
      } else {
        Kernel[x][y] = 0;
      }
    }
  }
  int16_t (*MinVector)[2] = (int16_t (*)[2]) CALLOC(Size,sizeof(*MinVector));
  ptMemoryError(MinVector,__FILE__,__LINE__);
  int16_t (*MaxVector)[2] = (int16_t (*)[2]) CALLOC(Size,sizeof(*MaxVector));
  ptMemoryError(MaxVector,__FILE__,__LINE__);

  memset(MinVector,0,Size*sizeof(*MinVector));
  memset(MaxVector,0,Size*sizeof(*MaxVector));

  for (int runcounter=0; runcounter<1;runcounter++){
    // runcounter loop should be superfluous now.
    // clean up after some more testing.
    for (int k=1; k<=MaxExponent; k++) {
      Step=(int) powf(2.,(float)(k));
      SquareStep = (int) powf(2.,(float)(k-1));
      Blocksize = SquareStep;
      Shift = Blocksize;
#pragma omp parallel for private(x1, x2, y1, y2, pos1x, pos1y, pos2x, pos2y, value1, value2, ker) schedule(static)
      for (int32_t Row=-(runcounter&1)*Shift; Row < Height; Row += Step) {
        for (int32_t Col=-(runcounter&1)*Shift; Col < Width; Col += Step) {
          for (uint16_t incx=0; incx < Blocksize; incx++) {
            for (uint16_t incy=0; incy < Blocksize; incy++) {
              for (int fx=0; fx < 2; fx++) {
                for (int fy=0; fy < 2; fy++) {
                  x1 = MAX(MIN(Col+incx+fx*SquareStep,Width-1),0);
                  y1 = MAX(MIN(Row+incy+fy*SquareStep,Height-1),0);
                  for (int lx=0; lx < 2; lx++) {
                    for (int ly=0; ly < 2; ly++) {
                      x2 = MAX(MIN(Col+incx+lx*SquareStep,Width-1),0);
                      y2 = MAX(MIN(Row+incy+ly*SquareStep,Height-1),0);
                      Index1 = y1*Width+x1;
                      Index2 = y2*Width+x2;
                      // Calculate Min
                      pos1x = MinVector[Index1][0] + x1;
                      pos1y = MinVector[Index1][1] + y1;
                      pos2x = MinVector[Index2][0] + x2;
                      pos2y = MinVector[Index2][1] + y2;
                      value1 = (float)m_Image[pos1y*Width+pos1x][0];
                      value2 = (float)m_Image[pos2y*Width+pos2x][0];
                      ker = Kernel[(int32_t)fabs(pos2x - x1)][(int32_t)fabs(pos2y - y1)];
                      if (ker && value2 <= value1) {
                        MinVector[Index1][0] = pos2x - x1;
                        MinVector[Index1][1] = pos2y - y1;
                      }
                      // Calculate Max
                      pos1x = MaxVector[Index1][0] + x1;
                      pos1y = MaxVector[Index1][1] + y1;
                      pos2x = MaxVector[Index2][0] + x2;
                      pos2y = MaxVector[Index2][1] + y2;
                      value1 = (float)m_Image[pos1y*Width+pos1x][0];
                      value2 = (float)m_Image[pos2y*Width+pos2x][0];
                      ker = Kernel[(int32_t)fabs(pos2x - x1)][(int32_t)fabs(pos2y - y1)];
                      if (ker && value2 >= value1) {
                        MaxVector[Index1][0] = pos2x - x1;
                        MaxVector[Index1][1] = pos2y - y1;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  uint16_t (*MinLayer) = (uint16_t (*)) CALLOC(Size,sizeof(*MinLayer));
  ptMemoryError(MinLayer,__FILE__,__LINE__);
  uint16_t (*MaxLayer) = (uint16_t (*)) CALLOC(Size,sizeof(*MaxLayer));
  ptMemoryError(MaxLayer,__FILE__,__LINE__);

#pragma omp parallel for private(pos1x, pos1y, value1) schedule(static)
  for (uint16_t Row=0; Row < Height; Row++) {
    for (uint16_t Col=0; Col < Width; Col++) {
      int32_t Index = Row*Width+Col;
      pos1x = MinVector[Index][0] + Col;
      pos1y = MinVector[Index][1] + Row;
      value1 = (float)m_Image[pos1y*Width+pos1x][0];
      MinLayer[Index] = CLIP((int32_t) value1);
      pos1x = MaxVector[Index][0] + Col;
      pos1y = MaxVector[Index][1] + Row;
      value1 = (float)m_Image[pos1y*Width+pos1x][0];
      MaxLayer[Index] = CLIP((int32_t) value1);
    }
  }

  double BlurRadius = Radius1*pow(4,Feather);
  ptCimgBlurLayer(MinLayer, Width, Height, BlurRadius);
  ptCimgBlurLayer(MaxLayer, Width, Height, BlurRadius);

  float Z = 0;
  float N = 0;
  float Mask = 0;
  if (Method == 1) {
#pragma omp parallel for private(Z,N,Mask) schedule(static)
    for (int32_t i = 0; i < Size; i++) {
      Z = m_Image[i][0] - MinLayer[i];
      N = MaxLayer[i] - MinLayer[i];
      if (m>0) {
        Mask = m*N/(float)0xffff+(1.-m);
      } else {
        Mask = -m*(1-(N/(float)0xffff))+(1.+m);
      }
      m_Image[i][0]=CLIP((int32_t) ((Z/N*0xFFFF*Mask+(1-Mask)*m_Image[i][0])*Opacity+(1.-Opacity)*m_Image[i][0]));
    }
  } else if (Method == 2) {
#pragma omp parallel for private(Z,N,Mask) schedule(static)
    for (int32_t i = 0; i < Size; i++) {
      m_Image[i][0] = MinLayer[i];
    }
  } else {
#pragma omp parallel for private(Z,N,Mask) schedule(static)
    for (int32_t i = 0; i < Size; i++) {
      m_Image[i][0] = MaxLayer[i];
    }
  }

  FREE(MinVector);
  FREE(MinLayer);
  FREE(MaxVector);
  FREE(MaxLayer);
  for(int i = 0; i < MaxRadius; i++) FREE(Kernel[i]);
  FREE(Kernel);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Grain
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Grain(const double Sigma, // 0-1
                        const TGrainType NoiseType, // 0-5, Gaussian, uniform, salt&pepper
                        const double Radius, // 0-20
                        const double Opacity,
                        const TMaskType MaskType,
                        const double LowerLimit,
                        const double UpperLimit,
                        const short ScaleFactor) { // 0, 1 or 2 depending on pipe size

  assert (m_ColorSpace == ptSpace_Lab);

  ptImage *NoiseLayer = new ptImage;
  NoiseLayer->Set(this);  // allocation of free layer faster? TODO!
  float (*Mask);
  short Noise = LIM(static_cast<int>(NoiseType),0,5);
  Noise = (Noise > 2) ? (Noise - 3) : Noise;
  short ScaledRadius = Radius/powf(2.0,(float)ScaleFactor);

  ptCimgNoise(NoiseLayer, Sigma*10000, Noise, ScaledRadius);

  const float WPH = 0x7fff;

  // adaption to get the same optical impression when rescaled
  if (ScaleFactor != 2) {
    float m = 4/powf(2.0,(float)ScaleFactor);
    float t = (1-m)*WPH;
#pragma omp parallel for schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      NoiseLayer->m_Image[i][0] = CLIP((int32_t)(NoiseLayer->m_Image[i][0]*m+t));
    }
  }

  Mask = GetMask(MaskType, LowerLimit, UpperLimit, 0.0);
  if (Noise < 3) {
    Overlay(NoiseLayer->m_Image,Opacity,Mask,ptOverlayMode_SoftLight);
  } else {
    Overlay(NoiseLayer->m_Image,Opacity,Mask,ptOverlayMode_GrainMerge);
  }

  delete NoiseLayer;
  FREE(Mask);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Black&White Styler
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::BWStyler(
    const TBWFilmType FilmType,
    const TBWColorFilter ColorFilterType,
    const double MultR,
    const double MultG,
    const double MultB,
    const double Opacity)
{
  double R = 0,G = 0,B = 0;
  double FR = 0, FG = 0, FB = 0;

  switch (FilmType) {
  case TBWFilmType::LowSensitivity:
    R = 0.27;
    G = 0.27;
    B = 0.46;
    break;

  case TBWFilmType::HighSensitivity:
    R = 0.3;
    G = 0.28;
    B = 0.42;
    break;

  case TBWFilmType::Hyperpanchromatic:
    R = 0.41;
    G = 0.25;
    B = 0.34;
    break;

  case TBWFilmType::Orthochromatic:
    R = 0.0;
    G = 0.42;
    B = 0.58;
    break;

//Following values corresponding to http://photographynotes.pbworks.com/bwrecipe
  case TBWFilmType::NormalContrast:
    R = 0.43;
    G = 0.33;
    B = 0.3;
    break;

  case TBWFilmType::HighContrast:
    R = 0.4;
    G = 0.34;
    B = 0.60;
    break;

  case TBWFilmType::Luminance:
    R = 0.3;
    G = 0.59;
    B = 0.11;
    break;

  case TBWFilmType::Landscape:
    R = 0.66;
    G = 0.24;
    B = 0.10;
    break;

  case TBWFilmType::FaceInterior:
    R = 0.54;
    G = 0.44;
    B = 0.12;
    break;

  case TBWFilmType::ChannelMixer:
    R = MultR/(MultR+MultG+MultB);
    G = MultG/(MultR+MultG+MultB);
    B = MultB/(MultR+MultG+MultB);
    break;
  }

  switch (ColorFilterType) {
  //Dr. Herbert Kribben and http://epaperpress.com/psphoto/bawFilters.html
  case TBWColorFilter::None:
    FR = 1.0;
    FG = 1.0;
    FB = 1.0;
    break;

  case TBWColorFilter::Red:
    FR = 1.0;
    FG = 0.2;
    FB = 0.0;
    break;

  case TBWColorFilter::Orange:
    FR = 1.0;
    FG = 0.6;
    FB = 0.0;
    break;

  case TBWColorFilter::Yellow:
    FR = 1.0;
    FG = 1.0;
    FB = 0.1;
    break;

  case TBWColorFilter::Lime:
    FR = 0.6;
    FG = 1.0;
    FB = 0.3;
    break;

  case TBWColorFilter::Green:
    FR = 0.2;
    FG = 1.0;
    FB = 0.3;
    break;

  case TBWColorFilter::Blue:
    FR = 0.0;
    FG = 0.2;
    FB = 1.0;
    break;

  case TBWColorFilter::FakeIR:
    FR = 0.4;
    FG = 1.4;
    FB = -0.8;
    break;
  }

  R = R * FR;
  G = G * FG;
  B = B * FB;
  R = R / (R+G+B);
  G = G / (R+G+B);
  B = B / (R+G+B);

  TChannelMatrix Mixer;
  Mixer[0][0] = R;
  Mixer[1][0] = R;
  Mixer[2][0] = R;
  Mixer[0][1] = G;
  Mixer[1][1] = G;
  Mixer[2][1] = G;
  Mixer[0][2] = B;
  Mixer[1][2] = B;
  Mixer[2][2] = B;

  if (qFuzzyCompare(Opacity, 1.0)) {
    mixChannels(Mixer);
  } else {
    auto BWLayer = make_unique<ptImage>();
    BWLayer->Set(this);
    BWLayer->mixChannels(Mixer);
    Overlay(BWLayer->m_Image, Opacity, nullptr, ptOverlayMode_Normal);
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Simple toneing
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::SimpleTone(const double R,
           const double G,
           const double B) {

  assert (m_ColorSpace != ptSpace_Lab);

  if (R) {
    ptCurve* RedCurve = new ptCurve({TAnchor(0.0,       0.0),
                                     TAnchor(0.5-0.2*R, 0.5+0.2*R),
                                     TAnchor(1.0,       1.0),});
    ApplyCurve(RedCurve,1);
    delete RedCurve;
  }

  if (G) {
    ptCurve* GreenCurve = new ptCurve({TAnchor(0.0,       0.0),
                                       TAnchor(0.5-0.2*G, 0.5+0.2*G),
                                       TAnchor(1.0,       1.0),});
    ApplyCurve(GreenCurve,2);
    delete GreenCurve;
  }

  if (B) {
    ptCurve* BlueCurve = new ptCurve({TAnchor(0.0,       0.0),
                                      TAnchor(0.5-0.2*B, 0.5+0.2*B),
                                      TAnchor(1.0,       1.0),});
    ApplyCurve(BlueCurve,4);
    delete BlueCurve;
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Temperature
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Temperature(const double Temperature, const double Tint) {
  assert (m_ColorSpace == ptSpace_Lab);

  ptCurve* Temp1Curve = new ptCurve(
      {TAnchor(0.0, 0.0),
       TAnchor(0.5-0.05*Temperature+0.1*Tint, 0.5+0.05*Temperature-0.1*Tint),
       TAnchor(1.0, 1.0)} );
  ApplyCurve(Temp1Curve,2);
  delete Temp1Curve;

  ptCurve* Temp2Curve = new ptCurve(
      {TAnchor(0.0, 0.0),
       TAnchor(0.5-0.1*Temperature-0.05*Tint, 0.5+0.1*Temperature+0.05*Tint),
       TAnchor(1.0, 1.0)} );
  ApplyCurve(Temp2Curve,4);
  delete Temp2Curve;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LAB Transform
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LABTransform(const short Mode) {

  assert (m_ColorSpace != ptSpace_Lab);

  double R = 0;
  double G = 0;
  double B = 0;

  switch (Mode) {
    case ptLABTransform_R:
      R = 1.;
      break;
    case ptLABTransform_G:
      G = 1.;
      break;
    case ptLABTransform_B:
      B = 1.;
      break;
    default:
      break;
  }

  ptImage *TempLayer = new ptImage;
  TempLayer->Set(this);
  ptCurve* GammaCurve = new ptCurve();
  GammaCurve->setFromFunc(ptCurve::GammaTool,0.45,0.0);
  TempLayer->ApplyCurve(GammaCurve,7);
  delete GammaCurve;
  RGBToLab();

#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++)
    m_Image[i][0] = CLIP((int32_t) (R*(double)TempLayer->m_Image[i][0] +
            G*(double)TempLayer->m_Image[i][1] +
            B*(double)TempLayer->m_Image[i][2]));
  delete TempLayer;
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LAB Tone
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LABTone(const double Amount,
                          const double Hue,
                          const double Saturation, /* 0 */
                          const TMaskType MaskType, /* TMaskType::All */
                          const short ManualMask, /* 0 */
                          const double LowerLevel, /* 0 */
                          const double UpperLevel, /* 1 */
                          const double Softness /* 0 */) {

  assert (m_ColorSpace == ptSpace_Lab);

  double a = Amount * cos(Hue/180.*ptPI);
  double b = Amount * sin(Hue/180.*ptPI);

  const double WPH = 0x8080; // Neutral in Lab A and B

  double t = (1.-Saturation)*WPH;

  ptCurve* Temp1Curve = new ptCurve({TAnchor(0.0,       0.0),
                                     TAnchor(0.5-0.1*a, 0.5+0.1*a),
                                     TAnchor(1.0,       1.0)} );

  ptCurve* Temp2Curve = new ptCurve({TAnchor(0.0,       0.0),
                                     TAnchor(0.5-0.1*b, 0.5+0.1*b),
                                     TAnchor(1.0,       1.0)} );

  if (MaskType == TMaskType::All) {
    if (Saturation != 1) ColorBoost(Saturation, Saturation);
    if (Amount) {
      ApplyCurve(Temp1Curve,2);
      ApplyCurve(Temp2Curve,4);
    }
  } else {
    ptImage *ToneLayer = new ptImage;
    if (Amount) {
      ToneLayer->Set(this);
      if (Saturation != 1) ToneLayer->ColorBoost(Saturation, Saturation);
      ToneLayer->ApplyCurve(Temp1Curve,2);
      ToneLayer->ApplyCurve(Temp2Curve,4);
    }
    float (*Mask);

    if (MaskType == TMaskType::Shadows)
      if (ManualMask)
        Mask = GetMask(TMaskType::Shadows, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(TMaskType::Shadows, 0,0.5,0,1,0,0);
    else if (MaskType == TMaskType::Midtones)
      if (ManualMask)
        Mask = GetMask(TMaskType::Midtones, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(TMaskType::Midtones, 0.5,0.5,0,1,0,0);
    else
      if (ManualMask)
        Mask = GetMask(TMaskType::Highlights, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(TMaskType::Highlights, 0.5,1,0,1,0,0);

#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++)
      for (int Ch = 1; Ch < 3; Ch++) {
        if (Saturation != 1)
          m_Image[i][Ch] = CLIP((int32_t)((m_Image[i][Ch]*Saturation+t)*Mask[i] +
                           m_Image[i][Ch]*(1-Mask[i])));
        if (Amount)
          m_Image[i][Ch] = CLIP((int32_t)((ToneLayer->m_Image[i][Ch]*Mask[i] +
                           m_Image[i][Ch]*(1-Mask[i]))*Amount+m_Image[i][Ch]*(1-Amount)));
      }
    FREE(Mask);
    if (Amount) delete ToneLayer;
  }
  delete Temp1Curve;
  delete Temp2Curve;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Tone
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Tone(const uint16_t R,
                       const uint16_t G,
                       const uint16_t B,
                       const double   Amount,
                       const TMaskType MaskType,
                       const double   LowerLimit,
                       const double   UpperLimit,
                       const double   Softness) {

  assert (m_ColorSpace != ptSpace_Lab);
  uint16_t (*ToneImage)[3] =
    (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
  ptMemoryError(ToneImage,__FILE__,__LINE__);

  float (*Mask);
  if (MaskType <= TMaskType::All)
    Mask=GetMask(MaskType, LowerLimit, UpperLimit, Softness);
  else
    Mask=GetMask(TMaskType::Midtones, LowerLimit, UpperLimit, Softness);
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    ToneImage[i][0] = R;
    ToneImage[i][1] = G;
    ToneImage[i][2] = B;
  }

  if (MaskType <= TMaskType::All)
    Overlay(ToneImage, Amount, Mask);
  else if (MaskType == TMaskType::Screen)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_Screen);
  else if (MaskType == TMaskType::Multiply)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_Multiply);
  else if (MaskType == TMaskType::GammaDark)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_GammaDark);
  else if (MaskType == TMaskType::GammaBright)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_GammaBright);

  FREE(Mask);
  FREE(ToneImage);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Crossprocessing
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Crossprocess(const short Mode,
                               const double Color1,
                               const double Color2) {

  assert (m_ColorSpace != ptSpace_Lab);

  ptCurve* RedCurve = new ptCurve({TAnchor(0.0,  0.0),
                                   TAnchor(0.05, 0.05-0.05*Color1),
                                   TAnchor(0.15, 0.15),
                                   TAnchor(1.0,  1.0)});

  ptCurve* GreenCurve = new ptCurve({TAnchor(0.0,  0.0),
                                     TAnchor(0.03, 0.025),
                                     TAnchor(0.2,  0.2+0.3*Color1),
                                     TAnchor(1.0,  1.0)});

  ptCurve* BlueCurve = new ptCurve({TAnchor(0.0,  0.0),
                                    TAnchor(0.3,  0.3-0.2*Color2),
                                    TAnchor(1.0,  1.0)});

  ptImage *ColorLayer = new ptImage;
  ColorLayer->Set(this);

  switch (Mode) {
    case ptCrossprocessMode_GY:
      ColorLayer->ApplyCurve(RedCurve,1);
      ColorLayer->ApplyCurve(GreenCurve,2);
      ColorLayer->ApplyCurve(BlueCurve,4);
      break;
    case ptCrossprocessMode_GC:
      ColorLayer->ApplyCurve(RedCurve,4);
      ColorLayer->ApplyCurve(GreenCurve,2);
      ColorLayer->ApplyCurve(BlueCurve,1);
      break;
    case ptCrossprocessMode_RY:
      ColorLayer->ApplyCurve(RedCurve,2);
      ColorLayer->ApplyCurve(GreenCurve,1);
      ColorLayer->ApplyCurve(BlueCurve,4);
      break;
    case ptCrossprocessMode_RM:
      ColorLayer->ApplyCurve(RedCurve,4);
      ColorLayer->ApplyCurve(GreenCurve,1);
      ColorLayer->ApplyCurve(BlueCurve,2);
      break;
    case ptCrossprocessMode_BC:
      ColorLayer->ApplyCurve(RedCurve,2);
      ColorLayer->ApplyCurve(GreenCurve,4);
      ColorLayer->ApplyCurve(BlueCurve,1);
      break;
    case ptCrossprocessMode_BM:
      ColorLayer->ApplyCurve(RedCurve,1);
      ColorLayer->ApplyCurve(GreenCurve,4);
      ColorLayer->ApplyCurve(BlueCurve,2);
      break;
    default: assert(0);
  }

  Overlay(ColorLayer->m_Image,0.7,NULL,ptOverlayMode_Normal);
  delete ColorLayer;

  delete RedCurve;
  delete GreenCurve;
  delete BlueCurve;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Gradual Overlay Mask
//
////////////////////////////////////////////////////////////////////////////////

float *ptImage::GetGradualMask(const double Angle,
                               const double LowerLevel,
                               const double UpperLevel,
                               const double Softness) {

  float (*GradualMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*GradualMask));
  ptMemoryError(GradualMask,__FILE__,__LINE__);

  float Length = 0;
  if (fabs(Angle) == 0 || fabs(Angle) == 180 ) {
    Length = m_Height;
  } else if (fabs(Angle) == 90) {
    Length = m_Width;
  } else if (fabs(Angle) < 90) {
    Length = (((float)m_Width) + ((float)m_Height)/tan(fabs(Angle)/180*ptPI))*sin(fabs(Angle)/180*ptPI);
  } else {
    Length = (((float)m_Width) + ((float)m_Height)/tan((180.0-fabs(Angle))/180.0*ptPI))*sin((180.0-fabs(Angle))/180.0*ptPI);
  }

  bool Switch = UpperLevel < LowerLevel;

  float Eps = 0.0001f;

  float LL = Length*(Switch?UpperLevel:LowerLevel);
  float UL = Length*(Switch?LowerLevel:UpperLevel);
  float Black = Switch?1.0f:0.0f;
  float White = Switch?0.0f:1.0f;
  float Denom = 1.0f/MAX((UL-LL),Eps);

  float coordinate = 0;
  float Value = 0;

  float dist = 0;

  float Factor1 = 0;
  float Factor2 = 0;
  if (Angle >= 0.0 && Angle < 90.0) {
    Factor1 = 1.0/MAX(tanf(Angle/180*ptPI),Eps);
    Factor2 = sinf(Angle/180*ptPI);
  } else if (Angle >= 90.0 && Angle < 180.0) {
    Factor1 = 1.0/MAX(tanf((180.0-Angle)/180*ptPI),Eps);
    Factor2 = sinf((180.0-Angle)/180*ptPI);
  } else if (Angle >= -90.0 && Angle < 0.0) {
    Factor1 = 1.0/MAX(tanf(fabs(Angle)/180*ptPI),Eps);
    Factor2 = sinf(fabs(Angle)/180*ptPI);
  } else if (Angle >= -180.0 && Angle < -90.0) {
    Factor1 = 1.0/MAX(tanf((180.0-fabs(Angle))/180*ptPI),Eps);
    Factor2 = sinf((180.0-fabs(Angle))/180*ptPI);
  }

#pragma omp parallel for schedule(static) firstprivate(dist, Value, coordinate)
  for (uint16_t Row=0; Row<m_Height; Row++) {
    uint32_t Idx = Row*m_Width;
    for (uint16_t Col=0; Col<m_Width; Col++) {
      if (fabs(Angle) == 0.0)
        dist = m_Height-Row;
      else if (fabs(Angle) == 180.0)
        dist = Row;
      else if (Angle == 90.0)
        dist = Col;
      else if (Angle == -90.0)
        dist = m_Width-Col;
      else if (Angle > 0.0 && Angle < 90.0)
        dist = (Col + (float)(m_Height-Row)*Factor1)*Factor2;
      else if (Angle > 90.0 && Angle < 180.0)
        dist = Length-((m_Width-Col) + (float)(m_Height-Row)*Factor1)*Factor2;
      else if (Angle > -90.0 && Angle < 0.0)
        dist = ((m_Width-Col) + (float)(m_Height-Row)*Factor1)*Factor2;
      else if (Angle > -180.0 && Angle < -90.0)
        dist = Length-((float)Col + (float)(m_Height-Row)*Factor1)*Factor2;

      if (dist <= LL)
        GradualMask[Idx+Col] = Black;
      else if (dist >= UL)
        GradualMask[Idx+Col] = White;
      else {
        coordinate = 1.0f - (UL-dist)*Denom;
        Value = (1.0f-powf(cosf(coordinate*ptPI/2.0f),50.0f*Softness))
                  * powf(coordinate,0.07f*Softness);
        GradualMask[Idx+Col] = LIM(Value*White, 0.0f, 1.0f);
      }
    }
  }

  return GradualMask;
}

////////////////////////////////////////////////////////////////////////////////
//
// Gradual Overlay
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::GradualOverlay(const uint16_t R,
                 const uint16_t G,
                 const uint16_t B,
                 const short Mode,
                 const double Amount,
                 const double Angle,
                 const double LowerLevel,
                 const double UpperLevel,
                 const double Softness) {

  float* GradualMask = GetGradualMask(Angle, LowerLevel, UpperLevel, Softness);

  uint16_t (*ToneImage)[3] = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
  ptMemoryError(ToneImage,__FILE__,__LINE__);

#pragma omp for schedule(static)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    ToneImage[i][0] = R;
    ToneImage[i][1] = G;
    ToneImage[i][2] = B;
  }

  Overlay(ToneImage, Amount, GradualMask, Mode);

  FREE(GradualMask);
  FREE(ToneImage);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Vignette
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Vignette(const TVignetteMask VignetteMode,
                           const TVignetteShape Shape,
                           const double Amount,
                           const double InnerRadius,
                           const double OuterRadius,
                           const double Roundness,
                           const double CenterX,
                           const double CenterY,
                           const double Softness)
{
  float* VignetteMask = GetVignetteMask(
      0,
      static_cast<int>(Shape),
      InnerRadius,
      OuterRadius,
      Roundness,
      CenterX,
      CenterY,
      Softness);

  switch (VignetteMode) {
    case TVignetteMask::Disabled:
      assert("Should not be called with disabled vignette mask!");
      break;

    case TVignetteMask::Soft:
    case TVignetteMask::Hard:
      {
        uint16_t (*ToneImage)[3] = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
        ptMemoryError(ToneImage,__FILE__,__LINE__);
        uint16_t hColor = 0;
        short    hMode  = ptOverlayMode_SoftLight;
        if (Amount > 0) {
          hColor = 0;
          if (VignetteMode == TVignetteMask::Hard) hMode = ptOverlayMode_Multiply;
        } else {
          hColor = 0xffff;
          if (VignetteMode == TVignetteMask::Hard) hMode = ptOverlayMode_Screen;
      }

#pragma omp parallel for schedule(static) default(shared)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          ToneImage[i][0] = ToneImage[i][1] = ToneImage[i][2] = hColor;
        }

        Overlay(ToneImage, fabs(Amount), VignetteMask, hMode);
        FREE(ToneImage);
      }
      break;

    case TVignetteMask::Fancy:
      {
        ptImage *VignetteLayer = new ptImage;
        VignetteLayer->Set(this);
        VignetteLayer->Expose(pow(2,-Amount*5), TExposureClipMode::Hard);
        ptCurve* VignetteContrastCurve = new ptCurve();
        VignetteContrastCurve->setFromFunc(ptCurve::Sigmoidal,0.5,fabs(Amount)*10);
        VignetteLayer->ApplyCurve(VignetteContrastCurve, (m_ColorSpace == ptSpace_Lab) ? 1 : 7);
        delete VignetteContrastCurve;
        Overlay(VignetteLayer->m_Image, 1, VignetteMask, ptOverlayMode_Normal);
        delete VignetteLayer;
      }
      break;

    case TVignetteMask::ShowMask:
      if (m_ColorSpace == ptSpace_Lab) {
#       pragma omp parallel for schedule(static) default(shared)
        for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          m_Image[i][0] = CLIP((int32_t) (VignetteMask[i]*0xffff));
          m_Image[i][1] = m_Image[i][2] = 0x8080;
        }
      } else {
#       pragma omp parallel for schedule(static) default(shared)
        for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
          m_Image[i][0] = m_Image[i][1] = m_Image[i][2] =
            CLIP((int32_t) (VignetteMask[i]*0xffff));
        }
      }
      break;
  }

  FREE(VignetteMask);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Softglow
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Softglow(
    const TSoftglowMode SoftglowMode,
    const double  Radius,
    const double  Amount,
    const uint8_t ChannelMask,
    const double  Contrast, /* 5 */
    const int     Saturation /* -50 */)
{
  // Workflow for Orton from
  // http://www.flickr.com/groups/gimpusers/discuss/72157621381832321/
  ptImage *BlurLayer = new ptImage;
  BlurLayer->Set(this);
  // for Orton
  if (SoftglowMode==TSoftglowMode::OrtonScreen || SoftglowMode==TSoftglowMode::OrtonSoftlight) {
    BlurLayer->Overlay(BlurLayer->m_Image,1.0, nullptr, ptOverlayMode_Screen);
  }

  // Blur
  if (Radius != 0)
    BlurLayer->ptCIBlur(Radius, ChannelMask);
  // Contrast
  if (Contrast != 0) {
    ptCurve* MyContrastCurve = new ptCurve();
    MyContrastCurve->setFromFunc(ptCurve::Sigmoidal,0.5,Contrast);
    BlurLayer->ApplyCurve(MyContrastCurve,7);
    delete MyContrastCurve;
  }
  // Desaturate
  if (Saturation != 0) {
    int Value = Saturation;
    TChannelMatrix VibranceMixer;
    VibranceMixer[0][0] = 1.0+(Value/150.0);
    VibranceMixer[0][1] = -(Value/300.0);
    VibranceMixer[0][2] = VibranceMixer[0][1];
    VibranceMixer[1][0] = VibranceMixer[0][1];
    VibranceMixer[1][1] = VibranceMixer[0][0];
    VibranceMixer[1][2] = VibranceMixer[0][1];
    VibranceMixer[2][0] = VibranceMixer[0][1];
    VibranceMixer[2][1] = VibranceMixer[0][1];
    VibranceMixer[2][2] = VibranceMixer[0][0];
    BlurLayer->mixChannels(VibranceMixer);
  }
  // Overlay
  switch (SoftglowMode) {
    case TSoftglowMode::Lighten:
      Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_Lighten);
      break;
    case TSoftglowMode::Screen:
      Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_Screen);
      break;
    case TSoftglowMode::Softlight:
      Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_SoftLight);
      break;
    case TSoftglowMode::Normal:
      if (Amount != 0)
        Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_Normal);
      break;
    case TSoftglowMode::OrtonScreen:
      Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_Screen);
      break;
    case TSoftglowMode::OrtonSoftlight:
      Overlay(BlurLayer->m_Image,Amount,nullptr,ptOverlayMode_SoftLight);
      break;
    default:
      break;
  }
  delete BlurLayer;
  return this;

}

////////////////////////////////////////////////////////////////////////////////
//
// GetMask
//
////////////////////////////////////////////////////////////////////////////////

float *ptImage::GetMask(const TMaskType MaskType,
                        const double LowerLimit,
                        const double UpperLimit,
                        const double Softness,
                        const double FactorR,
                        const double FactorG,
                        const double FactorB) {

  float (*dMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*dMask));
  ptMemoryError(dMask,__FILE__,__LINE__);

  const float WP = 0xffff;
  float m  = 1.0/(UpperLimit-LowerLimit);
  float t  = -LowerLimit/(UpperLimit-LowerLimit)*WP;
  float m1 = -1.0/MAX(LowerLimit,0.001);
  float m2 = 1.0/MAX(0.001,(1.0-UpperLimit));
  float t2 = -(UpperLimit)/MAX(0.001,(1.0-UpperLimit))*WP;
  float Soft = pow(2,Softness);

  // Precalculated table for the mask.
  float MaskTable[0x10000];
  float FactorRTable[0x10000];
  float FactorGTable[0x10000];
  float FactorBTable[0x10000];
#pragma omp parallel
{ // begin OpenMP
#pragma omp for schedule(static)
  for (int32_t i=0; i<0x10000; i++) {
    switch(MaskType) {
    case TMaskType::All: // All values
      MaskTable[i] = WP;
        break;
    case TMaskType::Shadows: // Shadows
        MaskTable[i] = WP - CLIP((int32_t)(i*m+t));
        break;
    case TMaskType::Midtones: // Midtones
      MaskTable[i] = WP - CLIP((int32_t)(i*m1+WP)) - CLIP((int32_t)(i*m2+t2));
        break;
    case TMaskType::Highlights: // Highlights
         MaskTable[i] = CLIP((int32_t)(i*m+t));
         break;
    default:
      assert("Unexpected mask type");
    }
    if (Soft>1.0) {
      MaskTable[i] = ptBound((float)(pow(MaskTable[i]/0xffff,Soft)), 0.0f, 1.0f);
    } else {
      MaskTable[i] = ptBound((float)(MaskTable[i]/0xffff/Soft), 0.0f, 1.0f);
    }
    FactorRTable[i] = i*FactorR;
    FactorGTable[i] = i*FactorG;
    FactorBTable[i] = i*FactorB;
  }

#pragma omp for schedule(static)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    dMask[i] = MaskTable[CLIP((int32_t)
                              (FactorRTable[m_Image[i][0]] +
                               FactorGTable[m_Image[i][1]] +
                               FactorBTable[m_Image[i][2]]))];
  }
} // end OpenMP

  return dMask;
}

//==============================================================================

// InMask should only be called once per pixel!
void fill4stack(float         *AMask,
                const uint16_t APointX,
                const uint16_t APointY,
                const uint16_t AWidth,
                const uint16_t AHeight,
                std::function<float (uint16_t, uint16_t)> InMask){

  std::stack<uint16_t> hStack;
  hStack.push(APointX);
  hStack.push(APointY);

  uint16_t hX     = 0,
           hY     = 0;
  float    hValue = 0.0f;

  while (!hStack.empty()) {
    hY = hStack.top();
    hStack.pop();
    hX = hStack.top();
    hStack.pop();

    // array bounds
    // hX, hY >= 0 by design
    if (hX >= AWidth || hY >= AHeight ) continue;

    // already processed?
    if (AMask[hY*AWidth + hX] != 0.0f) continue;

    hValue = InMask(hX, hY);
    // pixel suitable for mask?
    if (hValue > 0.0f) {
      AMask[hY*AWidth + hX] = hValue;

      hStack.push(hX);
      hStack.push(hY + 1);

      if (hY > 0) {
        hStack.push(hX);
        hStack.push(hY - 1);
      }

      hStack.push(hX + 1);
      hStack.push(hY);

      if (hX > 0) {
        hStack.push(hX - 1);
        hStack.push(hY);
      }
    }
  }
}

//==============================================================================

float *ptImage::FillMask(const uint16_t APointX,
                         const uint16_t APointY,
                         const float    AThreshold,
                         const float    AColorWeight,
                         const uint16_t AMaxRadius,
                         const bool     AUseMaxRadius)
{
  assert (m_ColorSpace == ptSpace_LCH);

  float (*FillMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*FillMask));
  ptMemoryError(FillMask,__FILE__,__LINE__);

  memset(FillMask, 0, m_Width*m_Height*sizeof(*FillMask));

  float hThresholdHalf = AThreshold*0x2AAA;
  float hThreshold     = AThreshold*0x5555;
  float hRadiusOut     = ptSqr((float)AMaxRadius);
  float hLumaWeight    = 1.0f - AColorWeight;
  float hColorWeight   = AColorWeight*(float)0x7FFF;

  float hValueL  = 0.0f,
        hValueC  = 0.0f,
        hValueH  = 0.0f;
  float    hDiff = 0;
  int32_t  hIdx  = 0;
  float    hRad  = 0;

  // Calculate mean around the sample point
  short   hSample = 1;
  int32_t hCnt    = 0;

  for (int32_t lX = APointX-hSample; lX <= APointX+hSample; lX++) {
    if (lX < 0 || lX >= m_Width) continue;

    for (int32_t lY = APointY-hSample; lY <= APointY+hSample; lY++) {
      if (lY < 0 || lY >= m_Height) continue;

      hIdx     = (int32_t)lY*m_Width + lX;
      hValueL += m_ImageL[hIdx];
      hValueC += m_ImageC[hIdx];
      hValueH += m_ImageH[hIdx];
      hCnt++;
    }
  }
  hValueL /= hCnt;
  hValueC /= hCnt;
  hValueH /= hCnt;

  // fill mask with radius and value threshold
  fill4stack(FillMask, APointX, APointY, m_Width, m_Height,
             [&](uint16_t X, uint16_t Y) -> float {

    float hResult = 1.0f;
    if (AUseMaxRadius) {
      hRad = ptSqr(std::abs((int32_t)X-APointX)) + ptSqr(std::abs((int32_t)Y-APointY));
      if (hRad < hRadiusOut) hResult = ptSqr((hRadiusOut - hRad)/hRadiusOut);
      else                   hResult = 0.0f;
    }

    if (hResult == 0.0f) return hResult;

    hIdx  = (int32_t)Y*m_Width + X;

    hDiff = std::abs((float)m_ImageC[hIdx] - hValueC)             +
            std::abs((float)m_ImageL[hIdx] - hValueL)*hLumaWeight +
            std::abs((float)m_ImageH[hIdx] - hValueH)*hColorWeight*(float)m_ImageC[hIdx]/(float)0x1fff;

    if (hDiff > hThresholdHalf) {
      if (hDiff < hThreshold) hResult = hResult*powf((hThreshold - hDiff)/hThresholdHalf, 4.0f);
      else                    hResult = 0.0f;
    }

    return hResult;
  });

  return FillMask;
}

//==============================================================================

ptImage *ptImage::MaskedColorAdjust(const int       Ax,
                                    const int       Ay,
                                    const float     AThreshold,
                                    const float     AChromaWeight,
                                    const int       AMaxRadius,
                                    const bool      AHasMaxRadius,
                                    const ptCurve  *ACurve,
                                    const bool      ASatAdaptive,
                                    float           ASaturation,
                                    const float     AHueShift)
{
  assert (m_ColorSpace == ptSpace_LCH);

  float *hMask = FillMask(Ax, Ay, AThreshold*2.0f, AChromaWeight, AMaxRadius, AHasMaxRadius);
  const bool     hSatAdjust   = ASaturation != 0.0f;
  const bool     hHueAdjust   = AHueShift != 0.0f;
  const float    hHueShift    = AHueShift * pt2PI;

  // we enhance positive saturation values; negative values should give B&W with
  // -1.0, at least when not adaptive.
  if (ASaturation > 0) ASaturation *= 2.0f;

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i< (uint32_t)m_Height*m_Width; i++) {
    if (hMask[i] > 0.0f) {
      m_ImageL[i] = m_ImageL[i]                  * (1.0f - hMask[i]) +
                    ACurve->Curve[m_ImageL[i]] * hMask[i];

      if (hSatAdjust) {
        if (ASatAdaptive)
          m_ImageC[i] = m_ImageC[i] * (1.0f + hMask[i] * ASaturation * (2.0f - m_ImageC[i]/(float)0x3FFF));
        else
          m_ImageC[i] = m_ImageC[i] * (1.0f + hMask[i] * ASaturation);
      }

      if (hHueAdjust) {
        m_ImageH[i] = m_ImageH[i] + hHueShift * hMask[i];
      }
    }
  }

  FREE(hMask);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetVignetteMask
//
////////////////////////////////////////////////////////////////////////////////

float *ptImage::GetVignetteMask(const short  Inverted,
                                const short  Exponent,
                                const double InnerRadius,
                                const double OuterRadius,
                                const double Roundness,
                                const double CenterX,
                                const double CenterY,
                                const double Softness) {

  float Radius = MIN(m_Width, m_Height)/2;

  bool Switch = OuterRadius < InnerRadius;

  float OR = Radius*(Switch?InnerRadius:OuterRadius);
  float IR = Radius*(Switch?OuterRadius:InnerRadius);
  float Black = Inverted?1.0:0.0;
  float White = Inverted?0.0:1.0;
  if (Switch) {
    Black = 1.0 - Black;
    White = 1.0 - White;
  }
  float ColorDiff = White - Black;

  float CX = (1+CenterX)*m_Width/2;
  float CY = (1-CenterY)*m_Height/2;

  float InversExponent = 1.0/ Exponent;
  float coordinate = 0;
  float Value = 0;
  float (*VignetteMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*VignetteMask));
  ptMemoryError(VignetteMask,__FILE__,__LINE__);
  float dist = 0;
  float Denom = 1/MAX((OR-IR),0.0001f);
  float Factor1 = 1/powf(2,Roundness);
  float Factor2 = 1/powf(2,-Roundness);

  #pragma omp parallel for schedule(static) default(shared) firstprivate(dist, Value, coordinate)
  for (uint16_t Row=0; Row<m_Height; Row++) {
    int32_t Idx = Row*m_Width;
    for (uint16_t Col=0; Col<m_Width; Col++) {
      dist = powf(powf(fabsf((float)Col-CX)*Factor1,Exponent)
                  + powf(fabsf((float)Row-CY)*Factor2,Exponent),InversExponent);
      if (dist <= IR)
        VignetteMask[Idx+Col] = Black;
      else if (dist >= OR)
        VignetteMask[Idx+Col] = White;
      else {
        coordinate = 1.0f-(OR-dist)*Denom;
        Value = (1.0f-powf(cosf(coordinate*ptPI/2.0f),50.0f*Softness))
                * powf(coordinate,0.07f*Softness);
        VignetteMask[Idx+Col] = LIM(Value*ColorDiff+Black,0.0f,1.0f);
      }
    }
  }

  return VignetteMask;
}

////////////////////////////////////////////////////////////////////////////////
//
// Gradual Blur
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::GradualBlur(const TGradualBlurMode Mode,
    const double MaxRadius,
    const double LowerLevel,
    const double UpperLevel,
    const double Softness,
    const double Angle,
    const TVignetteShape Vignette,
    const double Roundness,
    const double CenterX,
    const double CenterY)
{

  float* Mask = nullptr;

  if (Mode == TGradualBlurMode::Linear || Mode == TGradualBlurMode::LinearMask) {
    Mask = GetGradualMask(Angle, LowerLevel, UpperLevel, Softness);
  } else if (Mode == TGradualBlurMode::Vignette || Mode == TGradualBlurMode::VignetteMask) {
    Mask = GetVignetteMask(0, static_cast<int>(Vignette), LowerLevel, UpperLevel, Roundness, CenterX, CenterY, Softness);
  }

  if ((Mode == TGradualBlurMode::LinearMask) || (Mode == TGradualBlurMode::VignetteMask)) {
    Overlay(m_Image, 1, Mask, ptOverlayMode_ShowMask);
  } else {
    Box((uint16_t)(ceil(MaxRadius)), Mask);
  }

  FREE(Mask);
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Box
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Box(const uint16_t MaxRadius, float* Mask) {
  ptImage* Source = new ptImage();
  Source->Set(this);

  const uint16_t Height1   = m_Height - 1;
  const uint16_t Width1    = m_Width  - 1;
  const uint16_t Height1_2 = 2*Height1;
  const uint16_t Width1_2  = 2*Width1;

  // Construct the distance matrix
  const uint16_t DistSize = MaxRadius+1;
  float** Dist = (float**) CALLOC(DistSize,sizeof(float*));
  for(int16_t i = 0; i < DistSize; i++) Dist[i] = (float*) CALLOC(DistSize,sizeof(float));

  // fill distance matrix
  for(int16_t i = 0; i <= MaxRadius; i++) {
    for(int16_t j = 0; j <= MaxRadius; j++) {
      Dist[i][j] = powf((float) i*i + (float) j*j, 0.5f);
    }
  }

  float     Sum0, Sum1, Sum2, Radius;
  int16_t   i, j;
  int32_t   Temp, NewRow, NewCol, Index;
  uint16_t  Row, Col, Count, IntRadius, *PtrSource, *PtrTarget;

#pragma omp parallel for private(Sum0, Sum1, Sum2, i, j, Temp, Row, Col, Count, Radius, IntRadius, NewRow, NewCol, PtrSource, PtrTarget, Index)
  for (Row=0; Row<m_Height; Row++) {
    Temp = Row*m_Width;
    for (Col=0; Col<m_Width; Col++) {
      Sum0  = 0.0f;
      Sum1  = 0.0f;
      Sum2  = 0.0f;
      Count = 0;

      Index     = Temp + Col;
      Radius    = MaxRadius * Mask[Index];
      IntRadius = ceil(Radius);
      if (IntRadius == 0) continue;

      for(i = -IntRadius; i <= IntRadius; i++) {
        NewRow = Row+i;
        NewRow = NewRow < 0? -NewRow : NewRow > Height1? Height1_2-NewRow : NewRow ;
        NewRow *= m_Width;
        for(j = -IntRadius; j <= IntRadius; j++) {
          if (Dist[abs(i)][abs(j)] < Radius) {
            NewCol = Col+j;
            NewCol = NewCol < 0? -NewCol : NewCol > Width1? Width1_2-NewCol : NewCol ;

            Count++;
            PtrSource = Source->m_Image[NewRow + NewCol];
            Sum0     += PtrSource[0];
            Sum1     += PtrSource[1];
            Sum2     += PtrSource[2];
          }
        }
      }

      PtrTarget    = m_Image[Index];
      PtrTarget[0] = Sum0 / Count;
      PtrTarget[1] = Sum1 / Count;
      PtrTarget[2] = Sum2 / Count;
    }
  }

  for(uint16_t i = 0; i < DistSize; i++) FREE(Dist[i]);
  FREE(Dist);

  delete Source;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// ViewLAB
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ViewLab(const TViewLabChannel Channel) {

  assert (m_ColorSpace == ptSpace_Lab);

  const double WPH = 0x7fff; // WPH=WP/2

  ptImage *ContrastLayer = new ptImage;
  ptCurve* MyContrastCurve = new ptCurve();

  switch(Channel) {
    case TViewLabChannel::L:
#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case TViewLabChannel::a:
#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=m_Image[i][1];
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case TViewLabChannel::b:
#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=m_Image[i][2];
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case TViewLabChannel::LStructure:
      ContrastLayer->Set(this);
      ContrastLayer->fastBilateralChannel(4, 0.2, 2, ChMask_L);

#     pragma omp parallel for default(shared) schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
      }
      MyContrastCurve->setFromFunc(ptCurve::Sigmoidal,0.5,30);
      ApplyCurve(MyContrastCurve,1);

#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case TViewLabChannel::C:
      ContrastLayer->Set(this);
      ContrastLayer->LabToLch();
#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=CLIP((int32_t)(2.0f*ContrastLayer->m_ImageC[i]));
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case TViewLabChannel::H:
      ContrastLayer->Set(this);
      ContrastLayer->LabToLch();
#     pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=CLIP((int32_t)(ContrastLayer->m_ImageH[i]/pt2PI*(float)0xffff));
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    default:
      assert("unexpected ViewLab channel mode");
  }

  delete MyContrastCurve;
  delete ContrastLayer;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Special Preview
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::SpecialPreview(const short Mode, const int Intent) {

  if (Mode == ptSpecialPreview_L ||
      Mode == ptSpecialPreview_A ||
      Mode == ptSpecialPreview_B) {
    int InColorSpace = m_ColorSpace;
    cmsHPROFILE InternalProfile = 0;
    if (InColorSpace != ptSpace_Profiled) {
      // linear case
      cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
      cmsToneCurve* Gamma3[3];
      Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

      cmsCIExyY       DFromReference;

      switch (m_ColorSpace) {
        case ptSpace_sRGB_D65 :
        case ptSpace_AdobeRGB_D65 :
          DFromReference = D65;
          break;
        case ptSpace_WideGamutRGB_D50 :
        case ptSpace_ProPhotoRGB_D50 :
          DFromReference = D50;
          break;
        default:
          assert(0);
      }

      InternalProfile = cmsCreateRGBProfile(&DFromReference,
                                      (cmsCIExyYTRIPLE*)&RGBPrimaries[m_ColorSpace],
                                      Gamma3);

      if (!InternalProfile) {
        ptLogError(ptError_Profile,"Could not open InternalProfile profile.");
        return this;
      }

      cmsFreeToneCurve(Gamma);
    } else {
      // profiled case
      InternalProfile = PreviewColorProfile;
    }

    cmsHPROFILE LabProfile = 0;
    LabProfile = cmsCreateLab4Profile(NULL);
    // to Lab
    cmsHTRANSFORM Transform;
    Transform = cmsCreateTransform(InternalProfile,
                                   TYPE_RGB_16,
                                   LabProfile,
                                   TYPE_Lab_16,
                                   Intent,
                                   cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

    int32_t Size = m_Width*m_Height;
    int32_t Step = 100000;
  #pragma omp parallel for schedule(static)
    for (int32_t i = 0; i < Size; i+=Step) {
      int32_t Length = (i+Step)<Size ? Step : Size - i;
      uint16_t* Tile = &(m_Image[i][0]);
      cmsDoTransform(Transform,Tile,Tile,Length);
    }
    m_ColorSpace = ptSpace_Lab;
    // ViewLAB
    if (Mode == ptSpecialPreview_L) ViewLab(TViewLabChannel::L);
    if (Mode == ptSpecialPreview_A) ViewLab(TViewLabChannel::a);
    if (Mode == ptSpecialPreview_B) ViewLab(TViewLabChannel::b);

    // to RGB
    Transform = cmsCreateTransform(LabProfile,
                                   TYPE_Lab_16,
                                   InternalProfile,
                                   TYPE_RGB_16,
                                   Intent,
                                   cmsFLAGS_NOOPTIMIZE | cmsFLAGS_BLACKPOINTCOMPENSATION);

  #pragma omp parallel for schedule(static)
    for (int32_t i = 0; i < Size; i+=Step) {
      int32_t Length = (i+Step)<Size ? Step : Size - i;
      uint16_t* Tile = &(m_Image[i][0]);
      cmsDoTransform(Transform,Tile,Tile,Length);
    }

    cmsDeleteTransform(Transform);
    cmsCloseProfile(LabProfile);
    if (InColorSpace != ptSpace_Profiled)
      cmsCloseProfile(InternalProfile);
    m_ColorSpace = InColorSpace;
  } else if (Mode==ptSpecialPreview_Structure) {
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][0] = RGB_2_L(m_Image[i]);
    }
    const double WPH = 0x7fff; // WPH=WP/2
    ptImage *ContrastLayer = new ptImage;
    ContrastLayer->Set(this);
    ContrastLayer->fastBilateralChannel(4, 0.2, 2, ChMask_L);
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0]) + m_Image[i][0]));
    }
    ptCurve* MyContrastCurve = new ptCurve();
    MyContrastCurve->setFromFunc(ptCurve::Sigmoidal,0.5,30);
    ApplyCurve(MyContrastCurve,1);
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][1] = m_Image[i][0];
      m_Image[i][2] = m_Image[i][0];
    }
    delete MyContrastCurve;
    delete ContrastLayer;
  } else if (Mode==ptSpecialPreview_Gradient) {
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][0] = RGB_2_L(m_Image[i]);
    }
    ptCimgEdgeDetection(this,1);
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][1] = m_Image[i][0];
      m_Image[i][2] = m_Image[i][0];
    }
  }
  return this;
}

// -----------------------------------------------------------------------------

ptImage* ptImage::mixChannels(const TChannelMatrix mixFactors) {
  assert (m_ColorSpace != ptSpace_Lab);
  double value[3];

# pragma omp parallel for default(shared) private(value)
  for (uint32_t px = 0; px < static_cast<uint32_t>(m_Height*m_Width); ++px) {
    for (int to = 0; to < 3; ++to) {
       value[to] = 0;
       for (int from = 0; from < 3; ++from) {
          value[to] += mixFactors[to][from] * m_Image[px][from];
       }
    }
    for (int to = 0; to < 3; ++to) {
      m_Image[px][to] = CLIP(static_cast<int32_t>(value[to]));
    }
  }
  return this;
}

//==============================================================================

ptImage *ptImage::Highlights(const float AHlRed, const float AHlGreen, const float AHlBlue) {
  TAnchorList hAnchors;
  for (int i=0; i<12; ++i) {  // 12th anchor is a dummy
    hAnchors.push_back(TAnchor((double) i/33.0, (double) i/33.0));
  }

  ptCurve HighlightsCurve(hAnchors, false);   // no immediate curve calc

  if (AHlRed < 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0, 1.0+0.5*AHlRed));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,1);
  } else if (AHlRed > 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0-0.5*AHlRed, 1.0));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,1);
  }

  if (AHlGreen < 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0, 1.0+0.5*AHlGreen));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,2);
  } else if (AHlGreen > 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0-0.5*AHlGreen, 1.0));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,2);
  }

  if (AHlBlue < 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0, 1.0+0.5*AHlBlue));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,4);
  } else if (AHlBlue > 0) {
    HighlightsCurve.setAnchor(11, TAnchor(1.0-0.5*AHlBlue, 1.0));
    HighlightsCurve.calcCurve();
    ApplyCurve(&HighlightsCurve,4);
  }

  return this;
}

//==============================================================================

ptImage *ptImage::GammaTool(const float AStrength, const float ALinearity)
{
  ptCurve RGBGammaCurve;
  RGBGammaCurve.setFromFunc(ptCurve::GammaTool, AStrength, ALinearity);
  ApplyCurve(&RGBGammaCurve, ChMask_RGB);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// WriteAsPpm
//
////////////////////////////////////////////////////////////////////////////////

short ptImage::WriteAsPpm(const char*  FileName,
                          const short  BitsPerColor) {

  assert ((8==BitsPerColor)||(16==BitsPerColor));
  assert (ptSpace_Lab != m_ColorSpace);
  assert (ptSpace_XYZ != m_ColorSpace);

  FILE *OutputFile = fopen(FileName,"wb");
  if (!OutputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  fprintf(OutputFile, "P%d\n%d %d\n%d\n", m_Colors/2+5, m_Width, m_Height,
          (1 << BitsPerColor)-1);

  uint8_t*  PpmRow = (uint8_t *) CALLOC(m_Width,m_Colors*BitsPerColor/8);
  ptMemoryError(PpmRow,__FILE__,__LINE__);

  // Same buffer, interpreted as short though (16bit)
  uint16_t* PpmRow2= (uint16_t*) PpmRow;

  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {
      if (8 == BitsPerColor) {
        for (short c=0;c<3;c++) {
          PpmRow [Col*m_Colors+c] = m_Image[Row*m_Width+Col][c] >>8;
        }
      } else  {
        for (short c=0;c<3;c++) {
          PpmRow2[Col*m_Colors+c] = m_Image[Row*m_Width+Col][c];
        }
      }
    }
    if (16 == BitsPerColor && htons(0x55aa) != 0x55aa) {
      swab((char *)PpmRow,(char *)PpmRow,m_Width*m_Colors*2);
    }
    uint16_t w = fwrite(PpmRow,m_Colors*BitsPerColor/8,m_Width,OutputFile);
    assert(m_Width == w);
  }

  FREE(PpmRow);
  FCLOSE(OutputFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// WriteAsJpeg
//
////////////////////////////////////////////////////////////////////////////////
#ifndef NO_JPEG
short ptImage::WriteAsJpeg(const char*    FileName,
                             const short    Quality,
                             const uint8_t* ExifBuffer,
                             const unsigned ExifBufferLen ) {

  assert (ptSpace_Lab != m_ColorSpace);
  assert (ptSpace_XYZ != m_ColorSpace);

  FILE *OutputFile = fopen(FileName,"wb");
  if (!OutputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo,OutputFile);
  cinfo.image_width  = m_Width;
  cinfo.image_height = m_Height;
  cinfo.input_components = m_Colors;
  cinfo.in_color_space = JCS_RGB; // m_colors = 1 grey ???
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo,Quality,TRUE);
  jpeg_start_compress(&cinfo,TRUE);
  // TODO Embed exif and profile stuff here in between.

  if (ExifBuffer) {
    if (ExifBufferLen > 65533) {
      ptLogWarning(ptWarning_Argument,
                   "Exif buffer length %d is too long. Ignored",
                   ExifBufferLen);
    } else {
      jpeg_write_marker(&cinfo,JPEG_APP0+1,ExifBuffer,ExifBufferLen);
    }
  }

  uint8_t*  PpmRow = (uint8_t *) CALLOC(m_Width,m_Colors);
  ptMemoryError(PpmRow,__FILE__,__LINE__);

  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {
      for (short c=0;c<3;c++) {
        PpmRow [Col*m_Colors+c] = m_Image[Row*m_Width+Col][c] >>8;
      }
    }
    jpeg_write_scanlines(&cinfo,&PpmRow,1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  FREE(PpmRow);
  FCLOSE(OutputFile);
  return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
// WaveletDenoise()
// Basically from from a gimp plugin from dcraw to do a 'wavelet' denoise.
//
// Threshold : 0..0xffff
// New implementation based on the wavelet denoise plugin for gimp 0.3.1
// which is still based on dcraw...
//
////////////////////////////////////////////////////////////////////////////////

// forward declaration of  helper.
static void hat_transform(float *temp, float *base,int st,int size,int sc);

ptImage* ptImage::WaveletDenoise(const uint8_t  ChannelMask,
         const double Threshold,
         const double low,
         const short WithMask,
         const double Sharpness,
         const double Anisotropy,
         const double Alpha,
         const double Sigma) {

  assert(m_Colors==3);
  if (WithMask) assert(m_ColorSpace == ptSpace_Lab);
  uint32_t Size = m_Width*m_Height;

  const float WP = 0xffff;
  float stdev[5];
  uint32_t samples[5];

  // 3 : Image, lpass 0, lpass 1 + some tail for temporary processing.
  // tail is shifted for multithreading
  float *fImage = (float *) MALLOC((Size*3)*sizeof(*fImage));
  ptMemoryError(fImage,__FILE__,__LINE__);

  for (short Channel=0; Channel<m_Colors; Channel++) {

    // Channel supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;

    // algorithm works between 0..1
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i<Size; i++) {
      fImage[i] = ToFloatTable[m_Image[i][Channel]];
    }

    uint32_t lpass;
    uint32_t hpass = 0;
    for (uint16_t lev = 0; lev < 5; lev++) {
      lpass = Size*((lev & 1) + 1);

#pragma omp parallel default(shared)
    {
#ifdef _OPENMP
      // We need a thread-private copy.
      float *TempTemp = (float *) MALLOC((m_Height+m_Width)*sizeof(*fImage));
#else
      // Working in the trail (foreseen at CALLOC) done here for multithreading
      float *Temp = (float *) MALLOC((m_Height+m_Width)*sizeof(*fImage));
#endif
#pragma omp for
      for (uint16_t Row=0; Row<m_Height; Row++) {
#ifdef _OPENMP
        hat_transform(TempTemp,fImage+hpass+Row*m_Width,1,m_Width,1<<lev);
        for (uint16_t Col = 0; Col<m_Width; Col++) {
          fImage[lpass+Row*m_Width+Col] = TempTemp[Col] * 0.25;
        }
#else
        hat_transform(Temp,fImage+hpass+Row*m_Width,1,m_Width,1<<lev);
        for (uint16_t Col = 0; Col<m_Width; Col++) {
          fImage[lpass+Row*m_Width+Col] = Temp[Col] * 0.25;
        }
#endif
      }
#pragma omp for
      for (uint16_t Col=0; Col<m_Width; Col++) {
#ifdef _OPENMP
        hat_transform(TempTemp,fImage+lpass+Col,m_Width,m_Height,1 << lev);
        for (uint16_t Row = 0; Row < m_Height; Row++) {
          fImage[lpass+Row*m_Width+Col] = TempTemp[Row] * 0.25;
        }
#else
        hat_transform(Temp,fImage+lpass+Col,m_Width,m_Height,1 << lev);
        for (uint16_t Row = 0; Row < m_Height; Row++) {
          fImage[lpass+Row*m_Width+Col] = Temp[Row] * 0.25;
        }
#endif
      }
#ifdef _OPENMP
      // Free thread-private copy.
      free (TempTemp);
#else
      // free
      free (Temp);
#endif

    } // End omp parallel zone.

      float THold = 5.0 / (1 << 6) * exp (-2.6 * sqrt (lev + 1)) * 0.8002 / exp (-2.6);

      /* initialize stdev values for all intensities */
      stdev[0] = stdev[1] = stdev[2] = stdev[3] = stdev[4] = 0.0;
      samples[0] = samples[1] = samples[2] = samples[3] = samples[4] = 0;

#pragma omp parallel default(shared)
  {
      /* calculate stdevs for all intensities */
#ifdef _OPENMP
      // We need a thread-private copy.
      float Tempstdev[5] = {0.0,0.0,0.0,0.0,0.0};
      uint32_t Tempsamples[5] = {0,0,0,0,0};
#endif
#pragma omp for
      for (uint32_t i = 0; i < Size; i++) {
        fImage[hpass+i] -= fImage[lpass+i];
        if (fImage[hpass+i] < THold && fImage[hpass+i] > -THold) {
#ifdef _OPENMP
          if (fImage[lpass+i] > 0.8) {
            Tempstdev[4] += fImage[hpass+i] * fImage[hpass+i];
            Tempsamples[4]++;
          } else if (fImage[lpass+i] > 0.6) {
            Tempstdev[3] += fImage[hpass+i] * fImage[hpass+i];
            Tempsamples[3]++;
          } else if (fImage[lpass+i] > 0.4) {
            Tempstdev[2] += fImage[hpass+i] * fImage[hpass+i];
            Tempsamples[2]++;
          } else if (fImage[lpass+i] > 0.2) {
            Tempstdev[1] += fImage[hpass+i] * fImage[hpass+i];
            Tempsamples[1]++;
          } else {
            Tempstdev[0] += fImage[hpass+i] * fImage[hpass+i];
            Tempsamples[0]++;
          }
#else
          if (fImage[lpass+i] > 0.8) {
            stdev[4] += fImage[hpass+i] * fImage[hpass+i];
            samples[4]++;
          } else if (fImage[lpass+i] > 0.6) {
            stdev[3] += fImage[hpass+i] * fImage[hpass+i];
            samples[3]++;
          } else if (fImage[lpass+i] > 0.4) {
            stdev[2] += fImage[hpass+i] * fImage[hpass+i];
            samples[2]++;
          } else if (fImage[lpass+i] > 0.2) {
            stdev[1] += fImage[hpass+i] * fImage[hpass+i];
            samples[1]++;
          } else {
            stdev[0] += fImage[hpass+i] * fImage[hpass+i];
            samples[0]++;
          }
#endif
        }
      }
#ifdef _OPENMP
#pragma omp critical
      for (int i = 0; i < 5; i++) {
        stdev[i] += Tempstdev[i];
        samples[i] += Tempsamples[i];
      }
#endif
    } // End omp parallel zone.
      stdev[0] = sqrt (stdev[0] / (samples[0] + 1));
      stdev[1] = sqrt (stdev[1] / (samples[1] + 1));
      stdev[2] = sqrt (stdev[2] / (samples[2] + 1));
      stdev[3] = sqrt (stdev[3] / (samples[3] + 1));
      stdev[4] = sqrt (stdev[4] / (samples[4] + 1));

      /* do thresholding */
#pragma omp parallel for default(shared) private(THold)
      for (uint32_t i = 0; i < Size; i++) {
        if (fImage[lpass+i] > 0.8) {
          THold = Threshold * stdev[4];
        } else if (fImage[lpass+i] > 0.6) {
          THold = Threshold * stdev[3];
        } else if (fImage[lpass+i] > 0.4) {
          THold = Threshold * stdev[2];
        } else if (fImage[lpass+i] > 0.2) {
          THold = Threshold * stdev[1];
        } else {
          THold = Threshold * stdev[0];
        }

        if (fImage[hpass+i] < -THold)
          fImage[hpass+i] += THold - THold * low;
        else if (fImage[hpass+i] > THold)
          fImage[hpass+i] -= THold - THold * low;
        else
          fImage[hpass+i] *= low;

        if (hpass)
          fImage[0+i] += fImage[hpass+i];
      }

      hpass = lpass;
    }

    if (Channel==0 && WithMask && (Sharpness || (Anisotropy > 0.5))) {
      ptImage *MaskLayer = new ptImage;
      MaskLayer->Set(this);
      ptCimgEdgeTensors(MaskLayer,Sharpness,Anisotropy,Alpha,Sigma);
#pragma omp parallel for default(shared)
      for (uint32_t i=0; i<Size; i++) {
        m_Image[i][Channel] = CLIP((int32_t)((fImage[i] + fImage[lpass+i])*MaskLayer->m_Image[i][0]+
                                             m_Image[i][Channel]*(1-(float)MaskLayer->m_Image[i][0]/(float)0xffff)));
      }
      delete MaskLayer;
    } else {
#pragma omp parallel for default(shared)
      for (uint32_t i=0; i<Size; i++) {
        m_Image[i][Channel] = CLIP((int32_t)((fImage[i] + fImage[lpass+i])*WP));
      }
    }
  }

  FREE(fImage);

  return this;
}

static void hat_transform (float *temp, float *base, int st, int size, int sc) {
  int i;
  for (i = 0; i < sc; i++)
    temp[i] = 2 * base[st * i] + base[st * (sc - i)] + base[st * (i + sc)];
  for (; i + sc < size; i++)
    temp[i] = 2 * base[st * i] + base[st * (i - sc)] + base[st * (i + sc)];
  for (; i < size; i++)
    temp[i] = 2 * base[st * i] + base[st * (i - sc)]
        + base[st * (2 * size - 2 - (i + sc))];
}

TAnchorList ptImage::createAmpAnchors(const double Amount,
                                               const double HaloControl)
{
// TODO: mike: Anpassen der Verstärkungskurve an den Sigmoidalen Kontrast.
// Der Berech mit Halocontrol soll linear sein, der andere sigmoidal.
// Wichtig ist, dass bei kleinen Werten der Halocontrol im schwächeren Bereich
// keine Verstärkung auftritt, da evtl die Ableitung der sigmoidalen Kurve
// schwächer ist, als bei der linearen Kurve.
// Anpassen bei allen Filtern, die dieses Verfahren benutzen.

  const double  t     = (1.0 - Amount)/2;
  const double  mHC   = Amount*(1.0-fabs(HaloControl)); // m with HaloControl
  const double  tHC   = (1.0 - mHC)/2; // t with HaloControl
  const int     Steps = 20;

  TAnchorList hAnchors;
  for (int i = 0; i<= Steps; i++) {
    auto x = (float)i/(float)Steps;
    float YAnchor = 0;
    if (x < 0.5) {
      if (HaloControl > 0) YAnchor=mHC*x+tHC;
      else                 YAnchor=Amount*x+t;
    } else if (x > 0.5) {
      if (HaloControl < 0) YAnchor=mHC*x+tHC;
      else                 YAnchor=Amount*x+t;
    } else {
      YAnchor=0.5;
    }
    hAnchors.push_back(TAnchor(x, YAnchor));
  }
  return hAnchors;
}

void ptImage::setCurrentRGB(const short ARGB)
{
  CurrentRGBMode = ARGB;
}

short ptImage::getCurrentRGB()
{
  return CurrentRGBMode;
}

// -----------------------------------------------------------------------------

typedef Array_2D<float> image_type;
extern float ToFloatTable[0x10000];

// From the theoretical part, the bilateral filter should blur more when values
// are closer together. Since we use it with linear data, an additional gamma
// correction could give better results.

ptImage* ptImage::fastBilateralChannel(
    const float Sigma_s,
    const float Sigma_r,
    const int Iterations,
    const TChannelMask ChannelMask)
{
  uint16_t Width  = m_Width;
  uint16_t Height = m_Height;
  int32_t  hIdx   = 0;

  image_type InImage(Width,Height);
  image_type FilteredImage(Width,Height);

  for (short Channel = 0; Channel<3; Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static) private(hIdx)
    for (uint16_t Row=0; Row<m_Height; Row++) {
      hIdx = Row*Width;
      for (uint16_t Col=0; Col<m_Width; Col++) {
        InImage(Col,Row) = ToFloatTable[m_Image[hIdx+Col][Channel]];
      }
    }

    for (int i=0;i<Iterations;i++)
      Image_filter::fast_LBF(InImage,InImage,
           Sigma_s,Sigma_r,
           1,
           &FilteredImage,&FilteredImage);

#pragma omp parallel for default(shared) schedule(static) private(hIdx)
    for (uint16_t Row=0; Row<m_Height; Row++) {
      hIdx = Row*Width;
      for (uint16_t Col=0; Col<m_Width; Col++) {
        m_Image[hIdx+Col][Channel] = CLIP((int32_t)(FilteredImage(Col,Row)*0xffff));
      }
    }
  }

  return this;
}
