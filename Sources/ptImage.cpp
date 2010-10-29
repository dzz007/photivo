////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender
// Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

// std stuff needs to be declared apparently for jpeglib
// which seems a bug in the jpeglib header ?
#include <cstdlib>
#include <cstdio>
#include <QMessageBox>

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
  #define ptraw_saved_cplusplus __cplusplus
  #undef __cplusplus
  extern "C" {
  #include <jpeglib.h>
  }
  #define __cplusplus ptraw_saved_cplusplus
#else
  #include <jpeglib.h>
#endif
#endif

#include "ptConstants.h"
#include "ptError.h"
#include "ptImage.h"
#include "ptResizeFilters.h"
#include "ptCurve.h"
#include "ptKernel.h"
#include "ptConstants.h"
#include "ptRefocusMatrix.h"
#include "ptCimg.h"
#include "ptFastBilateral.h"
//~ #include "ptImageMagick.h"
//~ #include "ptImageMagickC.h"

extern cmsCIExyY       D65;
extern cmsCIExyY       D50;

extern cmsHPROFILE PreviewColorProfile;

// Lut
extern float ToFloatTable[0x10000];

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
    QMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
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

  cmsDoTransform(Transform,m_Image,m_Image,m_Width*m_Height);
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
                               const int   Intent){

  // assert ((m_ColorSpace>0) && (m_ColorSpace<5));
  if (!((m_ColorSpace>0) && (m_ColorSpace<5))) {
    QMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
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
    uint16_t* Image = &m_Image[i][0];
    cmsDoTransform(Transform,Image,Image,Length);
  }

  cmsDeleteTransform(Transform);
  cmsCloseProfile(InProfile);

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

  cmsDoTransform(Transform,m_Image,m_Image,m_Width*m_Height);
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

  cmsDoTransform(Transform,m_Image,m_Image,m_Width*m_Height);
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
    m_Image[i][0] = CLIP(0xffff*(116.0/100.0 * xyz[1] - 16.0/100.0));
    m_Image[i][1] = CLIP(0x101*(128.0+500.0*(xyz[0]-xyz[1])));
    m_Image[i][2] = CLIP(0x101*(128.0+200.0*(xyz[1]-xyz[2])));

  }

  // And that's it.
  m_ColorSpace = ptSpace_Lab;

  return this;
};

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

  cmsDoTransform(Transform,m_Image,m_Image,m_Width*m_Height);
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  // And that's it.
  m_ColorSpace = ptSpace_Lab;

  return this;
};

////////////////////////////////////////////////////////////////////////////////
//
// LabToRGB
// Conversion from Lab space to RGB space.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LabToRGB(const short To) {

  if (!(m_ColorSpace == ptSpace_Lab)) {
    QMessageBox::critical(0,"Error","Too fast! Keep cool ;-)");
    return this;
  }

  assert (3 == m_Colors);
  assert ((To>0) && (To<5));
  //~ assert (m_ColorSpace == ptSpace_Lab);

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
};

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

  cmsDoTransform(Transform,m_Image,m_Image,m_Width*m_Height);
  cmsDeleteTransform(Transform);
  cmsCloseProfile(OutProfile);
  cmsCloseProfile(InProfile);

  // And that's it.

  m_ColorSpace = To;

  return this;
};

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage::ptImage(void (*UpdateGUI)()) {
  m_Width              = 0;
  m_Height             = 0;
  m_Image              = NULL;
  m_Colors             = 0;
  m_ColorSpace         = ptSpace_sRGB_D65;

  // Workaround to keep the GUI responsive.
  // Call with m_UpdateGUI();
  m_UpdateGUI = UpdateGUI;

  // Initialize the lookup table for the RGB->LAB function
  // if this would be the first time.
  if (!ToLABFunctionInited) {
    // Remark : we extend the table well beyond r>1.0 for numerical
    // stability purposes. XYZ>1.0 occurs sometimes and this way
    // it stays stable (srgb->lab->srgb correct within 0.02%)
    for (uint32_t i=0; i<0x20000; i++) {
      double r = (double)(i) / 0xffff;
      ToLABFunctionTable[i] = r > 216.0/24389.0 ? pow(r,1/3.0) : (24389.0/27.0*r + 16.0)/116.0;
    }
  ToLABFunctionInited = 1;
  }

  // Some lcsm initialization.
  //cmsErrorAction (LCMS_ERROR_SHOW);
  cmsWhitePointFromTemp(&D65, 6503);
  cmsWhitePointFromTemp(&D50, 5003);
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptImage::~ptImage() {
  FREE(m_Image);
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const DcRaw*  DcRawObject,
                      const short   TargetSpace,
                      const char*   ProfileName,
                      const int     Intent,
                      const int     ProfileGamma) {

  assert(NULL != DcRawObject);
  assert ((TargetSpace>0) && (TargetSpace<5));

  m_Width  = DcRawObject->m_Width;
  m_Height = DcRawObject->m_Height;

  // Free a maybe preexisting and allocate space.
  FREE(m_Image);
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

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
        m_Image[i][c] = (uint16_t) CLIP(Value[c]);
      }
    }

    m_Colors = MIN(DcRawObject->m_Colors,3);
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
  } // (if !ProfileName)

  if (ProfileName) {

    m_Colors = MIN(DcRawObject->m_Colors,3);
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

    cmsDoTransform(Transform,
                   DcRawObject->m_Image,
                   m_Image,
                   m_Width*m_Height);
    cmsDeleteTransform(Transform);
    for (; ProfileIdx>=0; ProfileIdx--) {
      cmsCloseProfile(Profiles[ProfileIdx]);
    }
  }

  // Flip image. With m_Flip as in dcraw.
  // (see also flip_index() function in dcraw)
  uint16_t (*ImageFlipped)[3];
  ImageFlipped =(uint16_t(*)[3])CALLOC(m_Width*m_Height,sizeof(*ImageFlipped));
  ptMemoryError(ImageFlipped,__FILE__,__LINE__);

  uint16_t TargetWidth  = m_Width;
  uint16_t TargetHeight = m_Height;
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
        ImageFlipped[TargetRow*TargetWidth+TargetCol][c] =
          m_Image[OriginRow*m_Width+OriginCol][c];
      }
    }
  }

  m_Height = TargetHeight;
  m_Width  = TargetWidth;

  FREE(m_Image);
  m_Image = ImageFlipped;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set
// To DcRaw's ImageAfterPhase2. (ad hoc, look at 'sensor' values)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const DcRaw*  DcRawObject,
                      const short   TargetSpace) {

  assert(NULL != DcRawObject);
  assert ((TargetSpace>0) && (TargetSpace<5));

  m_Width  = DcRawObject->m_Width;
  m_Height = DcRawObject->m_Height;
  m_ColorSpace = TargetSpace;

  // Free a maybe preexisting and allocate space.
  FREE(m_Image);
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  // Convert the image.
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)m_Height*m_Width; i++) {
    for (short c=0; c<3; c++) {
      m_Image[i][c] = DcRawObject->m_Image_AfterPhase2[i][c];
    }
  }

  m_Colors = MIN(DcRawObject->m_Colors,3);

  if (m_Colors != 3) {
    fprintf(stderr,
            "At this moment anything with m_Colors != 3 was not yet "
            "tested sufficiently. You can remove the assertion related "
            "to this, but unexpected issues can pop up. Consider submitting "
            "a picture from this camera to the author in order to test "
            "further.\n");
    assert(0);
  }

  // Flip image. With m_Flip as in dcraw.
  // (see also flip_index() function in dcraw)
  uint16_t (*ImageFlipped)[3];
  ImageFlipped =(uint16_t(*)[3])CALLOC(m_Width*m_Height,sizeof(*ImageFlipped));
  ptMemoryError(ImageFlipped,__FILE__,__LINE__);

  uint16_t TargetWidth  = m_Width;
  uint16_t TargetHeight = m_Height;
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
        ImageFlipped[TargetRow*TargetWidth+TargetCol][c] =
          m_Image[OriginRow*m_Width+OriginCol][c];
      }
    }
  }

  m_Height = TargetHeight;
  m_Width  = TargetWidth;

  FREE(m_Image);
  m_Image = ImageFlipped;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set from drop (dirty quick debug function : read a dump to file)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Set(const uint16_t Width,
                      const uint16_t Height,
                      const short    NrColors,
                      const short    NrBytesPerColor,
                      const char*    FileName) {

  m_Width  = Width;
  m_Height = Height;

  // Free a maybe preexisting and allocate space.
  FREE(m_Image);
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  uint16_t (*Buffer)[NrColors] =
    (uint16_t (*)[NrColors])CALLOC(Width*Height,NrBytesPerColor*NrColors);
  FILE *InputFile = fopen(FileName,"rb");
  if (!InputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return NULL;
  }
  if ((size_t)(Width*Height) !=
       fread(Buffer,NrBytesPerColor*NrColors,Width*Height,InputFile)) {
    ptLogError(ptError_FileOpen,FileName);
    return NULL;
  }
  FCLOSE(InputFile);
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t)Height*Width; i++) {
    for (short c=0; c<3; c++) {
      m_Image[i][c] = Buffer[i][c];
      //printf("DEBUG : Buffer[i][c] : %d\n",Buffer[i][c]);
      if (NrBytesPerColor == 1) m_Image[i][c] = m_Image[i][c] << 1;
    }
  }

  FREE(Buffer);

  m_Colors = 3;
  m_ColorSpace = ptSpace_sRGB_D65;

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
  FREE(m_Image);
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

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

  m_Width              = Origin->m_Width;
  m_Height             = Origin->m_Height;
  m_Colors             = Origin->m_Colors;
  m_ColorSpace         = Origin->m_ColorSpace;

  // And a deep copying of the image.
  // Free a maybe preexisting.
  FREE(m_Image);
  // Allocate new.
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);
  memcpy(m_Image,Origin->m_Image,m_Width*m_Height*sizeof(*m_Image));
  return this;
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
                         const short  ExposureClipMode) {

  assert (m_Colors == 3);
  assert (m_ColorSpace != ptSpace_XYZ);
  assert (Exposure < (2<<15));
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

#pragma omp parallel for
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
      if (ExposureClipMode == ptExposureClipMode_None) {
        for (short Color=0; Color<NrChannels; Color++) {
          m_Image[i][Color] = MIN(0XFFFF,Pixel[Color]);
        }
      } else if (ExposureClipMode == ptExposureClipMode_Ratio) {
        for (short Color=0; Color<NrChannels; Color++) {
          m_Image[i][Color] = MIN(0XFFFF,
                               (uint32_t)(Pixel[Color]*(double)0xFFFF/Highest));
        }
      } else {
        assert(0);
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
  for (uint32_t i=0; i<(uint32_t)(m_Height*m_Width); i++) {
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
#pragma omp parallel for default(shared)
  for (uint32_t i=0; i< (uint32_t)m_Height*m_Width; i++) {
    if (ChannelMask & 1) m_Image[i][0] = Curve->m_Curve[ m_Image[i][0] ];
    if (ChannelMask & 2) m_Image[i][1] = Curve->m_Curve[ m_Image[i][1] ];
    if (ChannelMask & 4) m_Image[i][2] = Curve->m_Curve[ m_Image[i][2] ];
  }

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

  float ValueA = 0.0;
  float ValueB = 0.0;

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

    float Factor = Curve->m_Curve[CLIP((int32_t)(Hue/ptPI*WPH))]/(float)0x7fff - 1.0;
    if (Factor == 0.0) continue;

    float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.25) / (float) 0xb5;
    Factor = powf(2,3*Factor*Col);

    m_Image[i][0] = CLIP((int32_t)(m_Image[i][0] * Factor));
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply Saturation Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplySaturationCurve(const ptCurve *Curve,
                                       const short Mode,
                                       const short Type) {

// Best solution would be to use the Lab <-> Lch conversion from lcms.
// This should be faster without sacrificing much quality.

  assert (m_ColorSpace == ptSpace_Lab);
  // neutral value for a* and b* channel
  const float WPH = 0x8080;

  float ValueA = 0.0;
  float ValueB = 0.0;

  if (Type == 0) { // by luma
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

      float Factor = Curve->m_Curve[CLIP((int32_t)(Hue/ptPI*WPH))]/(float)0x7fff;
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
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
  } else { // by chroma
#pragma omp parallel for schedule(static) private(ValueA, ValueB)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by luminance
      float Factor = Curve->m_Curve[m_Image[i][0]]/(float)0x7fff;
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
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1] * m + WPH * (1. - m)));
      m_Image[i][2] = CLIP((int32_t)(m_Image[i][2] * m + WPH * (1. - m)));
    }
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Apply Texture Curve
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ApplyTextureCurve(const ptCurve *Curve,
                           const short Type,
                           const short Scaling) {

  const double Threshold = 10.0/pow(2,Scaling);
  const double Softness = 0.01;
  //~ const double Amount -> curve
  const double Opacity = 1.0;
  const double EdgeControl = 1.0;

  assert (m_ColorSpace == ptSpace_Lab);
  // neutral value for a* and b* channel
  const float WPH = 0x8080;

  float ValueA = 0.0;
  float ValueB = 0.0;
  float m = 0.0;

  const short ChannelMask = 1;

  ptImage *ContrastLayer = new ptImage;
  ContrastLayer->Set(this);

  ptFastBilateralChannel(ContrastLayer, Threshold, Softness, 2, ChannelMask);

  if (Type == 0) { // by chroma
#pragma omp parallel for schedule(static) private(ValueA, ValueB, m)
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

      float Col = powf(ValueA * ValueA + ValueB * ValueB, 0.125);
      Col /= 0x7; // normalizing to 0..2

      float Factor = Curve->m_Curve[CLIP((int32_t)(Hue/ptPI*WPH))]/(float)0x3fff - 1.0;
      //~ m = powf(3.0,fabs(Factor) * Col);
      m = 20.0 * Factor * Col;
      float Scaling = 1.0/(1.0+exp(-0.5*m))-1.0/(1.0+exp(0.5*m));
      float Offset = -1.0/(1.0+exp(0.5*m));

      //~ for (short Ch=0; Ch<NrChannels; Ch++) {
        ContrastLayer->m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
        if (Factor < 0) ContrastLayer->m_Image[i][0] = 0xffff-ContrastLayer->m_Image[i][0];
        if (fabsf(Factor*Col)<0.1) continue;
        ContrastLayer->m_Image[i][0] = CLIP((int32_t)((((1.0/(1.0+
          exp(m*(0.5-(float)ContrastLayer->m_Image[i][0]/(float)0xffff))))+Offset)/Scaling)*0xffff));

        // instead of sigmoidal contrast, faster!
        //~ ContrastLayer->m_Image[i][0] = CLIP((int32_t)((ContrastLayer->m_Image[i][0]-0x7fff)*m+0x7fff));
      //~ }
    }
  } else { // by luma
#pragma omp parallel for schedule(static) private(ValueA, ValueB, m)
    for(uint32_t i = 0; i < (uint32_t) m_Width*m_Height; i++) {
      // Factor by luminance
      float Factor = Curve->m_Curve[m_Image[i][0]]/(float)0x3fff - 1.0;
      //~ m = powf(3.0,fabs(Factor));
      m = 20.0 * Factor;
      float Scaling = 1.0/(1.0+exp(-0.5*m))-1.0/(1.0+exp(0.5*m));
      float Offset = -1.0/(1.0+exp(0.5*m));

      //~ for (short Ch=0; Ch<NrChannels; Ch++) {
        ContrastLayer->m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
        if (Factor < 0) ContrastLayer->m_Image[i][0] = 0xffff-ContrastLayer->m_Image[i][0];
        if (fabsf(Factor)<0.1) continue;
        ContrastLayer->m_Image[i][0] = CLIP((int32_t)((((1.0/(1.0+
          exp(m*(0.5-(float)ContrastLayer->m_Image[i][0]/(float)0xffff))))+Offset)/Scaling)*0xffff));
        // instead of sigmoidal contrast, faster!
        //~ ContrastLayer->m_Image[i][0] = CLIP((int32_t)((ContrastLayer->m_Image[i][0]-0x7fff)*m+0x7fff));
      //~ }
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

  float Scaling = 1.0/(1.0+exp(-0.5*Contrast))-1.0/(1.0+exp(0.5*Contrast));
  float Offset = -1.0/(1.0+exp(0.5*Contrast));
  float logtf = -logf(Threshold)/logf(2.0);
  float logft = -logf(2.0)/logf(Threshold);

  uint16_t ContrastTable[0x10000];
  ContrastTable[0] = 0;
  if (Contrast > 0)
#pragma omp parallel for
    for (uint32_t i=1; i<0x10000; i++) {
      ContrastTable[i] = CLIP((int32_t)(powf((((1.0/(1.0+
        exp(Contrast*(0.5-powf((float)i/(float)0xffff,logft)))))+Offset)/Scaling),logtf)*0xffff));
    }
  else
#pragma omp parallel for
    for (uint32_t i=1; i<0x10000; i++) {
      ContrastTable[i] = CLIP((int32_t)(powf(0.5-1.0/Contrast*
        logf(1.0/(Scaling*powf((float)i/(float)0xffff,logft)-Offset)-1.0),logtf)*0xffff));
    }

// powf(0.5-1.0/c*log(1.0/(S*powf(in[(col+row*width)*3+ch],logft)-O)-1.0),logtf);
#pragma omp parallel for default(shared)
  for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
    if (ChannelMask & 1) m_Image[i][0] = ContrastTable[ m_Image[i][0] ];
    if (ChannelMask & 2) m_Image[i][1] = ContrastTable[ m_Image[i][1] ];
    if (ChannelMask & 4) m_Image[i][2] = ContrastTable[ m_Image[i][2] ];
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// SimpleResize
// A simple bilinear resize. Does not upscale. No resampling filter.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::SimpleResize(const uint16_t Size,
                               const short    InPlace) {

  assert(m_Colors ==3);

  uint16_t   Multiplier = Size;
  uint16_t   Divider = MAX(m_Height,m_Width);
  uint32_t   Normalizer = Divider * Divider;

  if (Multiplier > Divider) {
    ptLogError(ptError_Argument,"Size : %d is too large",Size);
    return NULL;
  }

  uint16_t NewHeight = m_Height * Multiplier / Divider;
  uint16_t NewWidth  = m_Width  * Multiplier / Divider;

  uint64_t (*Image64Bit)[3] =
    (uint64_t (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*Image64Bit));
  ptMemoryError(Image64Bit,__FILE__,__LINE__);

  for(uint16_t r=0; r<m_Height; r++) {
    /* r should be divided between ri and rii */
    uint16_t ri  = r * Multiplier / Divider;
    uint16_t rii = (r+1) * Multiplier / Divider;
    /* with weights riw and riiw (riw+riiw==Multiplier) */
    int64_t riw  = rii * Divider - r * Multiplier;
    int64_t riiw = (r+1) * Multiplier - rii * Divider;
    if (rii>=NewHeight) {
      rii  = NewHeight-1;
      riiw = 0;
    }
    if (ri>=NewHeight) {
      ri  = NewHeight-1;
      riw = 0;
    }
    for(uint16_t c=0; c<m_Width; c++) {
      uint16_t ci   = c * Multiplier / Divider;
      uint16_t cii  = (c+1) * Multiplier / Divider;
      int64_t ciw  = cii * Divider - c * Multiplier;
      int64_t ciiw = (c+1) * Multiplier - cii * Divider;
      if (cii>=NewWidth) {
        cii  = NewWidth-1;
        ciiw = 0;
      }
      if (ci>=NewWidth) {
        ci  = NewWidth-1;
        ciw = 0;
      }
      for (short cl=0; cl<3; cl++) {
        Image64Bit[ri *NewWidth+ci ][cl] += m_Image[r*m_Width+c][cl]*riw *ciw ;
        Image64Bit[ri *NewWidth+cii][cl] += m_Image[r*m_Width+c][cl]*riw *ciiw;
        Image64Bit[rii*NewWidth+ci ][cl] += m_Image[r*m_Width+c][cl]*riiw*ciw ;
        Image64Bit[rii*NewWidth+cii][cl] += m_Image[r*m_Width+c][cl]*riiw*ciiw;
      }
    }
  }

  // The image worked finally upon is 'this' or a new created one.
  ptImage* WorkImage = InPlace ? this : new (ptImage);

  if (InPlace) {
    FREE(m_Image); // free the old image.
  }

  WorkImage->m_Image =
    (uint16_t (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*m_Image));
  ptMemoryError(WorkImage->m_Image,__FILE__,__LINE__);

  // Fill the image from the Image64Bit.
  for (uint32_t c=0; c<(uint32_t)NewHeight*NewWidth; c++) {
    for (short cl=0; cl<3; cl++) {
      WorkImage->m_Image[c][cl] = Image64Bit[c][cl]/Normalizer;
    }
  }

  FREE(Image64Bit);

  WorkImage->m_Width  = NewWidth;
  WorkImage->m_Height = NewHeight;
  WorkImage->m_Colors = m_Colors;

  return WorkImage;
}

////////////////////////////////////////////////////////////////////////////////
//
// FilteredResize
// Resize with a resampling filter.
//
////////////////////////////////////////////////////////////////////////////////

// TODO
// Note : I have tried this in all possible combinations with
// integer types, in which case 64 bit needed for image, or with
// combinations of integer images and floating weights but nothing
// comes close to the float performance.
// Someone else ?
//
// Also this implementation has been tweaked by taking as much
// as possible calculations (especially * and /) from the inner loops.

ptImage* ptImage::FilteredResize(const uint16_t Size,
                                 const short    ResizeFilter,
                                 const short    InPlace) {

  assert(m_Colors ==3);

  const float Lobes = FilterLobes[ResizeFilter];

  if (not FilterTableInited[ResizeFilter]) {
    FilterTableSize[ResizeFilter] = (uint16_t) (Lobes*SamplesPerLobe+1);
    FilterTable[ResizeFilter] =
      (float*) CALLOC(FilterTableSize[ResizeFilter],sizeof(*FilterTable));
    ptMemoryError(FilterTable,__FILE__,__LINE__);
    for (uint16_t i=0; i<FilterTableSize[ResizeFilter]; i++) {
      float x = (float) i / SamplesPerLobe;
      FilterTable[ResizeFilter][i] = (*FilterFunction[ResizeFilter])(x);
    }
    FilterTableInited[ResizeFilter] = 1;
  }

  float*      Table = FilterTable[ResizeFilter];

  //printf("(%s,%d) ResizeFilter : %d Table : %p Lobes:%f\n",
  //       __FILE__,__LINE__,ResizeFilter,Table,Lobes);

  // Some precalculations based on the resizing factor
  // MIN/MAX corrections are for upsampling.
  float    Ratio     = (float) MAX(m_Width,m_Height)/Size;
  uint16_t NewHeight = (uint16_t)(m_Height / Ratio+0.5);
  uint16_t NewWidth  = (uint16_t)(m_Width / Ratio+0.5);
  float    Scale      = MIN(1.0,1.0/Ratio);
  float    Radius     = MAX(Lobes* Ratio,Lobes);
  int32_t  ScaledLobe = (int32_t)(SamplesPerLobe*Scale);

  // X Size change

  // Be aware, height still that of the original image in this pass!
  float (*DstImageX)[3] =
    (float (*)[3]) CALLOC(NewWidth*m_Height,sizeof(*DstImageX));
  ptMemoryError(DstImageX,__FILE__,__LINE__);

  for (uint16_t OrgRow=0; OrgRow<m_Height; OrgRow++) {
    for (uint16_t DstCol=0; DstCol<NewWidth; DstCol++) {
      uint32_t DstPointer = OrgRow*NewWidth+DstCol;
      float    OrgCenter  = (DstCol+0.5) * Ratio; // Checked.OK.
      int32_t  OrgLeft    = (int32_t)(OrgCenter-Radius);
      int32_t  OrgRight   = (int32_t)(OrgCenter+Radius);
      float    SumWeight = 0;
      float    x = (OrgCenter-OrgLeft-0.5)*Scale; // TODO -0.5 correct ?
      int32_t  idx = (int32_t)(x*SamplesPerLobe);
      for (int32_t i=OrgLeft; i<=OrgRight; i++,x-=Scale,idx-=ScaledLobe) {
        if (i<0 || i>=m_Width) continue;
        if (fabs(x) <= Lobes) {
          float Weight = Table[abs(idx)];
          SumWeight+=Weight;
          uint32_t OrgPointer = OrgRow*m_Width+i;
          DstImageX[DstPointer][0] += m_Image[OrgPointer][0] * Weight;
          DstImageX[DstPointer][1] += m_Image[OrgPointer][1] * Weight;
          DstImageX[DstPointer][2] += m_Image[OrgPointer][2] * Weight;
        }
      }
      // One division & three multiplications is faster then three divisions
      SumWeight = 1/SumWeight;
      DstImageX[DstPointer][0] *= SumWeight;
      DstImageX[DstPointer][1] *= SumWeight;
      DstImageX[DstPointer][2] *= SumWeight;
    }
  }

  // At this stage we can free memory of the original image to
  // reduce a bit the memory demand.
  FREE(m_Image);

  // Y Size reduction

  float (*DstImageY)[3] =
    (float (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*DstImageY));
  ptMemoryError(DstImageY,__FILE__,__LINE__);

  // NewWidth as the X direction is resized already.
  for (uint16_t OrgCol=0; OrgCol<NewWidth; OrgCol++) {
    for (uint16_t DstRow=0; DstRow<NewHeight; DstRow++) {
      uint32_t DstPointer = DstRow*NewWidth+OrgCol;
      float    OrgCenter  = (DstRow+0.5)*Ratio; // Checked. OK.
      int32_t  OrgLeft    = (int32_t)(OrgCenter-Radius);
      int32_t  OrgRight   = (int32_t)(OrgCenter+Radius);
      float    SumWeight = 0;
      float    x   = (OrgCenter-OrgLeft-0.5)*Scale; // TODO -0.5 correct ?
      int32_t  idx = (int32_t)(x*SamplesPerLobe);
      for (int32_t i=OrgLeft; i<=OrgRight; i++,x-=Scale,idx-=ScaledLobe) {
        if (i<0 || i>=m_Height) continue;
        if (fabs(x) <= Lobes) {
          float Weight = Table[abs(idx)];
          SumWeight+=Weight;
          uint32_t OrgPointer = i*NewWidth+OrgCol;
          DstImageY[DstPointer][0] += DstImageX[OrgPointer][0]*Weight;
          DstImageY[DstPointer][1] += DstImageX[OrgPointer][1]*Weight;
          DstImageY[DstPointer][2] += DstImageX[OrgPointer][2]*Weight;
        }
      }
      // One division & three multiplications is faster then three divisions
      SumWeight = 1/SumWeight;
      DstImageY[DstPointer][0] *= SumWeight;
      DstImageY[DstPointer][1] *= SumWeight;
      DstImageY[DstPointer][2] *= SumWeight;
    }
  }

  // At this stage we can free memory of the DstImageX image to
  // reduce a bit the memory demand.
  FREE(DstImageX);

  // The image worked finally upon is 'this' or a new created one.
  ptImage* WorkImage = InPlace ? this : new (ptImage);

  WorkImage->m_Image =
    (uint16_t (*)[3]) CALLOC(NewWidth*NewHeight,sizeof(*m_Image));
  ptMemoryError(WorkImage->m_Image,__FILE__,__LINE__);

  // Fill the image from the DstImage.
  for (uint32_t c=0; c<(uint32_t)NewHeight*NewWidth; c++) {
    for (short cl=0; cl<3; cl++) {
      WorkImage->m_Image[c][cl] = (uint16_t)CLIP(DstImageY[c][cl]+0.5);
    }
  }

  FREE(DstImageY);

  WorkImage->m_Width  = NewWidth;
  WorkImage->m_Height = NewHeight;
  WorkImage->m_Colors = m_Colors;

  return WorkImage;
}

////////////////////////////////////////////////////////////////////////////////
//
// Crop
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Crop(const uint16_t X,
                       const uint16_t Y,
                       const uint16_t W,
                       const uint16_t H,
                       const short    InPlace) {

  assert(m_Colors ==3);
  assert( (X+W) <= m_Width);
  assert( (Y+H) <= m_Height);

  uint16_t (*CroppedImage)[3] =
    (uint16_t (*)[3]) CALLOC(W*H,sizeof(*m_Image));
  ptMemoryError(CroppedImage,__FILE__,__LINE__);

#pragma omp parallel for
  for (uint16_t Row=0;Row<H;Row++) {
    for (uint16_t Column=0;Column<W;Column++) {
      CroppedImage[Row*W+Column][0] = m_Image[(Y+Row)*m_Width+X+Column][0];
      CroppedImage[Row*W+Column][1] = m_Image[(Y+Row)*m_Width+X+Column][1];
      CroppedImage[Row*W+Column][2] = m_Image[(Y+Row)*m_Width+X+Column][2];
    }
  }

  // The image worked finally upon is 'this' or a new created one.
  ptImage* WorkImage = InPlace ? this : new (ptImage);

  if (InPlace) {
    FREE(m_Image); // FREE the old image.
  }

  WorkImage->m_Image  = CroppedImage;
  WorkImage->m_Width  = W;
  WorkImage->m_Height = H;
  WorkImage->m_Colors = m_Colors;
    return WorkImage;
}

////////////////////////////////////////////////////////////////////////////////
//
// Overlay
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Overlay(const uint16_t (*OverlayImage)[3],
                          const double   Amount,
                          const float *Mask,
                          const short Mode /* SoftLight */,
                          const short Swap /* = 0 */) {

  const float WP = 0xffff;
  const float WPH = 0x7fff;
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;
  float Multiply = 0;
  float Screen = 0;
  float Overlay = 0;
  float Source = 0;
  float Blend = 0;
  float Temp = 0;

  for (short Ch=0; Ch<3; Ch++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Ch))) continue;
#pragma omp parallel for default(shared) private(Source, Blend, Multiply, Screen, Overlay, Temp)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      if (!Swap) {
        Source   = m_Image[i][Ch];
        Blend    = OverlayImage[i][Ch];
      } else {
        Blend    = m_Image[i][Ch];
        Source   = OverlayImage[i][Ch];
      }
      switch (Mode) {
        case ptOverlayMode_None: // just for completeness
          break;

        case ptOverlayMode_SoftLight:
        Multiply = CLIP((int32_t)(Source*Blend/WP));
        Screen   = CLIP((int32_t)(WP-(WP-Source)*(WP-Blend)/WP));
        Overlay  = CLIP((int32_t)((((WP-Source)*Multiply+Source*Screen)/WP)));
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Overlay*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Overlay*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

        case ptOverlayMode_Multiply:
        Multiply = CLIP((int32_t)(Source*Blend/WP));
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Multiply*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Multiply*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_Screen:
        Screen = CLIP((int32_t)(WP-(WP-Source)*(WP-Blend)/WP));
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Screen*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Screen*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_Normal:
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Blend*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Blend*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_Lighten:
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (MAX(Blend*Amount, Source)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)(MAX(Blend*Mask[i]+Source*(1-Mask[i])*Amount,Source)));
        }
        break;

      case ptOverlayMode_Overlay:
        if (Source <= WPH) {
        Overlay = CLIP((int32_t)(Source*Blend/WP));
        } else {
        Overlay = CLIP((int32_t)(WP-(WP-Source)*(WP-Blend)/WP));
        }
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Overlay*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Overlay*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_GrainMerge:
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) ((Blend+Source-WPH)*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)(((Blend+Source-WPH)*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_ColorDodge: // a/(1-b)
        if (Source == 0) Temp = 0;
        else {
          if (Blend == WP) Temp = WP;
          else Temp = CLIP((int32_t)(Source / (1 - Blend/WP)));
        }
        if (!Mask) {
          m_Image[i][Ch] = CLIP((int32_t) (Temp*Amount+Source*(1-Amount)));
        } else {
          m_Image[i][Ch] = CLIP((int32_t)((Temp*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_ColorBurn: // 1-(1-a)/b
        if (Source == WP) Temp = WP;
        else {
          if (Blend == 0) Temp = 0;
          else Temp = WP - CLIP((int32_t)( (WP - Source) / (Blend/WP)));
        }
        if (!Mask) {
        m_Image[i][Ch] = CLIP((int32_t) (Temp*Amount+Source*(1-Amount)));
        } else {
        m_Image[i][Ch] = CLIP((int32_t)((Temp*Mask[i]+Source*(1-Mask[i]))*Amount+Source*(1-Amount)));
        }
        break;

      case ptOverlayMode_ShowMask:
        if (Mask) {
        m_Image[i][0] = CLIP((int32_t) (Mask[i]*WP));
        m_Image[i][1] = CLIP((int32_t) (Mask[i]*WP));
        m_Image[i][2] = CLIP((int32_t) (Mask[i]*WP));
        }
        break;

      case ptOverlayMode_Replace: // Replace, just for testing
        m_Image[i][Ch] = CLIP((int32_t) Blend);
        break;

      }
    }
  }
  return this;
}

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

ptImage* ptImage::Levels(const double BlackPoint,
                         const double WhitePoint) {

  const double WP = 0xffff;
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

  if (fabs(BlackPoint-WhitePoint)>0.001) {
    double m = 1.0/(WhitePoint-BlackPoint);
    double t = -BlackPoint/(WhitePoint-BlackPoint)*WP;
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
// Reinhard 05 tone mapping operator
// Adapted from PFSTMO package, original Copyright (C) 2007 Grzegorz Krawczyk
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Reinhard05(const double Brightness,
                             const double Chromatic,
                             const double Light) {

  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

  const double DChromatic = 1 - Chromatic;
  const double DLight = 1 - Light;

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
      Y[i] =(0.3*m_Image[i][0] + 0.59*m_Image[i][1] + 0.11*m_Image[i][2]) /
             (float) 0xffff;
    }

  float max_lum = 0.0;
  float min_lum = 0.0;
  float world_lum = 0.0;
  float Cav[] = { 0.0f, 0.0f, 0.0f};
  float Lav = 0.0f;

  for (uint32_t i=0; i < m_Size; i++) {
    float lum = Y[i];
    max_lum = (max_lum > lum) ? max_lum : lum;
    min_lum = (min_lum < lum) ? min_lum : lum;
    world_lum += logf(2.3e-5+lum);
    for (int c = 0; c < NrChannels; c++)
      Cav[c] += m_Image[i][c];
    Lav += lum;
  }
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

        max_col = (col>max_col) ? col : max_col;
        min_col = (col<min_col) ? col : min_col;

        Temp[i][c]=col;
      }
    }
  }

  //--- normalize intensities
  for (uint32_t i=0; i < m_Size; i++) {
    for (int c = 0; c < NrChannels; c++)
      m_Image[i][c] = CLIP((int32_t) ((Temp[i][c]-min_col)/(max_col-min_col)*0xffff));
  }

  free (Temp);
  free (Y);

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
  if (ValueA!=1) {
#pragma omp parallel for
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][1] = CLIP((int32_t)(m_Image[i][1]*ValueA+t1));
    }
  }
  if (ValueB!=1) {
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

ptImage* ptImage::LAdjust(const double LC1, // 8 colors for L
                          const double LC2,
                          const double LC3,
                          const double LC4,
                          const double LC5,
                          const double LC6,
                          const double LC7,
                          const double LC8,
                          const double SC1, // 8 colors for saturation
                          const double SC2,
                          const double SC3,
                          const double SC4,
                          const double SC5,
                          const double SC6,
                          const double SC7,
                          const double SC8) {

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
// Color Enhance
// http://docs.google.com/View?id=dsgjq79_829f9wv8ncd
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ColorEnhance(const double Shadows,
                               const double Highlights) {

  assert (m_ColorSpace != ptSpace_Lab);
  uint16_t WP = 0xffff;

  if (Shadows) {
    ptImage *ShadowsLayer = new ptImage;
    ShadowsLayer->Set(this);

    // Invert and greyscale
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      ShadowsLayer->m_Image[i][0] = WP - CLIP((int32_t) (0.3*ShadowsLayer->m_Image[i][0]+
  0.59*ShadowsLayer->m_Image[i][1]+0.11*ShadowsLayer->m_Image[i][2]));
      ShadowsLayer->m_Image[i][1] = ShadowsLayer->m_Image[i][2] =
        ShadowsLayer->m_Image[i][0];
    }

    ShadowsLayer->Overlay(m_Image, 1.0, NULL, ptOverlayMode_ColorDodge, 1 /*Swap */);
    Overlay(ShadowsLayer->m_Image, Shadows, NULL, ptOverlayMode_ColorBurn);
    delete ShadowsLayer;
  }
  // I trade processing time for memory, so invert and greyscale will be
  // recalculated to save another parallel memory instance

  if (Highlights) {
    ptImage *HighlightsLayer = new ptImage;
    HighlightsLayer->Set(this);

    // Invert and greyscale
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      HighlightsLayer->m_Image[i][0] = WP - CLIP((int32_t) (0.3*HighlightsLayer->m_Image[i][0]+
  0.59*HighlightsLayer->m_Image[i][1]+0.11*HighlightsLayer->m_Image[i][2]));
      HighlightsLayer->m_Image[i][1] = HighlightsLayer->m_Image[i][2] =
        HighlightsLayer->m_Image[i][0];
    }

    HighlightsLayer->Overlay(m_Image, 1.0, NULL, ptOverlayMode_ColorBurn, 1 /*Swap */);
    Overlay(HighlightsLayer->m_Image, Highlights, NULL, ptOverlayMode_ColorDodge);
    delete HighlightsLayer;
  }
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// LMHLightRecovery
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::LMHLightRecovery(const short   MaskType,
                                   const double  Amount,
                                   const double  LowerLimit,
                                   const double  UpperLimit,
                                   const double  Softness) {

  const double WP = 0xffff;

  const double ExposureFactor = pow(2,Amount);
  const double InverseExposureFactor = 1/ExposureFactor;

  // Precalculated table for the transform of the original.
  // The transform is an exposure (>1) or a gamma driven darkening.
  // (Table generates less math except for images with < 20K pixels)
  uint16_t TransformTable[0x10000];
#pragma omp parallel for
  for (uint32_t i=0; i<0x10000; i++) {
    if (ExposureFactor<1.0) {
      TransformTable[i] = CLIP((int32_t)(pow(i/WP,InverseExposureFactor)*WP));
    } else {
      TransformTable[i] = CLIP((int32_t)(i*ExposureFactor+0.5));
    }
  }

  const double Soft = pow(2,Softness);

  // Precalculated table for softening the mask.
  double SoftTable[0x100]; // Assuming a 256 table is fine grained enough.
  for (int16_t i=0; i<0x100; i++) {
    if (Soft>1.0) {
      SoftTable[i] = LIM(pow(i/(double)0xff,Soft),0.0,1.0);
    } else {
      SoftTable[i] = LIM(i/(double)0xff/Soft,0.0,1.0);
    }
  }

  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;

  const double ReciprocalRange       = 1.0/MAX(UpperLimit-LowerLimit,0.001);
  const double ReciprocalLowerLimit  = 1.0/MAX(LowerLimit,0.001);
  const double ReciprocalUpperMargin = 1.0/MAX(1.0-UpperLimit,0.001);
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    // Init Mask with luminance
    double Mask = (m_ColorSpace == ptSpace_Lab) ?
      m_Image[i][0]/WP :
      // Remark this classic 30/59/11 should be in fact colour space
      // dependent. TODO
      (m_Image[i][0]*0.30+m_Image[i][1]*0.59+m_Image[i][2]*0.11)/WP;
    switch(MaskType) {
      case ptMaskType_Shadows:
        // Mask is an inverted luminance mask, normalized 0..1 and
        // shifted over the limits such that it clips beyond the limits.
        // The Mask varies from 1 at LowerLimit to 0 at UpperLimit
        // Meaning that deep shadows will be pulled up a lot and
        // approximating the upperlimit we will take more of the original
        // image.
        Mask = 1.0-LIM((Mask-LowerLimit)*ReciprocalRange,0.0,1.0);
        break;
      case ptMaskType_Midtones:
        // Not fully understood but generates a useful and nice
        // midtone luminance mask.
        Mask = 1.0 -
               LIM((LowerLimit-Mask)*ReciprocalLowerLimit,0.0,0.1) -
               LIM((Mask-UpperLimit)*ReciprocalUpperMargin,0.0,1.0);
        Mask = LIM(Mask,0.0,1.0);
        break;
      case ptMaskType_Highlights:
        // Mask is a luminance mask, normalized 0..1 and
        // shifted over the limits such that it clips beyond the limits.
        // The Mask varies from 0 at LowerLimit to 1 at UpperLimit
        // Meaning that as from the LowerLimit on , we will take more and
        // more of the darkened image.
        Mask = LIM((Mask-LowerLimit)*ReciprocalRange,0.0,1.0);
        break;
    case ptMaskType_All:
        Mask = 1.0;
        break;

      default :
        assert(0);
    }

    // Softening the mask
    Mask = SoftTable[(uint8_t)(Mask*0xff+0.5)];

    // Blend transformed and original according to mask.
    for (short Ch=0; Ch<NrChannels; Ch++) {
      uint16_t PixelValue = m_Image[i][Ch];
      m_Image[i][Ch] = CLIP((int32_t)
        (TransformTable[PixelValue]*Mask + PixelValue*(1-Mask)));
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
                           const double Denoise) {

  double LowerLimit = 0.1;
  double UpperLimit = 1 - LowerLimit;
  double Softness = 0;

  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;

  ptImage *HighpassLayer = new ptImage;
  HighpassLayer->Set(this);

  ptCimgBlur(HighpassLayer, ChannelMask, Radius);

  const double t = (1.0 - Amount)/2;
  const double mHC = Amount*(1.0-fabs(HaloControl)); // m with HaloControl
  const double tHC = (1.0 - mHC)/2; // t with HaloControl

  int Steps = 20;
  ptCurve* AmpCurve = new ptCurve();
  AmpCurve->m_Type = ptCurveType_Anchor;
  for (int i = 0; i<= Steps; i++) {
    double x = (double) i/(double) Steps;
    AmpCurve->m_XAnchor[i]=x;
    if (x < 0.5)
      if (HaloControl > 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
  AmpCurve->m_YAnchor[i]=Amount*x+t;
    else if (x > 0.5)
      if (HaloControl < 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
  AmpCurve->m_YAnchor[i]=Amount*x+t;
    else
      AmpCurve->m_YAnchor[i]=0.5;
  }
  AmpCurve->m_NrAnchors=Steps+1;
  AmpCurve->SetCurveFromAnchors();

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short Ch=0; Ch<NrChannels; Ch++) {
      HighpassLayer->m_Image[i][Ch] = CLIP((int32_t) ((WPH-(int32_t)HighpassLayer->m_Image[i][Ch])+m_Image[i][Ch]));
    }
  }

  HighpassLayer->ApplyCurve(AmpCurve,ChannelMask);
  delete AmpCurve;

  if (Denoise)
    //~ HighpassLayer->WaveletDenoise(ChannelMask, Denoise, 0, 0);
    ptFastBilateralChannel(HighpassLayer, 4.0, Denoise/3.0, 1, 1);

  float (*Mask);
  Mask = (m_ColorSpace == ptSpace_Lab)?
  GetMask(ptMaskType_Midtones, LowerLimit, UpperLimit, Softness,1,0,0):
  GetMask(ptMaskType_Midtones, LowerLimit, UpperLimit, Softness);

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
                                  const double Weight) {

  assert (m_ColorSpace == ptSpace_Lab);

  int32_t offset,offset2,c,i,j,col,row,n;
  float v,s,contrast,temp;
  float CompWeight = 1.0f - Weight;

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

#pragma omp parallel for private(j,i,offset,s,signs,v,n,row,col,offset2,contrast,temp) schedule(static)
  for(j=1;j<height-1;j++)
    for(i=1,offset=j*width+i;i<width-1;i++,offset++){
      s=Strength;
      v=L[offset];

      n=0;
      for(row=j-1;row<=j+1;row++)
        for(col=i-1,offset2=row*width+col;col<=i+1;col++,offset2++){
          signs[n]=0;
          if(v<L[offset2]) signs[n]=-1;
          if(v>L[offset2]) signs[n]=1;
          n++;
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

      temp = MAX(0,temp);

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

  const double WPH = 0x7fff; // WPH=WP/2

  uint16_t (*CoarseLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*CoarseLayer));
  ptMemoryError(CoarseLayer,__FILE__,__LINE__);
  uint16_t (*FineLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*FineLayer));
  ptMemoryError(FineLayer,__FILE__,__LINE__);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    CoarseLayer[i] = m_Image[i][0];
  }

  ptFastBilateralChannel(this,Radius,0.14,2,1);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    FineLayer[i] = CLIP((int32_t) ((WPH-(int32_t)m_Image[i][0])+CoarseLayer[i]));
    CoarseLayer[i] = m_Image[i][0];
  }

  ptFastBilateralChannel(this,4*Radius,0.14,2,1);

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    CoarseLayer[i] = CLIP((int32_t) ((WPH-(int32_t)m_Image[i][0])+CoarseLayer[i]));
  }

  //Curve on residual
  if (Curve != NULL) {
    ApplyCurve(Curve,1);
  }

  //Sigmoidal Contrast
  float Threshold = 0.5;
  float logtf = -logf(Threshold)/logf(2.0);
  float logft = -logf(2.0)/logf(Threshold);
  float Contrast = AmountCoarse;
  float Scaling = 1.0/(1.0+exp(-0.5*Contrast))-1.0/(1.0+exp(0.5*Contrast));
  float Offset = -1.0/(1.0+exp(0.5*Contrast));

  uint16_t ContrastTable[0x10000];
  ContrastTable[0] = 0;

  if (Contrast > 0) {
#pragma omp parallel for
    for (uint32_t i=1; i<0x10000; i++) {
      ContrastTable[i] = CLIP((int32_t)(powf((((1.0/(1.0+
        exp(Contrast*(0.5-powf((float)i/(float)0xffff,logft)))))+Offset)/Scaling),logtf)*0xffff));
    }
  } else if (Contrast < 0) {
#pragma omp parallel for
      for (uint32_t i=1; i<0x10000; i++) {
        ContrastTable[i] = CLIP((int32_t)(powf(0.5-1.0/Contrast*
          logf(1.0/(Scaling*powf((float)i/(float)0xffff,logft)-Offset)-1.0),logtf)*0xffff));
      }
  }

if (AmountCoarse !=0)
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
      CoarseLayer[i] = ContrastTable[ CoarseLayer[i] ];
    }

  Contrast = AmountFine;
  Scaling = 1.0/(1.0+exp(-0.5*Contrast))-1.0/(1.0+exp(0.5*Contrast));
  Offset = -1.0/(1.0+exp(0.5*Contrast));

  if (Contrast > 0) {
#pragma omp parallel for
    for (uint32_t i=1; i<0x10000; i++) {
      ContrastTable[i] = CLIP((int32_t)(powf((((1.0/(1.0+
        exp(Contrast*(0.5-powf((float)i/(float)0xffff,logft)))))+Offset)/Scaling),logtf)*0xffff));
    }
  } else if (Contrast < 0) {
#pragma omp parallel for
      for (uint32_t i=1; i<0x10000; i++) {
        ContrastTable[i] = CLIP((int32_t)(powf(0.5-1.0/Contrast*
          logf(1.0/(Scaling*powf((float)i/(float)0xffff,logft)-Offset)-1.0),logtf)*0xffff));
      }
  }

if (AmountFine !=0)
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i < (uint32_t)m_Height*m_Width; i++) {
      FineLayer[i]   = ContrastTable[ FineLayer[i] ];
    }

#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    m_Image[i][0] = CLIP((int32_t) (((int32_t)CoarseLayer[i]-WPH)+m_Image[i][0]));
    m_Image[i][0] = CLIP((int32_t) (((int32_t)FineLayer[i]-WPH)+m_Image[i][0]));
    //~ m_Image[i][0] = CoarseLayer[i];
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
        const short MaskType,
        const double LowerLimit,
        const double UpperLimit,
        const double Softness) {

  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = (m_ColorSpace == ptSpace_Lab)?1:3;
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;

  ptImage *MicrocontrastLayer = new ptImage;
  MicrocontrastLayer->Set(this);

  ptCimgBlur(MicrocontrastLayer, ChannelMask, Radius);

  const double t = (1.0 - Amount)/2;
  const double mHC = Amount*(1.0-fabs(HaloControl)); // m with HaloControl
  const double tHC = (1.0 - mHC)/2; // t with HaloControl

// TODO mike: Anpassen der Verstärkungskurve an den Sigmoidalen Kontrast.
// Der Berech mit Halocontrol soll linear sein, der andere sigmoidal.
// Wichtig ist, dass bei kleinen Werten der Halocontrol im schwächeren Bereich
// keine Verstärkung auftritt, da evtl die Ableitung der sigmoidalen Kurve
// schwächer ist, als bei der linearen Kurve.
// Anpassen bei allen Filtern, die dieses Verfahren benutzen.

  int Steps = 20;
  ptCurve* AmpCurve = new ptCurve();
  AmpCurve->m_Type = ptCurveType_Anchor;
  for (int i = 0; i<= Steps; i++) {
    double x = (double) i/(double) Steps;
    AmpCurve->m_XAnchor[i]=x;
    if (x < 0.5)
      if (HaloControl > 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
          AmpCurve->m_YAnchor[i]=Amount*x+t;
    else if (x > 0.5)
      if (HaloControl < 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
        AmpCurve->m_YAnchor[i]=Amount*x+t;
    else
      AmpCurve->m_YAnchor[i]=0.5;
  }
  AmpCurve->m_NrAnchors=Steps+1;
  AmpCurve->SetCurveFromAnchors();

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
        const double HaloControl) {

  const double WP = 0xffff;
  const double WPH = 0x7fff; // WPH=WP/2
  const short NrChannels = 3;
  const short ChannelMask = 6;

  ptImage *MicrocontrastLayer = new ptImage;
  MicrocontrastLayer->Set(this);

  ptCimgBlur(MicrocontrastLayer, ChannelMask, Radius);

  const double t = (1.0 - Amount)/2;
  const double mHC = Amount*(1.0-fabs(HaloControl)); // m with HaloControl
  const double tHC = (1.0 - mHC)/2; // t with HaloControl

  int Steps = 20;
  ptCurve* AmpCurve = new ptCurve();
  AmpCurve->m_Type = ptCurveType_Anchor;
  for (int i = 0; i<= Steps; i++) {
    double x = (double) i/(double) Steps;
    AmpCurve->m_XAnchor[i]=x;
    if (x < 0.5)
      if (HaloControl > 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
  AmpCurve->m_YAnchor[i]=Amount*x+t;
    else if (x > 0.5)
      if (HaloControl < 0)
        AmpCurve->m_YAnchor[i]=mHC*x+tHC;
      else
  AmpCurve->m_YAnchor[i]=Amount*x+t;
    else
      AmpCurve->m_YAnchor[i]=0.5;
  }
  AmpCurve->m_NrAnchors=Steps+1;
  AmpCurve->SetCurveFromAnchors();

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
      m_Image[i][Ch] = CLIP((int32_t) (-WP*MIN(Opacity, 0)+Overlay*Opacity+Source*(1-fabs(Opacity))));
    }
  }

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
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;
  const double WPH = 0x7fff;

  ptFastBilateralChannel(DenoiseLayer, Threshold, Softness, 2, ChannelMask);

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
    //~ MaskLayer->m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
    //~ ptMemoryError(MaskLayer->m_Image,__FILE__,__LINE__);

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
  MaskLayer->m_Image[i][0] = CLIP((int32_t) (0.3*MaskLayer->m_Image[i][0]+
    0.59*MaskLayer->m_Image[i][1]+0.11*MaskLayer->m_Image[i][2]));
    }

    ptCurve* Curve = new ptCurve();
    Curve->m_Type = ptCurveType_Anchor;
    Curve->m_XAnchor[0]=0.0;
    Curve->m_YAnchor[0]=1.0;
    Curve->m_XAnchor[1]=0.4;
    Curve->m_YAnchor[1]=0.3;
    Curve->m_XAnchor[2]=0.5;
    Curve->m_YAnchor[2]=0.0;
    Curve->m_XAnchor[3]=0.6;
    Curve->m_YAnchor[3]=0.3;
    Curve->m_XAnchor[4]=1.0;
    Curve->m_YAnchor[4]=1.0;
    Curve->m_NrAnchors=5;
    Curve->SetCurveFromAnchors();
    MaskLayer->ApplyCurve(Curve,1);

    ptCimgBlur(MaskLayer, 1, UseMask);

    Curve->m_XAnchor[0]=0.0;
    Curve->m_YAnchor[0]=0.0;
    Curve->m_XAnchor[1]=0.6;
    Curve->m_YAnchor[1]=0.4;
    Curve->m_XAnchor[2]=1.0;
    Curve->m_YAnchor[2]=0.8;
    Curve->m_NrAnchors=3;
    Curve->SetCurveFromAnchors();
    MaskLayer->ApplyCurve(Curve,1);
    delete Curve;

    float (*Mask);
    Mask = MaskLayer->GetMask(ptMaskType_Shadows, 0.0, 1.0, 0.0, 1,0,0);
    Overlay(DenoiseLayer->m_Image,Opacity,Mask,ptOverlayMode_Normal);
    FREE(Mask);
    delete MaskLayer;

    //~ Mask = ptGradientMask(this, Threshold/2.0,UseMask);
    //~ Overlay(DenoiseLayer->m_Image,Opacity,Mask,ptOverlayMode_Normal);
    //~
    //~ FREE(Mask);
  } else {
    Overlay(DenoiseLayer->m_Image,Opacity,NULL,ptOverlayMode_Normal);
  }
  delete DenoiseLayer;
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
  const short ChannelMask = (m_ColorSpace == ptSpace_Lab)?1:7;

  ptImage *ContrastLayer = new ptImage;
  ContrastLayer->Set(this);

  ptFastBilateralChannel(ContrastLayer, Threshold, Softness, 2, ChannelMask);

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
        MaskLayer->m_Image[i][0] = CLIP((int32_t) (0.3*MaskLayer->m_Image[i][0]+
          0.59*MaskLayer->m_Image[i][1]+0.11*MaskLayer->m_Image[i][2]));
    }

    ptCurve* Curve = new ptCurve();
    Curve->m_Type = ptCurveType_Anchor;
    Curve->m_XAnchor[0]=0.0;
    Curve->m_YAnchor[0]=1.0;
    Curve->m_XAnchor[1]=0.4;
    Curve->m_YAnchor[1]=0.3;
    Curve->m_XAnchor[2]=0.5;
    Curve->m_YAnchor[2]=0.0;
    Curve->m_XAnchor[3]=0.6;
    Curve->m_YAnchor[3]=0.3;
    Curve->m_XAnchor[4]=1.0;
    Curve->m_YAnchor[4]=1.0;
    Curve->m_NrAnchors=5;
    Curve->SetCurveFromAnchors();
    MaskLayer->ApplyCurve(Curve,1);

    ptCimgBlur(MaskLayer, 1, Masking);

    Curve->m_XAnchor[0]=0.0;
    Curve->m_YAnchor[0]=0.0;
    Curve->m_XAnchor[1]=0.4;
    Curve->m_YAnchor[1]=0.6;
    Curve->m_XAnchor[2]=0.8;
    Curve->m_YAnchor[2]=1.0;
    Curve->m_NrAnchors=3;
    Curve->SetCurveFromAnchors();
    MaskLayer->ApplyCurve(Curve,1);
    delete Curve;

    float (*Mask);
    Mask = MaskLayer->GetMask(ptMaskType_Highlights, 0.0, 1.0, 0.0, 1,0,0);
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

  int MaxExponent=(int)floor(log((double)Radius1)/log(2.))+1;
  uint16_t MaxRadius=(int)pow(2.,(double)(MaxExponent+1));
  int Step = 0;
  int Blocksize = 0;
  int SquareStep = 0;
  int Shift = 0;
  uint16_t x1 = 0;
  uint16_t x2 = 0;
  uint16_t y1 = 0;
  uint16_t y2 = 0;
  uint16_t pos1x = 0;
  uint16_t pos1y = 0;
  uint16_t pos2x = 0;
  uint16_t pos2y = 0;
  double value1 = 0;
  double value2 = 0;
  double ker = 0;

  double **Kernel;
  Kernel = (double **)CALLOC(MaxRadius,sizeof(double*));
  for(int i = 0; i < MaxRadius; i++) Kernel[i] = (double*)CALLOC(MaxRadius,sizeof(double));

  double r=0;
  for (uint16_t x=0; x<MaxRadius; x++) {
    for (uint16_t y=0; y<MaxRadius; y++) {
      r=pow(double(x*x+y*y),0.5);
      if (r<=Radius1) {
        Kernel[x][y] = 1;
      } else {
        Kernel[x][y] = 0;
      }
    }
  }
  int16_t (*MinVector)[2] = (int16_t (*)[2]) CALLOC(m_Width*m_Height,sizeof(*MinVector));
  ptMemoryError(MinVector,__FILE__,__LINE__);
  int16_t (*MaxVector)[2] = (int16_t (*)[2]) CALLOC(m_Width*m_Height,sizeof(*MaxVector));
  ptMemoryError(MaxVector,__FILE__,__LINE__);

#pragma omp parallel for schedule(static)
  for (uint16_t Row=0; Row < Height; Row++) {
    for (uint16_t Col=0; Col < Width; Col++) {
      MinVector[Row*Width+Col][0] = 0;
      MinVector[Row*Width+Col][1] = 0;
      MaxVector[Row*Width+Col][0] = 0;
      MaxVector[Row*Width+Col][1] = 0;
    }
  }

  for (int runcounter=0; runcounter<2;runcounter++){
    for (int k=1; k<=MaxExponent; k++) {
      Step=(int) pow(2.,(double)(k));
      SquareStep = (int) pow(2.,(double)(k-1));
      Blocksize = SquareStep;
      Shift = Blocksize;
m_UpdateGUI();
      for (int32_t Row=-(runcounter&1)*Shift; Row < Height; Row=Row+Step) {
        for (int32_t Col=-(runcounter&1)*Shift; Col < Width; Col=Col+Step) {
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
                      // Calculate Min
                      pos1x = MinVector[y1*Width+x1][0] + x1;
                      pos1y = MinVector[y1*Width+x1][1] + y1;
                      pos2x = MinVector[y2*Width+x2][0] + x2;
                      pos2y = MinVector[y2*Width+x2][1] + y2;
                      value1 = (double)m_Image[pos1y*Width+pos1x][0];
                      ker = Kernel[(int)fabs(pos2x - x1)][(int)fabs(pos2y - y1)];
                      value2 = (double)m_Image[pos2y*Width+pos2x][0];
                      if (ker && value2 <= value1) {
                        MinVector[y1*Width+x1][0] = pos2x - x1;
                        MinVector[y1*Width+x1][1] = pos2y - y1;
                      }
                      // Calculate Max
                      pos1x = MaxVector[y1*Width+x1][0] + x1;
                      pos1y = MaxVector[y1*Width+x1][1] + y1;
                      pos2x = MaxVector[y2*Width+x2][0] + x2;
                      pos2y = MaxVector[y2*Width+x2][1] + y2;
                      value1 = (double)m_Image[pos1y*Width+pos1x][0];
                      ker = Kernel[(int)fabs(pos2x - x1)][(int)fabs(pos2y - y1)];
                      value2 = (double)m_Image[pos2y*Width+pos2x][0];
                      if (ker && value2 >= value1) {
                        MaxVector[y1*Width+x1][0] = pos2x - x1;
                        MaxVector[y1*Width+x1][1] = pos2y - y1;
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

  uint16_t (*MinLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*MinLayer));
  ptMemoryError(MinLayer,__FILE__,__LINE__);
  uint16_t (*MaxLayer) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*MaxLayer));
  ptMemoryError(MaxLayer,__FILE__,__LINE__);

#pragma omp parallel for private(pos1x, pos1y, value1) schedule(static)
  for (uint16_t Row=0; Row < Height; Row++) {
    for (uint16_t Col=0; Col < Width; Col++) {
      pos1x = MinVector[Row*Width+Col][0] + Col;
      pos1y = MinVector[Row*Width+Col][1] + Row;
      value1 = (double)m_Image[pos1y*Width+pos1x][0];
      MinLayer[Row*Width+Col] = CLIP((int32_t) value1);
      pos1x = MaxVector[Row*Width+Col][0] + Col;
      pos1y = MaxVector[Row*Width+Col][1] + Row;
      value1 = (double)m_Image[pos1y*Width+pos1x][0];
      MaxLayer[Row*Width+Col] = CLIP((int32_t) value1);
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
    for (uint16_t Row=0; Row < Height; Row++) {
      for (uint16_t Col=0; Col < Width; Col++) {
      Z = m_Image[Row*Width+Col][0]-MinLayer[Row*Width+Col];
      N = MaxLayer[Row*Width+Col]-MinLayer[Row*Width+Col];
      if (m>0) {
        Mask=m*N/(float)0xffff+(1.-m);
      } else {
        Mask=-m*(1-(N/(float)0xffff))+(1.+m);
      }
      m_Image[Row*Width+Col][0]=CLIP((int32_t) ((Z/N*0xFFFF*Mask+(1-Mask)*m_Image[Row*Width+Col][0])*Opacity+(1.-Opacity)*m_Image[Row*Width+Col][0]));
      }
    }
  } else if (Method == 2) {
#pragma omp parallel for private(Z,N,Mask) schedule(static)
    for (uint16_t Row=0; Row < Height; Row++) {
      for (uint16_t Col=0; Col < Width; Col++) {
      m_Image[Row*Width+Col][0]=MinLayer[Row*Width+Col];
      }
    }
  } else {
#pragma omp parallel for private(Z,N,Mask) schedule(static)
    for (uint16_t Row=0; Row < Height; Row++) {
      for (uint16_t Col=0; Col < Width; Col++) {
      m_Image[Row*Width+Col][0]=MaxLayer[Row*Width+Col];
      }
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
                        const short NoiseType, // 0-5, Gaussian, uniform, salt&pepper
                        const double Radius, // 0-20
                        const double Opacity,
                        const short MaskType,
                        const double LowerLimit,
                        const double UpperLimit,
      const short ScaleFactor) { // 1, 2 or 4 depending on pipe size

  ptImage *NoiseLayer = new ptImage;
  NoiseLayer->Set(this);  // allocation of free layer faster? TODO!
  float (*Mask);
  short Noise = LIM(NoiseType,0,5);
  Noise = (Noise > 2) ? (Noise - 3) : Noise;
  short ScaledRadius = Radius/ScaleFactor;

  ptCimgNoise(NoiseLayer, Sigma*10000, Noise, ScaledRadius);

  const double WPH = 0x7fff;

  if (4/ScaleFactor != 1) {
    double m = 4/ScaleFactor;
    double t = (1-m)*WPH;
#pragma omp parallel for default(shared)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      NoiseLayer->m_Image[i][0] = CLIP((int32_t)(NoiseLayer->m_Image[i][0]*m+t));
    }
  }

  Mask=GetMask(MaskType, LowerLimit, UpperLimit, 0.0);
  if (NoiseType < 3) {
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

ptImage* ptImage::BWStyler(const short FilmType,
         const short ColorFilterType,
         const double MultR,
         const double MultG,
         const double MultB,
         const double Opacity) {

  double Mixer[3][3];
  double R = 0,G = 0,B = 0;
  double FR = 0, FG = 0, FB = 0;
  switch (FilmType) {
  case ptFilmType_LowSensitivity:
    R = 0.27;
    G = 0.27;
    B = 0.46;
    break;

  case ptFilmType_HighSensitivity:
    R = 0.3;
    G = 0.28;
    B = 0.42;
    break;

  case ptFilmType_Hyperpanchromatic:
    R = 0.41;
    G = 0.25;
    B = 0.34;
    break;

  case ptFilmType_Orthochromatic:
    R = 0.0;
    G = 0.42;
    B = 0.58;
    break;
//Following values corresponding to http://photographynotes.pbworks.com/bwrecipe
  case ptFilmType_NormalContrast:
    R = 0.43;
    G = 0.33;
    B = 0.3;
    break;

  case ptFilmType_HighContrast:
    R = 0.4;
    G = 0.34;
    B = 0.60;
    break;

  case ptFilmType_Luminance:
    R = 0.3;
    G = 0.59;
    B = 0.11;
    break;

  case ptFilmType_Landscape:
    R = 0.66;
    G = 0.24;
    B = 0.10;
    break;

  case ptFilmType_FaceInterior:
    R = 0.54;
    G = 0.44;
    B = 0.12;
    break;

  case ptFilmType_ChannelMixer:
    R = MultR/(MultR+MultG+MultB);
    G = MultG/(MultR+MultG+MultB);
    B = MultB/(MultR+MultG+MultB);
    break;
  }

  switch (ColorFilterType) {
  //Dr. Herbert Kribben and http://epaperpress.com/psphoto/bawFilters.html
  case ptColorFilterType_None:
    FR = 1.0;
    FG = 1.0;
    FB = 1.0;
    break;

  case ptColorFilterType_Red:
    FR = 1.0;
    FG = 0.2;
    FB = 0.0;
    break;

  case ptColorFilterType_Orange:
    FR = 1.0;
    FG = 0.6;
    FB = 0.0;
    break;

  case ptColorFilterType_Yellow:
    FR = 1.0;
    FG = 1.0;
    FB = 0.1;
    break;

  case ptColorFilterType_YellowGreen:
    FR = 0.6;
    FG = 1.0;
    FB = 0.3;
    break;

  case ptColorFilterType_Green:
    FR = 0.2;
    FG = 1.0;
    FB = 0.3;
    break;

  case ptColorFilterType_Blue:
    FR = 0.0;
    FG = 0.2;
    FB = 1.0;
    break;

  case ptColorFilterType_fakeIR:
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

  ptImage *BWLayer = new ptImage;
  if (Opacity != 1.0)
    BWLayer->Set(this);

  Mixer[0][0] = R;
  Mixer[1][0] = R;
  Mixer[2][0] = R;
  Mixer[0][1] = G;
  Mixer[1][1] = G;
  Mixer[2][1] = G;
  Mixer[0][2] = B;
  Mixer[1][2] = B;
  Mixer[2][2] = B;

  if (Opacity == 1)
    MixChannels(Mixer);
  else {
    BWLayer->MixChannels(Mixer);
    Overlay(BWLayer->m_Image,Opacity,NULL,ptOverlayMode_Normal);
    delete BWLayer;
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
    ptCurve* RedCurve = new ptCurve();
    RedCurve->m_Type = ptCurveType_Anchor;
    RedCurve->m_IntendedChannel = ptCurveChannel_R;
    RedCurve->m_XAnchor[0]=0.0;
    RedCurve->m_YAnchor[0]=0.0;
    RedCurve->m_XAnchor[1]=0.5-0.2*R;
    RedCurve->m_YAnchor[1]=0.5+0.2*R;
    RedCurve->m_XAnchor[2]=1.0;
    RedCurve->m_YAnchor[2]=1.0;
    RedCurve->m_NrAnchors=3;
    RedCurve->SetCurveFromAnchors();
    ApplyCurve(RedCurve,1);
    delete RedCurve;
  }
  if (G) {
    ptCurve* GreenCurve = new ptCurve();
    GreenCurve->m_Type = ptCurveType_Anchor;
    GreenCurve->m_IntendedChannel = ptCurveChannel_G;
    GreenCurve->m_XAnchor[0]=0.0;
    GreenCurve->m_YAnchor[0]=0.0;
    GreenCurve->m_XAnchor[1]=0.5-0.2*G;
    GreenCurve->m_YAnchor[1]=0.5+0.2*G;
    GreenCurve->m_XAnchor[2]=1.0;
    GreenCurve->m_YAnchor[2]=1.0;
    GreenCurve->m_NrAnchors=3;
    GreenCurve->SetCurveFromAnchors();
    ApplyCurve(GreenCurve,2);
    delete GreenCurve;
  }
  if (B) {
    ptCurve* BlueCurve = new ptCurve();
    BlueCurve->m_Type = ptCurveType_Anchor;
    BlueCurve->m_IntendedChannel = ptCurveChannel_B;
    BlueCurve->m_XAnchor[0]=0.0;
    BlueCurve->m_YAnchor[0]=0.0;
    BlueCurve->m_XAnchor[1]=0.5-0.2*B;
    BlueCurve->m_YAnchor[1]=0.5+0.2*B;
    BlueCurve->m_XAnchor[2]=1.0;
    BlueCurve->m_YAnchor[2]=1.0;
    BlueCurve->m_NrAnchors=3;
    BlueCurve->SetCurveFromAnchors();
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

ptImage* ptImage::Temperature(const double Temperature,
            const double Tint) {

  assert (m_ColorSpace == ptSpace_Lab);

  ptCurve* Temp1Curve = new ptCurve();
  Temp1Curve->m_Type = ptCurveType_Anchor;
  Temp1Curve->m_XAnchor[0]=0.0;
  Temp1Curve->m_YAnchor[0]=0.0;
  Temp1Curve->m_XAnchor[1]=0.5-0.05*Temperature+0.1*Tint;
  Temp1Curve->m_YAnchor[1]=0.5+0.05*Temperature-0.1*Tint;
  Temp1Curve->m_XAnchor[2]=1.0;
  Temp1Curve->m_YAnchor[2]=1.0;
  Temp1Curve->m_NrAnchors=3;
  Temp1Curve->SetCurveFromAnchors();

  ptCurve* Temp2Curve = new ptCurve();
  Temp2Curve->m_Type = ptCurveType_Anchor;
  Temp2Curve->m_XAnchor[0]=0.0;
  Temp2Curve->m_YAnchor[0]=0.0;
  Temp2Curve->m_XAnchor[1]=0.5-0.1*Temperature-0.05*Tint;
  Temp2Curve->m_YAnchor[1]=0.5+0.1*Temperature+0.05*Tint;
  Temp2Curve->m_XAnchor[2]=1.0;
  Temp2Curve->m_YAnchor[2]=1.0;
  Temp2Curve->m_NrAnchors=3;
  Temp2Curve->SetCurveFromAnchors();

  ApplyCurve(Temp1Curve,2);
  ApplyCurve(Temp2Curve,4);
  delete Temp1Curve;
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
  GammaCurve->SetCurveFromFunction(GammaTool,0.45,0.);
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
                          const short MaskType, /* ptMaskType_All */
                          const short ManualMask, /* 0 */
                          const double LowerLevel, /* 0 */
                          const double UpperLevel, /* 1 */
                          const double Softness /* 0 */) {

  assert (m_ColorSpace == ptSpace_Lab);

  double a = Amount * cos(Hue/180.*ptPI);
  double b = Amount * sin(Hue/180.*ptPI);

  const double WPH = 0x8080; // Neutral in Lab A and B

  double t = (1.-Saturation)*WPH;

  ptCurve* Temp1Curve = new ptCurve();
  Temp1Curve->m_Type = ptCurveType_Anchor;
  Temp1Curve->m_XAnchor[0]=0.0;
  Temp1Curve->m_YAnchor[0]=0.0;
  Temp1Curve->m_XAnchor[1]=0.5-0.1*a;
  Temp1Curve->m_YAnchor[1]=0.5+0.1*a;
  Temp1Curve->m_XAnchor[2]=1.0;
  Temp1Curve->m_YAnchor[2]=1.0;
  Temp1Curve->m_NrAnchors=3;
  Temp1Curve->SetCurveFromAnchors();

  ptCurve* Temp2Curve = new ptCurve();
  Temp2Curve->m_Type = ptCurveType_Anchor;
  Temp2Curve->m_XAnchor[0]=0.0;
  Temp2Curve->m_YAnchor[0]=0.0;
  Temp2Curve->m_XAnchor[1]=0.5-0.1*b;
  Temp2Curve->m_YAnchor[1]=0.5+0.1*b;
  Temp2Curve->m_XAnchor[2]=1.0;
  Temp2Curve->m_YAnchor[2]=1.0;
  Temp2Curve->m_NrAnchors=3;
  Temp2Curve->SetCurveFromAnchors();

  if (MaskType == ptMaskType_All) {
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

    if (MaskType == ptMaskType_Shadows)
      if (ManualMask)
        Mask = GetMask(ptMaskType_Shadows, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(ptMaskType_Shadows, 0,0.5,0,1,0,0);
    else if (MaskType == ptMaskType_Midtones)
      if (ManualMask)
        Mask = GetMask(ptMaskType_Midtones, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(ptMaskType_Midtones, 0.5,0.5,0,1,0,0);
    else
      if (ManualMask)
        Mask = GetMask(ptMaskType_Highlights, LowerLevel, UpperLevel, Softness,1,0,0);
      else
        Mask = GetMask(ptMaskType_Highlights, 0.5,1,0,1,0,0);

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
                       const short    MaskType,
                       const double   LowerLimit,
                       const double   UpperLimit,
                       const double   Softness) {

  assert (m_ColorSpace != ptSpace_Lab);
  uint16_t (*ToneImage)[3] =
    (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
  ptMemoryError(ToneImage,__FILE__,__LINE__);

  float (*Mask);
  if (MaskType <= ptMaskType_All)
    Mask=GetMask(MaskType, LowerLimit, UpperLimit, Softness);
  else
    Mask=GetMask(ptMaskType_Midtones, LowerLimit, UpperLimit, Softness);
#pragma omp parallel for
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    ToneImage[i][0] = R;
    ToneImage[i][1] = G;
    ToneImage[i][2] = B;
  }

  if (MaskType <= ptMaskType_All)
    Overlay(ToneImage, Amount, Mask);
  else if (MaskType == ptMaskType_Screen)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_Screen);
  else if (MaskType == ptMaskType_Multiply)
    Overlay(ToneImage, Amount, Mask, ptOverlayMode_Multiply);

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

  ptCurve* RedCurve = new ptCurve();
  RedCurve->m_Type = ptCurveType_Anchor;
  RedCurve->m_XAnchor[0]=0.0;
  RedCurve->m_YAnchor[0]=0.0;
  RedCurve->m_XAnchor[1]=0.05;
  RedCurve->m_YAnchor[1]=0.05-0.05*Color1;
  RedCurve->m_XAnchor[2]=0.15;
  RedCurve->m_YAnchor[2]=0.15;
  RedCurve->m_XAnchor[3]=1.0;
  RedCurve->m_YAnchor[3]=1.0;
  RedCurve->m_NrAnchors=4;
  RedCurve->SetCurveFromAnchors();

  ptCurve* GreenCurve = new ptCurve();
  GreenCurve->m_Type = ptCurveType_Anchor;
  GreenCurve->m_XAnchor[0]=0.0;
  GreenCurve->m_YAnchor[0]=0.0;
  GreenCurve->m_XAnchor[1]=0.03;
  GreenCurve->m_YAnchor[1]=0.025;
  GreenCurve->m_XAnchor[2]=0.2;
  GreenCurve->m_YAnchor[2]=0.2+0.3*Color1;
  GreenCurve->m_XAnchor[3]=1.0;
  GreenCurve->m_YAnchor[3]=1.0;
  GreenCurve->m_NrAnchors=4;
  GreenCurve->SetCurveFromAnchors();

  ptCurve* BlueCurve = new ptCurve();
  BlueCurve->m_Type = ptCurveType_Anchor;
  BlueCurve->m_XAnchor[0]=0.0;
  BlueCurve->m_YAnchor[0]=0.0;
  BlueCurve->m_XAnchor[1]=0.3;
  BlueCurve->m_YAnchor[1]=0.3-0.2*Color2;
  BlueCurve->m_XAnchor[2]=1.0;
  BlueCurve->m_YAnchor[2]=1.0;
  BlueCurve->m_NrAnchors=3;
  BlueCurve->SetCurveFromAnchors();

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

  double Length = 0;
  if (fabs(Angle) == 0 || fabs(Angle) == 180 ) {
    Length = m_Height;
  } else if (fabs(Angle) == 90) {
    Length = m_Width;
  } else if (fabs(Angle) < 90) {
    Length = (((double)m_Width) + ((double)m_Height)/tan(fabs(Angle)/180*ptPI))*sin(fabs(Angle)/180*ptPI);
  } else {
    Length = (((double)m_Width) + ((double)m_Height)/tan((180.0-fabs(Angle))/180.0*ptPI))*sin((180.0-fabs(Angle))/180.0*ptPI);
  }

  double LL = Length*LowerLevel;
  double UL = Length*UpperLevel;
  double Black = 0.0;
  double White = 1.0;

  float coordinate = 0;
  float Value = 0;
  float (*GradualMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*GradualMask));
  ptMemoryError(GradualMask,__FILE__,__LINE__);
  float dist = 0;
  uint16_t (*ToneImage)[3] = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
  ptMemoryError(ToneImage,__FILE__,__LINE__);

#pragma omp parallel for schedule(static) default(shared) firstprivate(dist, Value, coordinate)
  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {
      if (fabs(Angle) == 0.0)
        dist = (float)(m_Height-Row);
      else if (fabs(Angle) == 180.0)
        dist = (float)Row;
      else if (Angle == 90.0)
        dist = (float)Col;
      else if (Angle == -90.0)
        dist = (float)(m_Width-Col);
      else if (Angle > 0.0 && Angle < 90.0)
        dist = ((float)Col + (float)(m_Height-Row)/tanf(Angle/180*ptPI))*sinf(Angle/180*ptPI);
      else if (Angle > 90.0 && Angle < 180.0)
        dist = Length-((float)(m_Width-Col) + (float)(m_Height-Row)/tanf((180.0-Angle)/180*ptPI))*sinf((180.0-Angle)/180*ptPI);
      else if (Angle > -90.0 && Angle < 0.0)
        dist = ((float)(m_Width-Col) + (float)(m_Height-Row)/tanf(fabs(Angle)/180*ptPI))*sinf(fabs(Angle)/180*ptPI);
      else if (Angle > -180.0 && Angle < -90.0)
        dist = Length-((float)Col + (float)(m_Height-Row)/tanf((180.0-fabs(Angle))/180*ptPI))*sinf((180.0-fabs(Angle))/180*ptPI);

      if (dist <= LL)
        GradualMask[Row*m_Width+Col] = Black;
      else if (dist > UL)
        GradualMask[Row*m_Width+Col] = White;
      else {
        coordinate = (UL-dist)/(UL-LL);
        Value = (1.0-powf(cosf((1.0-coordinate)*ptPI/2.0),50.0*Softness))
                  * powf((1.0-coordinate),0.07*Softness);
        GradualMask[Row*m_Width+Col] = LIM(Value*White, 0.0, 1.0);
      }
    }
  }

#pragma omp parallel for schedule(static) default(shared)
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

ptImage* ptImage::Vignette(const short VignetteMode,
               const short Exponent,
               const double Amount,
               const double InnerRadius,
               const double OuterRadius,
               const double Roundness,
               const double CenterX,
               const double CenterY,
               const double Softness) {

  float Radius = MIN(m_Width, m_Height)/2;
  float OR = Radius*OuterRadius;
  float IR = Radius*InnerRadius;
  float Black = 0;
  float White = 1.0;

  float CX = (1+CenterX)*m_Width/2;
  float CY = (1-CenterY)*m_Height/2;

  float InversExponent = 1.0/ Exponent;
  float coordinate = 0;
  float Value = 0;
  float (*VignetteMask) = (float (*)) CALLOC(m_Width*m_Height,sizeof(*VignetteMask));
  ptMemoryError(VignetteMask,__FILE__,__LINE__);
  float dist = 0;

#pragma omp parallel for schedule(static) default(shared) firstprivate(dist, Value, coordinate)
  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {
      dist = powf(powf(fabsf(Col-CX)/powf(2,Roundness),Exponent)
                  + powf(fabsf(Row-CY)/powf(2,-Roundness),Exponent),InversExponent);
      if (dist < IR)
        VignetteMask[Row*m_Width+Col] = Black;
      else if (dist > OR)
        VignetteMask[Row*m_Width+Col] = White;
      else {
        coordinate = (OR-dist)/(OR-IR);
        Value = (1.0-powf(cosf((1.0-coordinate)*ptPI/2.0),50.0*Softness))
                * powf((1.0-coordinate),0.07*Softness);
        //~ Value = pow(cos(coordinate*ptPI/2),2);
        VignetteMask[Row*m_Width+Col] = LIM(Value*White,0.0,1.0);
      }
    }
  }

  switch (VignetteMode) {
    case ptVignetteMode_Soft:
      {
        uint16_t (*ToneImage)[3] = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
        ptMemoryError(ToneImage,__FILE__,__LINE__);
        if (Amount > 0) {
#pragma omp parallel for schedule(static) default(shared)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            ToneImage[i][0] = 0;
            ToneImage[i][1] = 0;
            ToneImage[i][2] = 0;
          }
        } else {
#pragma omp parallel for schedule(static) default(shared)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            ToneImage[i][0] = 0xffff;
            ToneImage[i][1] = 0xffff;
            ToneImage[i][2] = 0xffff;
          }
        }
        Overlay(ToneImage, fabs(Amount), VignetteMask);
        FREE(ToneImage);
      }
      break;

    case ptVignetteMode_Hard:
      {
        uint16_t (*ToneImage)[3] = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*ToneImage));
        ptMemoryError(ToneImage,__FILE__,__LINE__);
        if (Amount > 0) {
#pragma omp parallel for schedule(static) default(shared)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            ToneImage[i][0] = 0;
            ToneImage[i][1] = 0;
            ToneImage[i][2] = 0;
          }
          Overlay(ToneImage, fabs(Amount), VignetteMask, ptOverlayMode_Multiply);
        } else {
#pragma omp parallel for schedule(static) default(shared)
          for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
            ToneImage[i][0] = 0xffff;
            ToneImage[i][1] = 0xffff;
            ToneImage[i][2] = 0xffff;
          }
          Overlay(ToneImage, fabs(Amount), VignetteMask, ptOverlayMode_Screen);
        }
        FREE(ToneImage);
      }
      break;

    case ptVignetteMode_Fancy:
      {
        ptImage *VignetteLayer = new ptImage;
        VignetteLayer->Set(this);
        VignetteLayer->Expose(pow(2,-Amount*5), ptExposureClipMode_None);
        ptCurve* VignetteContrastCurve = new ptCurve();
        VignetteContrastCurve->SetCurveFromFunction(Sigmoidal,0.5,fabs(Amount)*10);
        VignetteLayer->ApplyCurve(VignetteContrastCurve,m_ColorSpace == ptSpace_Lab? 1:7);
        delete VignetteContrastCurve;
        Overlay(VignetteLayer->m_Image, 1, VignetteMask, ptOverlayMode_Normal);
        delete VignetteLayer;
      }
      break;

    case ptVignetteMode_Mask:
#pragma omp parallel for schedule(static) default(shared)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0] = CLIP((int32_t) (VignetteMask[i]*0xffff));
        m_Image[i][1] = CLIP((int32_t) (VignetteMask[i]*0xffff));
        m_Image[i][2] = CLIP((int32_t) (VignetteMask[i]*0xffff));
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

ptImage* ptImage::Softglow(const short SoftglowMode,
         const double  Radius,
         const double  Amount,
         const uint8_t ChannelMask,
         const double  Contrast, /* 5 */
         const int     Saturation /* -50 */) {

  // Workflow for Orton from
  // http://www.flickr.com/groups/gimpusers/discuss/72157621381832321/
  ptImage *BlurLayer = new ptImage;
  BlurLayer->Set(this);
  // for Orton
  if (SoftglowMode==ptSoftglowMode_OrtonScreen ||
      SoftglowMode==ptSoftglowMode_OrtonSoftLight)
    BlurLayer->Overlay(BlurLayer->m_Image,1.0, NULL, ptOverlayMode_Screen);

  // Blur
  if (Radius != 0)
    ptCimgBlur(BlurLayer, ChannelMask, Radius);
  // Contrast
  if (Contrast != 0) {
    ptCurve* MyContrastCurve = new ptCurve();
    MyContrastCurve->SetCurveFromFunction(Sigmoidal,0.5,Contrast);
    BlurLayer->ApplyCurve(MyContrastCurve,7);
    delete MyContrastCurve;
  }
  // Desaturate
  if (Saturation != 0) {
    int Value = Saturation;
    double VibranceMixer[3][3];
    VibranceMixer[0][0] = 1.0+(Value/150.0);
    VibranceMixer[0][1] = -(Value/300.0);
    VibranceMixer[0][2] = VibranceMixer[0][1];
    VibranceMixer[1][0] = VibranceMixer[0][1];
    VibranceMixer[1][1] = VibranceMixer[0][0];
    VibranceMixer[1][2] = VibranceMixer[0][1];
    VibranceMixer[2][0] = VibranceMixer[0][1];
    VibranceMixer[2][1] = VibranceMixer[0][1];
    VibranceMixer[2][2] = VibranceMixer[0][0];
    BlurLayer->MixChannels(VibranceMixer);
  }
  // Overlay
  switch (SoftglowMode) {
  case ptSoftglowMode_Lighten:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_Lighten);
    break;
  case ptSoftglowMode_Screen:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_Screen);
    break;
  case ptSoftglowMode_SoftLight:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_SoftLight);
    break;
  case ptSoftglowMode_Normal:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_Normal);
    break;
  case ptSoftglowMode_OrtonScreen:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_Screen);
    break;
  case ptSoftglowMode_OrtonSoftLight:
    Overlay(BlurLayer->m_Image,Amount,NULL,ptOverlayMode_SoftLight);
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

float *ptImage::GetMask(const short MaskType,
                        const double LowerLimit,
                        const double UpperLimit,
                        const double Softness,
                        const double FactorR,
                        const double FactorG,
                        const double FactorB) {

  uint16_t (*Mask) = (uint16_t (*)) CALLOC(m_Width*m_Height,sizeof(*Mask));
  ptMemoryError(Mask,__FILE__,__LINE__);

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
#pragma omp parallel for default(shared)
  for (int32_t i=0; i<0x10000; i++) {
    switch(MaskType) {
    case ptMaskType_All: // All values
      MaskTable[i] = WP;
        break;
    case ptMaskType_Shadows: // Shadows
        MaskTable[i] = WP - CLIP((int32_t)(i*m+t));
        break;
    case ptMaskType_Midtones: // Midtones
      MaskTable[i] = WP - CLIP((int32_t)(i*m1+WP)) - CLIP((int32_t)(i*m2+t2));
        break;
    case ptMaskType_Highlights: // Highlights
         MaskTable[i] = CLIP((int32_t)(i*m+t));
         break;
    }
    if (Soft>1.0) {
      MaskTable[i] = LIM(pow(MaskTable[i]/(float)0xffff,Soft),0.0,1.0);
    } else {
      MaskTable[i] = LIM(MaskTable[i]/(float)0xffff/Soft,0.0,1.0);
    }
  }
#pragma omp parallel for default(shared)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
  Mask[i] = CLIP((int32_t)
          (m_Image[i][0]*FactorR + m_Image[i][1]*FactorG + m_Image[i][2]*FactorB));
    dMask[i] = MaskTable[Mask[i]];
  }

  FREE(Mask);
  return dMask;

}

////////////////////////////////////////////////////////////////////////////////
//
// Blur
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Blur(const uint8_t ChannelMask,
                       const double  Radius,
                       const double  Sigma) {

  assert (Sigma > 0.0);

  // KernelWidth is per construction oneven. No need to assert.
  uint16_t    KernelWidth   = ptGetOptimalKernelWidth1D(Radius,Sigma);

  double *Kernel       = GetBlurKernel(KernelWidth,Sigma);

  double Value;

  // Blurred image , integer and ptoat version.
  uint16_t (*BlurredImage)[3] = NULL;

  // Blur rows.

  // Copy image
  BlurredImage =  (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(BlurredImage,__FILE__,__LINE__);
  memcpy(BlurredImage,m_Image,m_Width*m_Height*sizeof(*m_Image));

  for (short Channel=0;Channel<3;Channel++) {

    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;

    for (uint16_t Row=0; Row < m_Height; Row++) {
      for (uint16_t Column=0; Column < m_Width-KernelWidth+1; Column++) {
        Value = 0.0;
        for (uint16_t i=0; i < KernelWidth; i++) {
          Value += Kernel[i]*m_Image[Row*m_Width+Column+i][Channel];
        }
        BlurredImage[Row*m_Width+Column+KernelWidth/2][Channel] =
          CLIP((int32_t)Value);
      }
    }
  }

  // Get rid of the original image and substitute it by the partly blurred.
  FREE(m_Image);
  m_Image = BlurredImage;

  //  Blur Columns. Starting from the row blurred one.

  // Copy image
  BlurredImage = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(BlurredImage,__FILE__,__LINE__);
  memcpy(BlurredImage,m_Image,m_Width*m_Height*sizeof(*m_Image));

  for (short Channel=0;Channel<3;Channel++) {

    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;

    for (uint16_t Column=0; Column < m_Width; Column++) {
      for (uint16_t Row=0; Row < m_Height-KernelWidth+1; Row++) {
        Value = 0.0;
        for (uint16_t i=0; i < KernelWidth; i++) {
          Value += Kernel[i]*m_Image[(Row+i)*m_Width+Column][Channel];
        }
        BlurredImage[(Row+KernelWidth/2)*m_Width+Column][Channel] =
          CLIP((int32_t)Value);
      }
    }
  }

  // Get rid of the partly blurred image and substitute it by the blurred.
  FREE(m_Image);
  m_Image = BlurredImage;
  FREE(Kernel);

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// USM
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::USM(const uint8_t ChannelMask,
                      const double  Radius,
                      const double  Amount,
                      const double  Threshold) {

  assert ((ChannelMask == 1) || (m_ColorSpace != ptSpace_Lab));

  // Copy of the original message.
  uint16_t (*OriginalImage)[3] = NULL;

  // Copy original image
  OriginalImage = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(OriginalImage,__FILE__,__LINE__);
  memcpy(OriginalImage,m_Image,m_Width*m_Height*sizeof(*m_Image));

  // Blur the original. (this)
  ptCimgBlur(this, ChannelMask,Radius);

  // Combine the original and the blurred one.
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short c=0; c<3; c++) {

      // Is it a channel we are supposed to handle ?
      if  (! (ChannelMask & (1<<c))) continue;

      int32_t Delta = OriginalImage[i][c]-m_Image[i][c];
      if (abs(Delta) > Threshold*0xffff) {
        OriginalImage[i][c] = CLIP((int32_t)(OriginalImage[i][c]+Delta*Amount));
      }
    }
  }


  // Get rid of the (blurred) this and substitute it.
  // With the combined one
  FREE(m_Image);
  m_Image = OriginalImage;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// ViewLAB
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::ViewLAB(const short Channel) {

  assert (m_ColorSpace == ptSpace_Lab);

  const double WPH = 0x7fff; // WPH=WP/2

  ptImage *ContrastLayer = new ptImage;
  ptCurve* MyContrastCurve = new ptCurve();

  switch(Channel) {

    case ptViewLAB_L:
#pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case ptViewLAB_A:
#pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=m_Image[i][1];
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case ptViewLAB_B:
#pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0]=m_Image[i][2];
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    case ptViewLAB_L_Grad:
      ContrastLayer->Set(this);
      ptFastBilateralChannel(ContrastLayer, 4, 0.2, 2, 1);

#pragma omp parallel for default(shared) schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0])+m_Image[i][0]));
      }
      MyContrastCurve->SetCurveFromFunction(Sigmoidal,0.5,30);
      ApplyCurve(MyContrastCurve,1);

      //~ ptCimgEdgeDetection(this,1);
#pragma omp parallel for schedule(static)
      for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
        m_Image[i][1]=0x8080;
        m_Image[i][2]=0x8080;
      }
      break;

    default:
      break;
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
    if (Mode == ptSpecialPreview_L) ViewLAB(ptViewLAB_L);
    if (Mode == ptSpecialPreview_A) ViewLAB(ptViewLAB_A);
    if (Mode == ptSpecialPreview_B) ViewLAB(ptViewLAB_B);

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
      m_Image[i][0] = CLIP((int32_t) (m_Image[i][0]*0.3 +
        m_Image[i][1]*0.59 + m_Image[i][2]*0.11));
    }
    const double WPH = 0x7fff; // WPH=WP/2
    ptImage *ContrastLayer = new ptImage;
    ContrastLayer->Set(this);
    ptFastBilateralChannel(ContrastLayer, 4, 0.2, 2, 1);
#pragma omp parallel for default(shared) schedule(static)
    for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
      m_Image[i][0] = CLIP((int32_t) ((WPH-(int32_t)ContrastLayer->m_Image[i][0]) + m_Image[i][0]));
    }
    ptCurve* MyContrastCurve = new ptCurve();
    MyContrastCurve->SetCurveFromFunction(Sigmoidal,0.5,30);
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
      m_Image[i][0] = CLIP((int32_t) (m_Image[i][0]*0.3 +
        m_Image[i][1]*0.59 + m_Image[i][2]*0.11));
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

////////////////////////////////////////////////////////////////////////////////
//
// MixChannels
// MixFactors[To][From]
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::MixChannels(const double MixFactors[3][3]) {

  assert (m_ColorSpace != ptSpace_Lab);
  double Value[3];

#pragma omp parallel for default(shared) private(Value)
  for (uint32_t i=0; i<(uint32_t) m_Height*m_Width; i++) {
    for (short To=0; To<3; To++) {
       Value[To] = 0;
       for ( short From=0; From<3; From++) {
          Value[To] += MixFactors[To][From] * m_Image[i][From];
       }
    }
    for (short To=0; To<3; To++) {
      m_Image[i][To] = CLIP((int32_t)Value[To]);
    }
  }
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
    assert
      (m_Width == fwrite(PpmRow,m_Colors*BitsPerColor/8,m_Width,OutputFile));
  }

  FREE(PpmRow);
  FCLOSE(OutputFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// ReadPpm
//
// Remark : Extremely naive and not meant for production !
// (just enough for testcase support)
//
////////////////////////////////////////////////////////////////////////////////

short ptImage::ReadPpm(const char* FileName) {

  printf("(%s,%d) %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);

  FILE *InputFile = fopen(FileName,"rb");
  if (!InputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  short    Colors;
  uint16_t Width;
  uint16_t Height;
  uint16_t BitsPerColor;
  char     Buffer[128];

  // Extremely naive. Probably just enough for testcases.
  assert ( fgets(Buffer,127,InputFile) );
  assert ( 1 == sscanf(Buffer,"P%hd",&Colors) );
  assert(Colors == 6 );
  do {
    assert ( fgets(Buffer,127,InputFile) );
  } while (Buffer[0] == '#');
  sscanf(Buffer,"%hd %hd",&Width,&Height);
  assert ( fgets(Buffer,127,InputFile) );
  sscanf(Buffer,"%hd",&BitsPerColor);
  assert(BitsPerColor == 0xffff);

  m_Colors = 3;
  m_Width  = Width;
  m_Height = Height;

  // Free a maybe preexisting and allocate space.
  FREE(m_Image);
  m_Image = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(m_Image,__FILE__,__LINE__);

  uint16_t*  PpmRow = (uint16_t *) CALLOC(m_Width*m_Height,sizeof(*PpmRow));
  ptMemoryError(PpmRow,__FILE__,__LINE__);

  for (uint16_t Row=0; Row<m_Height; Row++) {
    size_t RV = fread(PpmRow,m_Colors*2,m_Width,InputFile);
    if (RV != (size_t) m_Width) {
      printf("ReadPpm error. Expected %d bytes. Got %d\n",m_Width,(int)RV);
      exit(EXIT_FAILURE);
    }
    if (htons(0x55aa) != 0x55aa) {
      swab((char *)PpmRow,(char *)PpmRow,m_Width*m_Colors*2);
    }
    for (uint16_t Col=0; Col<m_Width; Col++) {
      for (short c=0;c<3;c++) {
        m_Image[Row*m_Width+Col][c] = PpmRow[Col*m_Colors+c];
      }
    }
  }

  FREE(PpmRow);
  FCLOSE(InputFile);
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
// HorizontalSkew
//
////////////////////////////////////////////////////////////////////////////////

void ptImage::HorizontalSkew(uint16_t (*DestinationImage)[3],
                             const    uint16_t DestinationWidth,
                             const    uint16_t DestinationHeight,
                             const    uint16_t Row,
                             const    int16_t  SkewOffset,
                             const    double Weight) {
  assert(Row<DestinationHeight);

  uint16_t Left[3];
  uint16_t OldLeft[3];
  uint16_t Source[3];
  int32_t  i;

  // Left has to be read as in 'left over'.
  OldLeft[0] = 0;
  OldLeft[1] = 0;
  OldLeft[2] = 0;
  for (i = 0; i < m_Width; i++) {
    Source[0] = m_Image[Row*m_Width+i][0];
    Source[1] = m_Image[Row*m_Width+i][1];
    Source[2] = m_Image[Row*m_Width+i][2];
    Left[0] =  (uint16_t) (m_Image[Row*m_Width+i][0] * Weight);
    Left[1] =  (uint16_t) (m_Image[Row*m_Width+i][1] * Weight);
    Left[2] =  (uint16_t) (m_Image[Row*m_Width+i][2] * Weight);
    Source[0] -= Left[0] - OldLeft[0];
    Source[1] -= Left[1] - OldLeft[1];
    Source[2] -= Left[2] - OldLeft[2];
    if ((SkewOffset+i >= 0) && (SkewOffset+i < DestinationWidth)) {
      DestinationImage[Row*DestinationWidth+SkewOffset+i][0] = Source[0];
      DestinationImage[Row*DestinationWidth+SkewOffset+i][1] = Source[1];
      DestinationImage[Row*DestinationWidth+SkewOffset+i][2] = Source[2];
    }
    OldLeft[0] = Left[0];
    OldLeft[1] = Left[1];
    OldLeft[2] = Left[2];
  }
  // Go to rightmost point of skew
  i += SkewOffset;
  if (i<DestinationWidth) {
    DestinationImage[Row*DestinationWidth+i][0] = OldLeft[0];
    DestinationImage[Row*DestinationWidth+i][1] = OldLeft[1];
    DestinationImage[Row*DestinationWidth+i][2] = OldLeft[2];
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// VerticalSkew
// Inplace
//
////////////////////////////////////////////////////////////////////////////////

void ptImage::VerticalSkew(uint16_t (*DestinationImage)[3],
                           const uint16_t DestinationWidth,
                           const uint16_t DestinationHeight,
                           const uint16_t Column,
                           const int16_t  SkewOffset,
                           const double Weight) {

  assert(Column<DestinationWidth);

  uint16_t Left[3];
  uint16_t OldLeft[3];
  uint16_t Source[3];
  int32_t  i;

  // Left has to be read as in 'left over'.
  OldLeft[0] = 0;
  OldLeft[1] = 0;
  OldLeft[2] = 0;
  for (i = 0; i < m_Height; i++) {
    Source[0] = m_Image[i*m_Width+Column][0];
    Source[1] = m_Image[i*m_Width+Column][1];
    Source[2] = m_Image[i*m_Width+Column][2];
    Left[0] = (uint16_t) (m_Image[i*m_Width+Column][0] * Weight);
    Left[1] = (uint16_t) (m_Image[i*m_Width+Column][1] * Weight);
    Left[2] = (uint16_t) (m_Image[i*m_Width+Column][2] * Weight);
    Source[0] -= Left[0] - OldLeft[0];
    Source[1] -= Left[1] - OldLeft[1];
    Source[2] -= Left[2] - OldLeft[2];
    if ((SkewOffset+i >= 0) && (SkewOffset+i < DestinationHeight)) {
      DestinationImage[(i+SkewOffset)*DestinationWidth+Column][0] = Source[0];
      DestinationImage[(i+SkewOffset)*DestinationWidth+Column][1] = Source[1];
      DestinationImage[(i+SkewOffset)*DestinationWidth+Column][2] = Source[2];
    }
    OldLeft[0] = Left[0];
    OldLeft[1] = Left[1];
    OldLeft[2] = Left[2];
  }
  // Go to bottom point of skew
  i += SkewOffset;
  if (i<DestinationHeight) {
    DestinationImage[i*DestinationWidth+Column][0] = OldLeft[0];
    DestinationImage[i*DestinationWidth+Column][1] = OldLeft[1];
    DestinationImage[i*DestinationWidth+Column][2] = OldLeft[2];
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Rotate90 - Always in place !
// Dimensions are adapted to fit the rotation.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Rotate90() {

  uint16_t DestinationWidth  = m_Height;
  uint16_t DestinationHeight = m_Width;
  uint16_t (*DestinationImage)[3] = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);
  for (uint16_t y=0; y<m_Height; y++) {
    for (uint16_t x=0; x<m_Width; x++) {
      uint32_t DestinationIndex = (DestinationHeight-x-1)*DestinationWidth+y;
      uint32_t SourceIndex = y*m_Width+x;
      DestinationImage[DestinationIndex][0] = m_Image[SourceIndex][0];
      DestinationImage[DestinationIndex][1] = m_Image[SourceIndex][1];
      DestinationImage[DestinationIndex][2] = m_Image[SourceIndex][2];
    }
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Rotate180 - Always in place !
// Dimensions are adapted to fit the rotation.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Rotate180() {

  uint16_t DestinationWidth  = m_Width;
  uint16_t DestinationHeight = m_Height;
  uint16_t (*DestinationImage)[3] = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);
#pragma omp parallel for
  for (uint16_t y=0; y<m_Height; y++) {
    for (uint16_t x=0; x<m_Width; x++) {
      uint32_t DestinationIndex = (DestinationHeight-y-1)*DestinationWidth +
                                 DestinationWidth-x-1;
      uint32_t SourceIndex = y*m_Width+x;
      DestinationImage[DestinationIndex][0] = m_Image[SourceIndex][0];
      DestinationImage[DestinationIndex][1] = m_Image[SourceIndex][1];
      DestinationImage[DestinationIndex][2] = m_Image[SourceIndex][2];
    }
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  return this;
}


////////////////////////////////////////////////////////////////////////////////
//
// Rotate270 - Always in place !
// Dimensions are adapted to fit the rotation.
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Rotate270() {

  uint16_t DestinationWidth  = m_Height;
  uint16_t DestinationHeight = m_Width;
  uint16_t (*DestinationImage)[3] = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);
#pragma omp parallel for
  for (uint16_t y=0; y<m_Height; y++) {
    for (uint16_t x=0; x<m_Width; x++) {
      uint32_t DestinationIndex = x*DestinationWidth+DestinationWidth-y-1;
      uint32_t SourceIndex = y*m_Width+x;
      DestinationImage[DestinationIndex][0] = m_Image[SourceIndex][0];
      DestinationImage[DestinationIndex][1] = m_Image[SourceIndex][1];
      DestinationImage[DestinationIndex][2] = m_Image[SourceIndex][2];
    }
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Rotate45 (ie angle -45..+45) - Always in place !
// Dimensions are adapted to fit the rotation.
// (3 shear algorithm for quality)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Rotate45(double Angle) {

  if (0.0 == Angle) return this;

  double RadianAngle  = Angle * ptPI / double(180);
  double SinusAngle   = sin(RadianAngle);
  double TanHalfAngle = tan(RadianAngle/2.0);

  uint16_t OriginalWidth  = m_Width;
  uint16_t OriginalHeight = m_Height;

  // Calc first shear (horizontal) destination image dimensions
  uint16_t DestinationWidth  = m_Width + (int)( m_Height * fabs(TanHalfAngle) );
  uint16_t DestinationHeight = m_Height;

  uint16_t (*DestinationImage)[3] = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);

  /******* Perform 1st shear (horizontal) ******/

  for (uint16_t u = 0; u < DestinationHeight; u++) {
    double Shear;
    if (TanHalfAngle >= 0.0) {
      Shear = (double(u) + 0.5) * TanHalfAngle;
    } else {
      Shear = (double(u - DestinationHeight) + 0.5) * TanHalfAngle;
    }
    int16_t IntShear = (int16_t)(floor(Shear));
    HorizontalSkew(DestinationImage,
                   DestinationWidth,
                   DestinationHeight,
                   u,
                   IntShear,
                   Shear - double(IntShear));
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  /******* Perform 2nd shear  (vertical) ******/

  // Calc 2nd shear (vertical) destination image dimensions
  DestinationWidth  = m_Width;
  DestinationHeight = 1 + (uint16_t)(OriginalWidth * fabs(SinusAngle) +
                                     OriginalHeight * cos(RadianAngle) );

  DestinationImage = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);

  double Offset;     // Variable skew offset
  if (SinusAngle > 0.0) {
    Offset = double (OriginalWidth - 1) * SinusAngle;
  } else {
    Offset = -SinusAngle * double (OriginalWidth-DestinationWidth);
  }

  for (uint16_t u = 0; u < DestinationWidth; u++,Offset-=SinusAngle) {
    int16_t IntShear = (int16_t)(floor(Offset));
    VerticalSkew(DestinationImage,
                 DestinationWidth,
                 DestinationHeight,
                 u,
                 IntShear,
                 Offset-double(IntShear));
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  /******* Perform 3rd shear (horizontal) ******/

  // Calc 3rd shear (horizontal) destination image dimensions
  DestinationWidth = 1 + (int)(OriginalHeight * fabs(SinusAngle) +
                               OriginalWidth * cos(RadianAngle) );
  DestinationHeight = m_Height;
  DestinationImage = (uint16_t (*)[3])
    CALLOC(DestinationWidth*DestinationHeight,sizeof(*DestinationImage));
  ptMemoryError(DestinationImage,__FILE__,__LINE__);

  if (SinusAngle >= 0.0) {
    Offset = double(OriginalWidth - 1) * SinusAngle * -TanHalfAngle;
  } else {
    Offset = double(OriginalWidth - 1) * SinusAngle * -TanHalfAngle +
             DestinationHeight*-TanHalfAngle;

  }

  for (uint16_t u = 0; u < DestinationHeight; u++,Offset+=TanHalfAngle) {
    int16_t IntShear = (int16_t)(floor(Offset));
    HorizontalSkew(DestinationImage,
                   DestinationWidth,
                   DestinationHeight,
                   u,
                   IntShear,
                   Offset-double(IntShear));
  }

  // Take over the result.
  FREE(m_Image);
  m_Image  = DestinationImage;
  m_Width  = DestinationWidth;
  m_Height = DestinationHeight;

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Rotate - Always in place !
// Dimensions are adapted to fit the rotation.
// (3 shear algorithm for quality)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Rotate(double  Angle) {

  while (Angle >= 360.0) Angle -= 360.0;
  while (Angle < 0.0)    Angle += 360.0;
  if ((Angle > 45.0) && (Angle <= 135.0)) {
    Rotate90();
    Angle -= 90.0;
  } else if ((Angle > 135.0) && (Angle <= 225.0)) {
    Rotate180();
    Angle -= 180.0;
  } else if ((Angle > 225.0) && (Angle <= 315.0)) {
    Rotate270();
    Angle -= 270.0;
  }

  // Within -45..45 now !
#ifdef WIN32
  // ToDo: On windows the imagemagick rotation doesn't work right now,
  // so we use the "old" method, although it is less sharp.
  //~ Rotate45(Angle);
  if (Angle!=0)
    ptGMRotate(Angle);
#else
  if (Angle!=0)
    ptGMRotate(Angle);
#endif

  return this;
}

////////////////////////////////////////////////////////////////////////////////
//
// Refocus
//
// Taken (and slightly reworked) from DigiKam
// Copyright (C) 2005-2007 by Gilles Caulier
// (which in turn took it from http://refocus.sourceforge.net/)
//
////////////////////////////////////////////////////////////////////////////////

ptImage* ptImage::Refocus(const uint8_t ChannelMask,
                          const short   MatrixRadius,
                          const double  Radius,
                          const double  Gauss,
                          const double  Correlation,
                          const double  Noise) {

  assert ((ChannelMask == 1) || (m_ColorSpace != ptSpace_Lab));

  // Copy of the original message.
  uint16_t (*OriginalImage)[3] = NULL;

  // Copy original image
  OriginalImage = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*m_Image));
  ptMemoryError(OriginalImage,__FILE__,__LINE__);
  memcpy(OriginalImage,m_Image,m_Width*m_Height*sizeof(*m_Image));

  //
  ptCMat *Matrix = NULL;

  ptCMat CircleMatrix;
  ptCMat GaussianMatrix;
  ptCMat ConvolutionMatrix;

  ptRefocusMatrix::MakeGaussianConvolution(Gauss,&GaussianMatrix,MatrixRadius);
  ptRefocusMatrix::MakeCircleConvolution(Radius,&CircleMatrix,MatrixRadius);
  ptRefocusMatrix::InitCMatrix(&ConvolutionMatrix,MatrixRadius);
  ptRefocusMatrix::ConvolveStarMatrix(&ConvolutionMatrix,
                                      &GaussianMatrix,
                                      &CircleMatrix);
  Matrix = ptRefocusMatrix::ComputeGMatrix(&ConvolutionMatrix,
                                           MatrixRadius,
                                           Correlation,
                                           Noise,
                                           0.0,
                                           1);
  ptRefocusMatrix::FinishCMatrix(&ConvolutionMatrix);
  ptRefocusMatrix::FinishCMatrix(&GaussianMatrix);
  ptRefocusMatrix::FinishCMatrix(&CircleMatrix);


  const uint32_t ImageSize = m_Height*m_Width;

  const short MatrixSize   = 1+2*MatrixRadius;
  const short MatrixOffset = MatrixSize/2;

  double   Value[3];

  for (uint16_t Row=0; Row<m_Height; Row++) {
    for (uint16_t Col=0; Col<m_Width; Col++) {

      uint32_t Index = Row*m_Width+Col;

      for (short c=0; c<3; c++) {
        // Is it a channel we are supposed to handle ?
        if  (! (ChannelMask & (1<<c))) continue;
        Value[c] = 0;
        for(short y=0; y<MatrixSize; y++) {
          for(short x=0; x<MatrixSize; x++) {
            int32_t OtherIndex = Index+m_Width*(y-MatrixOffset)+x-MatrixOffset;
            if (OtherIndex >= 0 && (uint32_t)OtherIndex < ImageSize) {
              Value[c] += Matrix->Data[y*MatrixSize+x] *
                          OriginalImage[OtherIndex][c];
            }
          }
        }
        m_Image[Index][c] = CLIP((int32_t)Value[c]);
      }
    }
  }

  delete Matrix;
  FREE(OriginalImage);

  return this;
}

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

////////////////////////////////////////////////////////////////////////////////
