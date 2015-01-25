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

#ifndef DLIMAGE_H
#define DLIMAGE_H

#include "ptDefines.h"
#include "ptConstants.h"
#include "ptCurve.h"

#include <lensfun.h>

#include <vector>
#include <array>

//==============================================================================

class ptDcRaw;

//==============================================================================

// RGB type
struct RGBValue {
  uint16_t R;
  uint16_t G;
  uint16_t B;
};

// Channel masks.
enum TChannelMask: uint8_t {
  ChMask_R   = 1,
  ChMask_G   = 2,
  ChMask_B   = 4,
  ChMask_RGB = ChMask_R | ChMask_G | ChMask_B,
  ChMask_L   = 1,
  ChMask_a   = 2,
  ChMask_b   = 4,
  ChMask_Lab = ChMask_L | ChMask_a | ChMask_b
};

//==============================================================================

/*! Class containing an image and its operations. */
class ptImage {
private:
  inline TAnchorList createAmpAnchors(const double Amount, const double HaloControl);

  // The currently chosen RGB colorspace mode
  static short CurrentRGBMode;


public:
  // The container of the image, assumed 3 channels and 0..0xffff values.
  // Representation for RGB
  // [0] = R
  // [1] = G
  // [2] = B
  TImage16Data m_Data;

  // Pointer to the data buffer, since most algorithms still use that.
  uint16_t (*m_Image)[3];

  // LCH image data, m_Image is NULL when the image is in ptSpace_LCH
  std::vector<uint16_t> m_ImageL;
  std::vector<float>    m_ImageC;
  std::vector<float>    m_ImageH;

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

  // The currently chosen RGB colorspace mode
  static void  setCurrentRGB(const short ARGB);
  static short getCurrentRGB();

  // Constructor
  ptImage();

  // Destructor
  ~ptImage();

  // Allocates m_Data and sets m_Image to the buffer
  void setSize(size_t Size);

  // Initialize it via dcraw (from a DcRawObject).
  // By the way , the copying is always deep (and
  // might involve 4->3 color reduction.
  ptImage* Set(const ptDcRaw*  DcRawObject,
               const short   TargetSpace,
               const char*   ProfileName,
               const int     Intent,
               const int     ProfileGamma);
  // Through connect variant of above (DcRawObject->m_Image as RGB)
  ptImage* Set(const ptDcRaw*  DcRawObject,
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

  // Get the RGB at a given point
  RGBValue GetRGB(const uint16_t x, const uint16_t y);

  // Crop
  ptImage* Crop(const uint16_t X,
                const uint16_t Y,
                const uint16_t W,
                const uint16_t H);

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
  ptImage* lcmsRGBToPreviewRGB(const bool Fast = false);

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

  ptImage* toRGB();
  ptImage* toLab();
  
  /*! Converts the image from RGB to Lch colour space. Returns a pointer to itself. */
  ptImage* RGBToLch();

  /*! Converts the image from Lch to RGB colour space. Returns a pointer to itself. */
  ptImage* LchToRGB(const short To);

  /*! Converts the image from Lab to Lch colour space. Returns a pointer to itself. */
  ptImage* LabToLch();

  /*! Converts the image from Lch to Lab colour space. Returns a pointer to itself. */
  ptImage* LchToLab();

  /*!
   * Mix channels according to the factors in mixFactors.
   * mixFactors’s first dimension represents output channels (indexes 0 to 2 represent
   * 1st to 3rd color component); second dimension represents input channels.
   * Image must not be in Lab color space.
   */
  ptImage* mixChannels(const TChannelMatrix mixFactors);

  ptImage* Highlights(const float AHlRed, const float AHlGreen, const float AHlBlue);

  ptImage* GammaTool(const float AStrength, const float ALinearity);

  // Overlay
  ptImage* Overlay(uint16_t (*OverlayImage)[3],
                   const float Amount,
                   const float *Mask = NULL,
                   const short Mode = ptOverlayMode_SoftLight,
                   const short Swap = 0);

  // Flip
  ptImage* Flip(const short FlipMode);

  // Levels
  ptImage* Levels(const float BlackP,
                  const float WhiteP);

  // DeFringe
  ptImage* DeFringe(const double Radius,
                    const short Threshold,
                    const int Flags,
                    const double Shift);

  // Impulse noise reduction
  ptImage* DenoiseImpulse(const double ThresholdL,
                          const double ThresholdAB);

  // Reinhard 05
  ptImage* Reinhard05(const float Brightness,
                      const float Chromatic,
                      const float Light);

  ptImage* ColorIntensity(int AVibrance,
                          int ARed,
                          int AGreen,
                          int ABlue);

  // Color Boost
  ptImage* ColorBoost(const double ValueA,
                      const double ValueB);

  // LCH L Adjustment
  ptImage* LumaAdjust(const double LC1, // 8 colors for L
                      const double LC2,
                      const double LC3,
                      const double LC4,
                      const double LC5,
                      const double LC6,
                      const double LC7,
                      const double LC8);

  ptImage* SatAdjust(const double SC1, // 8 colors for saturation
                     const double SC2,
                     const double SC3,
                     const double SC4,
                     const double SC5,
                     const double SC6,
                     const double SC7,
                     const double SC8);

  // Outline
  ptImage* Outline(const short Mode,
                   const short GradientMode,
                   const ptCurve *Curve,
                   const double Weight,
                   const double Radius,
                   const short SwitchLayer);

  // Color Enhance
  ptImage* ColorEnhance(const float AShadows,
                        const float AHighlights);

  // LMHLightRecovery
  ptImage* LMHRecovery(const TMaskType MaskType,
                       const float Amount,
                       const float LowerLimit,
                       const float UpperLimit,
                       const float Softness);

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
                           const ptCurve *Curve = NULL);

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
                         const TMaskType MaskType,
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

  // Denoise curve
  ptImage* ApplyDenoiseCurve(const double Threshold,
                             const double Softness,
                             const ptCurve *MaskCurve);

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
                 const TMaskType MaskType,
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
                   const TMaskType MaskType = TMaskType::All,
                   const short  ManualMask = 0,
                   const double LowerLevel = 0.,
                   const double UpperLevel = 1.,
                   const double Softness = 0.);

  // Tone
  ptImage* Tone(const uint16_t R,
                const uint16_t G,
                const uint16_t B,
                const double   Amount,
                const TMaskType MaskType,
                const double   LowerLimit,
                const double   UpperLimit,
                const double   Softness);

  // Crossprocessing
  ptImage* Crossprocess(const short Mode,
            const double Color1,
            const double Color2);

  // Gradual Mask
  float *GetGradualMask(const double Angle,
                        const double LowerLevel,
                        const double UpperLevel,
                        const double Softness);

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
  ptImage* Softglow(const short   SoftglowMode,
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
  float *GetMask(const TMaskType MaskType,
                 const double LowerLimit,
                 const double UpperLimit,
                 const double Softness,
                 const double FactorR = 0.3,
                 const double FactorG = 0.59,
                 const double FactorB = 0.11);

// FillMask
float *FillMask(const uint16_t APointX,
                const uint16_t APointY,
                const float    AThreshold,
                const float    AColorWeight,
                const uint16_t AMaxRadius,
                const bool     AUseMaxRadius);

// MaskedContrast
ptImage* MaskedColorAdjust(const int       Ax,
                           const int       Ay,
                           const float     AThreshold,
                           const float     AChromaWeight,
                           const int       AMaxRadius,
                           const bool      AHasMaxRadius,
                           const ptCurve  *ACurve,
                           const bool      ASatAdaptive,
                           float           ASaturation,
                           const float     AHueShift);

  // GetVignetteMask
  float *GetVignetteMask(const short Inverted,
                         const short Exponent,
                         const double InnerRadius,
                         const double OuterRadius,
                         const double Roundness,
                         const double CenterX,
                         const double CenterY,
                         const double Softness);

  // GradualBlur
  ptImage* GradualBlur(const int    Mode,
                       const double MaxRadius,
                       const double LowerLevel,
                       const double UpperLevel,
                       const double Softness,
                       const double Angle,
                       const int    Vignette,
                       const double Roundness,
                       const double CenterX,
                       const double CenterY);

  // Box
  ptImage* Box(const uint16_t MaxRadius, float* Mask);

  // Apply Refocus sharpening
  /* TODO: Filter needs work to make it useful. Disabled for now to remove
    dependency on clapack.
  ptImage* Refocus(const uint8_t ChannelMask,
                   const short   MatrixSize,
                   const double  Radius,
                   const double  Gauss,
                   const double  Correlation,
                   const double  Noise);
  */

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

  ptImage* ApplyHueCurve(const ptCurve *Curve);

  ptImage* ApplySaturationCurve(const ptCurve *Curve,
                                const short Mode);

  ptImage* ApplyTextureCurve(const ptCurve *Curve,
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

  // Write the image as a jpeg file.
  // Be aware : the writing function does *not* add gamma, thus
  // it needs to be done before if needed.
  // Quality : 0..100
  short WriteAsJpeg(const char*    FileName,
                    const short    Quality,
                    const uint8_t* ExifBuffer    = NULL,
                    const unsigned ExifBufferLen = 0);

  // ptImage_DRC.cpp
  ptImage* DRC(const float alpha,
               const float beta,
               const float color);

  // ptImage_EAW.cpp
  ptImage* EAWChannel(const double scaling,
                      const double level1,
                      const double level2,
                      const double level3,
                      const double level4,
                      const double level5,
                      const double level6);

  /* not used atm
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
  */
  ptImage* ptGMResize(const uint16_t Size,
                      const uint16_t Height,
                      const short Filter,
                      const short Mode);
  ptImage* ptGMResizeWH(const uint16_t NewWidth,
                        const uint16_t NewHeight,
                        const short Filter);
  /* not used atm
  ptImage* ptGMBlur(const double Radius);
  */
  ptImage* ptGMUnsharp(const double Radius, const double Amount, const double Threshold);

  ptImage* ptGMNormalize(const double Opacity);


  // ptImage_GMC.cpp
  bool ptGMCWriteImage(const char* FileName,
                       const short Format,
                       const int Quality,
                       const int Sampling,
                       const int Resolution,
                       const char* ColorProfileFileName,
                       const int Intent);

  bool DumpImage(const char* FileName) const;

  ptImage* ptGMCOpenImage(const char*        FileName,
                          short              ColorSpace,
                          short              Intent,
                          short              ScaleFactor,
                          bool               IsRAW,
                          TImage8RawData*    ImgData,
                          int&               Success);

  // ptImage_Pyramid.cpp
  ptImage* dirpyrLab_denoise(const int luma,
                             const int chroma,
                             const double gamma,
                             const int levels);

  // ptImage_Cimg.cpp
  ptImage* ptCIBlur(const double Sigma, const short ChannelMask = 7);
  ptImage* ptCIDeriche(const float sigma,
                       const int order = 0,
                       const char axis = 'x',
                       const bool cond = true,
                       const short ChannelMask = 7);

  ptImage* ptCIPerspective(const float RotateAngle,
                           const float FocalLength,
                           const float TiltAngle,
                           const float TurnAngle,
                           const float ScaleX,
                           const float ScaleY);

  // ptImage_Lqr.cpp
  ptImage* LiquidRescaleRelative(const double HorScale,
                                 const double VertScale,
                                 const short Energy,
                                 const short VertFirst);

  ptImage* LiquidRescale(const uint16_t Width,
                         const uint16_t Height,
                         const short Energy,
                         const short VertFirst);

  // ptImage.cpp
  ptImage* fastBilateralChannel(
      const float Sigma_s,
      const float Sigma_r,
      const int Iterations = 1,
      const TChannelMask ChannelMask = ChMask_L);

  /**
   * ptImage_Lensfun.cpp
   * Wrapper for the actual execution of lensfun processing. Returns the stage 2 (vignetting)
   * corrected image. Returns stage 1/3 converted pixel coordinates in TransformedCoords.
   *
   * LfunActions
   *     A bitmask of LF_MODIFY_XX values stating which image corrections to perform.
   *
   * LfunData
   *     A pointer to an lfModifier object containing the data for all corrections. Must be
   *     properly instantiated and initialised before calling Lensfun().
   */
  ptImage* Lensfun(const int LfunActions,
                   const lfModifier* LfunData);

private:
  void ResizeLCH(size_t ASize);
};

#endif
