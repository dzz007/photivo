////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLIMAGE_H
#define DLIMAGE_H

#include "ptDefines.h"
#include "ptConstants.h"
#include "ptDcRaw.h"

// A forward declaration to the curve class.

class ptCurve;

// Class containing an image and its operations.

class ptImage {
public:

// The image , assumed 3 channels and 0..0xffff values.
// Representation for RGB
// [0] = R
// [1] = G
// [2] = B
uint16_t (*m_Image)[3];

// Width and height of the image
uint16_t m_Width;
uint16_t m_Height;

// Nr of colors in the image (probably always 3 ?)
short m_Colors;

// Color space.
// Can be one of
//   ptSpace_sRGB_D65         1
//   ptSpace_AdobeRGB_D65     2
//   ptSpace_WideGamutRGB_D65 3
//   ptSpace_ProPhotoRGB_D65  4
//   ptSpace_LAB              10
//   ptSpace_XYZ              11
short m_ColorSpace;

// Constructor
ptImage();

// Destructor
~ptImage();

// Initialize it via dcraw (from a DcRawObject).
// By the way , the copying is always deep (and
// might involve 4->3 color reduction.
ptImage* Set(const DcRaw*  DcRawObject,
             const short   TargetSpace,
             const char*   ProfileName,
             const int     Intent,
             const int     ProfileGamma);
// Through connect variant of above (DcRawObject->m_Image as RGB)
ptImage* Set(const DcRaw*  DcRawObject,
             const short   TargetSpace);

// Just allocation
ptImage* Set(const uint16_t Width,
             const uint16_t Height);

// Initialize it from another image.
// Copying is always deep (so including copying the image).
ptImage* Set(const ptImage *Origin);

// Copy from another image and scale to pipe size.
ptImage* SetScaled(const ptImage *Origin,
                   const short ScaleFactor);

// Resize such that the maximum dimension becomes Size.
// Typically the resizing is done in place, but one
// can request a new ptImage object to be created
// by setting InPlace to 0.
// The basis resize is a bilinear interpolation.
ptImage* SimpleResize(const uint16_t Size,
                      const short    InPlace=1);
// Following filtered resizes are done in floats and are equally
// or better performant then above on machines with FPU.
// At least if the lobes are restricted to 1 or 2 such as in Triangle
// or Mitchell
ptImage* FilteredResize(const uint16_t Size,
                        const short    ResizeFilter = ptResizeFilter_Triangle,
                        const short    InPlace = 1);

// Crop
ptImage* Crop(const uint16_t X,
              const uint16_t Y,
              const uint16_t W,
              const uint16_t H,
              const short    InPlace=1);

// A bunch of color space conversion functions.
// lcms ones are going via the lcms library, the others via matrices.
// The origin space is implicit in the class.
ptImage* RGBToRGB(const short To,
                  const short EvenIfEqual=0);
ptImage* lcmsRGBToRGB(const short To,
                      const short EvenIfEqual = 0,
                      const int   Intent = INTENT_PERCEPTUAL);
ptImage* lcmsRGBToRGB(cmsHPROFILE OutProfile, //with ICC profile
                      const int   Intent = INTENT_PERCEPTUAL,
                      const short Quality = ptCMQuality_HighResPreCalc);
ptImage* lcmsRGBToPreviewRGB();

ptImage* RGBToXYZ();
ptImage* lcmsRGBToXYZ(const int Intent = INTENT_PERCEPTUAL);
ptImage* XYZToRGB(const short To);
ptImage* lcmsXYZToRGB(const short To,
                      const int   Intent = INTENT_PERCEPTUAL);
ptImage* RGBToLab();
ptImage* lcmsRGBToLab(const int Intent = INTENT_PERCEPTUAL);
ptImage* LabToRGB(const short To);
ptImage* lcmsLabToRGB(const short To,
                      const int Intent = INTENT_PERCEPTUAL);

// MixChannels
// MixFactors[To][From]
ptImage* MixChannels(const double MixFactors[3][3]);

// Overlay
ptImage* Overlay(uint16_t (*OverlayImage)[3],
                 const double   Amount,
                 const float *Mask = NULL,
                 const short Mode = ptOverlayMode_SoftLight,
                 const short Swap = 0);

// Flip
ptImage* Flip(const short FlipMode);

// Levels
ptImage* Levels(const double BlackP,
                const double WhiteP);

// DeFringe
ptImage* DeFringe(const double Radius,
                  const short Threshold,
                  const int Flags,
                  const double Shift);

// Impulse noise reduction
ptImage* DenoiseImpulse(const double ThresholdL,
                        const double ThresholdAB);

// Reinhard 05
ptImage* Reinhard05(const double Brightness,
                    const double Chromatic,
                    const double Light);

// Color Boost
ptImage* ColorBoost(const double ValueA,
                    const double ValueB);

// LCH L Adjustment
ptImage* LAdjust(const double LC1, // 8 colors for L
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
                 const double SC8);

// Color Enhance
ptImage* ColorEnhance(const double Shadows,
                      const double Highlights);

// LMHLightRecovery
ptImage* LMHLightRecovery(const short   MaskType,
                          const double  Amount,
                          const double  LowerLimit,
                          const double  UpperLimit,
                          const double  Softness);

// Highpass
ptImage* Highpass(const double Radius,
                  const double Amount,
                  const double HaloControl,
                  const double Denoise);

// Gradient Sharpen
ptImage* GradientSharpen(const short Passes,
                         const double Strength);

ptImage* MLMicroContrast(const double Strength,
                         const double Scaling,
                         const double Weight,
                         const ptCurve *Curve = NULL,
                         const short Type = 0);

// Hotpixel
ptImage* HotpixelReduction(const double Threshold);

// Shadows Highlights
ptImage* ShadowsHighlights(const ptCurve *Curve,
                           const double Radius,
                           const double AmountCoarse,
                           const double AmountFine);

// Microcontrast
ptImage* Microcontrast(const double Radius,
                       const double Amount,
                       const double Opacity,
                       const double HaloControl,
                       const short MaskType,
                       const double LowerLimit,
                       const double UpperLimit,
                       const double Softness);

// Colorcontrast
ptImage* Colorcontrast(const double Radius,
                       const double Amount,
                       const double Opacity,
                       const double HaloControl);

// Bilateral Denosie
ptImage* BilateralDenoise(const double Threshold,
                          const double Softness,
                          const double Opacity,
                          const double UseMask = 0);

// Texturecontrast
ptImage* TextureContrast(const double Threshold,
                         const double Softness,
                         const double Amount,
                         const double Opacity,
                         const double EdgeControl,
                         const double Masking = 0.0);

// Localcontrast
ptImage* Localcontrast(const int Radius1,
                       const double Opacity,
                       const double m,
                       const double Feather,
                       const short Method);

// Grain
ptImage* Grain(const double Sigma,
               const short NoiseType,
               const double Radius,
               const double Opacity,
               const short MaskType,
               const double LowerLimit,
               const double UpperLimit,
               const short ScaleFactor);

// I wanted to add some film and color filter simulation, but I didn't
// want to use the numbers flying around in the internet, because I couldn't
// find any real justification for them. So I asked a good friend of mine,
// Dr. Herbert Kribben (Photo-Designer), he had some sensitivity sheets from
// the old days, for most of the fancy films and estimated the values from them.
// It turned out, that many film were equivalently responding to the different colors,
// so we decided to put them in reasonable groups... But he also made clear,
// from a technical point of view, it's completely impossible to fully simulate
// the behaviour of a film, because the camera records to little spectral information.
// Further discussion welcome! Mike.
// Black&White Styler
ptImage* BWStyler(const short FilmType,
                  const short ColorFilterType,
                  const double MultR,
                  const double MultG,
                  const double MultB,
                  const double Opacity = 1.0);

// Simple toning
ptImage* SimpleTone(const double R,
                    const double G,
                    const double B);

// Temperature
ptImage* Temperature(const double Temperature,
                     const double Tint);

// LAB Transform
ptImage* LABTransform(const short Mode);

// LABTone
ptImage* LABTone(const double Amount,
                 const double Hue,
                 const double Saturation = 0,
                 const short MaskType = ptMaskType_All,
                 const short ManualMask = 0,
                 const double LowerLevel = 0.,
                 const double UpperLevel = 1.,
                 const double Softness = 0.);

// Tone
ptImage* Tone(const uint16_t R,
              const uint16_t G,
              const uint16_t B,
              const double   Amount,
              const short    MaskType,
        const double   LowerLimit,
        const double   UpperLimit,
        const double   Softness);

// Crossprocessing
ptImage* Crossprocess(const short Mode,
          const double Color1,
          const double Color2);


// Gradual Overlay
ptImage* GradualOverlay(const uint16_t R,
            const uint16_t G,
            const uint16_t B,
            const short Mode,
            const double Amount,
            const double Angle,
            const double LowerLevel,
            const double UpperLevel,
            const double Softness);

// Vignette
ptImage* Vignette(const short VignetteMode,
          const short Exponent,
          const double Amount,
          const double InnerRadius,
          const double OuterRadius,
          const double Roundness,
          const double CenterX,
          const double CenterY,
          const double Softness);

// Softglow
ptImage* Softglow(const short SoftglowMode,
      const double  Radius,
                  const double  Amount,
      const uint8_t ChannelMask = 7,
      const double  Contrast = 5,
      const int     Saturation = -50);

// GetMask
// The FactorR/G/B values refer to the channelmixing for a black
// and white image, which will be the basis for the mask.
// 30/59/11 seem to be a standard triple representing the luminance,
// but this should be color space dependend, so TODO
// For an image in LAB choose 1/0/0.
float *GetMask(const short  MaskType,
               const double LowerLimit,
               const double UpperLimit,
               const double Softness,
               const double FactorR = 0.3,
               const double FactorG = 0.59,
               const double FactorB = 0.11);

// GetVignetteMask
float *GetVignetteMask(const short Inverted,
                       const short Exponent,
                       const double InnerRadius,
                       const double OuterRadius,
                       const double Roundness,
                       const double CenterX,
                       const double CenterY,
                       const double Softness);

// Blur the image. Probably not used standalone, but
// it is part of the USM algorithm.
//   ChannelMask : has a '1' on the bitposition of the channel that needs
//                 to be operated on. Typical 7 for RGB, 1 for Lab
//   Radius      : The radius of the Gaussian kernel.
//                 In pixels, not counting the center pixel.
//   Sigma       : The standard deviation of the Gaussian, in pixels.
ptImage* Blur(const uint8_t ChannelMask,
              const double  Radius,
              const double  Sigma);

// Apply Unsharp Sharp Mask (USM) sharpening.
//   ChannelMask : has a '1' on the bitposition of the channel that needs
//                 to be operated on. Typical 7 for RGB, 1 for Lab
//   Radius      : The radius of the Gaussian kernel.
//   Sigma       : The standard deviation of the Gaussian, in pixels.
//   Amount      : The percentage of the difference between the original and the
//                 blur image that is added back into the original.
//   Threshold   : The threshold in pixels needed to apply the diffence amount.
ptImage* USM(const uint8_t ChannelMask,
             const double  Radius,
             const double  Amount,
             const double  Threshold);

// Apply Refocus sharpening
ptImage* Refocus(const uint8_t ChannelMask,
                 const short   MatrixSize,
                 const double  Radius,
                 const double  Gauss,
                 const double  Correlation,
                 const double  Noise);

// Wavelet denoise
ptImage* WaveletDenoise(const uint8_t  ChannelMask,
                        const double Threshold,
                        const double low,
                        const short WithMask = 1,
                        const double Sharpness = 1.0,
                        const double Anisotropy = 0.2,
                        const double Alpha = 1.0,
                        const double Sigma = 1.1);

// View LAB
ptImage* ViewLAB(const short Channel);

// Special Preview
ptImage* SpecialPreview(const short Mode, const int Intent = 0);

// Apply a curve to an image.
//   ChannelMask : has a '1' on the bitposition of the channel that needs
//                 to be operated on. Typical 7 for RGB, 1 for LAB on L
ptImage* ApplyCurve(const ptCurve *Curve,
                    const uint8_t ChannelMask);

ptImage* ApplyLByHueCurve(const ptCurve *Curve);

ptImage* ApplyHueCurve(const ptCurve *Curve,
                       const short Type);

ptImage* ApplySaturationCurve(const ptCurve *Curve,
                              const short Mode,
                              const short Type);

ptImage* ApplyTextureCurve(const ptCurve *Curve,
                           const short Type,
                           const short Scaling);

// Sigmoidal Contrast
ptImage* SigmoidalContrast(const double Contrast,
                           const double Threshold,
                           const short ChannelMask = 7);

// Indicate exposure
ptImage* IndicateExposure(const short    Over,
                          const short    Under,
                          const uint8_t  ChannelMask,
                          const uint16_t OverExposureLevel[3],
                          const uint16_t UnderExposureLevel[3]);

ptImage* IndicateExposure(ptImage* ValueImage,
                          const short    Over,
                          const short    Under,
                          const uint8_t  ChannelMask,
                          const uint16_t OverExposureLevel[3],
                          const uint16_t UnderExposureLevel[3]);

// Expose (multiply) an image.
ptImage* Expose(const double Exposure,
                const short  ExposureClipMode);

// Calculates the level in the image where Fraction of the pixels is above.
uint16_t CalculateFractionLevel(const double  Fraction,
                                const uint8_t ChannelMask);

// Write the image as a ppm file.
//   BitsPerColor can be 8 or 16.
// Be aware : the writing function does *not* add gamma, thus
// it needs to be done before if needed.
short WriteAsPpm(const char*  FileName,
                 const short  BitsPerColor=8);
// Just enough to testcase with above. Don't use it in production.
short ReadPpm(const char* FileName);


// Write the image as a jpeg file.
// Be aware : the writing function does *not* add gamma, thus
// it needs to be done before if needed.
// Quality : 0..100
short WriteAsJpeg(const char*    FileName,
                  const short    Quality,
                  const uint8_t* ExifBuffer    = NULL,
                  const unsigned ExifBufferLen = 0);

// Rotate by shear members.
ptImage* Rotate(double Angle);
ptImage* Rotate45(double Angle);
ptImage* Rotate90();
ptImage* Rotate180();
ptImage* Rotate270();
void HorizontalSkew(      uint16_t (*DestinationImage)[3],
                    const uint16_t DestinationWidth,
                    const uint16_t DestinationHeight,
                    const uint16_t Row,
                    const int16_t  SkewOffset,
                    const double   Weight);
void VerticalSkew(      uint16_t (*DestinationImage)[3],
                  const uint16_t DestinationWidth,
                  const uint16_t DestinationHeight,
                  const uint16_t Column,
                  const int16_t  SkewOffset,
                  const double   Weight);

// ptImage_DRC.cpp
ptImage* DRC(const double alpha,
             const double beta,
             const double color);

// ptImage_EAW.cpp
ptImage* EAWChannel(const double scaling,
                    const double level1,
                    const double level2,
                    const double level3,
                    const double level4,
                    const double level5,
                    const double level6);

// ptImage_GM.cpp
ptImage* ptGMRotate(const double Angle);

int ptGMIdentify(const char* FileName,
                 uint16_t& InputWidth,
                 uint16_t& InputHeight);

ptImage* ptGMOpenImage(const char* FileName,
                       const short ColorSpace,
                       const short Intent,
                       const short ScaleFactor,
                       int& Success);

ptImage* ptGMSimpleOpen(const char* FileName);

ptImage* ptGMWriteImage(const char* FileName,
                        const short Format,
                        const int Quality,
                        const char* ColorProfileFileName,
                        const int Intent);

ptImage* ptGMResize(uint16_t Size, const short Filter);
ptImage* ptGMResize(uint16_t NewWidth, uint16_t NewHeight, const short Filter);

ptImage* ptGMBlur(const double Radius);

ptImage* ptGMUnsharp(const double Radius, const double Amount, const double Threshold);

ptImage* ptGMNormalize(const double Opacity);

// ptImage_GMC.cpp
ptImage* ptGMCWriteImage(const char* FileName,
                         const short Format,
                         const int Quality,
                         const int Sampling,
                         const int Resolution,
                         const char* ColorProfileFileName,
                         const int Intent);

// ptImage_Pyramid.cpp
ptImage* dirpyrLab_denoise(const int luma,
                           const int chroma,
                           const double gamma,
                           const int levels);

};

#endif
////////////////////////////////////////////////////////////////////////////////
