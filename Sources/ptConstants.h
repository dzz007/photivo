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

#ifndef PTCONSTANTS_H
#define PTCONSTANTS_H

//==============================================================================

#include <lcms2.h>
#include <lensfun.h>

#include <QString>

//==============================================================================

/* !!!
  IMPORTANT: Photivo uses groups of const short for historical reasons.
  They are deprecated for new constants groups! Instead use enums or, even better,
  C++11 enum classes.
  Up to at least v2.5 beta Qt Creator does not support enum classes properly.
  Do not let red error underlining impress you. Compiling works fine. ;)
!!! */

//==============================================================================

const char ProgramName[] = "Photivo";
const char CompanyName[] = "http://photivo.org/";

const int PhotivoSettingsVersion = 1;

struct ptFiles {
  QString GlobalIni;
  QString TagsIni;
};
const ptFiles PhotivoFile = { "photivo.ini",
                              "tags.ini"};

//==============================================================================

// Mathematical constants.
const double ptPI     = 3.14159265358979323846264338327950288419716939937510;
const double ptSQ2PI  = 2.50662827463100024161235523934010416269302368164062;
const float  pt2PI    = 6.28318530717f;

// Some program limits.
const short ptMaxAnchors    = 50; // Curve anchors.
const short ptMaxInputFiles = 2048;

// White point
const uint16_t ptWP    = 0xffff;
const float    ptWPf   = (float)ptWP;
const float    ptInvWP = 1.0f/(float)ptWP;

const uint16_t ptWPH   = 0x7fff;
const float    ptWPHf  = (float)ptWPH;

// Neutral AB value
const float ptWPHLab = 0x8080;

/*! Processor phases.
    Don't mess with the numbers of any of those constants.
    Often they are relied upon, for instance as index in an array.
    Or the numbers are assumptions from dcraw.
*/
const short ptProcessorPhase_Raw           = 1;   // dcraw
  // subphases of Raw
  const short ptProcessorPhase_Load          = 1; // Same constant, subphase !
  const short ptProcessorPhase_Demosaic      = 2; // Same constant, subphase !
  const short ptProcessorPhase_Highlights    = 3; // Same constant, subphase !
const short ptProcessorPhase_LocalEdit     = 2;
const short ptProcessorPhase_Geometry      = 3;
const short ptProcessorPhase_RGB           = 4;
const short ptProcessorPhase_LabCC         = 5;
const short ptProcessorPhase_LabSN         = 6;
const short ptProcessorPhase_LabEyeCandy   = 7;
const short ptProcessorPhase_EyeCandy      = 8;
const short ptProcessorPhase_Output        = 9;
// special phases, not your usual pipe run
const short ptProcessorPhase_Preview       = 10; // escape characters
const short ptProcessorPhase_OnlyHistogram = 11;
const short ptProcessorPhase_WriteOut      = 12;
const short ptProcessorPhase_ToGimp        = 13;

// if stop is set, we have no console output of processing, to prevent
// spamming while crop preview
enum class ptProcessorStopBefore {
  NoStop       = 0,
  SpotTuning   = 1,
  SpotRepair   = 2,
  Rotate       = 3,
  Crop         = 4
};

// Processor modes.

const short ptProcessorMode_Preview     = 0;
const short ptProcessorMode_Full        = 1;
const short ptProcessorMode_Thumb       = 2;

// Run modes.

const short ptRunMode_None     = 0;
const short ptRunMode_Once     = 1;
const short ptRunMode_Auto     = 2;

// Color spaces.

const short ptSpace_sRGB_D65         = 1;
const short ptSpace_AdobeRGB_D65     = 2;
const short ptSpace_WideGamutRGB_D50 = 3;
const short ptSpace_ProPhotoRGB_D50  = 4;
const short ptSpace_Lab              = 10;
const short ptSpace_XYZ              = 11;
const short ptSpace_LCH              = 15;
const short ptSpace_Profiled         = 20;

// Color profiles.

const short ptCameraColor_Flat              = 0;
const short ptCameraColor_Adobe_Matrix      = 1;
const short ptCameraColor_Adobe_Profile     = 2;
const short ptCameraColor_Embedded          = 3;
const short ptCameraColor_Profile           = 4;

const short ptCameraColorGamma_None     = 0;
const short ptCameraColorGamma_sRGB     = 1;
const short ptCameraColorGamma_BT709    = 2;
const short ptCameraColorGamma_Pure22   = 3;

const short ptScreenColor_Profile    = 20;/* Avoid collision with RGB or LAB */
const short ptOutputColor_Profile    = 21;/* Avoid collision with RGB or LAB */

const short ptCMQuality_NoOptimize         = 0;
const short ptCMQuality_HighResPreCalc     = 1;
const short ptCMQuality_FastSRGB           = 2;

// Size of the pipe.

const short ptPipeSize_Thirtyfour    = 5;
const short ptPipeSize_Sixteen       = 4;
const short ptPipeSize_Eighth        = 3;
const short ptPipeSize_Quarter       = 2; // Relying on this values for shift.
const short ptPipeSize_Half          = 1;
const short ptPipeSize_Full          = 0;

// Way of viewing.

const short ptPreviewMode_End        = 0;
const short ptPreviewMode_Tab        = 1;

// Histogram Channels

const short ptHistogramChannel_R     = 1; // Be aware : encoded values !
const short ptHistogramChannel_G     = 2;
const short ptHistogramChannel_B     = 4;
const short ptHistogramChannel_RGB   = 7;

// Histogram Gamma Mode
const short ptHistogramMode_Linear  = 0;
const short ptHistogramMode_Preview = 1;
const short ptHistogramMode_Output  = 2;

// Style
const short ptStyle_None            = 0;
const short ptStyle_Normal          = 1;
const short ptStyle_50Grey          = 2;
const short ptStyle_DarkGrey        = 3;
const short ptStyle_VeryDark        = 4;

// Style highlight color
const short ptStyleHighLight_White    = 0;
const short ptStyleHighLight_Purple   = 1;
const short ptStyleHighLight_Blue     = 2;
const short ptStyleHighLight_Green    = 3;
const short ptStyleHighLight_Orange   = 4;

// Starup UI mode
const short ptStartupUIMode_Tab       = 0;
const short ptStartupUIMode_Favourite = 1;
const short ptStartupUIMode_AllTools  = 2;

// local adjust modes
enum ptLocalAdjustMode {
  lamFloodFill    = 0,
  lamSearch       = 1
};

// Lensfun
const short ptLfunCAModel_None        = LF_TCA_MODEL_NONE;
const short ptLfunCAModel_Linear      = LF_TCA_MODEL_LINEAR;
const short ptLfunCAModel_Poly3       = LF_TCA_MODEL_POLY3;

const short ptLfunVignetteModel_None  = LF_VIGNETTING_MODEL_NONE;
const short ptLfunVignetteModel_Poly6 = LF_VIGNETTING_MODEL_PA;

const short ptLfunGeo_Unknown         = LF_UNKNOWN;
const short ptLfunGeo_Rectilinear     = LF_RECTILINEAR;
const short ptLfunGeo_Fisheye         = LF_FISHEYE;
const short ptLfunGeo_Panoramic       = LF_PANORAMIC;
const short ptLfunGeo_Equirectangular = LF_EQUIRECTANGULAR;

const short ptLfunDistModel_None      = LF_DIST_MODEL_NONE;
const short ptLfunDistModel_Poly3     = LF_DIST_MODEL_POLY3;
const short ptLfunDistModel_Poly5     = LF_DIST_MODEL_POLY5;
const short ptLfunDistModel_Fov1      = LF_DIST_MODEL_FOV1;
const short ptLfunDistModel_PTLens    = LF_DIST_MODEL_PTLENS;


// White balances.

const short ptWhiteBalance_Camera    = 0;
const short ptWhiteBalance_Auto      = 1;
const short ptWhiteBalance_Spot      = 2;
const short ptWhiteBalance_Manual    = 3;

// CA correction

const short ptCACorrect_Off          = 0;
const short ptCACorrect_Auto         = 1;
const short ptCACorrect_Manual       = 2;

// Interpolations.

const short ptInterpolation_Linear   = 0;
const short ptInterpolation_VNG      = 1;
const short ptInterpolation_VNG4     = 2;
const short ptInterpolation_PPG      = 3;
const short ptInterpolation_AHD      = 4;
const short ptInterpolation_DCB      = 5;
const short ptInterpolation_DCBSoft  = 6;
const short ptInterpolation_DCBSharp = 7;
const short ptInterpolation_AHD_mod  = 8;
const short ptInterpolation_VCD      = 9;
const short ptInterpolation_LMMSE    = 10;
const short ptInterpolation_AMaZE    = 11;
const short ptInterpolation_Bayer    = 12;

// Bayer Denoise

const short ptBayerDenoise_None       = 0;
const short ptBayerDenoise_FBDD1      = 1;
const short ptBayerDenoise_FBDD2      = 2;

// Clip modes;
const short ptClipMode_Clip          = 0;
const short ptClipMode_NoClip        = 1;
const short ptClipMode_Lab           = 2;
const short ptClipMode_HSV           = 3;
const short ptClipMode_Blend         = 4;
const short ptClipMode_Rebuild       = 5;

// Aspect Ratio modes

const short ptAspectRatio_Free      = 0;
const short ptAspectRatio_Original  = 1;
const short ptAspectRatio_Manual    = 2;

// Crop/Selection rectangle guidelines modes

const short ptGuidelines_None        = 0;
const short ptGuidelines_RuleThirds  = 1;
const short ptGuidelines_GoldenRatio = 2;
const short ptGuidelines_Diagonals   = 3;
const short ptGuidelines_Centerlines = 4;

// Lights Out modes

const short ptLightsOutMode_None    = 0;
const short ptLightsOutMode_Dimmed  = 1;
const short ptLightsOutMode_Black   = 2;

enum class TExposureClipMode: int {
  Hard,
  Ratio,
  FilmCurve
};

// Curves.

// Beware the “channel”. It’s a historical name and not accurate anymore. Read: “curve type”
const short ptCurveChannel_RGB               = 0;
const short ptCurveChannel_R                 = 1;
const short ptCurveChannel_G                 = 2;
const short ptCurveChannel_B                 = 3;
const short ptCurveChannel_L                 = 4;
const short ptCurveChannel_a                 = 5;
const short ptCurveChannel_b                 = 6;
const short ptCurveChannel_Saturation        = 7;
const short ptCurveChannel_Base              = 8;
const short ptCurveChannel_Base2             = 9;
const short ptCurveChannel_LByHue            = 10;
const short ptCurveChannel_Texture           = 11;
const short ptCurveChannel_ShadowsHighlights = 12;
const short ptCurveChannel_Denoise           = 13;
const short ptCurveChannel_Hue               = 14;
const short ptCurveChannel_Denoise2          = 15;
const short ptCurveChannel_Outline           = 16;
const short ptCurveChannel_SpotLuma          = 17;

const short ptCurveType_Full         = 0;
const short ptCurveType_Anchor       = 1;

const short ptCurveChoice_None       = 0;
const short ptCurveChoice_Manual     = 1;
const short ptCurveChoice_File       = 2;

// Curve Interpolation Type

const short ptCurveIT_Spline = 0;
const short ptCurveIT_Linear = 1;
const short ptCurveIT_Cosine = 2;

enum class TGreyCInterpol: int {
  NearestNeighbour,
  Linear,
  RungeKutta
};

enum class TGreyCDenoiseMask: int {
  All,
  Shadows1,
  Shadows2,
  Shadows3,
  Shadows4,
  Shadows5
};

// FlipModes

const short ptFlipMode_None       = 0;
const short ptFlipMode_Horizontal = 1;
const short ptFlipMode_Vertical   = 2;

enum class TGradualBlurMode: int {
  // There used to be an unused "none" mode with value 0. For settings compatibility
  // values need to start at 1.
  Linear       = 1,
  Vignette     = 2,
  LinearMask   = 3,
  VignetteMask = 4
};

enum class TGrainType: int {
  // ptImage::Grain() relies on the integer values! Do not change them!
  SoftGaussian   = 0,
  SoftUniform    = 1,
  SoftSaltPepper = 2,
  HardGaussian   = 3,
  HardUniform    = 4,
  HardSaltPepper = 5
};

// GradientModes

const short ptGradientMode_Backward    = 0;
const short ptGradientMode_Centered    = 1;
const short ptGradientMode_Forward     = 2;
const short ptGradientMode_Sobel       = 3;
const short ptGradientMode_RotInv      = 4;
const short ptGradientMode_Deriche     = 5;

// MaskTypes (DEPRECATED! To be replaced by TMaskType)

const short ptMaskType_None       = 0;
const short ptMaskType_Shadows    = 1;
const short ptMaskType_Midtones   = 2;
const short ptMaskType_Highlights = 3;
const short ptMaskType_All        = 4;
const short ptMaskType_Screen     = 5;
const short ptMaskType_Multiply   = 6;

enum class TMaskType: int {
  Disabled    = 0,
  Shadows     = 1,
  Midtones    = 2,
  Highlights  = 3,
  All         = 4,
  Screen      = 5,
  Multiply    = 6,
  GammaBright = 12, // keep in sync with overlay mode of same name
  GammaDark   = 13  // keep in sync with overlay mode of same name
};

const short ptSaturationMode_Absolute = 0;
const short ptSaturationMode_Adaptive = 1;


// Special Preview

const short ptSpecialPreview_RGB       = 0;
const short ptSpecialPreview_L         = 1;
const short ptSpecialPreview_Gradient  = 2;
const short ptSpecialPreview_Structure = 3;
const short ptSpecialPreview_A         = 4;
const short ptSpecialPreview_B         = 5;

// LABTransform

const short ptLABTransform_L  = 0;
const short ptLABTransform_R  = 1;
const short ptLABTransform_G  = 2;
const short ptLABTransform_B  = 3;

enum class TViewLabChannel: int {
  Lab,
  L,
  a,
  b,
  LStructure,
  C,
  H
};

// Enable (DEPRECATED! To be replaced by TFilterMode)

const short ptEnable_None      = 0;
const short ptEnable_NoPreview = 1;
const short ptEnable_Preview   = 2;
const short ptEnable_ShowMask  = 3;

enum class TFilterMode: int {
  Disabled,
  FinalRun,
  AlwaysOn,
  ShowMask
};

enum class TBWFilmType: int {
  // There used to be an unused "none" value. Integer representation needs to start at
  // 1 for settings compatibility.
  LowSensitivity    = 1,
  HighSensitivity   = 2,
  Hyperpanchromatic = 3,
  Orthochromatic    = 4,
  NormalContrast    = 5,
  HighContrast      = 6,
  Luminance         = 7,
  Landscape         = 8,
  FaceInterior      = 9,
  ChannelMixer      = 10

};

enum class TBWColorFilter: int {
  None,
  Red,
  Orange,
  Yellow,
  Lime,
  Green,
  Blue,
  FakeIR
};

enum class TCrossProcessMode {
  Disabled,
  GreenYellow,
  GreenCyan,
  RedYellow,
  RedMagenta,
  BlueCyan,
  BlueMagenta
};

// OverlayMode

const short ptOverlayMode_None         = 0;
const short ptOverlayMode_SoftLight    = 1;
const short ptOverlayMode_Multiply     = 2;
const short ptOverlayMode_Screen       = 3;
const short ptOverlayMode_Normal       = 4;
const short ptOverlayMode_Lighten      = 5;
const short ptOverlayMode_Overlay      = 6;
const short ptOverlayMode_GrainMerge   = 7;
const short ptOverlayMode_ShowMask     = 8;
const short ptOverlayMode_Replace      = 9;
const short ptOverlayMode_ColorDodge   = 10;
const short ptOverlayMode_ColorBurn    = 11;
const short ptOverlayMode_GammaDark    = 12;
const short ptOverlayMode_GammaBright  = 13;
const short ptOverlayMode_Darken       = 14;

// OverlayMaskMode

const short ptOverlayMaskMode_FullImage   = 0;
const short ptOverlayMaskMode_Vignette    = 1;
const short ptOverlayMaskMode_InvVignette = 2;

enum class TVignetteMask: int {
  Disabled,
  Soft,
  Hard,
  Fancy,
  ShowMask
};

enum class TVignetteShape: int {
  // ptImage::Vignette() relies on the integer values. Do not change them.
  Diamond = 1,
  Circle  = 2,
  Rect1   = 3,
  Rect2   = 4,
  Rect3   = 5,
  Rect4   = 6,
  Rect5   = 7,
  Rect6   = 8,
  Rect7   = 9,
  Rect8   = 10
};

enum class TSoftglowMode {
  Disabled,
  Lighten,
  Screen,
  Softlight,
  Normal,
  OrtonScreen,
  OrtonSoftlight
};

// Output formats.

const short ptSaveFormat_PPM8  = 0;
const short ptSaveFormat_PPM16 = 1;
const short ptSaveFormat_TIFF8 = 2;
const short ptSaveFormat_TIFF16= 3;
const short ptSaveFormat_JPEG  = 4;
const short ptSaveFormat_PNG   = 5;
const short ptSaveFormat_PNG16 = 6;

// Sampling for JPEG

const short ptSaveSampling_111 = 0;
const short ptSaveSampling_211 = 1;

// Output modes

const short ptOutputMode_Full = 0;
const short ptOutputMode_Pipe = 1;
const short ptOutputMode_Jobfile = 2;
const short ptOutputMode_Settingsfile = 3;
const short ptOutputMode_Batch = 4;

// Export modes

const short ptExportMode_GimpFull = 0;
const short ptExportMode_GimpPipe = 1;

// Reset modes

const short ptResetMode_Full = 0;
const short ptResetMode_User = 1;
const short ptResetMode_OpenPreset = 2;
const short ptResetMode_OpenSettings = 3;

// Gui Tabs.

const short ptGenericTab      = 0;
const short ptCameraTab       = 1;
const short ptLocalTab        = 2;
const short ptGeometryTab     = 3;
const short ptRGBTab          = 4;
const short ptLabCCTab        = 5;
const short ptLabSNTab        = 6;
const short ptEyeCandyTab     = 7;
const short ptLabEyeCandyTab  = 8;
const short ptOutTab          = 9;

// Resize filters

const short ptResizeFilter_Box              = 0;
const short ptResizeFilter_Triangle         = 1;
const short ptResizeFilter_Quadratic        = 2;
const short ptResizeFilter_CubicBSpline     = 3;
const short ptResizeFilter_QuadraticBSpline = 4;
const short ptResizeFilter_CubicConvolution = 5;
const short ptResizeFilter_Lanczos3         = 6;
const short ptResizeFilter_Mitchell         = 7;
const short ptResizeFilter_CatmullRom       = 8;
const short ptResizeFilter_Cosine           = 9;
const short ptResizeFilter_Bell             = 10;
const short ptResizeFilter_Hermite          = 11;

// Resize filters for Imagemagick

const short ptIMFilter_Point     = 0;
const short ptIMFilter_Box       = 1;
const short ptIMFilter_Triangle  = 2;
const short ptIMFilter_Hermite   = 3;
const short ptIMFilter_Hanning   = 4;
const short ptIMFilter_Hamming   = 5;
const short ptIMFilter_Blackman  = 6;
const short ptIMFilter_Gaussian  = 7;
const short ptIMFilter_Quadratic = 8;
const short ptIMFilter_Cubic     = 9;
const short ptIMFilter_Catrom    = 10;
const short ptIMFilter_Mitchell  = 11;
const short ptIMFilter_Lanczos   = 12;
//const short ptIMFilter_Bessel    = 13;
//const short ptIMFilter_Sinc      = 14;

// Resize modes
const short ptResizeDimension_LongerEdge  = 0;
const short ptResizeDimension_Width       = 1;
const short ptResizeDimension_Height      = 2;
const short ptResizeDimension_WidthHeight = 3;

// Liquid rescale energies
const short ptLqr_Disabled         = 0;
const short ptLqr_GradXabs         = 1;
const short ptLqr_GradSumabs       = 2;
const short ptLqr_GradNorm         = 3;
const short ptLqr_LumaGradXabs     = 4;
const short ptLqr_LumaGradSumabs   = 5;
const short ptLqr_LumaGradNorm     = 6;

// Liquid rescale scaling
const short ptLqr_ScaleRelative    = 0;
const short ptLqr_ScaleAbsolute    = 1;

// Spot repair algos
// indexes MUST be consecutive integers starting from 0
enum ptSpotRepairAlgo {
  SpotRepairAlgo_Clone = 0,
  SpotRepairAlgo_Heal  = 1
};

// Zoom modes
const short ptZoomMode_Fit    = 0;
const short ptZoomMode_NonFit = 1;

// Zoom levels
const short ptZoomLevel_Current = 0;
const short ptZoomLevel_Fit     = 1;
const short ptZoomLevel_5       = 5;
const short ptZoomLevel_10      = 10;
const short ptZoomLevel_20      = 20;
const short ptZoomLevel_25      = 25;
const short ptZoomLevel_33      = 33;
const short ptZoomLevel_50      = 50;
const short ptZoomLevel_66      = 66;
const short ptZoomLevel_100     = 100;
const short ptZoomLevel_150     = 150;
const short ptZoomLevel_200     = 200;
const short ptZoomLevel_300     = 300;
const short ptZoomLevel_400     = 400;

// Status overlay modes
const short ptStatus_Done = 0;
const short ptStatus_Updating = 1;
const short ptStatus_Processing = 2;

// Gui events : timeout filter
// 1s after releasing input arrows, processing will be triggered.
// Should be working also for sufficiently fast typing :)
const short ptTimeout_Input = 300;

// Gui Elements
const short ptGT_None            = 0;
const short ptGT_Input           = 1;
const short ptGT_InputSlider     = 2;
const short ptGT_InputSliderHue  = 3;
const short ptGT_Choice          = 4;
const short ptGT_Check           = 5;

// Warning values;
const short ptWarning_Argument      = 2;
const short ptWarning_Exiv2         = 8;

// Error values.
const short ptError_FileOpen        = 1;
const short ptError_Argument        = 2;
const short ptError_Spline          = 3;
const short ptError_FileFormat      = 4;
const short ptError_Profile         = 5;
const short ptError_NotForeseen     = 6;
const short ptError_lcms            = 7;
const short ptError_Lensfun         = 8;

// Matrices to transform from XYZ to RGB space.
// For different RGB spaces.
// XYZ = Matrix * RGB
// Dummy : Index 0 : don't use => 0
// ptSpace_sRGB_D65=1
// ptSpace_AdobeRGB_D65 = 2
// ptSpace_WideGamutRGB_D50 = 3
// ptSpace_ProPhotoRGB_D50 = 4

const double MatrixRGBToXYZ [5][3][3] =
  {
  // Dummy : Index 0 : don't use => 0
  {
  {0.0,0.0,0.0},
  {0.0,0.0,0.0},
  {0.0,0.0,0.0}
  },

  //ptSpace_sRGB_D65=1

  {
  {0.412424  , 0.357579  , 0.180464},
  {0.212656  , 0.715158  , 0.0721856},
  {0.0193324 , 0.119193  , 0.950444}
  },

  // ptSpace_AdobeRGB_D65 = 2
  {
  {0.576700  , 0.185556  , 0.188212},
  {0.297361  , 0.627355  , 0.0752847},
  {0.0270328 , 0.0706879 , 0.991248}
  },

  // ptSpace_WideGamutRGB_D50 = 3
  {
  {0.716105  , 0.100930  , 0.147186},
  {0.258187  , 0.724938  , 0.0168748},
  {0.000000  , 0.0517813 , 0.773429}
  },

  // ptSpace_ProPhotoRGB_D50 = 4
  {
  {0.797675  , 0.135192  , 0.0313534},
  {0.288040  , 0.711874  , 0.000086},
  {0.000000  , 0.000000  , 0.825210}
  }
  };

// Matrices to transform from RGB to XYZ space.
// For different RGB spaces.
// RGB = Matrix * XYZ

const double MatrixXYZToRGB [5][3][3] =
  {
  // Dummy : Index 0 : don't use => 0
  {
  {0.0,0.0,0.0},
  {0.0,0.0,0.0},
  {0.0,0.0,0.0}
  },

  //ptSpace_sRGB_D65=1
  {
  { 3.24071   ,-1.53726   ,-0.498571},
  {-0.969258  , 1.87599   , 0.0415557},
  { 0.0556352 ,-0.203996  , 1.05707}
  },

  // ptSpace_AdobeRGB_D65 = 2
  {
  { 2.04148   ,-0.564977  ,-0.344713},
  {-0.969258  , 1.87599   , 0.0415557},
  { 0.0134455 ,-0.118373  , 1.01527}
  },

  // ptSpace_WideGamutRGB_D50 = 3
  {
  { 1.46281   ,-0.184062  ,-0.274361},
  {-0.521793  , 1.44724   , 0.0677228},
  { 0.0349342 ,-0.0968931 , 1.28841}
  },

  // ptSpace_ProPhotoRGB_D50 = 4
  {
  { 1.34594   ,-0.255608  ,-0.0511118},
  {-0.544599  , 1.50817   , 0.0205351},
  { 0.000000  , 0.000000  , 1.21181}
  }
  };

const double MatrixBradfordD50ToD65[3][3] =
  {
  { 0.955577  , -0.023039 , 0.063164},
  {-0.028290  ,  1.009941 , 0.021007},
  { 0.012298  , -0.020483 , 1.329910}
  };

// RGB Primaries for the different color spaces.
// Same indexing as matrices, also with index 0 being dummy.

const cmsCIExyYTRIPLE RGBPrimaries[5] =
  {
  // Dummy : Index 0 : don't use => 0
  {
  {0.0,0.0,0.0},
  {0.0,0.0,0.0},
  {0.0,0.0,0.0}
  },

  //ptSpace_sRGB_D65=1
  {
  {0.6400, 0.3300, 0.212656},
  {0.3000, 0.6000, 0.715158},
  {0.1500, 0.0600, 0.072186}
  },

  // ptSpace_AdobeRGB_D65 = 2
  {
  {0.6400, 0.3300, 0.297361},
  {0.2100, 0.7100, 0.627355},
  {0.1500, 0.0600, 0.075285}
  },

  // ptSpace_WideGamutRGB_D50 = 3
  {
  {0.7350, 0.2650, 0.258187},
  {0.1150, 0.8260, 0.724938},
  {0.1570, 0.0180, 0.016875}
  },
  //
  // ptSpace_ProPhotoRGB_D50 = 4
  {
  {0.7347, 0.2653, 0.288040},
  {0.1596, 0.8404, 0.711874},
  {0.0366, 0.0001, 0.000086}
  }
  };

// D50 and D65 references.

const double D50Reference[3] = {0.96422,  1.0, 0.82521};
const double D65Reference[3] = {0.950456, 1.0, 1.088754};

// Possible tool states

enum ptToolState {
  tsHidden = 0,
  tsNormal = 1,
  tsFavourite = 2
};

/*! General status return values.
  If you add more states make sure to use values >= 2. */
enum ptStatus {
  stUndetermined = -1,
  stSuccess = 0,
  stFailure = 1
};

/*! This enum defines the available methods for blocking and unblocking tools. */
enum ptBlockToolsMode {
  btmUnblock             = 0,
  btmBlockAll            = 1,
  btmBlockForCrop        = 2,
  btmBlockForSpotRepair  = 3,
  btmBlockForSpotTuning = 4
};

/*! This enum defines the different modes for loading a pipe configuration
  (preset or settings file). */
enum ptLoadCfgMode {
  lcmGeneralCfg     = 0,
  lcmNeutralPreset  = 1,
  lcmStartupPreset  = 2,
  lcmPresetFile     = 3,
  lcmSettingsFile   = 4
};

/*! This enum defines the possible types of images.
    When you add new types make sure that error types have negative values and
    valid types have positive values.
*/
enum ptImageType {
  itNotSupported  = -1,
  itUndetermined  = 0,
  itRaw           = 1,
  itBitmap        = 2
};

/*! This enum defines the type of a file system object. */
enum ptFSOType {
  fsoUnknown    = 0,
  fsoFile       = 1,
  fsoDir        = 2,
  fsoParentDir  = 3,
  fsoRoot       = 4,   // "My Computer" on Windows, "/" on Linux
  fsoDrive      = 5    // A drive with an assigned letter, only relevant on Windows
};

/*! This enum defines the state of the UI. */
enum ptUIState {
  uisNone       = 0,
  uisProcessing = 1,
  uisFileMgr    = 2,
  uisBatch      = 3
};

/*! This enum defines types of autosaving current batch list. */
enum ptBatchSaveFile {
  bsfStandard = 0,
  bsfLocal    = 1
};


//==============================================================================
#endif
