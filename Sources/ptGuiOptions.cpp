/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include <cstdlib>

#include <QtCore>

#include "ptGuiOptions.h"
#include "ptDefines.h"

////////////////////////////////////////////////////////////////////////////////
//
// Bunch of structured options for the Gui choice elements;
//
////////////////////////////////////////////////////////////////////////////////

const ptGuiOptionsItem ptGuiOptions::RememberSettingLevel[] = {
  {0,    QObject::tr("None: even this setting is lost ...")  },
  {1,    QObject::tr("Minimal: dirs, available curves ...")     },
  {2,    QObject::tr("Medium: most used settings ...")     },
  {3,    QObject::tr("All: remember everything")     },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CameraColor[] = {
  {ptCameraColor_Flat,           QObject::tr("Flat Profile")},
  {ptCameraColor_Adobe_Matrix,   QObject::tr("Adobe Matrix")},
  {ptCameraColor_Adobe_Profile,  QObject::tr("Adobe Profile")},
  //{ptCameraColor_Embedded,       QObject::tr("Embedded Profile")  },
  {ptCameraColor_Profile,        QObject::tr("External Profile")  },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CameraColorProfileIntent[] = {
  {INTENT_PERCEPTUAL,            QObject::tr("Perceptual")            },
  {INTENT_RELATIVE_COLORIMETRIC, QObject::tr("Relative Colorimetric") },
  {INTENT_SATURATION,            QObject::tr("Saturation")            },
  {INTENT_ABSOLUTE_COLORIMETRIC, QObject::tr("Absolute Colorimetric") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CameraColorGamma[] = {
  {ptCameraColorGamma_None,            QObject::tr("None")            },
  {ptCameraColorGamma_sRGB,            QObject::tr("sRGB")            },
  {ptCameraColorGamma_BT709,           QObject::tr("BT709")           },
  {ptCameraColorGamma_Pure22,          QObject::tr("Pure 2.2")        },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::WorkColor[] = {
  {ptSpace_sRGB_D65,         QObject::tr("sRGB - D65")              },
  {ptSpace_AdobeRGB_D65,     QObject::tr("Adobe RGB - D65")         },
  {ptSpace_WideGamutRGB_D50, QObject::tr("Wide Gamut RGB - D50")    },
  {ptSpace_ProPhotoRGB_D50,  QObject::tr("Kodak Pro PhotoRGB - D50")},
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CMQuality[] = {
  {ptCMQuality_NoOptimize,       QObject::tr("No optimization")     },
  {ptCMQuality_HighResPreCalc,   QObject::tr("High res pre calc")   },
  {ptCMQuality_FastSRGB,         QObject::tr("Fast sRGB preview")   },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::PreviewColorProfileIntent[] = {
  {INTENT_PERCEPTUAL,            QObject::tr("Perceptual")            },
  {INTENT_RELATIVE_COLORIMETRIC, QObject::tr("Relative Colorimetric") },
  {INTENT_SATURATION,            QObject::tr("Saturation")            },
  {INTENT_ABSOLUTE_COLORIMETRIC, QObject::tr("Absolute Colorimetric") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::OutputColorProfileIntent[] = {
  {INTENT_PERCEPTUAL,            QObject::tr("Perceptual")            },
  {INTENT_RELATIVE_COLORIMETRIC, QObject::tr("Relative Colorimetric") },
  {INTENT_SATURATION,            QObject::tr("Saturation")            },
  {INTENT_ABSOLUTE_COLORIMETRIC, QObject::tr("Absolute Colorimetric") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::Style[] = {
  {ptStyle_None,       QObject::tr("None") },
  {ptStyle_Normal,     QObject::tr("Normal") },
  {ptStyle_50Grey,     QObject::tr("50% grey") },
  {ptStyle_DarkGrey,   QObject::tr("Dark grey") },
  {ptStyle_VeryDark,   QObject::tr("Night") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::StyleHighLight[] = {
  {ptStyleHighLight_White,      QObject::tr("White") },
  {ptStyleHighLight_Purple,     QObject::tr("Purple") },
  {ptStyleHighLight_Blue,       QObject::tr("Blue") },
  {ptStyleHighLight_Green,      QObject::tr("Green") },
  {ptStyleHighLight_Orange,     QObject::tr("Orange") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::StartupUIMode[] = {
  {ptStartupUIMode_Tab,         QObject::tr("Tab mode") },
  {ptStartupUIMode_Favourite,   QObject::tr("Favourite tools") },
  {ptStartupUIMode_AllTools,    QObject::tr("All tools") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::PipeSize[] = {
  {ptPipeSize_Thirtyfour, QObject::tr("1:32") },
  {ptPipeSize_Sixteen,    QObject::tr("1:16") },
  {ptPipeSize_Eighth,     QObject::tr("1:8") },
  {ptPipeSize_Quarter,    QObject::tr("1:4") },
  {ptPipeSize_Half,       QObject::tr("1:2") },
  {ptPipeSize_Full,       QObject::tr("1:1") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::RunMode[] = {
  {ptRunMode_None, QObject::tr("Halt") },
  {ptRunMode_Once, QObject::tr("Run once") },
  {ptRunMode_Auto, QObject::tr("Run always") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LfunCAModel[] = {
  {ptLfunCAModel_None,    QObject::tr("None") },
  {ptLfunCAModel_Linear,  QObject::tr("Linear") },
  {ptLfunCAModel_Poly3,   QObject::tr("3rd order polynomial") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LfunVignetteModel[] = {
  {ptLfunVignetteModel_None,    QObject::tr("None") },
  {ptLfunVignetteModel_Poly6,  QObject::tr("6th order polynomial") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LfunGeo[] = {
  {ptLfunGeo_Unknown,           QObject::tr("Unknown") },
  {ptLfunGeo_Rectilinear,       QObject::tr("Rectilinear") },
  {ptLfunGeo_Fisheye,           QObject::tr("Fisheye") },
  {ptLfunGeo_Panoramic,         QObject::tr("Panoramic") },
  {ptLfunGeo_Equirectangular,   QObject::tr("Equirectangular") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LfunDistModel[] = {
  {ptLfunDistModel_None,     QObject::tr("None") },
  {ptLfunDistModel_Poly3,    QObject::tr("3rd order polynomial") },
  {ptLfunDistModel_Poly5,    QObject::tr("5th order polynomial") },
  {ptLfunDistModel_Fov1,     QObject::tr("1st order field of view") },
  {ptLfunDistModel_PTLens,   QObject::tr("Panotools lens model") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CropGuidelines[] = {
  {ptCropGuidelines_None,         QObject::tr("No guidelines") },
  {ptCropGuidelines_RuleThirds,   QObject::tr("Rule of thirds") },
  {ptCropGuidelines_GoldenRatio,  QObject::tr("Golden ratio") },
  {ptCropGuidelines_Diagonals,    QObject::tr("Diagonals") },
  {ptCropGuidelines_Centerlines,  QObject::tr("Center lines") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LightsOutMode[] = {
  {ptLightsOutMode_None,    QObject::tr("None") },
  {ptLightsOutMode_Dimmed,  QObject::tr("Dimmed") },
  {ptLightsOutMode_Black,   QObject::tr("Black") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ResizeFilter[] = {
  {ptResizeFilter_Box,              QObject::tr("Box filter") },
  {ptResizeFilter_Triangle,         QObject::tr("Triangle filter") },
  {ptResizeFilter_Quadratic,        QObject::tr("Quadratic filter") },
  {ptResizeFilter_CubicBSpline,     QObject::tr("Cubic spline filter") },
  {ptResizeFilter_QuadraticBSpline, QObject::tr("Quadratic spline filter") },
  {ptResizeFilter_CubicConvolution, QObject::tr("Cubic convolution filter") },
  {ptResizeFilter_Lanczos3,         QObject::tr("Lanczos3 filter") },
  {ptResizeFilter_Mitchell,         QObject::tr("Mitchell filter") },
  {ptResizeFilter_CatmullRom,       QObject::tr("Catmull Rom filter") },
  {ptResizeFilter_Cosine,           QObject::tr("Cosine filter") },
  {ptResizeFilter_Bell,             QObject::tr("Bell filter") },
  {ptResizeFilter_Hermite,          QObject::tr("Hermite filter") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::IMResizeFilter[] = {
  {ptIMFilter_Point,              QObject::tr("Point filter") },
  {ptIMFilter_Box,                QObject::tr("Box filter") },
  {ptIMFilter_Triangle,           QObject::tr("Triangle filter") },
  {ptIMFilter_Hermite,            QObject::tr("Hermite filter") },
  {ptIMFilter_Hanning,            QObject::tr("Hanning filter") },
  {ptIMFilter_Hamming,            QObject::tr("Hamming filter") },
  {ptIMFilter_Blackman,           QObject::tr("Blackman filter") },
  {ptIMFilter_Gaussian,           QObject::tr("Gaussian filter") },
  {ptIMFilter_Quadratic,          QObject::tr("Quadratic filter") },
  {ptIMFilter_Cubic,              QObject::tr("Cubic filter") },
  {ptIMFilter_Catrom,             QObject::tr("Catmull-Rom filter") },
  {ptIMFilter_Mitchell,           QObject::tr("Mitchell filter") },
  {ptIMFilter_Lanczos,            QObject::tr("Lanczos3 filter") },
//  {ptIMFilter_Bessel,             QObject::tr("Bessel filter") },
//  {ptIMFilter_Sinc,               QObject::tr("Sinc filter") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LqrEnergy[] = {
  {ptLqr_Disabled,                QObject::tr("Disabled") },
  {ptLqr_GradXabs,                QObject::tr("Directional grad brightness") },
  {ptLqr_GradSumabs,              QObject::tr("Average grad brightness") },
  {ptLqr_GradNorm,                QObject::tr("Norm brightness") },
  {ptLqr_LumaGradXabs,            QObject::tr("Directional grad luminance") },
  {ptLqr_LumaGradSumabs,          QObject::tr("Average grad luminance") },
  {ptLqr_LumaGradNorm,            QObject::tr("Norm luminance") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LqrScaling[] = {
  {ptLqr_ScaleRelative,           QObject::tr("Relative") },
  {ptLqr_ScaleAbsolute,           QObject::tr("Absolute") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::WhiteBalance[] = {
  {ptWhiteBalance_Camera,   QObject::tr("Camera")    },
  {ptWhiteBalance_Auto,     QObject::tr("Automatic") },
  {ptWhiteBalance_Spot,     QObject::tr("Spot")      },
  {ptWhiteBalance_Manual,   QObject::tr("Manual")    },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CACorrect[] = {
  {ptCACorrect_Off,         QObject::tr("No CA correction")    },
  {ptCACorrect_Auto,        QObject::tr("Automatic CA cor.")   },
  {ptCACorrect_Manual,      QObject::tr("Manual CA cor.")      },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::Interpolation[] = {
  {ptInterpolation_Linear,   QObject::tr("Bilinear")     },
  {ptInterpolation_VNG,      QObject::tr("VNG")          },
  {ptInterpolation_VNG4,     QObject::tr("VNG 4 color")  },
  {ptInterpolation_PPG,      QObject::tr("PPG")          },
  {ptInterpolation_AHD,      QObject::tr("AHD")          },
  {ptInterpolation_AHD_mod,  QObject::tr("AHD modified") },
  {ptInterpolation_AMaZE,    QObject::tr("AMaZE")        },
  {ptInterpolation_DCB,      QObject::tr("DCB")          },
  {ptInterpolation_DCBSoft,  QObject::tr("DCB soft")     },
  {ptInterpolation_DCBSharp, QObject::tr("DCB sharp")    },
  {ptInterpolation_VCD,      QObject::tr("VCD")          },
  {ptInterpolation_LMMSE,    QObject::tr("LMMSE")        },
  {ptInterpolation_Bayer,    QObject::tr("Bayer pattern")},
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::BayerDenoise[] = {
  {ptBayerDenoise_None,       QObject::tr("None")    },
  {ptBayerDenoise_FBDD1,      QObject::tr("FBDD 1")  },
  {ptBayerDenoise_FBDD2,      QObject::tr("FBDD 2")  },
  {-1,NULL}};


const ptGuiOptionsItem ptGuiOptions::ClipMode[] = {
  {ptClipMode_Clip,   QObject::tr("Clip")                },
  {ptClipMode_NoClip, QObject::tr("No clipping")         },
  {ptClipMode_Lab,    QObject::tr("Restore in Lab")      },
  {ptClipMode_HSV,    QObject::tr("Restore in HSV")      },
  {ptClipMode_Blend,  QObject::tr("Blend in Lab")        },
  {ptClipMode_Rebuild,QObject::tr("Rebuild")             },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LMHLightRecoveryMaskType[] = {
  {ptMaskType_None,              QObject::tr("Disabled") },
  {ptMaskType_Shadows,           QObject::tr("Shadows") },
  {ptMaskType_Midtones,          QObject::tr("Midtones") },
  {ptMaskType_Highlights,        QObject::tr("Highlights") },
  {ptMaskType_All,               QObject::tr("All values") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::LABTransformMode[] = {
  {ptLABTransform_L,             QObject::tr("Regular L*") },
  {ptLABTransform_R,             QObject::tr("R -> L*") },
  {ptLABTransform_G,             QObject::tr("G -> L*") },
  {ptLABTransform_B,             QObject::tr("B -> L*") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::GREYCInterpolation[] = {
  {ptGREYCInterpolation_NearestNeighbour, QObject::tr("Nearest Neighbour") },
  {ptGREYCInterpolation_Linear,           QObject::tr("Linear")            },
  {ptGREYCInterpolation_RungeKutta,       QObject::tr("Runge-Kutta")       },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::DenoiseMask[] = {
  {ptDenoiseMask_All,      QObject::tr("All values") },
  {ptDenoiseMask_Shadows1, QObject::tr("Shadows 1") },
  {ptDenoiseMask_Shadows2, QObject::tr("Shadows 2") },
  {ptDenoiseMask_Shadows3, QObject::tr("Shadows 3") },
  {ptDenoiseMask_Shadows4, QObject::tr("Shadows 4") },
  {ptDenoiseMask_Shadows5, QObject::tr("Shadows 5") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::FilmType[] = {
  {ptFilmType_LowSensitivity,    QObject::tr("Low sensitivity") },
  {ptFilmType_HighSensitivity,   QObject::tr("High sensitivity") },
  {ptFilmType_Hyperpanchromatic, QObject::tr("Hyperpanchromatic") },
  {ptFilmType_Orthochromatic,    QObject::tr("Orthochromatic") },
  {ptFilmType_NormalContrast,    QObject::tr("Normal contrast") },
  {ptFilmType_HighContrast,      QObject::tr("High contrast") },
  {ptFilmType_Luminance,         QObject::tr("Luminance") },
  {ptFilmType_Landscape,         QObject::tr("Landscape") },
  {ptFilmType_FaceInterior,      QObject::tr("Face in interior") },
  {ptFilmType_ChannelMixer,      QObject::tr("Channelmixer") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ColorFilterType[] = {
  {ptColorFilterType_None,       QObject::tr("None") },
  {ptColorFilterType_Red,        QObject::tr("Red") },
  {ptColorFilterType_Orange,     QObject::tr("Orange") },
  {ptColorFilterType_Yellow,     QObject::tr("Yellow") },
  {ptColorFilterType_YellowGreen,QObject::tr("Lime") },
  {ptColorFilterType_Green,      QObject::tr("Green") },
  {ptColorFilterType_Blue,       QObject::tr("Blue") },
//  {ptColorFilterType_fakeIR,     QObject::tr("fake IR") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::FlipMode[] = {
  {ptFlipMode_None,              QObject::tr("No flip") },
  {ptFlipMode_Horizontal,        QObject::tr("Horizontal flip") },
  {ptFlipMode_Vertical,          QObject::tr("Vertical flip") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::GrainMode[] = {
  {ptGrainMode_SoftGaussian,     QObject::tr("Soft gaussian") },
  {ptGrainMode_SoftUniform,      QObject::tr("Soft uniform") },
  {ptGrainMode_SoftSaltPepper,   QObject::tr("Soft salt'n pepper") },
  {ptGrainMode_HardGaussian,     QObject::tr("Hard gaussian") },
  {ptGrainMode_HardUniform,      QObject::tr("Hard uniform") },
  {ptGrainMode_HardSaltPepper,   QObject::tr("Hard salt'n pepper") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::MaskType[] = {
  {ptMaskType_None,              QObject::tr("Disabled") },
  {ptMaskType_Shadows,           QObject::tr("Shadows") },
  {ptMaskType_Midtones,          QObject::tr("Midtones") },
  {ptMaskType_Highlights,        QObject::tr("Highlights") },
  {ptMaskType_All,               QObject::tr("All values") },
  {ptMaskType_Screen,            QObject::tr("Midtones - Screen") },
  {ptMaskType_Multiply,          QObject::tr("Midtones - Multiply") },
  {ptOverlayMode_GammaBright,    QObject::tr("Midtones - Gamma bright") },
  {ptOverlayMode_GammaDark,      QObject::tr("Midtones - Gamma dark") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::GrainMaskType[] = {
  {ptMaskType_None,              QObject::tr("Disabled") },
  {ptMaskType_Shadows,           QObject::tr("Shadows") },
  {ptMaskType_Midtones,          QObject::tr("Midtones") },
  {ptMaskType_Highlights,        QObject::tr("Highlights") },
  {ptMaskType_All,               QObject::tr("All values") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::OverlayMode[] = {
  {ptOverlayMode_None,              QObject::tr("Disabled") },
  {ptOverlayMode_SoftLight,         QObject::tr("SoftLight") },
  {ptOverlayMode_Multiply,          QObject::tr("Multiply") },
  {ptOverlayMode_Screen,            QObject::tr("Screen") },
  {ptOverlayMode_GammaDark,         QObject::tr("Gamma dark") },
  {ptOverlayMode_GammaBright,       QObject::tr("Gamma bright") },
  {ptOverlayMode_Normal,            QObject::tr("Normal") },
  {ptOverlayMode_Replace,           QObject::tr("Replace") },
//  {ptOverlayMode_Overlay,           QObject::tr("Overlay") },
  {ptOverlayMode_ShowMask,          QObject::tr("Show Mask") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::OverlayMaskMode[] = {
  {ptOverlayMaskMode_FullImage,     QObject::tr("Full image") },
  {ptOverlayMaskMode_Vignette,      QObject::tr("Vignette") },
  {ptOverlayMaskMode_InvVignette,   QObject::tr("Inv vignette") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::CrossprocessMode[] = {
  {ptCrossprocessMode_None,  QObject::tr("Disabled") },
  {ptCrossprocessMode_GY,    QObject::tr("Green - yellow") },
  {ptCrossprocessMode_GC,    QObject::tr("Green - cyan") },
  {ptCrossprocessMode_RY,    QObject::tr("Red - yellow") },
  {ptCrossprocessMode_RM,    QObject::tr("Red - magenta") },
  {ptCrossprocessMode_BC,    QObject::tr("Blue - cyan") },
  {ptCrossprocessMode_BM,    QObject::tr("Blue - magenta") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::VignetteMode[] = {
  {ptVignetteMode_None,              QObject::tr("Disabled") },
  {ptVignetteMode_Soft,              QObject::tr("Soft") },
  {ptVignetteMode_Hard,              QObject::tr("Hard") },
  {ptVignetteMode_Fancy,             QObject::tr("Fancy") },
  {ptVignetteMode_Mask,              QObject::tr("Show Mask") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::SoftglowMode[] = {
  {ptSoftglowMode_None,              QObject::tr("Disabled") },
  {ptSoftglowMode_Lighten,           QObject::tr("Lighten") },
  {ptSoftglowMode_Screen,            QObject::tr("Screen") },
  {ptSoftglowMode_SoftLight,         QObject::tr("Softlight") },
  {ptSoftglowMode_Normal,            QObject::tr("Normal") },
  {ptSoftglowMode_OrtonScreen,       QObject::tr("Orton screen") },
  {ptSoftglowMode_OrtonSoftLight,    QObject::tr("Orton softlight") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::Enable[] = {
  {ptEnable_None,              QObject::tr("Disabled") },
  {ptEnable_NoPreview,         QObject::tr("Only final run") },
  {ptEnable_Preview,           QObject::tr("With Preview") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::EnableGreyC[] = {
  {ptEnable_None,              QObject::tr("Disabled") },
  {ptEnable_NoPreview,         QObject::tr("Only final run") },
  {ptEnable_Preview,           QObject::tr("With Preview") },
  {ptEnable_ShowMask,          QObject::tr("Show mask") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::AspectRatioW[] = {
  {21,   "21"    },
  {16,   "16"    },
  {10,   "10"    },
  {9,    "9"     },
  {8,    "8"     },
  {7,    "7"     },
  {6,    "6"     },
  {5,    "5"     },
  {4,    "4"     },
  {3,    "3"     },
  {2,    "2"     },
  {1,    "1"     },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::AspectRatioH[] = {
  {21,   "21"    },
  {16,   "16"    },
  {10,   "10"    },
  {9,    "9"     },
  {8,    "8"     },
  {7,    "7"     },
  {6,    "6"     },
  {5,    "5"     },
  {4,    "4"     },
  {3,    "3"     },
  {2,    "2"     },
  {1,    "1"     },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ExposureClipMode[] = {
  {ptExposureClipMode_None,   QObject::tr("None")                },
  {ptExposureClipMode_Ratio,  QObject::tr("Ratio")               },
  {ptExposureClipMode_Curve,  QObject::tr("Film curve")          },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::AutoExposureMode[] = {
  {ptAutoExposureMode_Auto,   QObject::tr("Auto")                },
  {ptAutoExposureMode_Ufraw,  QObject::tr("Like UFRaw")          },
  {ptAutoExposureMode_Zero,   QObject::tr("Null")                },
  {ptAutoExposureMode_Manual, QObject::tr("Manual")              },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ChannelMixer[] = {
  {ptChannelMixerChoice_None,    QObject::tr("None") },
  {ptChannelMixerChoice_Manual,  QObject::tr("Manual") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::Curve[] = {
  {ptCurveChoice_None,    QObject::tr("None") },
  {ptCurveChoice_Manual,  QObject::tr("Manual") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::SpecialPreview[] = {
  {ptSpecialPreview_RGB,        QObject::tr("RGB") },
  {ptSpecialPreview_Structure,  QObject::tr("Structure") },
  {ptSpecialPreview_L,          QObject::tr("L*") },
  {ptSpecialPreview_A,          QObject::tr("a*") },
  {ptSpecialPreview_B,          QObject::tr("b*") },
  {ptSpecialPreview_Gradient,   QObject::tr("Gradient") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ViewLAB[] = {
  {ptViewLAB_LAB,         QObject::tr("LAB") },
  {ptViewLAB_L,           QObject::tr("L") },
  {ptViewLAB_L_Grad,      QObject::tr("Structure on L") },
  {ptViewLAB_A,           QObject::tr("A") },
  {ptViewLAB_B,           QObject::tr("B") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::SaveFormat[] = {
  {ptSaveFormat_PPM8,   QObject::tr("PPM 8-bit")    },
  {ptSaveFormat_PPM16,  QObject::tr("PPM 16-bit")   },
  {ptSaveFormat_TIFF8,  QObject::tr("TIFF 8-bit")   },
  {ptSaveFormat_TIFF16, QObject::tr("TIFF 16-bit")  },
  {ptSaveFormat_JPEG,   QObject::tr("JPEG")         },
  {ptSaveFormat_PNG,    QObject::tr("PNG 8-bit")    },
  {ptSaveFormat_PNG16,  QObject::tr("PNG 16-bit")   },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::SaveSampling[] = {
  {ptSaveSampling_111,   QObject::tr("4:4:4")    },
  {ptSaveSampling_211,   QObject::tr("4:2:2")    },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::OutputMode[] = {
  {ptOutputMode_Full,      QObject::tr("Full size")    },
  {ptOutputMode_Pipe,      QObject::tr("Pipe size")    },
  {ptOutputMode_Jobfile,   QObject::tr("Only jobfile")    },
  {ptOutputMode_Settingsfile, QObject::tr("Only settings") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::ResetMode[] = {
  {ptResetMode_Full,         QObject::tr("Neutral reset")    },
  {ptResetMode_User,         QObject::tr("User reset")    },
  {ptResetMode_OpenPreset,   QObject::tr("Open preset")    },
  {ptResetMode_OpenSettings, QObject::tr("Open settings") },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::DarkFrame[] = {
  {0, QObject::tr("None")   },
  {1, QObject::tr("Load one")   },
  {-1,NULL}};

const ptGuiOptionsItem ptGuiOptions::BadPixels[] = {
  {0, QObject::tr("None")   },
  {1, QObject::tr("Load one")   },
  {-1,NULL}};

////////////////////////////////////////////////////////////////////////////////
