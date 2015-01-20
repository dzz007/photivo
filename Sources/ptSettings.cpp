/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
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
#include "ptError.h"
#include "ptDcRaw.h"
#include "ptRGBTemperature.h"
#include "ptGuiOptions.h"
#include "filemgmt/ptFileMgrConstants.h"
#include <filters/ptFilterUids.h>

#include <cassert>

//==============================================================================

// Macro for inserting a key into the hash and checking it is a new one.
#define M_InsertKeyIntoHash(Key,Item)                      \
  if (m_Hash.contains(Key)) {                              \
    ptLogError(ptError_Argument,                           \
               "Inserting an existing key (%s)",           \
               Key.toLocal8Bit().data());                      \
    assert (!m_Hash.contains(Key));                        \
  }                                                        \
  m_Hash[Key] = Item;

///////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
///////////////////////////////////////////////////////////////////////////////

ptSettings::ptSettings(const short InitLevel, const QString Path) {

  assert(InitLevel<9); // 9 reserved for never to be remembered.

  // Load in the gui input elements
  const ptGuiInputItem GuiInputItems[] = {
    // Attention : Default,Min,Max,Step should be consistent int or double. Double *always* in X.Y notation to indicate so.
    // Unique Name              uiElement,InitLevel,InJobFile,HasDefault  Default     Min       Max       Step    Decimals  Label,ToolTip
    {"FileMgrThumbnailSize"          ,ptGT_InputSlider     ,1,0,1 ,100  ,50   ,500   ,25   ,0 ,tr("Thumbnail size")     ,tr("Thumbnail size in pixel")},
    {"FileMgrThumbnailPadding"       ,ptGT_InputSlider     ,1,0,1 ,8    ,0    ,50    ,2    ,0 ,tr("Thumbnail padding")  ,tr("Thumbnail padding in pixel")},
    {"FileMgrThumbMaxRowCol"         ,ptGT_Input           ,1,0,1 ,3    ,1    ,1000  ,1    ,0 ,tr("Thumbnails in a row/column") ,tr("Maximum number of thumbnails that should be placed in a row or column.")},
    {"FileMgrThumbSaveSize"          ,ptGT_InputSlider     ,1,0,1 ,1000 ,50   ,8000  ,100  ,0 ,tr("Thumbnail export size")      ,tr("Thumbnail export size in pixel")},
    {"FileMgrThumbCacheSize"         ,ptGT_InputSlider     ,1,0,1 ,250  ,0    ,2000  ,100  ,0 ,tr("Thumbnail cache (MB)")       ,tr("Maximum size of thumbnail cache in MBytes.\nRequires a restart to take effect.")},
    {"MemoryTest"                    ,ptGT_InputSlider     ,9,0,1 ,0    ,0    ,500   ,50   ,0 ,tr("MB")                 ,tr("MB to waste")},
    {"TabStatusIndicator"            ,ptGT_Input           ,1,0,1 ,8    ,0    ,16    ,1    ,0 ,tr("Pixel")              ,tr("Size of the LED")},
    {"SliderWidth"                   ,ptGT_Input           ,1,0,1 ,0    ,0    ,500   ,50   ,0 ,tr("Maximum slider width") ,tr("Maximum slider width. Enter 0 to remove restriction")},
    {"Zoom"                          ,ptGT_InputSlider     ,9,0,0 ,100  ,5    ,400   ,10   ,0 ,tr("Zoom")               ,tr("Zoom factor")},
    {"ColorTemperature"              ,ptGT_InputSlider     ,2,1,1 ,6500 ,2000 ,15000 ,50   ,0 ,tr("Temp")               ,tr("Color Temperature")},
    {"GreenIntensity"                ,ptGT_Input           ,2,1,1 ,1.0  ,0.001,5.0   ,0.01 ,3 ,tr("WB-G")               ,tr("Green Intensity in balance")},
    {"RMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("R")                  ,tr("Red Multiplier in balance")},
    {"GMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("G")                  ,tr("Green Multiplier in balance")},
    {"BMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("B")                  ,tr("Blue Multiplier in balance")},
    {"BlackPoint"                    ,ptGT_Input           ,2,1,0 ,0    ,0    ,0xffff,1    ,0 ,tr("BP")                 ,tr("Black point in raw")},
    {"WhitePoint"                    ,ptGT_Input           ,2,1,0 ,0    ,0    ,0xffff,10   ,0 ,tr("WP")                 ,tr("White point in raw")},
    {"CaRed"                         ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-5.0 ,5.0   ,0.5  ,2 ,tr("CA red factor")      ,tr("CA red factor")},
    {"CaBlue"                        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-5.0 ,5.0   ,0.5  ,2 ,tr("CA blue factor")     ,tr("CA blue factor")},
    {"GreenEquil"                    ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,100   ,1    ,0 ,tr("Green equilibration"),tr("Green equilibration")},
    {"CfaLineDenoise"                ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,50    ,1    ,0 ,tr("Line denoise")       ,tr("Raw line denoise threshold")},
    {"AdjustMaximumThreshold"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,0.50  ,0.01 ,2 ,tr("Adjust maximum")     ,tr("Threshold to prevent pink highlights")},
    {"RawDenoiseThreshold"           ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,2000  ,100  ,0 ,tr("Wavelet denoise")    ,tr("Raw wavelet denoise threshold")},
    {"HotpixelReduction"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05 ,3 ,tr("Badpixel reduction") ,tr("Automatic badpixel reduction")},
    {"InterpolationPasses"           ,ptGT_Input           ,1,1,1 ,1    ,0    ,10    ,1    ,0 ,tr("Passes")             ,tr("Nr of refinement passes")},
    {"MedianPasses"                  ,ptGT_Input           ,2,1,1 ,0    ,0    ,10     ,1    ,0 ,tr("Median passes")      ,tr("Nr of median filter passes")},
    {"ESMedianPasses"                ,ptGT_Input           ,2,1,1 ,0    ,0    ,10     ,1    ,0 ,tr("Edge sensitive median passes")      ,tr("Nr of edge sensitive median filter passes")},
    {"ClipParameter"                 ,ptGT_InputSlider     ,1,1,1 ,0    ,0    ,100   ,1    ,0 ,tr("Parameter")          ,tr("Clip function dependent parameter")},
    {"LfunFocal"                     ,ptGT_Input           ,2,1,1 ,50.0 ,4.0  ,1000.0,1.0  ,0 ,tr("Focal length (35mm equiv.)"), tr("Focal length (35mm equiv.)")},
    {"LfunAperture"                  ,ptGT_Input           ,2,1,1 ,8.0  ,0.8  ,32.0  ,1.0  ,1 ,tr("Aperture"), tr("")},
    {"LfunDistance"                  ,ptGT_Input           ,2,1,1 ,1.0  ,0.01 ,500.0 ,1.0  ,2 ,tr("Distance"), tr("Distance between object and camera")},
    {"LfunScale"                     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.01  ,5.0   ,0.01 ,2 ,tr("Scale"),tr("Image scaling.\nUseful to avoid losing content through the distortion/geometry tools.\n0.0 means auto-scaling.")},
    {"LfunCALinearKr"                ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.99 ,1.01  ,0.001,5 ,tr("kr"),tr("")},
    {"LfunCALinearKb"                ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.99 ,1.01  ,0.001,5 ,tr("kb"),tr("")},
    {"LfunCAPoly3Vr"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.99 ,1.01  ,0.001,5 ,tr("vr"),tr("")},
    {"LfunCAPoly3Vb"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.99 ,1.01  ,0.001,5 ,tr("vb"),tr("")},
    {"LfunCAPoly3Cr"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.01,0.01  ,0.001,5 ,tr("cr"),tr("")},
    {"LfunCAPoly3Cb"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.01,0.01  ,0.001,5 ,tr("cb"),tr("")},
    {"LfunCAPoly3Br"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.01,0.01  ,0.001,5 ,tr("br"),tr("")},
    {"LfunCAPoly3Bb"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.01,0.01  ,0.001,5 ,tr("bb"),tr("")},
    {"LfunVignettePoly6K1"           ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,2.0   ,0.01 ,3 ,tr("k1"),tr("")},
    {"LfunVignettePoly6K2"           ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,2.0   ,0.01 ,3 ,tr("k2"),tr("")},
    {"LfunVignettePoly6K3"           ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,2.0   ,0.01 ,3 ,tr("k3"),tr("")},
    {"LfunDistPoly3K1"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("k1"),tr("")},
    {"LfunDistPoly5K1"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("k1"),tr("")},
    {"LfunDistPoly5K2"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("k2"),tr("")},
    {"LfunDistFov1Omega"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.4   ,0.1  ,3 ,tr("omega"),tr("")},
    {"LfunDistPTLensA"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("a"),tr("")},
    {"LfunDistPTLensB"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("b"),tr("")},
    {"LfunDistPTLensC"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-0.2 ,0.2   ,0.01 ,4 ,tr("c"),tr("")},

    {"DefishFocalLength"             ,ptGT_Input           ,9,1,1 ,15.0 ,4.0  ,50.0  ,1.0  ,1 ,tr("Focal length (35mm equiv.)")  ,tr("Focal length (35mm equiv.)")},
    {"DefishScale"                   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.01  ,5.0   ,0.01 ,2 ,tr("Scale")              ,tr("Image scaling.\n0.0 means auto-scaling.")},
    {"Rotate"                        ,ptGT_InputSlider     ,9,1,1 ,0.0  ,-180.0,180.0 ,0.1 ,2 ,tr("Rotate")             ,tr("Rotate")},
    {"PerspectiveFocalLength"        ,ptGT_Input           ,9,1,1 ,50.0 ,4.0  ,600.0 ,1.0  ,0 ,tr("Focal length (35mm equiv.)")  ,tr("Focal length (35mm equiv.)")},
    {"PerspectiveTilt"               ,ptGT_InputSlider     ,9,1,1 ,0.0  ,-45.0,45.0  ,0.1  ,2 ,tr("Tilt")               ,tr("Tilt")},
    {"PerspectiveTurn"               ,ptGT_InputSlider     ,9,1,1 ,0.0  ,-45.0,45.0  ,0.1  ,2 ,tr("Turn")               ,tr("Turn")},
    {"PerspectiveScaleX"             ,ptGT_InputSlider     ,9,1,1 ,1.0  ,0.2  ,5.0   ,0.05 ,2 ,tr("Horizontal scale")   ,tr("Horizontal scale")},
    {"PerspectiveScaleY"             ,ptGT_InputSlider     ,9,1,1 ,1.0  ,0.2  ,5.0   ,0.05 ,2 ,tr("Vertical scale")     ,tr("Vertical scale")},
    {"GridX"                         ,ptGT_Input           ,1,0,0 ,5    ,0    ,20    ,1    ,0 ,tr("X")                  ,tr("Vertical lines")},
    {"GridY"                         ,ptGT_Input           ,1,0,0 ,5    ,0    ,20    ,1    ,0 ,tr("Y")                  ,tr("Horizontal lines")},
    {"CropExposure"                  ,ptGT_InputSlider     ,1,1,1 ,0.0  ,-5.0 ,5.0   ,0.1  ,2 ,tr("Crop exposure")      ,tr("Temporary exposure in EV")},
    {"LqrHorScale"                   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.2  ,2.0   ,0.02 ,3 ,tr("Horizontal scale")   ,tr("Horizontal scale")},
    {"LqrVertScale"                  ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.2  ,2.0   ,0.02 ,3 ,tr("Vertical scale")     ,tr("Vertical scale")},
    {"LqrWidth"                      ,ptGT_Input           ,1,1,1 ,1200  ,200 ,6000  ,100  ,0 ,tr("Width")              ,tr("Width")},
    {"LqrHeight"                     ,ptGT_Input           ,1,1,1 ,800   ,200 ,6000  ,100  ,0 ,tr("Height")             ,tr("Height")},
    {"ResizeScale"                   ,ptGT_Input           ,1,1,1 ,1200  ,200 ,6000  ,100  ,0 ,tr("Pixels")             ,tr("Image size")},
    {"ResizeHeight"                  ,ptGT_Input           ,1,1,1 ,800   ,200 ,6000  ,100  ,0 ,tr("Height")             ,tr("Image height")},
    {"WhiteFraction"                 ,ptGT_InputSlider     ,2,1,1 ,10   ,1    ,50    ,1    ,0 ,tr("% white")             ,tr("Percentage of white aimed at")},
    {"WhiteLevel"                    ,ptGT_InputSlider     ,2,1,1 ,90   ,50   ,99    ,1    ,0 ,tr("WhiteLevel")         ,tr("WhiteLevel")},
    {"Exposure"                      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-5.0 ,5.0   ,0.1 ,2 ,tr("EV")                 ,tr("Exposure in EV")},
    {"EAWMaster"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Master")              ,tr("Quick setup for the levels")},
    {"EAWLevel1"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 1")             ,tr("Boosting of level 1")},
    {"EAWLevel2"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 2")             ,tr("Boosting of level 2")},
    {"EAWLevel3"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 3")             ,tr("Boosting of level 3")},
    {"EAWLevel4"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 4")             ,tr("Boosting of level 4")},
    {"EAWLevel5"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 5")             ,tr("Boosting of level 5")},
    {"EAWLevel6"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0  ,0.05 ,2 ,tr("Level 6")             ,tr("Boosting of level 6")},
    {"GREYCLabOpacity"               ,ptGT_InputSlider     ,2,1,1 ,1.0 ,0.0 ,1.0  ,0.1  ,1 ,tr("Opacity")          ,tr("Opacity")},
    {"GREYCLabAmplitude"             ,ptGT_InputSlider     ,2,1,1 ,45.0 ,0.0 ,500.0  ,5.0  ,1 ,tr("Amplitude")          ,tr("Amplitude")},
    {"GREYCLabIterations"            ,ptGT_Input           ,2,1,1 ,1    ,1    ,10    ,1    ,0 ,tr("Iterations")         ,tr("Iterations")},
    {"GREYCLabSharpness"             ,ptGT_InputSlider     ,2,1,1 ,0.6  ,0.0  ,2.0   ,0.05 ,2 ,tr("Sharpness")          ,tr("Sharpness")},
    {"GREYCLabAnisotropy"            ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.05 ,2 ,tr("Anisotropy")         ,tr("Anisotropy")},
    {"GREYCLabAlpha"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,20.0   ,0.1  ,1 ,tr("Gradient smoothness")              ,tr("Alpha")},
    {"GREYCLabSigma"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,20.0   ,0.1  ,1 ,tr("Tensor smoothness")              ,tr("Sigma")},
    {"GREYCLabdl"                    ,ptGT_InputSlider     ,2,1,1 ,0.8  ,0.0  ,2.0   ,0.01 ,2 ,tr("Spacial precision")                 ,tr("dl")},
    {"GREYCLabda"                    ,ptGT_InputSlider     ,2,1,1 ,30   ,0    ,180    ,1    ,0 ,tr("Angular precision")                 ,tr("da")},
    {"GREYCLabGaussPrecision"        ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Value precision")              ,tr("Gauss")},
    {"DefringeRadius"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,10.0  ,0.5  ,1 ,tr("Radius")             ,tr("Radius")},
    {"DefringeThreshold"             ,ptGT_InputSlider     ,2,1,1 ,25   ,0    ,100   ,5    ,0 ,tr("Threshold")          ,tr("Threshold")},
    {"DefringeShift"                 ,ptGT_InputSlider     ,1,1,1 ,0.0  ,-1.0 ,1.0   ,0.1  ,2 ,tr("Tune masks")          ,tr("Fine tune the color masks")},
    {"BilateralASigmaR"              ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,3.0   ,0.02 ,2 ,tr("A amount")         ,tr("Color A denoise")},
    {"BilateralASigmaS"              ,ptGT_InputSlider     ,2,1,1 ,8.0    ,4.0 ,50.0  ,4.0  ,1 ,tr("A scale")         ,tr("Denoise scale on A")},
    {"BilateralBSigmaR"              ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,3.0   ,0.02 ,2 ,tr("B amount")         ,tr("Color B denoise")},
    {"BilateralBSigmaS"              ,ptGT_InputSlider     ,2,1,1 ,8.0    ,4.0 ,50.0  ,4.0  ,1 ,tr("B scale")         ,tr("Denoise scale on B")},
    {"WaveletDenoiseL"               ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,10.0  ,0.1  ,1 ,tr("L amount")             ,tr("Threshold for wavelet L denoise (with edge mask)")},
    {"WaveletDenoiseLSoftness"       ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,1.0   ,0.01 ,2 ,tr("L softness")           ,tr("Softness for wavelet L denoise (with edge mask)")},
    {"WaveletDenoiseLSharpness"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,2.0   ,0.1  ,1 ,tr("Sharpness")          ,tr("Sharpness")},
    {"WaveletDenoiseLAnisotropy"     ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.01 ,2 ,tr("Anisotropy")         ,tr("Anisotropy")},
    {"WaveletDenoiseLAlpha"          ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,5.0   ,0.10 ,1 ,tr("Gradient smoothness")              ,tr("Alpha")},
    {"WaveletDenoiseLSigma"          ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Tensor smoothness")              ,tr("Sigma")},
    {"WaveletDenoiseA"               ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,10.0  ,0.1  ,1 ,tr("A amount")             ,tr("Threshold for wavelet A denoise")},
    {"WaveletDenoiseASoftness"       ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,1.0   ,0.01 ,2 ,tr("A softness")           ,tr("Softness for wavelet A denoise")},
    {"WaveletDenoiseB"               ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,10.0  ,0.1  ,1 ,tr("B amount")             ,tr("Threshold for wavelet B denoise")},
    {"WaveletDenoiseBSoftness"       ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,1.0   ,0.01 ,2 ,tr("B softness")           ,tr("Softness for wavelet B denoise")},
    {"GradientSharpenPasses"         ,ptGT_Input           ,2,1,1 ,0      ,0   ,10    ,1   ,0 ,tr("Passes")             ,tr("Number of passes")},
    {"GradientSharpenStrength"       ,ptGT_InputSlider     ,2,1,1 ,0.4    ,0.0 ,1.0  ,0.05 ,2 ,tr("Strength")           ,tr("Strength")},
    {"MLMicroContrastStrength"       ,ptGT_InputSlider     ,2,1,1 ,0.0    ,-0.1,0.5 ,0.05  ,2 ,tr("Microcontrast")      ,tr("Microcontrast strength")},
    {"MLMicroContrastScaling"        ,ptGT_InputSlider     ,2,1,1 ,40.0   ,1.0 ,200.0,10.0 ,0 ,tr("Halo control")       ,tr("Microcontrast Halo control")},
    {"MLMicroContrastWeight"         ,ptGT_InputSlider     ,2,1,1 ,0.5    ,0.0 ,1.0 ,0.05  ,2 ,tr("Weight")             ,tr("Microcontrast weight")},
    {"LabHotpixel"                   ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,1.0 ,0.1   ,2 ,tr("Clean up")           ,tr("Automatic badpixel reduction")},
    {"InverseDiffusionIterations"    ,ptGT_Input           ,2,1,1 ,0      ,0   ,5     ,1   ,0 ,tr("Iterations")         ,tr("Number of iterations")},
    {"InverseDiffusionAmplitude"     ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,2.0 ,0.05  ,2 ,tr("Amplitude")          ,tr("Amplitude")},
    {"USMRadius"                     ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.1  ,10.0  ,0.1  ,1 ,tr("Radius")             ,tr("Radius for USM")},
    {"USMAmount"                     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.1  ,5.0   ,0.1  ,1 ,tr("Amount")             ,tr("Amount for USM")},
    {"USMThreshold"                  ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.00 ,0.20  ,0.01 ,2 ,tr("Threshold")          ,tr("Threshold for USM")},
    {"HighpassRadius"                ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.0  ,10.0  ,0.5  ,1 ,tr("Radius")             ,tr("Radius for Highpass")},
    {"HighpassAmount"                ,ptGT_InputSlider     ,2,1,1 ,4.0  ,0.0  ,20.0  ,0.5  ,1 ,tr("Amount")             ,tr("Amount for Highpass")},
    {"HighpassDenoise"               ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,10.0  ,0.1  ,1 ,tr("Denoise")            ,tr("Denoise for Highpass")},
    {"Grain1Strength"                ,ptGT_InputSlider     ,2,1,1 ,0.3  ,0.0  ,1.0   ,0.1  ,2 ,tr("Strength")           ,tr("Strength for film grain")},
    {"Grain1Radius"                  ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,10.0  ,1.0  ,1 ,tr("Radius")             ,tr("Radius for film grain")},
    {"Grain1Opacity"                 ,ptGT_InputSlider     ,2,1,1 ,0.3  ,0.0  ,1.0   ,0.1  ,2 ,tr("Opacity")            ,tr("Opacity for film grain")},
    {"Grain1LowerLimit"              ,ptGT_InputSlider     ,2,1,1 ,0.1  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Grain1UpperLimit"              ,ptGT_InputSlider     ,2,1,1 ,0.4  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Grain2Strength"                ,ptGT_InputSlider     ,2,1,1 ,0.3  ,0.0  ,1.0   ,0.1  ,2 ,tr("Strength")           ,tr("Strength for film grain")},
    {"Grain2Radius"                  ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.0  ,10.0  ,1.0  ,1 ,tr("Radius")             ,tr("Radius for film grain")},
    {"Grain2Opacity"                 ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.1  ,2 ,tr("Opacity")            ,tr("Opacity for film grain")},
    {"Grain2LowerLimit"              ,ptGT_InputSlider     ,2,1,1 ,0.1  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Grain2UpperLimit"              ,ptGT_InputSlider     ,2,1,1 ,0.4  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LabVignette"                   ,ptGT_Input           ,2,1,1 ,2    ,1    ,10    ,1    ,0  ,tr("Shape")             ,tr("Shape of the vignette")},
    {"LabVignetteAmount"             ,ptGT_InputSlider     ,2,1,1 ,0.3  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Amount")            ,tr("Amount")},
    {"LabVignetteInnerRadius"        ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2  ,tr("Inner Radius")      ,tr("Inner Radius")},
    {"LabVignetteOuterRadius"        ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2  ,tr("Outer Radius")      ,tr("Outer Radius")},
    {"LabVignetteRoundness"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.05  ,2,tr("Roundness")         ,tr("Roundness")},
    {"LabVignetteCenterX"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center X")          ,tr("Center X")},
    {"LabVignetteCenterY"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center Y")          ,tr("Center Y")},
    {"LabVignetteSoftness"           ,ptGT_InputSlider     ,2,1,1 ,0.06 ,0.0   ,1.0   ,0.1  ,2 ,tr("Softness")          ,tr("Softness")},
    {"BWStylerOpacity"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05 ,2 ,tr("Opacity")            ,tr("Opacity")},
    {"BWStylerMultR"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Red")                ,tr("Red multiplicity")},
    {"BWStylerMultG"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Green")              ,tr("Green multiplicity")},
    {"BWStylerMultB"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Blue")               ,tr("Blue multiplicity")},
    {"SimpleToneR"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Red")                ,tr("Red toning")},
    {"SimpleToneG"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Green")              ,tr("Green toning")},
    {"SimpleToneB"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Blue")               ,tr("Blue toning")},
    {"Tone1Amount"                   ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of toning")},
    {"Tone1LowerLimit"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Tone1UpperLimit"               ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Tone1Softness"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"Tone2Amount"                   ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of toning")},
    {"Tone2LowerLimit"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Tone2UpperLimit"               ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Tone2Softness"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"CrossprocessingColor1"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Main color")         ,tr("Intensity of the main color")},
    {"CrossprocessingColor2"         ,ptGT_InputSlider     ,2,1,1 ,0.4  ,0.0  ,1.0   ,0.1  ,2 ,tr("Second color")       ,tr("Intensity of the second color")},
    {"TextureOverlayOpacity"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Opacity")            ,tr("Opacity")},
    {"TextureOverlaySaturation"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,2.0   ,0.1  ,2 ,tr("Saturation")         ,tr("Saturation")},
    {"TextureOverlayExponent"        ,ptGT_Input           ,2,1,1 ,2    ,1    ,6     ,1    ,0 ,tr("Shape")              ,tr("Shape of the mask")},
    {"TextureOverlayInnerRadius"     ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2 ,tr("Inner Radius")       ,tr("Inner Radius")},
    {"TextureOverlayOuterRadius"     ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2 ,tr("Outer Radius")       ,tr("Outer Radius")},
    {"TextureOverlayRoundness"       ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Roundness")          ,tr("Roundness")},
    {"TextureOverlayCenterX"         ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.1  ,2 ,tr("Center X")           ,tr("Center X")},
    {"TextureOverlayCenterY"         ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.1  ,2 ,tr("Center Y")           ,tr("Center Y")},
    {"TextureOverlaySoftness"        ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0 ,1.0   ,0.1  ,2 ,tr("Softness")           ,tr("Softness")},
    {"TextureOverlay2Opacity"        ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Opacity")            ,tr("Opacity")},
    {"TextureOverlay2Saturation"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,2.0   ,0.1  ,2 ,tr("Saturation")         ,tr("Saturation")},
    {"TextureOverlay2Exponent"       ,ptGT_Input           ,2,1,1 ,2    ,1    ,6     ,1    ,0 ,tr("Shape")              ,tr("Shape of the mask")},
    {"TextureOverlay2InnerRadius"    ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2 ,tr("Inner Radius")       ,tr("Inner Radius")},
    {"TextureOverlay2OuterRadius"    ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2 ,tr("Outer Radius")       ,tr("Outer Radius")},
    {"TextureOverlay2Roundness"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Roundness")          ,tr("Roundness")},
    {"TextureOverlay2CenterX"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.1  ,2 ,tr("Center X")           ,tr("Center X")},
    {"TextureOverlay2CenterY"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.1  ,2 ,tr("Center Y")           ,tr("Center Y")},
    {"TextureOverlay2Softness"       ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0 ,1.0   ,0.1  ,2 ,tr("Softness")           ,tr("Softness")},
    {"GradualOverlay1Amount"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount")},
    {"GradualOverlay1Angle"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0  ,5.0  ,0 ,tr("Angle")            ,tr("Angle")},
    {"GradualOverlay1LowerLevel"     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,3.0   ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradualOverlay1UpperLevel"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0   ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradualOverlay1Softness"       ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"GradualOverlay2Amount"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")        ,tr("Amount")},
    {"GradualOverlay2Angle"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0  ,5.0  ,0 ,tr("Angle")        ,tr("Angle")},
    {"GradualOverlay2LowerLevel"     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,3.0   ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradualOverlay2UpperLevel"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0   ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradualOverlay2Softness"       ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"Vignette"                      ,ptGT_Input           ,2,1,1 ,2    ,1    ,10    ,1    ,0 ,tr("Shape")             ,tr("Shape of the vignette")},
    {"VignetteAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Amount")        ,tr("Amount")},
    {"VignetteInnerRadius"           ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2 ,tr("Inner Radius")        ,tr("Inner Radius")},
    {"VignetteOuterRadius"           ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2 ,tr("Outer Radius")        ,tr("Outer Radius")},
    {"VignetteRoundness"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.05  ,2 ,tr("Roundness")        ,tr("Roundness")},
    {"VignetteCenterX"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center X")        ,tr("Center X")},
    {"VignetteCenterY"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center Y")        ,tr("Center Y")},
    {"VignetteSoftness"              ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"GradBlur1Radius"               ,ptGT_InputSlider     ,2,1,1 ,0.0 ,0.0  ,200.0 ,5.0  ,0 ,tr("Radius")             ,tr("Maximal radius for the blur")},
    {"GradBlur1LowerLevel"           ,ptGT_InputSlider     ,2,1,1 ,0.3  ,0.0  ,10.0  ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradBlur1UpperLevel"           ,ptGT_InputSlider     ,2,1,1 ,1.5  ,0.0  ,10.0  ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradBlur1Softness"             ,ptGT_InputSlider     ,2,1,1 ,0.01 ,0.0  ,1.0   ,0.001,3 ,tr("Softness")           ,tr("Softness")},
    {"GradBlur1Angle"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0,5.0  ,0 ,tr("Angle")              ,tr("Angle")},
    {"GradBlur1Vignette"             ,ptGT_Input           ,2,1,1 ,2    ,1    ,10    ,1    ,0 ,tr("Shape")              ,tr("Shape of the vignette")},
    {"GradBlur1Roundness"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2 ,tr("Roundness")          ,tr("Roundness")},
    {"GradBlur1CenterX"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Center X")           ,tr("Center X")},
    {"GradBlur1CenterY"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Center Y")           ,tr("Center Y")},
    {"GradBlur2Radius"               ,ptGT_InputSlider     ,2,1,1 ,0.0 ,0.0  ,200.0 ,5.0  ,0 ,tr("Radius")             ,tr("Maximal radius for the blur")},
    {"GradBlur2LowerLevel"           ,ptGT_InputSlider     ,2,1,1 ,0.3  ,0.0  ,10.0  ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradBlur2UpperLevel"           ,ptGT_InputSlider     ,2,1,1 ,1.5  ,0.0  ,10.0  ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradBlur2Softness"             ,ptGT_InputSlider     ,2,1,1 ,0.01 ,0.0  ,1.0   ,0.001,3 ,tr("Softness")           ,tr("Softness")},
    {"GradBlur2Angle"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0,5.0  ,0 ,tr("Angle")              ,tr("Angle")},
    {"GradBlur2Vignette"             ,ptGT_Input           ,2,1,1 ,2    ,1    ,10    ,1    ,0 ,tr("Shape")              ,tr("Shape of the vignette")},
    {"GradBlur2Roundness"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2 ,tr("Roundness")          ,tr("Roundness")},
    {"GradBlur2CenterX"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Center X")           ,tr("Center X")},
    {"GradBlur2CenterY"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.05 ,2 ,tr("Center Y")           ,tr("Center Y")},
    {"SoftglowRadius"                ,ptGT_InputSlider     ,2,1,1 ,10.0  ,0.0  ,30.0  ,1.0 ,1 ,tr("Radius")        ,tr("Radius")},
    {"SoftglowAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,1  ,tr("Amount")        ,tr("Amount")},
    {"SoftglowSaturation"            ,ptGT_InputSlider     ,2,1,1 ,-50  ,-100  ,100    ,5  ,0 ,tr("Saturation")     ,tr("Saturation")},
    {"SoftglowContrast"              ,ptGT_InputSlider     ,2,1,1 ,5.0  ,0.0  ,20.0  ,0.5  ,1 ,tr("Contrast")           ,tr("Contrast")},
    {"OutputGamma"                   ,ptGT_InputSlider     ,1,1,1 ,0.33  ,0.1  ,1.0   ,0.01 ,3 ,tr("Gamma")              ,tr("Gamma")},
    {"OutputLinearity"               ,ptGT_InputSlider     ,1,1,1 ,0.06  ,0.0  ,1.0   ,0.01 ,3 ,tr("Linearity")          ,tr("Linearity")},
    {"WebResizeScale"                ,ptGT_Input           ,1,1,1 ,1200  ,200 ,2600  ,100  ,0 ,tr("pixels")             ,tr("Image size")},
    //{"GREYCAmplitude"                ,ptGT_Input           ,2,1,1 ,60.0 ,20.0 ,80.0  ,0.1  ,1 ,tr("Amplitude")          ,tr("Amplitude")},
    //{"GREYCIterations"               ,ptGT_Input           ,2,1,1 ,1    ,1    ,10    ,1    ,0 ,tr("Iterations")         ,tr("Iterations")},
    //{"GREYCSharpness"                ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,5.0   ,0.1  ,1 ,tr("Sharpness")          ,tr("Sharpness")},
    //{"GREYCAnisotropy"               ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,1.0   ,0.01 ,2 ,tr("Anisotropy")         ,tr("Anisotropy")},
    //{"GREYCAlpha"                    ,ptGT_Input           ,2,1,1 ,0.6  ,0.0  ,5.0   ,0.10 ,1 ,tr("Alpha")              ,tr("Alpha")},
    //{"GREYCdl"                       ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,1.0   ,0.01 ,2 ,tr("dl")                 ,tr("dl")},
    //{"GREYCda"                       ,ptGT_Input           ,2,1,1 ,30   ,0    ,90    ,1    ,0 ,tr("da")                 ,tr("da")},
    //{"GREYCSigma"                    ,ptGT_Input           ,2,1,1 ,1.1  ,0.0  ,5.0   ,0.1  ,1 ,tr("Sigma")              ,tr("Sigma")},
    //{"GREYCGaussPrecision"           ,ptGT_Input           ,2,1,1 ,2.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Gauss")              ,tr("Gauss")},
    {"SaveQuality"                   ,ptGT_Input           ,1,1,1 ,97   ,25   ,100   ,1    ,0 ,tr("Quality")            ,tr("Quality")},
    {"SaveResolution"                ,ptGT_Input           ,1,1,1 ,300  ,25   ,1200  ,100  ,0 ,tr("dpi")                ,tr("Resolution in dpi")},
    {"ImageRating"                   ,ptGT_Input           ,2,1,1 ,0    ,0    ,5     ,1    ,0 ,tr("Rating")             ,tr("Image rating")}
  };

  // Load in the gui choice (combo) elements
  const ptGuiChoiceItem GuiChoiceItems[] = {
    // Unique Name          GuiElement,InitLevel,InJobFile,HasDefault, Default            Choices (from ptGuiOptions.h),         ToolTip
    {"BatchMgrAutosaveFile"        ,ptGT_Choice       ,1,0,0 ,bsfStandard                 ,GuiOptions->BatchMgrAutosaveFile      ,tr("File for autosaving batch list")},
    {"RememberSettingLevel"        ,ptGT_Choice       ,1,0,0 ,2                           ,GuiOptions->RememberSettingLevel      ,tr("Remember setting level")},
    {"CameraColor"                 ,ptGT_Choice       ,1,1,1 ,ptCameraColor_Adobe_Profile ,GuiOptions->CameraColor               ,tr("Transform camera RGB to working space RGB")},
    {"CameraColorProfileIntent"    ,ptGT_Choice       ,1,1,1 ,INTENT_PERCEPTUAL           ,GuiOptions->CameraColorProfileIntent  ,tr("Intent of the profile")},
    {"CameraColorGamma"            ,ptGT_Choice       ,1,1,1 ,ptCameraColorGamma_None     ,GuiOptions->CameraColorGamma          ,tr("Gamma that was applied before this profile")},
    {"WorkColor"                   ,ptGT_Choice       ,1,1,1 ,ptSpace_sRGB_D65            ,GuiOptions->WorkColor                 ,tr("Working colorspace")},
    {"CMQuality"                   ,ptGT_Choice       ,1,1,0 ,ptCMQuality_FastSRGB        ,GuiOptions->CMQuality                 ,tr("Color management quality")},
    {"PreviewColorProfileIntent"   ,ptGT_Choice       ,1,0,1 ,INTENT_PERCEPTUAL           ,GuiOptions->PreviewColorProfileIntent ,tr("Intent of the profile")},
    {"OutputColorProfileIntent"    ,ptGT_Choice       ,1,1,1 ,INTENT_PERCEPTUAL           ,GuiOptions->OutputColorProfileIntent  ,tr("Intent of the profile")},
    {"SaveButtonMode"              ,ptGT_Choice       ,1,0,1 ,ptOutputMode_Pipe           ,GuiOptions->OutputMode                ,tr("Output mode of save button")},
    {"ResetButtonMode"             ,ptGT_Choice       ,1,0,1 ,ptResetMode_User            ,GuiOptions->ResetMode                 ,tr("Output mode of reset button")},
    {"Style"                       ,ptGT_Choice       ,1,0,0 ,ptStyle_DarkGrey            ,GuiOptions->Style                     ,tr("Set the theme.")},
    {"StyleHighLight"              ,ptGT_Choice       ,1,0,0 ,ptStyleHighLight_Green      ,GuiOptions->StyleHighLight            ,tr("Set the highlight color of the theme.")},
    {"StartupUIMode"               ,ptGT_Choice       ,1,0,0 ,ptStartupUIMode_Tab         ,GuiOptions->StartupUIMode             ,tr("Set the start up mode for the UI.")},
    {"PipeSize"                    ,ptGT_Choice       ,2,0,1 ,ptPipeSize_Quarter          ,GuiOptions->PipeSize                  ,tr("Size of image processed vs original.")},
    {"StartupPipeSize"             ,ptGT_Choice       ,1,0,1 ,ptPipeSize_Quarter          ,GuiOptions->PipeSize                  ,tr("Initial pipe size when Photivo starts.")},
    {"SpecialPreview"              ,ptGT_Choice       ,2,0,1 ,ptSpecialPreview_RGB        ,GuiOptions->SpecialPreview            ,tr("Special preview for image analysis")},
    {"BadPixels"                   ,ptGT_Choice       ,1,1,0 ,0                           ,GuiOptions->BadPixels                 ,tr("Bad pixels file")},
    {"DarkFrame"                   ,ptGT_Choice       ,1,1,0 ,0                           ,GuiOptions->DarkFrame                 ,tr("Darkframe file")},
    {"WhiteBalance"                ,ptGT_Choice       ,2,1,1 ,ptWhiteBalance_Camera       ,GuiOptions->WhiteBalance              ,tr("WhiteBalance")},
    {"CaCorrect"                   ,ptGT_Choice       ,2,1,1 ,ptCACorrect_Off             ,GuiOptions->CACorrect                 ,tr("CA correction")},
    {"Interpolation"               ,ptGT_Choice       ,2,1,1 ,ptInterpolation_DCB         ,GuiOptions->Interpolation             ,tr("Demosaicing algorithm")},
    {"BayerDenoise"                ,ptGT_Choice       ,2,1,1 ,ptBayerDenoise_None         ,GuiOptions->BayerDenoise              ,tr("Denosie on Bayer pattern")},

    {"CropGuidelines"              ,ptGT_Choice       ,1,0,0 ,ptGuidelines_GoldenRatio    ,GuiOptions->CropGuidelines            ,tr("Guide lines for crop")},
    {"LightsOut"                   ,ptGT_Choice       ,1,0,0 ,ptLightsOutMode_Dimmed      ,GuiOptions->LightsOutMode             ,tr("Dim areas outside the crop rectangle")},
    {"ClipMode"                    ,ptGT_Choice       ,1,1,1 ,ptClipMode_Blend            ,GuiOptions->ClipMode                  ,tr("How to handle clipping")},
    {"LfunCAModel"                 ,ptGT_Choice       ,2,1,1 ,ptLfunCAModel_None          ,GuiOptions->LfunCAModel               ,tr("Mathematical model for CA correction")},
    {"LfunVignetteModel"           ,ptGT_Choice       ,2,1,1 ,ptLfunVignetteModel_None    ,GuiOptions->LfunVignetteModel         ,tr("Mathematical model for vignetting correction")},
    {"LfunSrcGeo"                  ,ptGT_Choice       ,2,1,1 ,ptLfunGeo_Unknown           ,GuiOptions->LfunGeo                   ,tr("Geometry of the lens the image was taken with")},
    {"LfunTargetGeo"               ,ptGT_Choice       ,2,1,1 ,ptLfunGeo_Unknown           ,GuiOptions->LfunGeo                   ,tr("Convert image to this lens geometry")},
    {"LfunDistModel"               ,ptGT_Choice       ,2,1,1 ,ptLfunDistModel_None        ,GuiOptions->LfunDistModel             ,tr("Mathematical distortion model to apply to the image")},
    {"LqrEnergy"                   ,ptGT_Choice       ,2,1,1 ,ptLqr_Disabled              ,GuiOptions->LqrEnergy                 ,tr("Energy method for liquid rescale")},
    {"LqrScaling"                  ,ptGT_Choice       ,1,1,1 ,ptLqr_ScaleRelative         ,GuiOptions->LqrScaling                ,tr("Scaling method for liquid rescale")},
    {"ResizeFilter"                ,ptGT_Choice       ,1,1,1 ,ptIMFilter_Mitchell         ,GuiOptions->IMResizeFilter            ,tr("Filter to be used for resizing")},
    {"ResizeDimension"             ,ptGT_Choice       ,2,1,1 ,ptResizeDimension_LongerEdge,GuiOptions->ResizeDimension           ,tr("Image dimension the resize value applies to")},
    {"FlipMode"                    ,ptGT_Choice       ,2,1,1 ,ptFlipMode_None             ,GuiOptions->FlipMode                  ,tr("Flip mode")},
    {"AspectRatioW"                ,ptGT_Choice       ,2,0,0 ,3                           ,GuiOptions->AspectRatio               ,tr("Aspect width")},
    {"AspectRatioH"                ,ptGT_Choice       ,2,0,0 ,2                           ,GuiOptions->AspectRatio               ,tr("Aspect height")},
    {"ExposureClipMode"            ,ptGT_Choice       ,1,1,1 ,ptExposureClipMode_Curve    ,GuiOptions->ExposureClipMode          ,tr("Clip mode")},
    {"AutoExposure"                ,ptGT_Choice       ,1,1,1 ,ptAutoExposureMode_Zero     ,GuiOptions->AutoExposureMode          ,tr("Auto exposure mode")},
    {"GREYCLab"                    ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->EnableGreyC               ,tr("Enable GreyCStoration on L")},
    {"GREYCLabMaskType"            ,ptGT_Choice       ,2,1,1 ,ptDenoiseMask_Shadows3      ,GuiOptions->DenoiseMask               ,tr("Shadow mask for denoising")},
    {"GREYCLabInterpolation"       ,ptGT_Choice       ,2,1,1 ,ptGREYCInterpolation_NearestNeighbour,GuiOptions->GREYCInterpolation, tr("GREYC Interpolation")},
    {"USM"                         ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable                    ,tr("Enable USM sharpening")},
    {"Highpass"                    ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable                    ,tr("Enable Highpass sharpening")},
    {"Grain1MaskType"              ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->GrainMaskType             ,tr("Values for film grain")},
    {"Grain1Mode"                  ,ptGT_Choice       ,2,1,1 ,ptGrainMode_SoftGaussian    ,GuiOptions->GrainMode                 ,tr("Mode for film grain")},
    {"Grain2MaskType"              ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->GrainMaskType             ,tr("Values for film grain")},
    {"Grain2Mode"                  ,ptGT_Choice       ,2,1,1 ,ptGrainMode_SoftGaussian    ,GuiOptions->GrainMode                 ,tr("Mode for film grain")},
    {"LabVignetteMode"             ,ptGT_Choice       ,2,1,1 ,ptVignetteMode_None         ,GuiOptions->VignetteMode              ,tr("Mode for Vignette")},
    {"ViewLAB"                     ,ptGT_Choice       ,2,1,1 ,ptViewLAB_LAB               ,GuiOptions->ViewLAB                   ,tr("View seperate LAB channels")},
    {"BWStylerFilmType"            ,ptGT_Choice       ,2,1,1 ,ptFilmType_Luminance        ,GuiOptions->FilmType                  ,tr("Film emulation")},
    {"BWStylerColorFilterType"     ,ptGT_Choice       ,2,1,1 ,ptColorFilterType_None      ,GuiOptions->ColorFilterType           ,tr("Color filter emulation")},
    {"Tone1MaskType"               ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->MaskType                  ,tr("Values for Toning")},
    {"Tone2MaskType"               ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->MaskType                  ,tr("Values for Toning")},
    {"CrossprocessingMode"         ,ptGT_Choice       ,2,1,1 ,ptCrossprocessMode_None     ,GuiOptions->CrossprocessMode          ,tr("Colors for cross processing")},
    {"TextureOverlayMode"          ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode               ,tr("Mode for Texture Overlay")},
    {"TextureOverlayMask"          ,ptGT_Choice       ,2,1,1 ,ptOverlayMaskMode_FullImage ,GuiOptions->OverlayMaskMode           ,tr("Mask for Texture Overlay")},
    {"TextureOverlay2Mode"         ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode               ,tr("Mode for Texture Overlay")},
    {"TextureOverlay2Mask"         ,ptGT_Choice       ,2,1,1 ,ptOverlayMaskMode_FullImage ,GuiOptions->OverlayMaskMode           ,tr("Mask for Texture Overlay")},
    {"GradualOverlay1"             ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode               ,tr("Mode for Gradual Overlay")},
    {"GradualOverlay2"             ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode               ,tr("Mode for Gradual Overlay")},
    {"VignetteMode"                ,ptGT_Choice       ,2,1,1 ,ptVignetteMode_None         ,GuiOptions->VignetteMode              ,tr("Mode for Vignette")},
    {"GradBlur1"                   ,ptGT_Choice       ,1,1,1 ,ptGradualBlur_Linear        ,GuiOptions->GradualBlurMode           ,tr("Mode for the gradual blur")},
    {"GradBlur2"                   ,ptGT_Choice       ,1,1,1 ,ptGradualBlur_Linear        ,GuiOptions->GradualBlurMode           ,tr("Mode for the gradual blur")},
    {"SoftglowMode"                ,ptGT_Choice       ,2,1,1 ,ptSoftglowMode_None         ,GuiOptions->SoftglowMode              ,tr("Mode for Softglow")},
    {"WebResize"                   ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable                    ,tr("Enable web resizing")},
    {"WebResizeDimension"          ,ptGT_Choice       ,2,1,1 ,ptResizeDimension_LongerEdge,GuiOptions->WebResizeDimension        ,tr("Image dimension the resize value applies to")},
    {"WebResizeFilter"             ,ptGT_Choice       ,1,1,1 ,ptIMFilter_Lanczos          ,GuiOptions->IMResizeFilter            ,tr("Filter to be used for resizing")},
    {"SaveFormat"                  ,ptGT_Choice       ,1,1,1 ,ptSaveFormat_JPEG           ,GuiOptions->SaveFormat                ,tr("Output format")},
    {"SaveSampling"                ,ptGT_Choice       ,1,1,1 ,ptSaveSampling_211          ,GuiOptions->SaveSampling              ,tr("JPEG color sampling")},
    {"CropInitialZoom"             ,ptGT_Choice       ,1,0,1 ,ptZoomLevel_Fit             ,GuiOptions->ZoomLevel                 ,tr("Switch to this zoom level when starting to crop.")}
  };

  // Load in the gui check elements
  const ptGuiCheckItem GuiCheckItems[] = {
    // Name   GuiType,InitLevel,InJobFile,Default,Label,Tip
    {"FileMgrUseThumbMaxRowCol"   ,ptGT_Check ,1,0,0,tr("At most")         ,tr("Maximum number of thumbnails that should be placed in a row or column.")},
    {"FileMgrStartupOpen"         ,ptGT_Check ,1,0,0,tr("Open file manager on startup"), tr("Opens the file manager when Photivo starts without an image")},

    {"BatchMgrAutosave"           ,ptGT_Check ,1,0,1,tr("Automatically save batch list"), tr("Automatically save current batch list")},
    {"BatchMgrAutoload"           ,ptGT_Check ,1,0,0,tr("Automatically load batch list"), tr("Automatically load previous batch list saved to standard path on startup")},

    {"StartupSettings"            ,ptGT_Check ,1,0,1,tr("User settings")   ,tr("Load user settings on startup")},
    {"StartupSettingsReset"       ,ptGT_Check ,1,0,0,tr("Reset on new image") ,tr("Reset to user settings when new image is opened")},
    {"StartupSwitchAR"            ,ptGT_Check ,1,0,1,tr("Adjust aspect ratio") ,tr("Adjust crop aspect ratio to image aspect ratio")},
    {"InputsAddPowerLaw"          ,ptGT_Check ,1,1,1,tr("Nonlinear slider response")   ,tr("Alter the slider behaviour")},
    {"ExportToGimp"               ,ptGT_Check ,1,0,1,tr("Use gimp plugin") ,tr("Use gimp plugin for export")},
    {"ToolBoxMode"                ,ptGT_Check ,1,0,0,tr("Enabled")         ,tr("Show seperate toolboxes")},
    {"PreviewTabMode"             ,ptGT_Check ,1,0,0,tr("Tab mode")        ,tr("Show the preview after the active tab")},
    {"BackgroundColor"            ,ptGT_Check ,1,0,0,tr("Override default"),tr("Override the default color")},
    {"SearchBarEnable"            ,ptGT_Check ,1,0,1,tr("Display search bar"),tr("Display search bar")},
    {"WriteBackupSettings"        ,ptGT_Check ,1,0,0,tr("Backup settings") ,tr("Write backup settings during processing")},
    {"RunMode"                    ,ptGT_Check ,1,0,0,tr("manual")          ,tr("manual or automatic pipe")},
    {"UseThumbnail"               ,ptGT_Check ,1,1,0,tr("Use thumbnail")   ,tr("Use the embedded thumbnail of RAW images")},
    {"MultiplierEnhance"          ,ptGT_Check ,1,1,0,tr("Intensify")       ,tr("Normalize lowest channel to 1")},
    {"ManualBlackPoint"           ,ptGT_Check ,2,1,0,tr("Manual BP")       ,tr("Manual black point setting enabled")},
    {"ManualWhitePoint"           ,ptGT_Check ,2,1,0,tr("Manual WP")       ,tr("Manual white point setting enabled")},
    {"EeciRefine"                 ,ptGT_Check ,2,1,0,tr("Eeci refinement") ,tr("Eeci refinement")},
    {"LfunAutoScale"              ,ptGT_Check ,2,1,0,tr("Auto scale")      ,tr("Auto scale to avoid black borders after distortion correction or geometry conversion.")},
    {"Defish"                     ,ptGT_Check ,2,1,0,tr("Enable")          ,tr("Enable defishing")},
    {"DefishAutoScale"            ,ptGT_Check ,2,1,0,tr("Auto scale")      ,tr("Auto scale to avoid black borders")},
    {"Grid"                       ,ptGT_Check ,9,1,0,tr("Grid")            ,tr("Enable the overlay grid")},
    {"Crop"                       ,ptGT_Check ,9,1,0,tr("Crop")            ,tr("Enable to make a crop")},
    {"FixedAspectRatio"           ,ptGT_Check ,1,0,0,tr("Aspect Ratio")    ,tr("Crop with a fixed aspect ratio")},
    {"LqrVertFirst"               ,ptGT_Check ,2,1,0,tr("Vertical first")  ,tr("Resizing starts with vertical direction")},
    {"Resize"                     ,ptGT_Check ,9,1,0,tr("Resize")          ,tr("Enable resize")},
    {"AutomaticPipeSize"          ,ptGT_Check ,1,1,0,tr("Automatic pipe size") ,tr("Automatic pipe size")},
    {"GeometryBlock"              ,ptGT_Check ,9,0,0,tr("Block pipe")      ,tr("Disable the pipe")},
    {"GREYCLabFast"               ,ptGT_Check ,2,1,1,tr("Enable 'fast'")   ,tr("Enable GREYC 'fast'")},
    {"DefringeColor1"             ,ptGT_Check ,1,1,1,tr("Red")             ,tr("Red")},
    {"DefringeColor2"             ,ptGT_Check ,1,1,1,tr("Yellow")          ,tr("Yellow")},
    {"DefringeColor3"             ,ptGT_Check ,1,1,1,tr("Green")           ,tr("Green")},
    {"DefringeColor4"             ,ptGT_Check ,1,1,1,tr("Cyan")            ,tr("Cyan")},
    {"DefringeColor5"             ,ptGT_Check ,1,1,1,tr("Blue")            ,tr("Blue")},
    {"DefringeColor6"             ,ptGT_Check ,1,1,1,tr("Purple")          ,tr("Purple")},
    {"InverseDiffusionUseEdgeMask",ptGT_Check ,2,1,1,tr("Only edges")      ,tr("Sharpen only edges")},
    {"WebResizeBeforeGamma"       ,ptGT_Check ,1,1,0,tr("before gamma")    ,tr("Webresizing before gamma compensation")},
    {"OutputGammaCompensation"    ,ptGT_Check ,1,1,0,tr("sRGB gamma compensation")    ,tr("sRGB gamma compensation")},
    {"IncludeExif"                ,ptGT_Check ,2,1,1,tr("Include metadata"),tr("Include metadata (only in jpeg and tiff)")},
    {"EraseExifThumbnail"         ,ptGT_Check ,2,1,1,tr("Erase thumbnail") ,tr("Erase the exif thumbnail (only in jpeg and tiff)")},
    {"SaveConfirmation"           ,ptGT_Check ,1,0,1,tr("Save image")      ,tr("Confirm any action that would discard an unsaved image")},
    {"AutosaveSettings"           ,ptGT_Check ,1,0,1,tr("Autosave settings"),tr("Autosave settings when loading another image (if save confirmation is off)")},
    {"ResetSettingsConfirmation"  ,ptGT_Check, 1,0,1,tr("Reset settings")  ,tr("Confirm resetting settings or dropping a settings file onto an image")},
    {"FullPipeConfirmation"       ,ptGT_Check ,1,0,1,tr("Switch to 1:1 pipe"), tr("Confirm switch to the full sized pipe")},
    {"EscToExit"                  ,ptGT_Check ,1,0,0,tr("Esc key exits Photivo"),tr("Use the Esc key not only to exit special view modes (e.g. full screen) but also to close Photivo.")},
    {"LoadTags"                   ,ptGT_Check ,1,0,0,tr("Load tags from sidecar files"),tr("Load tags from sidecar XMP files when opening an image.")}
  };

  // Load in the non gui elements
  const ptItem Items[] = {
    // Name                             InitLevel  Default                               JobFile
    {"PipeIsRunning"                        ,9    ,0                                     ,0},
    {"BlockTools"                           ,9    ,0                                     ,0},
    {"InputPowerFactor"                     ,0    ,2.2                                   ,1},
    {"PreviewMode"                          ,1    ,ptPreviewMode_End                     ,0},
    {"ZoomMode"                             ,9    ,ptZoomMode_Fit                        ,0},
    {"Scaled"                               ,9    ,0                                     ,0},
    {"IsRAW"                                ,9    ,1                                     ,0},
    {"HaveImage"                            ,9    ,0                                     ,0},
    {"RawsDirectory"                        ,0    ,""                                    ,0},
    {"OutputDirectory"                      ,0    ,""                                    ,1},
    {"MainDirectory"                        ,0    ,"@INSTALL@/"                          ,0},
    {"ShareDirectory"                       ,0    ,"@INSTALL@/"                          ,1},
    {"UserDirectory"                        ,0    ,"@INSTALL@/"                          ,1},
    {"UIDirectory"                          ,0    ,"@INSTALL@/UISettings"                ,0},
    {"TranslationsDirectory"                ,0    ,"@INSTALL@/Translations"              ,0},
    {"CurvesDirectory"                      ,0    ,"@INSTALL@/Curves"                    ,1},
    {"ChannelMixersDirectory"               ,0    ,"@INSTALL@/ChannelMixers"             ,1},
    {"PresetDirectory"                      ,0    ,"@INSTALL@/Presets"                   ,0},
    {"CameraColorProfilesDirectory"         ,0    ,"@INSTALL@/Profiles/Camera"           ,1},
    {"PreviewColorProfilesDirectory"        ,0    ,"@INSTALL@/Profiles/Preview"          ,0},
    {"OutputColorProfilesDirectory"         ,0    ,"@INSTALL@/Profiles/Output"           ,1},
    {"StandardAdobeProfilesDirectory"       ,0    ,"@INSTALL@/Profiles/Camera/Standard"  ,1},
    {"LensfunDatabaseDirectory"             ,0    ,"@INSTALL@/LensfunDatabase"           ,1},
    {"PreviewColorProfile"                  ,1    ,"@INSTALL@/Profiles/Preview/sRGB.icc" ,1},
    {"OutputColorProfile"                   ,1    ,"@INSTALL@/Profiles/Output/sRGB.icc"  ,1},
    {"GimpExecCommand"                      ,1    ,"gimp"                                ,0},
    {"StartupSettingsFile"                  ,1    ,"@INSTALL@/Presets/MakeFancy.pts"     ,0},
    {"CameraMake"                           ,9    ,""                                    ,0},
    {"CameraModel"                          ,9    ,""                                    ,0},
    {"CameraColorProfile"                   ,1    ,""                                    ,1},
    {"HaveBadPixels"                        ,1    ,0                                     ,1},
    {"BadPixelsFileName"                    ,1    ,""                                    ,1},
    {"HaveDarkFrame"                        ,1    ,0                                     ,1},
    {"DarkFrameFileName"                    ,1    ,""                                    ,1},
    {"Sidecar"                              ,1    ,""                                    ,1},
    {"VisualSelectionX"                     ,9    ,0                                     ,1},
    {"VisualSelectionY"                     ,9    ,0                                     ,1},
    {"VisualSelectionWidth"                 ,9    ,0                                     ,1},
    {"VisualSelectionHeight"                ,9    ,0                                     ,1},
    {"ImageW"                               ,9    ,0                                     ,0},
    {"ImageH"                               ,9    ,0                                     ,0},
    {"PipeImageW"                           ,9    ,0                                     ,0},
    {"PipeImageH"                           ,9    ,0                                     ,0},
    {"CropX"                                ,9    ,0                                     ,1},
    {"CropY"                                ,9    ,0                                     ,1},
    {"CropW"                                ,9    ,0                                     ,1},
    {"CropH"                                ,9    ,0                                     ,1},
    {"RotateW"                              ,9    ,0                                     ,1},
    {"RotateH"                              ,9    ,0                                     ,1},
    {"ExposureNormalization"                ,9    ,0.0                                   ,0},
    {"CurveFileNamesRGB"                    ,0    ,QStringList()                         ,1},
    {"CurveFileNamesR"                      ,0    ,QStringList()                         ,1},
    {"CurveFileNamesG"                      ,0    ,QStringList()                         ,1},
    {"CurveFileNamesB"                      ,0    ,QStringList()                         ,1},
    {"CurveFileNamesL"                      ,0    ,QStringList()                         ,1},
    {"CurveFileNamesLa"                     ,0    ,QStringList()                         ,1},
    {"CurveFileNamesLb"                     ,0    ,QStringList()                         ,1},
    {"CurveFileNamesOutline"                ,0    ,QStringList()                         ,1},
    {"CurveFileNamesLByHue"                 ,0    ,QStringList()                         ,1},
    {"CurveFileNamesHue"                    ,0    ,QStringList()                         ,1},
    {"CurveFileNamesTexture"                ,0    ,QStringList()                         ,1},
    {"CurveFileNamesSaturation"             ,0    ,QStringList()                         ,1},
    {"CurveFileNamesBase"                   ,0    ,QStringList()                         ,1},
    {"CurveFileNamesBase2"                  ,0    ,QStringList()                         ,1},
    {"CurveFileNamesShadowsHighlights"      ,0    ,QStringList()                         ,1},
    {"CurveFileNamesDenoise"                ,0    ,QStringList()                         ,1},
    {"CurveFileNamesDenoise2"               ,0    ,QStringList()                         ,1},
    {"OutputFileName"                       ,9    ,""                                    ,0}, // Not in JobFile. Constructed.
    {"JobMode"                              ,9    ,0                                     ,0}, // Not in JobFile !! Overwrites else.
    {"InputFileNameList"                    ,9    ,QStringList()                         ,1},
    {"Tone1ColorRed"                        ,2    ,0                                     ,1},
    {"Tone1ColorGreen"                      ,2    ,200                                   ,1},
    {"Tone1ColorBlue"                       ,2    ,255                                   ,1},
    {"Tone2ColorRed"                        ,2    ,255                                   ,1},
    {"Tone2ColorGreen"                      ,2    ,200                                   ,1},
    {"Tone2ColorBlue"                       ,2    ,0                                     ,1},
    {"GradualOverlay1ColorRed"              ,2    ,0                                     ,1},
    {"GradualOverlay1ColorGreen"            ,2    ,0                                     ,1},
    {"GradualOverlay1ColorBlue"             ,2    ,0                                     ,1},
    {"GradualOverlay2ColorRed"              ,2    ,255                                   ,1},
    {"GradualOverlay2ColorGreen"            ,2    ,200                                   ,1},
    {"GradualOverlay2ColorBlue"             ,2    ,0                                     ,1},
    {"TextureOverlayFile"                   ,2    ,""                                    ,1},
    {"TextureOverlay2File"                  ,2    ,""                                    ,1},
    {"DigikamTagsList"                      ,9    ,QStringList()                         ,1},
    {"TagsList"                             ,9    ,QStringList()                         ,1},
    {"OutputFileNameSuffix"                 ,9    ,""                                    ,1},
    {"ImageTitle"                           ,9    ,""                                    ,1},
    {"Copyright"                            ,1    ,""                                    ,1},
    {"BackgroundRed"                        ,1    ,0                                     ,0},
    {"BackgroundGreen"                      ,1    ,0                                     ,0},
    {"BackgroundBlue"                       ,1    ,0                                     ,0},
    {"HistogramChannel"                     ,1    ,ptHistogramChannel_RGB                ,0},
    {"HistogramLogX"                        ,1    ,0                                     ,0},
    {"HistogramLogY"                        ,1    ,1                                     ,0},
    {"HistogramMode"                        ,1    ,ptHistogramMode_Preview               ,0},
    {"HistogramCrop"                        ,9    ,0                                     ,0},
    {"HistogramCropX"                       ,9    ,0                                     ,0},
    {"HistogramCropY"                       ,9    ,0                                     ,0},
    {"HistogramCropW"                       ,9    ,0                                     ,0},
    {"HistogramCropH"                       ,9    ,0                                     ,0},
    {"PixelReader"                          ,1    ,0                                     ,0},
    {"ExposureIndicator"                    ,1    ,0                                     ,0},
    {"ExposureIndicatorSensor"              ,0    ,0                                     ,0},
    {"ExposureIndicatorR"                   ,0    ,1                                     ,0},
    {"ExposureIndicatorG"                   ,0    ,1                                     ,0},
    {"ExposureIndicatorB"                   ,0    ,1                                     ,0},
    {"ExposureIndicatorOver"                ,0    ,1                                     ,0},
    {"ExposureIndicatorUnder"               ,0    ,1                                     ,0},
    {"ShowExposureIndicatorSensor"          ,0    ,0                                     ,0},
    {"ShowBottomContainer"                  ,1    ,1                                     ,0},
    {"ShowToolContainer"                    ,9    ,1                                     ,0},
    {"SatCurveType"                         ,1    ,0                                     ,1},
    {"TextureCurveType"                     ,1    ,0                                     ,1},
    {"DenoiseCurveType"                     ,1    ,0                                     ,1},
    {"Denoise2CurveType"                    ,1    ,0                                     ,1},
    {"HueCurveType"                         ,1    ,0                                     ,1},
    {"FullOutput"                           ,9    ,0                                     ,0},
    {"HiddenTools"                          ,0    ,QStringList(Fuid::LumaDenoiseCurve2_LabSN)
                                                                                         ,1},
    {"FavouriteTools"                       ,0    ,QStringList()                         ,0},
    {"BlockedTools"                         ,0    ,QStringList()                         ,1},
    {"DisabledTools"                        ,9    ,QStringList()                         ,0},
    {"BlockUpdate"                          ,9    ,0                                     ,0},
    {"FocalLengthIn35mmFilm"                ,0    ,0.0                                   ,0},
    {"ApertureFromExif"                     ,0    ,0.0                                   ,0},   // aperture from exif data
    {"DetailViewActive"                     ,9    ,0                                     ,0},
    {"DetailViewScale"                      ,9    ,0                                     ,0},
    {"DetailViewCropX"                      ,9    ,0                                     ,0},
    {"DetailViewCropY"                      ,9    ,0                                     ,0},
    {"DetailViewCropW"                      ,9    ,0                                     ,0},
    {"DetailViewCropH"                      ,9    ,0                                     ,0},
    {"TranslationMode"                      ,1    ,0                                     ,0},  // 0 no transl (English), 1 load qm file
    {"UiLanguage"                           ,1    ,""                                    ,0},  // Language name to load from qm file, e.g. "Deutsch"
    {"CustomCSSFile"                        ,1    ,""                                    ,0},
    {"FullscreenActive"                     ,9    ,0                                     ,0},

    // stuff for the file manager
    {"PreventFileMgrStartup"                ,9    ,0                                     ,0},
    {"FileMgrIsOpen"                        ,9    ,0                                     ,0},
    {"LastFileMgrLocation"                  ,1    ,""                                    ,0},
    {"FileMgrShowDirThumbs"                 ,1    ,1                                     ,0},
    {"FileMgrShowImageView"                 ,1    ,1                                     ,0},
    {"FileMgrShowSidebar"                   ,1    ,1                                     ,0},
    {"FileMgrThumbLayoutType"               ,1    ,tlVerticalByRow                       ,0},
    {"FileMgrShowRAWs"                      ,1    ,1                                     ,0},
    {"FileMgrShowBitmaps"                   ,1    ,1                                     ,0},
    {"BatchIsOpen"                          ,9    ,0                                     ,0},
    {"BatchLogIsVisible"                    ,1    ,0                                     ,0}
  };

   // Gui Numerical inputs. Copy them from the const array in ptSettingItem.
  short NrSettings = sizeof(GuiInputItems)/sizeof(ptGuiInputItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiInputItem Description = GuiInputItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Description.GuiType;
    SettingItem->InitLevel       = Description.InitLevel;
    SettingItem->InJobFile       = Description.InJobFile;
    SettingItem->HasDefaultValue = Description.HasDefaultValue;
    SettingItem->DefaultValue    = Description.DefaultValue;
    SettingItem->MinimumValue    = Description.MinimumValue;
    SettingItem->MaximumValue    = Description.MaximumValue;
    SettingItem->Step            = Description.Step;
    SettingItem->NrDecimals      = Description.NrDecimals;
    SettingItem->Label           = Description.Label;
    SettingItem->ToolTip         = Description.ToolTip;
    M_InsertKeyIntoHash(Description.KeyName,SettingItem);
  }
  // Gui Choice inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiChoiceItems)/sizeof(ptGuiChoiceItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiChoiceItem Description = GuiChoiceItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Description.GuiType;
    SettingItem->InitLevel       = Description.InitLevel;
    SettingItem->InJobFile       = Description.InJobFile;
    SettingItem->HasDefaultValue = Description.HasDefaultValue;
    SettingItem->DefaultValue    = Description.DefaultValue;
    SettingItem->Value           = Description.DefaultValue;
    SettingItem->ToolTip         = Description.ToolTip;
    SettingItem->InitialOptions  = Description.InitialOptions;
    M_InsertKeyIntoHash(Description.KeyName,SettingItem);
  }
  // Gui Check inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiCheckItems)/sizeof(ptGuiCheckItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiCheckItem Description = GuiCheckItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = Description.GuiType;
    SettingItem->InitLevel    = Description.InitLevel;
    SettingItem->InJobFile    = Description.InJobFile;
    SettingItem->DefaultValue = Description.DefaultValue;
    SettingItem->Value        = Description.DefaultValue;
    SettingItem->Label        = Description.Label;
    SettingItem->ToolTip      = Description.ToolTip;
    M_InsertKeyIntoHash(Description.KeyName,SettingItem);
  }
  // Non gui elements
  NrSettings = sizeof(Items)/sizeof(ptItem);
  for (short i=0; i<NrSettings; i++) {
    ptItem Description = Items[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = ptGT_None;
    SettingItem->InitLevel    = Description.InitLevel;
    if (Description.DefaultValue.type() == QVariant::String) {
      QString Tmp = Description.DefaultValue.toString();
      Tmp.replace(QString("@INSTALL@"),QCoreApplication::applicationDirPath());
      Description.DefaultValue = Tmp;
    }
    SettingItem->DefaultValue = Description.DefaultValue;
    SettingItem->Value        = Description.DefaultValue;
    SettingItem->InJobFile    = Description.InJobFile;
    M_InsertKeyIntoHash(Description.KeyName,SettingItem);
  }

  // Now we have initialized from static values.
  // In the second round we overwrite now with what's coming from the ini
  // files.

  // Persistent settings.
  m_IniSettings = new QSettings(Path + "photivo.ini", QSettings::IniFormat);

  QStringList Keys = m_Hash.keys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    ptSettingItem* Setting = m_Hash[Key];

    if (InitLevel > Setting->InitLevel) {
      // Default needs to be overwritten by something coming from ini file.
      Setting->Value = m_IniSettings->value(Key,Setting->DefaultValue);
      // Correction needed as the values coming from the ini file are
      // often interpreted as strings even if they could be int or so.
      const QVariant::Type TargetType = Setting->DefaultValue.type();
      if (Setting->Value.type() != TargetType) {
        if (TargetType == QVariant::Int ||
          TargetType == QVariant::UInt) {
          Setting->Value = Setting->Value.toInt();
        } else if (TargetType == QVariant::Double ||
                   (QMetaType::Type) TargetType == QMetaType::Float) {
          Setting->Value = Setting->Value.toDouble();
        } else if (TargetType == QVariant::StringList) {
          Setting->Value = Setting->Value.toStringList();
        } else {
          ptLogError(ptError_Argument,"Unexpected type %d",TargetType);
          assert(0);
        }
      }
      // Seen above this shouldn't happen, but better safe then sorry.
      if (Setting->Value.type() != Setting->DefaultValue.type()) {
        ptLogError(ptError_Argument,
                   "Type conversion error from ini file. Should : %d Is : %d\n",
                   Setting->DefaultValue.type(),Setting->Value.type());
        assert(Setting->Value.type() == Setting->DefaultValue.type());
      }
    } else {
      Setting->Value = Setting->DefaultValue;
    }
  }

  // Some ad-hoc corrections
  QFileInfo PathInfo(GetValue("PreviewColorProfile").toString());
  SetValue("PreviewColorProfile",PathInfo.absoluteFilePath());
  PathInfo.setFile(GetValue("PreviewColorProfile").toString());
  SetValue("PreviewColorProfile",PathInfo.absoluteFilePath());
  SetValue("Scaled",GetValue("PipeSize"));
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
// Basically dumping the whole Settings hash to ini files.
//
////////////////////////////////////////////////////////////////////////////////

ptSettings::~ptSettings() {
  QStringList Keys = m_Hash.keys();
  for (int i=0; i<Keys.size(); i++) {
    QString Key = Keys[i];
    ptSettingItem* Setting = m_Hash[Key];
    m_IniSettings->setValue(Key,Setting->Value);
  }
  // Explicit call destructor (such that synced to disk)
  delete m_IniSettings;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetValue
// Access to the hash, but with protection on non existing key.
//
////////////////////////////////////////////////////////////////////////////////

const QVariant ptSettings::GetValue(const QString Key) const {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  return m_Hash[Key]->Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetInt
//
////////////////////////////////////////////////////////////////////////////////

int ptSettings::GetInt(const QString Key) const {
  // Remark : UInt and Int are mixed here.
  // The only settings related type where u is important is uint16_t
  // (dimensions). uint16_t fits in an integer which is 32 bit.
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::Int && Tmp.type() != QVariant::UInt) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::(U)Int' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toLocal8Bit().data());
    if (Tmp.type() == QVariant::String) {
      ptLogError(ptError_Argument,
                 "Additionally : it's a string '%s'\n",
                 Tmp.toString().toLocal8Bit().data());
    }
    assert(Tmp.type() == QVariant::Int);
  }
  return Tmp.toInt();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetDouble
//
////////////////////////////////////////////////////////////////////////////////

double ptSettings::GetDouble(const QString Key) const {
  QVariant Tmp = GetValue(Key);
  if (static_cast<QMetaType::Type>(Tmp.type()) == QMetaType::Float)
    Tmp.convert(QVariant::Double);
  if (Tmp.type() != QVariant::Double) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::Double' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toLocal8Bit().data());
    assert(Tmp.type() == QVariant::Double);
  }
  return Tmp.toDouble();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetString
//
////////////////////////////////////////////////////////////////////////////////

const QString ptSettings::GetString(const QString Key) const {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::String) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::String' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toLocal8Bit().data());
    assert(Tmp.type() == QVariant::String);
  }
  return Tmp.toString();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetStringList
//
////////////////////////////////////////////////////////////////////////////////

const QStringList ptSettings::GetStringList(const QString Key) const {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::StringList) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::StringList' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toLocal8Bit().data());
    assert(Tmp.type() == QVariant::StringList);
  }
  return Tmp.toStringList();
}

////////////////////////////////////////////////////////////////////////////////
//
// SetValue
// Access to the hash, but with protection on non existing key.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetValue(const QString Key, const QVariant Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->Value = Value;
  // In job mode there are no gui elements and we have to return.
  if (GetInt("JobMode")) return;
  // If it's a gui element, we have to update it at once for consistency.
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->SetValue(Value);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->SetValue(Value);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->SetValue(Value);
      break;
    default:
      assert(m_Hash[Key]->GuiType == ptGT_None); // Else we missed a gui one
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetEnabled
// Enable the underlying gui element (if one)
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetEnabled(const QString Key, const short Enabled) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->SetEnabled(Enabled);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->SetEnabled(Enabled);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->SetEnabled(Enabled);
      break;
    default:
      ptLogError(ptError_Argument,
                 "%s is no (expected) gui element.",
                 Key.toLocal8Bit().data());
      assert(m_Hash[Key]->GuiType); // Should have gui type !
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// SetMaximum
// Makes only sense for gui input element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetMaximum(const QString Key, const QVariant Maximum) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiInput) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiInput\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiInput->SetMaximum(Maximum);
}

////////////////////////////////////////////////////////////////////////////////
//
// Show (or hide)
// Makes only sense for gui element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void  ptSettings::Show(const QString Key, const short Show) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  switch (m_Hash[Key]->GuiType) {
    case ptGT_Input :
    case ptGT_InputSlider :
    case ptGT_InputSliderHue :
      m_Hash[Key]->GuiInput->Show(Show);
      break;
    case ptGT_Choice :
      m_Hash[Key]->GuiChoice->Show(Show);
      break;
    case ptGT_Check :
      m_Hash[Key]->GuiCheck->Show(Show);
      break;
    default:
      ptLogError(ptError_Argument,
                 "%s is no (expected) gui element.",
                 Key.toLocal8Bit().data());
      assert(m_Hash[Key]->GuiType);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// AddOrReplaceOption
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::AddOrReplaceOption(const QString  Key,
                                    const QString  Text,
                                    const QVariant Value) {
  // In job mode there are no gui elements and we have to return.
  if (GetInt("JobMode")) return;
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  m_Hash[Key]->GuiChoice->AddOrReplaceItem(Text,Value);
}

////////////////////////////////////////////////////////////////////////////////
//
// ClearOptions
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::ClearOptions(const QString  Key, const short WithDefault) {
  // In job mode there are no gui elements and we have to return.
  if (GetInt("JobMode")) return;
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  m_Hash[Key]->GuiChoice->Clear(WithDefault);
}

////////////////////////////////////////////////////////////////////////////////
//
// GetNrOptions
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

int ptSettings::GetNrOptions(const QString Key) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->Count();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetOptionsValue (at index Index)
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

const QVariant ptSettings::GetOptionsValue(const QString Key,const int Index){
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->GetItemData(Index);
}

////////////////////////////////////////////////////////////////////////////////
//
// GetCurrentText
// Makes only sense for gui choice (combo) element, which is asserted.
//
////////////////////////////////////////////////////////////////////////////////

const QString ptSettings::GetCurrentText(const QString Key){
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toLocal8Bit().data());
    assert (m_Hash[Key]->GuiChoice);
  }
  return m_Hash[Key]->GuiChoice->CurrentText();
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiInput
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiInput(const QString Key, ptInput* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiInput  = Value;
  m_Hash[Key]->GuiChoice = NULL;
  m_Hash[Key]->GuiCheck  = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiChoice
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiChoice(const QString Key, ptChoice* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiInput  = NULL;
  m_Hash[Key]->GuiChoice = Value;
  m_Hash[Key]->GuiCheck  = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetGuiCheck
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::SetGuiCheck(const QString Key, ptCheck* Value) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiInput  = NULL;
  m_Hash[Key]->GuiChoice = NULL;
  m_Hash[Key]->GuiCheck  = Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetGuiWidget
// low level access to the underlying QWidget
//
////////////////////////////////////////////////////////////////////////////////

QWidget* ptSettings::GetGuiWidget(const QString Key) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toLocal8Bit().data());
    assert (m_Hash.contains(Key));
  }
  if (m_Hash[Key]->GuiInput) {
    return (QWidget*)m_Hash[Key]->GuiInput;
  } else if (m_Hash[Key]->GuiChoice) {
    return (QWidget*)m_Hash[Key]->GuiChoice;
  } else if (m_Hash[Key]->GuiCheck) {
    return (QWidget*)m_Hash[Key]->GuiCheck;
  }

  assert (!"No Gui widget");
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Transfer the settings from Settings To DcRaw UserSettings.
// This is sometimes not straightforward as DcRaw assumes certain
// combinations. That's taken care of here.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::ToDcRaw(ptDcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Relying on m_PipeSize being as defined in the constants ! (1<<Size)
  TheDcRaw->m_UserSetting_HalfSize = GetInt("PipeSize");

  // Input file name

  QString InputFileName = GetStringList("InputFileNameList")[0];
  FREE(TheDcRaw->m_UserSetting_InputFileName);
  TheDcRaw->m_UserSetting_InputFileName =
    (char*) MALLOC(1+strlen(InputFileName.toLocal8Bit().data()));
  ptMemoryError(TheDcRaw->m_UserSetting_InputFileName,__FILE__,__LINE__);
  strcpy(TheDcRaw->m_UserSetting_InputFileName,InputFileName.toLocal8Bit().data());

  // Detail view
  TheDcRaw->m_UserSetting_DetailView = Settings->GetInt("DetailViewActive");
  TheDcRaw->m_UserSetting_DetailViewCropX = Settings->GetInt("DetailViewCropX");
  TheDcRaw->m_UserSetting_DetailViewCropY = Settings->GetInt("DetailViewCropY");
  TheDcRaw->m_UserSetting_DetailViewCropW = Settings->GetInt("DetailViewCropW");
  TheDcRaw->m_UserSetting_DetailViewCropH = Settings->GetInt("DetailViewCropH");

  // Adjust Maximum
  TheDcRaw->m_UserSetting_AdjustMaximum = GetDouble("AdjustMaximumThreshold");

  // Denoise.
  TheDcRaw->m_UserSetting_DenoiseThreshold = GetInt("RawDenoiseThreshold");

  // Hotpixel reduction
  TheDcRaw->m_UserSetting_HotpixelReduction = GetDouble("HotpixelReduction");

  // Bayer denoise
  TheDcRaw->m_UserSetting_BayerDenoise = GetInt("BayerDenoise");

  // Green equilibration
  TheDcRaw->m_UserSetting_GreenEquil = GetInt("GreenEquil");

  // CA auto correction
  TheDcRaw->m_UserSetting_CaCorrect = GetInt("CaCorrect");
  TheDcRaw->m_UserSetting_CaRed = GetDouble("CaRed");
  TheDcRaw->m_UserSetting_CaBlue = GetDouble("CaBlue");

  // CFA Line denoise
  TheDcRaw->m_UserSetting_CfaLineDn = GetInt("CfaLineDenoise");

  // Interpolation
  TheDcRaw->m_UserSetting_Quality = GetInt("Interpolation");

  // White balance settings.
  switch (GetInt("WhiteBalance")) {
    case ptWhiteBalance_Camera :
      TheDcRaw->m_UserSetting_CameraWb = 1;
      TheDcRaw->m_UserSetting_AutoWb   = 0;
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
      break;
    case ptWhiteBalance_Auto :
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 1;
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
      break;
    case ptWhiteBalance_Spot :
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 1; // GreyBox must have auto on !
      TheDcRaw->m_UserSetting_Multiplier[0] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[1] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[2] = 0.0;
      TheDcRaw->m_UserSetting_Multiplier[3] = 0.0;

      // The selection, which is in preview coordinates, must
      // be transformed back to the original.
      // Express always in size of original image !
      // Also take into account TheDcRaw->m_Flip

      { // Jump to case label issue.
      int TmpPipeSize = GetInt("PipeSize");
      uint16_t X = (1<<TmpPipeSize) * GetInt("VisualSelectionX");
      uint16_t Y = (1<<TmpPipeSize) * GetInt("VisualSelectionY");
      uint16_t W = (1<<TmpPipeSize) * GetInt("VisualSelectionWidth");
      uint16_t H = (1<<TmpPipeSize) * GetInt("VisualSelectionHeight");

      uint16_t TargetW = W;
      uint16_t TargetH = H;
      if (TheDcRaw->m_Flip & 4) {
        SWAP(X,Y);
        SWAP(TargetW,TargetH);
      }
      if (TheDcRaw->m_Flip & 2) Y = GetInt("ImageH")-1-Y-TargetH;
      if (TheDcRaw->m_Flip & 1) X = GetInt("ImageW")-1-X-TargetW;
      TheDcRaw->m_UserSetting_GreyBox[0] = X;
      TheDcRaw->m_UserSetting_GreyBox[1] = Y;
      TheDcRaw->m_UserSetting_GreyBox[2] = TargetW;
      TheDcRaw->m_UserSetting_GreyBox[3] = TargetH;
      }

      TRACEKEYVALS("GreyBox[0]","%d",TheDcRaw->m_UserSetting_GreyBox[0]);
      TRACEKEYVALS("GreyBox[1]","%d",TheDcRaw->m_UserSetting_GreyBox[1]);
      TRACEKEYVALS("GreyBox[2]","%d",TheDcRaw->m_UserSetting_GreyBox[2]);
      TRACEKEYVALS("GreyBox[3]","%d",TheDcRaw->m_UserSetting_GreyBox[3]);

      break;
    default : // this entails as well manual as preset from ptWhiteBalances.
      TheDcRaw->m_UserSetting_CameraWb = 0;
      TheDcRaw->m_UserSetting_AutoWb   = 0;
      TheDcRaw->m_UserSetting_Multiplier[0]= GetDouble("RMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[1]= GetDouble("GMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[2] =GetDouble("BMultiplier");
      TheDcRaw->m_UserSetting_Multiplier[3] =
        TheDcRaw->m_Colors == 4 ? GetDouble("GMultiplier") : 0.0;
      TheDcRaw->m_UserSetting_GreyBox[0] = 0;
      TheDcRaw->m_UserSetting_GreyBox[1] = 0;
      TheDcRaw->m_UserSetting_GreyBox[2] = 0xFFFF;
      TheDcRaw->m_UserSetting_GreyBox[3] = 0xFFFF;
  }

  // Bad pixels settings.
  switch(GetInt("HaveBadPixels")) {
    case 0 : // None
      TheDcRaw->m_UserSetting_BadPixelsFileName = NULL;
      break;
    case 1 : // Load one : should not happen !
      assert(0);
      break;
    case 2 :
      FREE(TheDcRaw->m_UserSetting_BadPixelsFileName);
      TheDcRaw->m_UserSetting_BadPixelsFileName = (char *)
        MALLOC(1+
             strlen(GetString("BadPixelsFileName").toLocal8Bit().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_BadPixelsFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_BadPixelsFileName,
             GetString("BadPixelsFileName").toLocal8Bit().data());
      break;
    default :
      assert(0);
  }

  // Dark frame settings.
  switch(GetInt("HaveDarkFrame")) {
    case 0 : // None
      TheDcRaw->m_UserSetting_DarkFrameFileName = NULL;
      break;
    case 1 : // Load one : should not happen !
      assert(0);
      break;
    case 2 :
      FREE(TheDcRaw->m_UserSetting_DarkFrameFileName);
      TheDcRaw->m_UserSetting_DarkFrameFileName = (char *)
        MALLOC(1+
             strlen(GetString("DarkFrameFileName").toLocal8Bit().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_DarkFrameFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_DarkFrameFileName,
             GetString("DarkFrameFileName").toLocal8Bit().data());
      break;
    default :
      assert(0);
  }

  // Blackpoint settings.
  switch (GetInt("ManualBlackPoint")) {
    case 0 : // Automatic.
      TheDcRaw->m_UserSetting_BlackPoint = -1;
      break;
    default : // Manual set
      TheDcRaw->m_UserSetting_BlackPoint = GetInt("BlackPoint");
      break;
  }

  // Whitepoint settings.
  switch (GetInt("ManualWhitePoint")) {
    case 0 : // Automatic.
      TheDcRaw->m_UserSetting_Saturation = -1;
      break;
    default : // Manual set
      TheDcRaw->m_UserSetting_Saturation = GetInt("WhitePoint");
      break;
  }
  // Normalization of Multipliers
  TheDcRaw->m_UserSetting_MaxMultiplier = GetInt("MultiplierEnhance");

  // Interpolation passes.
  TheDcRaw->m_UserSetting_InterpolationPasses = GetInt("InterpolationPasses");

  // Median Filter.
  TheDcRaw->m_UserSetting_MedianPasses = GetInt("MedianPasses");
  TheDcRaw->m_UserSetting_ESMedianPasses = GetInt("ESMedianPasses");
  TheDcRaw->m_UserSetting_EeciRefine = GetInt("EeciRefine");

  // Clip factor
  TheDcRaw->m_UserSetting_photivo_ClipMode       = GetInt("ClipMode");
  TheDcRaw->m_UserSetting_photivo_ClipParameter  = GetInt("ClipParameter");
}

////////////////////////////////////////////////////////////////////////////////
//
// Transfers the settings from DcRaw back to the Gui.
// In some situations , DcRaw can overwrite or calculate values.
// The output of those are here fed back to the Gui settings such
// that they are correctly reflected.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::FromDcRaw(ptDcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Copy make and model to our gui settings (f.i.
  // white balances rely on it)
  SetValue("CameraMake",TheDcRaw->m_CameraMake);
  SetValue("CameraModel",TheDcRaw->m_CameraModel);

  // Reset spot white balance
  if (GetInt("WhiteBalance") == ptWhiteBalance_Spot)
    SetValue("WhiteBalance", ptWhiteBalance_Manual);

  // Multipliers.
  SetValue("RMultiplier",VALUE(TheDcRaw->m_PreMultipliers[0]));
  SetValue("GMultiplier",VALUE(TheDcRaw->m_PreMultipliers[1]));
  SetValue("BMultiplier",VALUE(TheDcRaw->m_PreMultipliers[2]));

  // (D65) Multipliers to ColorTemperature.
  //
  // m_D65Multipliers are supposed to be D65
  // (setting Pre to the ratio of the D65 delivers
  // rgbWB = (x,x,x) and 6500 temperature).

  double RefRGB[3];
  if (TheDcRaw->m_RawColorPhotivo) {
    for (short c=0; c<3; c++) {
      RefRGB[c] = VALUE(TheDcRaw->m_D65Multipliers[c]) /
                  VALUE(TheDcRaw->m_PreMultipliers[c]);
    }
  } else {
    // If not raw, we have to calculate back sRGB references to the cam rgb.
    for (short c=0; c<3; c++) {
      RefRGB[c] = 0;
      for (short cc=0; cc<TheDcRaw->m_Colors ; cc++) {
        RefRGB[c] += TheDcRaw->m_MatrixCamRGBToSRGB[c][cc] *
                     VALUE(TheDcRaw->m_D65Multipliers[cc]) /
                     VALUE(TheDcRaw->m_PreMultipliers[cc]);
      }
    }
  }

  int    TmpColorTemperature;
  double TmpGreenIntensity;
  RGBToTemperature(RefRGB,
                   &TmpColorTemperature,
                   &TmpGreenIntensity);
  SetValue("ColorTemperature",TmpColorTemperature);
  SetValue("GreenIntensity",TmpGreenIntensity);

  // Blackpoint setting in case not manual.
  if (!GetInt("ManualBlackPoint")) {
    SetValue("BlackPoint",TheDcRaw->m_BlackLevel_AfterPhase1);
  }

  // Whitepoint setting in case not manual.
  if (!GetInt("ManualWhitePoint")) {
    SetValue("WhitePoint",TheDcRaw->m_WhiteLevel_AfterPhase1);
  }

  // EOS exposure normalization might be a result of running DcRaw
  // (ie we have to normalize the exposure further in the flow.
  // TODO This is coming from ufraw. Seems fair, but no clue
  // what's the logic behind the calculation. Someone ?

  double EOSExposureNormalization = 1.0;
  if (strcmp(TheDcRaw->m_CameraMake, "Canon")==0 &&
      strncmp(TheDcRaw->m_CameraModel, "EOS", 3)==0 ) {
    int Max = (int) VALUE(TheDcRaw->m_CameraMultipliers[0]);
    for (short c=1; c<TheDcRaw->m_Colors; c++) {
      if (VALUE(TheDcRaw->m_CameraMultipliers[c]) > Max) {
        Max = (int) VALUE(TheDcRaw->m_CameraMultipliers[c]);
      }
    }
    // The 100 is a pretty random value that makes sure above
    // was a 'right' number calculated and it's not bogus
    // due to for instance not being able reading CamMultipliers
    if ( Max > 100 ) {
      EOSExposureNormalization =
        (4096.0-TheDcRaw->m_BlackLevel_AfterPhase1)/Max;
    }
  }

  TRACEKEYVALS("EOSExposureNorm","%f",EOSExposureNormalization);

  SetValue("ExposureNormalization",
           // EV conversion !
           log(EOSExposureNormalization/TheDcRaw->m_MinPreMulti)/log(2));

  TRACEKEYVALS("ExposureNorm(EV)","%f", GetDouble("ExposureNormalization"));
}

//==============================================================================

bool ptSettings::useRAWHandling() const
{
  return (GetInt("IsRAW") == 1) && (GetInt("UseThumbnail") == 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Tool Info
// IsActive contains, if the filter will be processed!
//
////////////////////////////////////////////////////////////////////////////////

struct sToolInfo {
  QString               Name;
  short                 IsActive;
  short                 IsHidden;
  short                 IsBlocked;
  short                 IsDisabled;
};

sToolInfo ToolInfo (const QString GuiName) {
  sToolInfo Info = {"N.N.",0,0,0,0};

  // Tab Geometry
  if (GuiName == "TabLensfunCA") {
    Info.Name = "Chromatic Aberration (Lensfun)";
    Info.IsActive = Settings->GetInt("LfunCAModel") != 0;
  } else if (GuiName == "TabLensfunVignette") {
    Info.Name = "Vignetting (Lensfun)";
    Info.IsActive = Settings->GetInt("LfunVignetteModel") != 0;
  } else if (GuiName == "TabLensfunDistortion") {
    Info.Name = "Lens Distortion (Lensfun)";
    Info.IsActive = Settings->GetInt("LfunDistModel") != 0;
  } else if (GuiName == "TabLensfunGeometry") {
    Info.Name = "Geometry Conversion (Lensfun)";
    Info.IsActive = Settings->GetInt("LfunSrcGeo") != Settings->GetInt("LfunTargetGeo");
  } else if (GuiName == "TabDefish") {
      Info.Name = "Defish";
      Info.IsActive = Settings->GetInt("Defish");
  } else if (GuiName == "TabRotation") {
      Info.Name = "Rotation";
      Info.IsActive = (Settings->GetDouble("Rotate")!=0.0f ||
                       Settings->GetDouble("PerspectiveTilt")!=0.0f ||
                       Settings->GetDouble("PerspectiveTurn")!=0.0f ||
                       Settings->GetDouble("PerspectiveScaleX")!=1.0f ||
                       Settings->GetDouble("PerspectiveScaleY")!=1.0f)?1:0;
  } else if (GuiName == "TabLiquidRescale") {
      Info.Name = "Seam carving";
      Info.IsActive = Settings->GetInt("LqrEnergy");
  } else if (GuiName == "TabCrop") {
      Info.Name = "Crop";
      Info.IsActive = Settings->GetInt("Crop");
  } else if (GuiName == "TabResize") {
      Info.Name = "Resize";
      Info.IsActive = Settings->GetInt("Resize");
  } else if (GuiName == "TabFlip") {
      Info.Name = "Flip";
      Info.IsActive = Settings->GetInt("FlipMode");
  } else if (GuiName == "TabBlock") {
      Info.Name = "Block";
      Info.IsActive = !Settings->GetInt("JobMode") &&
                      Settings->GetInt("GeometryBlock");
  }
  // Tab RGB
  else if (GuiName == "TabExposure") {
      Info.Name = "Exposure";
      Info.IsActive = Settings->GetDouble("Exposure")!=0.0?1:0;
  }
  // Lab Sharpen and Noise
  else if (GuiName == "TabLABEAW") {
      Info.Name = "Lab EAW equalizer";
      Info.IsActive = (Settings->GetDouble("EAWLevel1") != 0.0 ||
                       Settings->GetDouble("EAWLevel2") != 0.0 ||
                       Settings->GetDouble("EAWLevel3") != 0.0 ||
                       Settings->GetDouble("EAWLevel4") != 0.0 ||
                       Settings->GetDouble("EAWLevel5") != 0.0 ||
                       Settings->GetDouble("EAWLevel6") != 0.0)?1:0;
  } else if (GuiName == "TabLABGreyC") {
      Info.Name = "Lab GreyCStoration";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("GREYCLab")) ||
                       Settings->GetInt("GREYCLab")>=2)!=0?1:0;
  } else if (GuiName == "TabLABDefringe") {
      Info.Name = "Lab Defringe";
      Info.IsActive = Settings->GetDouble("DefringeRadius")!=0.0;
  } else if (GuiName == "TabWaveletDenoise") {
      Info.Name = "Lab Wavelet denoise";
      Info.IsActive = (Settings->GetDouble("WaveletDenoiseL")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseA")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseB")!=0.0)?1:0;
  } else if (GuiName == "TabColorDenoise") {
      Info.Name = "Lab Levels";
      Info.IsActive = (Settings->GetDouble("BilateralASigmaR")!=0.0 ||
                      Settings->GetDouble("BilateralBSigmaR")!=0.0)?1:0;
  } else if (GuiName == "TabLABGradientSharpen") {
      Info.Name = "Lab Gradient Sharpen";
      Info.IsActive = Settings->GetInt("GradientSharpenPasses") ||
                      Settings->GetDouble("MLMicroContrastStrength")!=0.0 ||
                      Settings->GetDouble("LabHotpixel")!=0.0;
  } else if (GuiName == "TabInverseDiffusion") {
      Info.Name = "Lab inverse diffusion";
      Info.IsActive = Settings->GetInt("InverseDiffusionIterations")!=0?1:0;
  } else if (GuiName == "TabLABUSM") {
      Info.Name = "Lab USM";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("USM")) ||
                       Settings->GetInt("USM")==2)!=0?1:0;
  } else if (GuiName == "TabLABHighpass") {
      Info.Name = "Lab highpass";
      Info.IsActive = ((Settings->GetInt("FullOutput") &&
                        Settings->GetInt("Highpass")) ||
                       Settings->GetInt("Highpass")==2)!=0?1:0;
  } else if (GuiName == "TabLABFilmGrain") {
      Info.Name = "Lab film grain";
      Info.IsActive = (Settings->GetInt("Grain1MaskType") ||
                      Settings->GetInt("Grain2MaskType"))!=0?1:0;
  } else if (GuiName == "TabLABViewLab") {
      Info.Name = "Lab view Lab";
      Info.IsActive = Settings->GetInt("ViewLAB")!=0?1:0;
  }
  // Tab Lab EyeCandy
  else if (GuiName == "TabLABVignette") {
      Info.Name = "Lab vignette";
      Info.IsActive = (Settings->GetInt("LabVignetteMode") != 0 &&
                       Settings->GetDouble("LabVignetteAmount") != 0.0)!=0?1:0;
  }
  // Tab EyeCandy
  else if (GuiName == "TabBW") {
      Info.Name = "Black and White";
      Info.IsActive = Settings->GetDouble("BWStylerOpacity")!=0.0?1:0;
  } else if (GuiName == "TabSimpleTone") {
      Info.Name = "Simple toning";
      Info.IsActive = (Settings->GetDouble("SimpleToneR") !=0.0 ||
                       Settings->GetDouble("SimpleToneG") !=0.0 ||
                       Settings->GetDouble("SimpleToneB") !=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBTone1") {
      Info.Name = "RGB Toning 1";
      Info.IsActive = Settings->GetInt("Tone1MaskType")!=0?1:0;
  } else if (GuiName == "TabRGBTone2") {
      Info.Name = "RGB Toning 2";
      Info.IsActive = Settings->GetInt("Tone2MaskType")!=0?1:0;
  } else if (GuiName == "TabCrossProcessing") {
      Info.Name = "Cross processing";
      Info.IsActive = Settings->GetInt("CrossprocessingMode")!=0?1:0;
  } else if (GuiName == "TabTextureOverlay") {
    Info.Name = "Texture Overlay";
    Info.IsActive = (Settings->GetInt("TextureOverlayMode") &&
                     Settings->GetDouble("TextureOverlayOpacity")!=0.0)!=0?1:0;
  } else if (GuiName == "TabTextureOverlay2") {
    Info.Name = "Texture Overlay 2";
    Info.IsActive = (Settings->GetInt("TextureOverlay2Mode") &&
                     Settings->GetDouble("TextureOverlay2Opacity")!=0.0)!=0?1:0;
  } else if (GuiName == "TabGradualOverlay1") {
      Info.Name = "Gradual Overlay 1";
      Info.IsActive = (Settings->GetInt("GradualOverlay1") &&
                       Settings->GetDouble("GradualOverlay1Amount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabGradualOverlay2") {
      Info.Name = "Gradual Overlay 2";
      Info.IsActive = (Settings->GetInt("GradualOverlay2") &&
                       Settings->GetDouble("GradualOverlay2Amount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBVignette") {
      Info.Name = "Eyecandy vignette";
      Info.IsActive = (Settings->GetInt("VignetteMode") &&
                       Settings->GetDouble("VignetteAmount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabGradualBlur1") {
      Info.Name = "Gradual blur 1";
      Info.IsActive = Settings->GetDouble("GradBlur1Radius")!=0.0?1:0;
  } else if (GuiName == "TabGradualBlur2") {
      Info.Name = "Gradual blur 2";
      Info.IsActive = Settings->GetDouble("GradBlur2Radius")!=0.0?1:0;
  } else if (GuiName == "TabSoftglow") {
      Info.Name = "Softglow / Orton";
      Info.IsActive = (Settings->GetInt("SoftglowMode") &&
                       Settings->GetDouble("SoftglowAmount")!=0.0)!=0?1:0;
  }
  // Tab Output
  else if (GuiName == "TabGammaCompensation") {
      Info.Name = "Output gamma compensation";
      Info.IsActive = Settings->GetInt("OutputGammaCompensation")!=0?1:0;
  } else if (GuiName == "TabWebResize") {
      Info.Name = "Output web resize";
      Info.IsActive = (Settings->GetInt("WebResize")==2 ||
                       (Settings->GetInt("FullOutput") &&
                        Settings->GetInt("WebResize")==1))!=0?1:0;
  }

  // tool blocked?
  Info.IsBlocked = (Settings->GetStringList("BlockedTools")).contains(GuiName)?1:0;

  // tool hidden?
  Info.IsHidden = (Settings->GetStringList("HiddenTools")).contains(GuiName)?1:0;

  // tool disabled?
  Info.IsDisabled = (Settings->GetStringList("DisabledTools")).contains(GuiName)?1:0;
  return Info;
}

int ptSettings::ToolAlwaysVisible(const QString GuiName) const {
  QStringList VisibleTools =
  (QStringList()
    // Settings tab
    << "TabWorkColorSpace"
    << "TabPreviewColorSpace"
    << "TabUISettings"
    << "TabGimpCommand"
    << "TabRememberSettings"
    << "TabStartupSettings"
    << "TabCropSettings"
    << "TabInputControl"
    << "TabToolBoxControl"
    << "TabTabStatusIndicator"
    << "TabPreviewControl"
    << "TabTheming"
    << "TabConfigSaveMode"
    << "TabSearchBar"
    << "TabConfirmDialogs"
    << "TabBackupSettings"
    << "TabTranslation"
    << "TabMemoryTest"
    << "TabVisibleTools"
    << "TabFileMgrSettings"
    // Info tab
    << "TabInfoPhotivo"
    << "TabInfoFile"
    << "TabInfoExif"
    << "TabInfoSizes"
    // Input tab
    << "TabInput"
    << "TabCameraColorSpace"
    << "TabGenCorrections"
    << "TabWhiteBalance"
    << "TabDemosaicing"
    << "TabHighlightRecovery"
    // Output Tab
    << "TabOutputColorSpace"
    << "TabOutParameters"
    << "TabOutput");
  if (VisibleTools.contains(GuiName)) return 1;
  return 0;
}

QString ptSettings::ToolGetName (const QString GuiName) const {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.Name;
}

int ptSettings::ToolIsActive (const QString GuiName) const {
  sToolInfo Info = ToolInfo(GuiName);
  return (Info.IsHidden || Info.IsBlocked || Info.IsDisabled)?0:Info.IsActive;
}

int ptSettings::ToolIsBlocked (const QString GuiName) const {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsBlocked;
}

int ptSettings::ToolIsHidden (const QString GuiName) const {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsHidden;
}
///////////////////////////////////////////////////////////////////////////////
