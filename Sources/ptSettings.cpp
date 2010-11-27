////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <assert.h>

#include "ptSettings.h"
#include "ptError.h"
#include "ptRGBTemperature.h"
#include "ptGuiOptions.h"

// Macro for inserting a key into the hash and checking it is a new one.
#define M_InsertKeyIntoHash(Key,Item)                      \
  if (m_Hash.contains(Key)) {                              \
    ptLogError(ptError_Argument,                           \
               "Inserting an existing key (%s)",           \
               Key.toAscii().data());                      \
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
    // Unique Name,GuiElement,InitLevel,InJobFile,HasDefault (causes button too !),Default,Min,Max,Step,NrDecimals,Label,ToolTip
    {"MemoryTest"                    ,ptGT_InputSlider     ,9,0,1 ,0    ,0    ,500   ,50   ,0 ,tr("MB")                 ,tr("MB to waste")},
    {"TabStatusIndicator"            ,ptGT_Input           ,1,0,1 ,8    ,0    ,16    ,1    ,0 ,tr("Pixel")              ,tr("Size of the LED")},
    {"Zoom"                          ,ptGT_InputSlider     ,9,0,0 ,100  ,5    ,400   ,10   ,0 ,tr("Zoom")               ,tr("Zoom factor")},
    {"ColorTemperature"              ,ptGT_InputSlider     ,2,1,1 ,6500 ,2000 ,15000 ,50   ,0 ,tr("Temp")               ,tr("Color Temperature")},
    {"GreenIntensity"                ,ptGT_Input           ,2,1,1 ,1.0  ,0.001,5.0   ,0.01 ,3 ,tr("WB-G")               ,tr("Green Intensity in balance")},
    {"RMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("R")                  ,tr("Red Multiplier in balance")},
    {"GMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("G")                  ,tr("Green Multiplier in balance")},
    {"BMultiplier"                   ,ptGT_Input           ,2,1,0 ,1.0  ,0.001,10.0  ,0.01 ,3 ,tr("B")                  ,tr("Blue Multiplier in balance")},
    {"BlackPoint"                    ,ptGT_Input           ,2,1,0 ,0    ,0    ,0xffff,1    ,0 ,tr("BP")                 ,tr("Black point in raw")},
    {"WhitePoint"                    ,ptGT_Input           ,2,1,0 ,0    ,0    ,0xffff,10   ,0 ,tr("WP")                 ,tr("White point in raw")},
    {"GreenEquil"                    ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,100   ,1    ,0 ,tr("Green equilibration"),tr("Green equilibration")},
    {"CfaLineDenoise"                ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,50    ,1    ,0 ,tr("Line denoise")       ,tr("Raw line denoise threshold")},
    {"AdjustMaximumThreshold"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,0.50  ,0.01 ,2 ,tr("Adjust maximum")     ,tr("Threshold to prevent pink highlights")},
    {"RawDenoiseThreshold"           ,ptGT_InputSlider     ,2,1,1 ,0    ,0    ,2000  ,100  ,0 ,tr("Wavelet denoise")    ,tr("Raw wavelet denoise threshold")},
    {"HotpixelReduction"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05 ,3 ,tr("Badpixel reduction") ,tr("Automatic badpixel reduction")},
    {"InterpolationPasses"           ,ptGT_Input           ,1,1,1 ,1    ,0    ,10    ,1    ,0 ,tr("Passes")             ,tr("Nr of refinement passes")},
    {"MedianPasses"                  ,ptGT_Input           ,2,1,1 ,0    ,0    ,10     ,1    ,0 ,tr("Median passes")      ,tr("Nr of median filter passes")},
    {"ESMedianPasses"                ,ptGT_Input           ,2,1,1 ,0    ,0    ,10     ,1    ,0 ,tr("Edge sensitive median passes")      ,tr("Nr of edge sensitive median filter passes")},
    {"ClipParameter"                 ,ptGT_InputSlider     ,1,1,1 ,0    ,0    ,100   ,1    ,0 ,tr("Parameter")          ,tr("Clip function dependent parameter")},
    {"LensfunFocalLength"            ,ptGT_InputSlider     ,9,0,0 ,50   ,4    ,1000  ,1    ,0 ,tr("Focal Length")       ,tr("Focal Length")},
    {"LensfunF"                      ,ptGT_InputSlider     ,9,0,0 ,4.0  ,1.0  ,50.0  ,0.1  ,1 ,tr("F Number")           ,tr("F Number")},
    {"LensfunDistance"               ,ptGT_InputSlider     ,9,0,0 ,1.0  ,0.1  ,100.0 ,0.1  ,1 ,tr("Distance")           ,tr("Subject distance")},
    {"LensfunScale"                  ,ptGT_InputSlider     ,9,0,1 ,1.0  ,0.1  ,10.0  ,0.1  ,1 ,tr("Scale")              ,tr("Scale")},
    {"Rotate"                        ,ptGT_InputSlider     ,9,1,1 ,0.0  ,-180.0,180.0 ,0.1 ,2 ,tr("Rotate")             ,tr("Rotate")},
    {"GridX"                         ,ptGT_Input           ,1,0,0 ,5    ,0    ,9     ,1    ,0 ,tr("X")                  ,tr("Vertical lines")},
    {"GridY"                         ,ptGT_Input           ,1,0,0 ,5    ,0    ,9     ,1    ,0 ,tr("Y")                  ,tr("Horizontal lines")},
    {"ResizeScale"                   ,ptGT_Input           ,1,1,1 ,1200  ,200 ,6000  ,100  ,0 ,tr("Size")         ,tr("Image size")},
    {"LevelsBlackPoint"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.002,3 ,tr("Blackpoint")  ,tr("Levels Blackpoint")},
    {"LevelsWhitePoint"              ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,2.0   ,0.002,3 ,tr("Whitepoint")  ,tr("Levels Whitepoint")},
    {"LabLevelsBlackPoint"           ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0 ,1.0   ,0.002,3 ,tr("Blackpoint")  ,tr("Levels Blackpoint")},
    {"LabLevelsWhitePoint"           ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,2.0   ,0.002,3 ,tr("Whitepoint")  ,tr("Levels Whitepoint")},
    {"ChannelMixerR2R"               ,ptGT_Input           ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of red to red")},
    {"ChannelMixerG2R"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of green to red")},
    {"ChannelMixerB2R"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of blue to red")},
    {"ChannelMixerR2G"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of red to green")},
    {"ChannelMixerG2G"               ,ptGT_Input           ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of green to green")},
    {"ChannelMixerB2G"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of blue to green")},
    {"ChannelMixerR2B"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of red to blue")},
    {"ChannelMixerG2B"               ,ptGT_Input           ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of green to blue")},
    {"ChannelMixerB2B"               ,ptGT_Input           ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.01 ,2 ,tr("")               ,tr("Contribution of blue to blue")},
    {"Vibrance"                      ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Vibrance")           ,tr("Vibrance")},
    {"IntensityRed"                  ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Red")                ,tr("Intensity red")},
    {"IntensityGreen"                ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Green")              ,tr("Intensity green")},
    {"IntensityBlue"                 ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Blue")               ,tr("Intensity blue")},
    {"ColorEnhanceShadows"           ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0    ,0.1,1 ,tr("Enhance shadows")    ,tr("Enhance shadows only")},
    {"ColorEnhanceHighlights"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0    ,0.1,1 ,tr("Enhance highlights") ,tr("Enhance highlights only")},
    {"HighlightsR"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Highlights R")       ,tr("Adjust the brightness of the highlights in R")},
    {"HighlightsG"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Highlights G")       ,tr("Adjust the brightness of the highlights in G")},
    {"HighlightsB"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Highlights B")       ,tr("Adjust the brightness of the highlights in B")},
    {"WhiteFraction"                 ,ptGT_InputSlider     ,2,1,1 ,10   ,1    ,50    ,1    ,0 ,tr("%White")             ,tr("Percentage of white aimed at")},
    {"WhiteLevel"                    ,ptGT_InputSlider     ,2,1,1 ,90   ,50   ,99    ,1    ,0 ,tr("WhiteLevel")         ,tr("WhiteLevel")},
    {"Exposure"                      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-5.0 ,5.0   ,0.1 ,2 ,tr("EV")                 ,tr("Exposure in EV")},
    {"ExposureGain"                  ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Gain")               ,tr("Exposure gain")},
    {"Reinhard05Brightness"          ,ptGT_InputSlider     ,2,1,1 ,-10.0,-90.0 ,10.0 ,2.0  ,0  ,tr("Brightness")         ,tr("Brightness")},
    {"Reinhard05Chroma"              ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0   ,1.0  ,0.1  ,2 ,tr("Chrominance")        ,tr("Chrominance adaption")},
    {"Reinhard05Light"               ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0   ,1.0  ,0.05 ,2 ,tr("Light")              ,tr("Light adaption")},
    {"CatchWhite"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Catch white")        ,tr("Darken just the brightest parts")},
    {"CatchBlack"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0  ,0.05,2  ,tr("Catch black")        ,tr("Brighten just the darkest parts")},
    {"LMHLightRecovery1Amount"       ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2  ,tr("Amount")             ,tr("Amount of recovery")},
    {"LMHLightRecovery1LowerLimit"   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LMHLightRecovery1UpperLimit"   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LMHLightRecovery1Softness"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LMHLightRecovery2Amount"       ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of recovery")},
    {"LMHLightRecovery2LowerLimit"   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LMHLightRecovery2UpperLimit"   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LMHLightRecovery2Softness"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"RGBTextureContrastAmount"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0  ,40.0   ,1.0,1 ,tr("Amount")           ,tr("Amount")},
    {"RGBTextureContrastThreshold"   ,ptGT_InputSlider     ,2,1,1 ,20.0  ,0.0  ,50.0   ,4.0,1 ,tr("Scale")        ,tr("Scale")},
    {"RGBTextureContrastSoftness"    ,ptGT_InputSlider     ,2,1,1 ,0.14  ,0.0  ,1.0    ,0.01,2 ,tr("Threshold")         ,tr("Threshold")},
    {"RGBTextureContrastOpacity"     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")           ,tr("Opacity")},
    {"RGBTextureContrastEdgeControl" ,ptGT_InputSlider     ,2,1,1 ,0.0   ,0.0  ,10.0   ,0.1,1 ,tr("Denoise")           ,tr("Don't amplify noise")},
    {"RGBTextureContrastMasking"     ,ptGT_InputSlider     ,2,1,1 ,100.0 ,0.0  ,100.0  ,10.0,0  ,tr("Masking")           ,tr("Don't amplify noise")},
    {"Microcontrast1Radius"          ,ptGT_InputSlider     ,2,1,1 ,100    ,0    ,500  ,25  ,0 ,tr("Radius")             ,tr("Radius")},
    {"Microcontrast1Amount"          ,ptGT_InputSlider     ,2,1,1 ,8.0  ,-10.0  ,20.0   ,1.0,1 ,tr("Amount")        ,tr("Amount")},
    {"Microcontrast1Opacity"         ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")        ,tr("Opacity")},
    {"Microcontrast1HaloControl"     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0, 1.0   ,0.1 ,1 ,tr("Halo Control")             ,tr("Halo Control")},
    {"Microcontrast1LowerLimit"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Microcontrast1UpperLimit"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Microcontrast1Softness"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"Microcontrast2Radius"          ,ptGT_InputSlider     ,2,1,1 ,500    ,0    ,1000  ,50  ,0 ,tr("Radius")             ,tr("Radius")},
    {"Microcontrast2Amount"          ,ptGT_InputSlider     ,2,1,1 ,8.0  ,-10.0  ,20.0   ,1.0,1 ,tr("Amount")        ,tr("Amount")},
    {"Microcontrast2Opacity"         ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")        ,tr("Opacity")},
    {"Microcontrast2HaloControl"     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0, 1.0   ,0.1 ,1 ,tr("Halo Control")             ,tr("Halo Control")},
    {"Microcontrast2LowerLimit"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Microcontrast2UpperLimit"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Microcontrast2Softness"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"ShadowsHighlightsFine"         ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0,10.0  ,0.5  ,1 ,tr("Fine Detail")        ,tr("Fine Detail")},
    {"ShadowsHighlightsCoarse"       ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0,10.0  ,0.5  ,1 ,tr("Coarse Detail")      ,tr("Coarse Detail")},
    {"ShadowsHighlightsRadius"       ,ptGT_InputSlider     ,2,1,1 ,10.0 ,0.0  ,30.0  ,4.0  ,1 ,tr("Scale")              ,tr("Scale")},
    {"DRCBeta"                       ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05 ,2 ,tr("Amount")             ,tr("Amount of compression")},
    {"DRCAlpha"                      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.01 ,2 ,tr("Bias")               ,tr("Bias of compression")},
    {"DRCColor"                      ,ptGT_InputSlider     ,2,1,1 ,0.25 ,0.0  ,1.0   ,0.05 ,2 ,tr("Color Adaption")     ,tr("Color adaption")},
    {"LabLMHLightRecovery1Amount"    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of recovery")},
    {"LabLMHLightRecovery1LowerLimit",ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LabLMHLightRecovery1UpperLimit",ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LabLMHLightRecovery1Softness"  ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LabLMHLightRecovery2Amount"    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0 ,3.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of recovery")},
    {"LabLMHLightRecovery2LowerLimit",ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.002,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LabLMHLightRecovery2UpperLimit",ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.002,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LabLMHLightRecovery2Softness"  ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"TextureContrast1Amount"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0  ,40.0   ,1.0,1 ,tr("Amount")           ,tr("Amount")},
    {"TextureContrast1Threshold"     ,ptGT_InputSlider     ,2,1,1 ,20.0  ,0.0  ,50.0   ,4.0,1 ,tr("Scale")        ,tr("Scale")},
    {"TextureContrast1Softness"      ,ptGT_InputSlider     ,2,1,1 ,0.14  ,0.0  ,1.0    ,0.01,2 ,tr("Threshold")         ,tr("Threshold")},
    {"TextureContrast1Opacity"       ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")           ,tr("Opacity")},
    {"TextureContrast1EdgeControl"   ,ptGT_InputSlider     ,2,1,1 ,0.0   ,0.0  ,10.0   ,0.1,1 ,tr("Denoise")           ,tr("Don't amplify noise")},
    {"TextureContrast1Masking"       ,ptGT_InputSlider     ,2,1,1 ,100.0 ,0.0  ,100.0  ,10.0,0  ,tr("Masking")           ,tr("Don't amplify noise")},
    {"TextureContrast2Amount"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0  ,40.0   ,1.0,1 ,tr("Amount")           ,tr("Amount")},
    {"TextureContrast2Threshold"     ,ptGT_InputSlider     ,2,1,1 ,100.0  ,0.0  ,400.0   ,4.0,1 ,tr("Scale")        ,tr("Scale")},
    {"TextureContrast2Softness"      ,ptGT_InputSlider     ,2,1,1 ,0.14  ,0.0  ,1.0    ,0.01,2 ,tr("Threshold")         ,tr("Threshold")},
    {"TextureContrast2Opacity"       ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")           ,tr("Opacity")},
    {"TextureContrast2EdgeControl"   ,ptGT_InputSlider     ,2,1,1 ,0.0   ,0.0  ,10.0   ,0.1,1 ,tr("Denoise")           ,tr("Don't amplify noise")},
    {"TextureContrast2Masking"       ,ptGT_InputSlider     ,2,1,1 ,100.0 ,0.0  ,100.0  ,10.0,0  ,tr("Masking")           ,tr("Don't amplify noise")},
    {"LabMicrocontrast1Radius"       ,ptGT_InputSlider     ,2,1,1 ,100    ,0    ,500  ,25  ,0 ,tr("Radius")             ,tr("Radius")},
    {"LabMicrocontrast1Amount"       ,ptGT_InputSlider     ,2,1,1 ,4.0  ,-10.0  ,20.0   ,1.0,1 ,tr("Amount")        ,tr("Amount")},
    {"LabMicrocontrast1Opacity"      ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")        ,tr("Opacity")},
    {"LabMicrocontrast1HaloControl"  ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0, 1.0   ,0.1 ,1 ,tr("Halo Control")             ,tr("Halo Control")},
    {"LabMicrocontrast1LowerLimit"   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LabMicrocontrast1UpperLimit"   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LabMicrocontrast1Softness"     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LabMicrocontrast2Radius"       ,ptGT_InputSlider     ,2,1,1 ,500    ,0    ,1000  ,50  ,0 ,tr("Radius")             ,tr("Radius")},
    {"LabMicrocontrast2Amount"       ,ptGT_InputSlider     ,2,1,1 ,4.0  ,-10.0  ,20.0   ,1.0,1 ,tr("Amount")        ,tr("Amount")},
    {"LabMicrocontrast2Opacity"      ,ptGT_InputSlider     ,2,1,1 ,0.2  ,0.0  ,1.0   ,0.1,1 ,tr("Opacity")        ,tr("Opacity")},
    {"LabMicrocontrast2HaloControl"  ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0, 1.0   ,0.1 ,1 ,tr("Halo Control")             ,tr("Halo Control")},
    {"LabMicrocontrast2LowerLimit"   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LabMicrocontrast2UpperLimit"   ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LabMicrocontrast2Softness"     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LC1Radius"                     ,ptGT_InputSlider     ,2,1,1 ,400  ,0  ,1000   ,20 ,0 ,tr("Radius")              ,tr("Radius")},
    {"LC1Feather"                    ,ptGT_InputSlider     ,2,1,1 ,-0.3  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Feather")              ,tr("Feather")},
    {"LC1Opacity"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1 ,2 ,tr("Opacity")              ,tr("Opacity")},
    {"LC1m"                          ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Masking")              ,tr("Masking")},
    {"LC2Radius"                     ,ptGT_InputSlider     ,2,1,1 ,800  ,0  ,1000   ,40 ,0 ,tr("Radius")              ,tr("Radius")},
    {"LC2Feather"                    ,ptGT_InputSlider     ,2,1,1 ,-0.3  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Feather")              ,tr("Feather")},
    {"LC2Opacity"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1 ,2 ,tr("Opacity")              ,tr("Opacity")},
    {"LC2m"                          ,ptGT_InputSlider     ,2,1,1 ,1.0  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Masking")              ,tr("Masking")},
    {"ColorcontrastRadius"           ,ptGT_InputSlider     ,2,1,1 ,100    ,0    ,2000  ,50  ,0 ,tr("Radius")             ,tr("Radius")},
    {"ColorcontrastAmount"           ,ptGT_InputSlider     ,2,1,1 ,4.0  ,0.0  ,20.0   ,1.0,1 ,tr("Amount")        ,tr("Amount")},
    {"ColorcontrastOpacity"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2 ,tr("Opacity")        ,tr("Opacity")},
    {"ColorcontrastHaloControl"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0, 1.0   ,0.1 ,1 ,tr("Halo Control")             ,tr("Halo Control")},
    {"RGBGammaAmount"                ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.1  ,2.0   ,0.01 ,2 ,tr("Gamma")              ,tr("Gamma")},
    {"RGBGammaLinearity"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,0.99   ,0.01 ,2 ,tr("Linearity")          ,tr("Linearity")},
    {"NormalizationOpacity"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0    ,0.05 ,2 ,tr("Opacity")            ,tr("Opacity")},
    //{"NormalizationBlackPoint"       ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0    ,0.01 ,2 ,tr("Black point")         ,tr("Black point")},
    //{"NormalizationWhitePoint"       ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0    ,0.01 ,2 ,tr("White point")         ,tr("White point")},
    {"RGBContrastAmount"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-20.0  ,20.0  ,0.5  ,1 ,tr("Contrast")           ,tr("Contrast")},
    {"RGBContrastThreshold"          ,ptGT_InputSlider     ,2,1,1 ,0.30 ,0.015,0.95  ,0.05 ,2 ,tr("Threshold")          ,tr("Threshold")},
    {"ContrastAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-20.0  ,20.0  ,0.5  ,1 ,tr("Amount")             ,tr("Amount of contrast")},
    {"ContrastThreshold"             ,ptGT_InputSlider     ,2,1,1 ,0.50 ,0.05 ,0.95   ,0.05 ,2 ,tr("Threshold")          ,tr("Threshold for contrast")},
    {"SaturationAmount"              ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-10.0  ,10.0  ,0.5  ,1 ,tr("Amount")             ,tr("Amount of saturation")},
    {"ColorBoostValueA"              ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0  ,0.1  ,1 ,tr("Value A")             ,tr("Amount of boosting A")},
    {"ColorBoostValueB"              ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0  ,0.1  ,1 ,tr("Value B")             ,tr("Amount of boosting B")},
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
    {"GREYCLabAlpha"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Gradient smoothness")              ,tr("Alpha")},
    {"GREYCLabSigma"                 ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Tensor smoothness")              ,tr("Sigma")},
    {"GREYCLabdl"                    ,ptGT_InputSlider     ,2,1,1 ,0.8  ,0.0  ,2.0   ,0.01 ,2 ,tr("Spacial precision")                 ,tr("dl")},
    {"GREYCLabda"                    ,ptGT_InputSlider     ,2,1,1 ,30   ,0    ,180    ,1    ,0 ,tr("Angular precision")                 ,tr("da")},
    {"GREYCLabGaussPrecision"        ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Value precision")              ,tr("Gauss")},
    {"PyrDenoiseLAmount"             ,ptGT_InputSlider     ,2,1,1 ,0      ,0   ,150   ,5    ,0 ,tr("L amount")        ,tr("Denoise amount on L")},
    {"PyrDenoiseABAmount"            ,ptGT_InputSlider     ,2,1,1 ,0      ,0   ,150   ,5    ,0 ,tr("Color amount")    ,tr("Denoise amount on AB")},
    {"PyrDenoiseGamma"               ,ptGT_InputSlider     ,2,1,1 ,2.0    ,1.0 ,4.0   ,0.1  ,1 ,tr("Gamma")           ,tr("Gamma")},
    {"PyrDenoiseLevels"              ,ptGT_Input           ,2,1,1 ,5      ,3   ,7     ,1    ,0 ,tr("Levels")          ,tr("Levels")},
    {"BilateralLOpacity"             ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,1.0   ,0.10 ,2 ,tr("Opacity")         ,tr("Opacity of denoising on L")},
    {"BilateralLUseMask"             ,ptGT_InputSlider     ,2,1,1 ,50.0   ,0.0 ,50.0  ,10.0 ,0 ,tr("Edge Threshold")  ,tr("Edge thresholding for denoising on L")},
    {"BilateralLSigmaS"              ,ptGT_InputSlider     ,2,1,1 ,8.0    ,4.0 ,50.0  ,4.0  ,1 ,tr("L scale")         ,tr("Denoise scale on L")},
    {"BilateralLSigmaR"              ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,3.0   ,0.02 ,2 ,tr("L amount")         ,tr("Denoise on L")},
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
    {"DetailCurveScaling"            ,ptGT_InputSlider     ,2,1,1 ,40.0   ,1.0 ,200.0,10.0 ,0 ,tr("Halo control")       ,tr("Halo control")},
    {"DetailCurveWeight"             ,ptGT_InputSlider     ,2,1,1 ,0.5    ,0.0 ,1.0 ,0.05  ,2 ,tr("Weight")             ,tr("Weight")},
    {"DetailCurveHotpixel"           ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,1.0 ,0.1   ,2 ,tr("Clean up")           ,tr("Automatic badpixel reduction")},
    {"MLMicroContrastStrength"       ,ptGT_InputSlider     ,2,1,1 ,0.0    ,-0.1,0.5 ,0.05  ,2 ,tr("Microcontrast")      ,tr("Microcontrast strength")},
    {"MLMicroContrastScaling"        ,ptGT_InputSlider     ,2,1,1 ,40.0   ,1.0 ,200.0,10.0 ,0 ,tr("Halo control")       ,tr("Microcontrast Halo control")},
    {"MLMicroContrastWeight"         ,ptGT_InputSlider     ,2,1,1 ,0.5    ,0.0 ,1.0 ,0.05  ,2 ,tr("Weight")             ,tr("Microcontrast weight")},
    {"LabHotpixel"                   ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,1.0 ,0.1   ,2 ,tr("Clean up")           ,tr("Automatic badpixel reduction")},
    {"WienerFilterAmount"            ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,1.0 ,0.05  ,2 ,tr("Amount")             ,tr("Amount")},
    {"WienerFilterGaussian"          ,ptGT_InputSlider     ,2,1,1 ,0.6    ,0.0 ,5.0 ,0.05  ,2 ,tr("Gaussian")           ,tr("Gaussian")},
    {"WienerFilterBox"               ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,5.0 ,0.05  ,2 ,tr("Box")                ,tr("Box")},
    {"WienerFilterLensBlur"          ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,5.0 ,0.05  ,2 ,tr("Lens blur")          ,tr("Lens blur")},
    {"InverseDiffusionIterations"    ,ptGT_Input           ,2,1,1 ,0      ,0   ,5     ,1   ,0 ,tr("Iterations")         ,tr("Number of iterations")},
    {"InverseDiffusionAmplitude"     ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,2.0 ,0.05  ,2 ,tr("Amplitude")          ,tr("Amplitude")},
    {"USMRadius"                     ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.1  ,10.0  ,0.1  ,1 ,tr("Radius")             ,tr("Radius for USM")},
    {"USMAmount"                     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.1  ,5.0   ,0.1  ,1 ,tr("Amount")             ,tr("Amount for USM")},
    {"USMThreshold"                  ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.00 ,0.20  ,0.01 ,2 ,tr("Threshold")          ,tr("Threshold for USM")},
    {"HighpassRadius"                ,ptGT_InputSlider     ,2,1,1 ,2.0  ,0.0  ,10.0  ,0.5  ,1 ,tr("Radius")             ,tr("Radius for Highpass")},
    {"HighpassAmount"                ,ptGT_InputSlider     ,2,1,1 ,4.0  ,0.0  ,10.0  ,0.5  ,1 ,tr("Amount")             ,tr("Amount for Highpass")},
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
    {"LABToneAdjust1Saturation"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneAdjust1Amount"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneAdjust1Hue"             ,ptGT_InputSliderHue  ,2,1,1 ,60.0 ,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LABToneAdjust1LowerLimit"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LABToneAdjust1UpperLimit"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LABToneAdjust1Softness"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LABToneAdjust2Saturation"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneAdjust2Amount"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneAdjust2Hue"             ,ptGT_InputSliderHue  ,2,1,1 ,60.0 ,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LABToneAdjust2LowerLimit"      ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.05,3 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"LABToneAdjust2UpperLimit"      ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.05,3 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"LABToneAdjust2Softness"        ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"LAdjustC1"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Red")               ,tr("Red")},
    {"LAdjustC2"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Orange")            ,tr("Orange")},
    {"LAdjustC3"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Yellow")            ,tr("Yellow")},
    {"LAdjustC4"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Light green")       ,tr("Light green")},
    {"LAdjustC5"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Dark green")        ,tr("Dark green")},
    {"LAdjustC6"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Cyan")              ,tr("Cyan")},
    {"LAdjustC7"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Blue")              ,tr("Blue")},
    {"LAdjustC8"                     ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Magenta")           ,tr("Magenta")},
    {"LAdjustSC1"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Red")               ,tr("Red")},
    {"LAdjustSC2"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Orange")            ,tr("Orange")},
    {"LAdjustSC3"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Yellow")            ,tr("Yellow")},
    {"LAdjustSC4"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Light green")       ,tr("Light green")},
    {"LAdjustSC5"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Dark green")        ,tr("Dark green")},
    {"LAdjustSC6"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Cyan")              ,tr("Cyan")},
    {"LAdjustSC7"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Blue")              ,tr("Blue")},
    {"LAdjustSC8"                    ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-3.0  ,3.0   ,0.05,2  ,tr("Magenta")           ,tr("Magenta")},
    {"LABToneSaturation"             ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneAmount"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneHue"                    ,ptGT_InputSliderHue  ,2,1,1 ,60.0 ,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LABToneSSaturation"            ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneSAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneSHue"                   ,ptGT_InputSliderHue  ,2,1,1 ,240.0,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LABToneMSaturation"            ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneMAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneMHue"                   ,ptGT_InputSliderHue  ,2,1,1 ,60.0 ,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LABToneHSaturation"            ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0   ,4.0   ,0.1 ,2  ,tr("Saturation")        ,tr("Saturation")},
    {"LABToneHAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0   ,1.0   ,0.05,2  ,tr("Amount")            ,tr("Amount")},
    {"LABToneHHue"                   ,ptGT_InputSliderHue  ,2,1,1 ,60.0 ,0.0  ,360.0  ,10.0,0  ,tr("Hue")               ,tr("Hue")},
    {"LabVignette"                   ,ptGT_Input           ,2,1,1 ,2    ,1    ,6     ,1    ,0  ,tr("Shape")             ,tr("Shape of the vignette")},
    {"LabVignetteAmount"             ,ptGT_InputSlider     ,2,1,1 ,0.3  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Amount")            ,tr("Amount")},
    {"LabVignetteInnerRadius"        ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2 ,tr("Inner Radius")        ,tr("Inner Radius")},
    {"LabVignetteOuterRadius"        ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2 ,tr("Outer Radius")        ,tr("Outer Radius")},
    {"LabVignetteRoundness"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.05  ,2 ,tr("Roundness")        ,tr("Roundness")},
    {"LabVignetteCenterX"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center X")        ,tr("Center X")},
    {"LabVignetteCenterY"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center Y")        ,tr("Center Y")},
    {"LabVignetteSoftness"           ,ptGT_InputSlider     ,2,1,1 ,0.06  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"BWStylerOpacity"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1  ,1 ,tr("Opacity")            ,tr("Opacity")},
    {"BWStylerMultR"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Red")                ,tr("Red multiplicity")},
    {"BWStylerMultG"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Green")              ,tr("Green multiplicity")},
    {"BWStylerMultB"                 ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1 ,2 ,tr("Blue")               ,tr("Blue multiplicity")},
    {"SimpleToneR"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Red")                  ,tr("Red toning")},
    {"SimpleToneG"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Green")                  ,tr("Green toning")},
    {"SimpleToneB"                   ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1,2  ,tr("Blue")                  ,tr("Blue toning")},
    {"Tone1Amount"                   ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of toning")},
    {"Tone1LowerLimit"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Tone1UpperLimit"               ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Tone1Softness"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"Tone2Amount"                   ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")             ,tr("Amount of toning")},
    {"Tone2LowerLimit"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Lower Limit")        ,tr("Lower Limit")},
    {"Tone2UpperLimit"               ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,1.0   ,0.1  ,2 ,tr("Upper Limit")        ,tr("Upper Limit")},
    {"Tone2Softness"                 ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-2.0 ,2.0   ,0.1  ,1 ,tr("Softness")           ,tr("Softness")},
    {"CrossprocessingColor1"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Main color")         ,tr("Intensity of the main color")},
    {"CrossprocessingColor2"         ,ptGT_InputSlider     ,2,1,1 ,0.4  ,0.0  ,1.0   ,0.1  ,2 ,tr("Second color")         ,tr("Intensity of the second color")},
    {"RGBContrast2Amount"            ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-20.0,20.0  ,0.5  ,1 ,tr("Contrast")           ,tr("Contrast")},
    {"RGBContrast2Threshold"         ,ptGT_InputSlider     ,2,1,1 ,0.30 ,0.05 ,0.95  ,0.05 ,2 ,tr("Threshold")          ,tr("Threshold")},
    {"GradualOverlay1Amount"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")        ,tr("Amount")},
    {"GradualOverlay1Angle"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0  ,5.0  ,0 ,tr("Angle")        ,tr("Angle")},
    {"GradualOverlay1LowerLevel"     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,3.0   ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradualOverlay1UpperLevel"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0   ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradualOverlay1Softness"       ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"GradualOverlay2Amount"         ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,2 ,tr("Amount")        ,tr("Amount")},
    {"GradualOverlay2Angle"          ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-180.0,180.0  ,5.0  ,0 ,tr("Angle")        ,tr("Angle")},
    {"GradualOverlay2LowerLevel"     ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,3.0   ,0.1  ,2 ,tr("Lower Level")        ,tr("Lower Level")},
    {"GradualOverlay2UpperLevel"     ,ptGT_InputSlider     ,2,1,1 ,1.0  ,0.0  ,3.0   ,0.1  ,2 ,tr("Upper Level")        ,tr("Upper Level")},
    {"GradualOverlay2Softness"       ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"Vignette"                      ,ptGT_Input           ,2,1,1 ,2    ,1    ,6     ,1    ,0 ,tr("Shape")             ,tr("Shape of the vignette")},
    {"VignetteAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.5  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Amount")        ,tr("Amount")},
    {"VignetteInnerRadius"           ,ptGT_InputSlider     ,2,1,1 ,0.7  ,0.0  ,3.0   ,0.1  ,2 ,tr("Inner Radius")        ,tr("Inner Radius")},
    {"VignetteOuterRadius"           ,ptGT_InputSlider     ,2,1,1 ,2.2  ,0.0  ,3.0   ,0.1  ,2 ,tr("Outer Radius")        ,tr("Outer Radius")},
    {"VignetteRoundness"             ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.05  ,2 ,tr("Roundness")        ,tr("Roundness")},
    {"VignetteCenterX"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center X")        ,tr("Center X")},
    {"VignetteCenterY"               ,ptGT_InputSlider     ,2,1,1 ,0.0  ,-1.0  ,1.0   ,0.1  ,2 ,tr("Center Y")        ,tr("Center Y")},
    {"VignetteSoftness"              ,ptGT_InputSlider     ,2,1,1 ,0.15  ,0.0  ,1.0   ,0.1  ,2 ,tr("Softness")        ,tr("Softness")},
    {"SoftglowRadius"                ,ptGT_InputSlider     ,2,1,1 ,10.0  ,0.0  ,30.0  ,1.0 ,1 ,tr("Radius")        ,tr("Radius")},
    {"SoftglowAmount"                ,ptGT_InputSlider     ,2,1,1 ,0.5  ,0.0  ,1.0   ,0.1  ,1  ,tr("Amount")        ,tr("Amount")},
    {"SoftglowSaturation"            ,ptGT_InputSlider     ,2,1,1 ,-50  ,-100  ,100    ,5  ,0 ,tr("Saturation")     ,tr("Saturation")},
    {"SoftglowContrast"              ,ptGT_InputSlider     ,2,1,1 ,5.0  ,0.0  ,20.0  ,0.5  ,1 ,tr("Contrast")           ,tr("Contrast")},
    {"Vibrance2"                     ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Vibrance")           ,tr("Vibrance")},
    {"Intensity2Red"                 ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Red")                ,tr("Intensity red")},
    {"Intensity2Green"               ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Green")              ,tr("Intensity green")},
    {"Intensity2Blue"                ,ptGT_InputSlider     ,2,1,1 ,0    ,-100  ,100    ,5  ,0 ,tr("Blue")               ,tr("Intensity blue")},
    {"OutputGamma"                   ,ptGT_InputSlider     ,1,1,1 ,0.33  ,0.1  ,1.0   ,0.01 ,3 ,tr("Gamma")              ,tr("Gamma")},
    {"OutputLinearity"               ,ptGT_InputSlider     ,1,1,1 ,0.06  ,0.0  ,1.0   ,0.01 ,3 ,tr("Linearity")          ,tr("Linearity")},
    {"RGBContrast3Amount"            ,ptGT_InputSlider     ,1,1,1 ,0.0  ,-20.0,20.0  ,0.5  ,1 ,tr("Contrast")           ,tr("Contrast")},
    {"RGBContrast3Threshold"         ,ptGT_InputSlider     ,1,1,1 ,0.50 ,0.05 ,0.95  ,0.05 ,2 ,tr("Threshold")          ,tr("Threshold")},
    {"WebResizeScale"                ,ptGT_Input           ,1,1,1 ,1200  ,400 ,2000  ,100  ,0 ,tr("Size")         ,tr("Image size")},
    {"WienerFilter2Amount"           ,ptGT_InputSlider     ,2,1,1 ,0.2    ,0.0 ,1.0 ,0.05   ,2 ,tr("Amount")               ,tr("Amount")},
    {"WienerFilter2Gaussian"         ,ptGT_InputSlider     ,2,1,1 ,0.6    ,0.0 ,5.0 ,0.05   ,2 ,tr("Gaussian")             ,tr("Gaussian")},
    {"WienerFilter2Box"              ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,5.0 ,0.05   ,2 ,tr("Box")                  ,tr("Box")},
    {"WienerFilter2LensBlur"         ,ptGT_InputSlider     ,2,1,1 ,0.0    ,0.0 ,5.0 ,0.05   ,2 ,tr("Lens blur")            ,tr("Lens blur")},
    //{"GREYCAmplitude"                ,ptGT_Input           ,2,1,1 ,60.0 ,20.0 ,80.0  ,0.1  ,1 ,tr("Amplitude")          ,tr("Amplitude")},
    //{"GREYCIterations"               ,ptGT_Input           ,2,1,1 ,1    ,1    ,10    ,1    ,0 ,tr("Iterations")         ,tr("Iterations")},
    //{"GREYCSharpness"                ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,5.0   ,0.1  ,1 ,tr("Sharpness")          ,tr("Sharpness")},
    //{"GREYCAnisotropy"               ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,1.0   ,0.01 ,2 ,tr("Anisotropy")         ,tr("Anisotropy")},
    //{"GREYCAlpha"                    ,ptGT_Input           ,2,1,1 ,0.6  ,0.0  ,5.0   ,0.10 ,1 ,tr("Alpha")              ,tr("Alpha")},
    //{"GREYCdl"                       ,ptGT_Input           ,2,1,1 ,0.8  ,0.0  ,1.0   ,0.01 ,2 ,tr("dl")                 ,tr("dl")},
    //{"GREYCda"                       ,ptGT_Input           ,2,1,1 ,30   ,0    ,90    ,1    ,0 ,tr("da")                 ,tr("da")},
    //{"GREYCSigma"                    ,ptGT_Input           ,2,1,1 ,1.1  ,0.0  ,5.0   ,0.1  ,1 ,tr("Sigma")              ,tr("Sigma")},
    //{"GREYCGaussPrecision"           ,ptGT_Input           ,2,1,1 ,2.0  ,0.0  ,5.0   ,0.1  ,1 ,tr("Gauss")              ,tr("Gauss")},
    {"SaveQuality"                   ,ptGT_Input           ,2,1,1 ,97   ,25   ,100   ,1    ,0 ,tr("Quality")            ,tr("Quality")},
    {"ImageRating"                   ,ptGT_Input           ,2,1,1 ,0    ,0    ,5     ,1    ,0 ,tr("Rating")             ,tr("Image rating")}
  };

  // Load in the gui choice (combo) elements
  const ptGuiChoiceItem GuiChoiceItems[] = {
    // Unique Name,GuiElement,InitLevel,InJobFile,HasDefault (causes button too !),Default,Choices (from ptGuiOptions.h),ToolTip
    {"RememberSettingLevel"        ,ptGT_Choice       ,1,0,0 ,2                           ,GuiOptions->RememberSettingLevel      ,tr("Remember setting level")},
    {"CameraColor"                 ,ptGT_Choice       ,1,1,1 ,ptCameraColor_Adobe_Profile ,GuiOptions->CameraColor               ,tr("Transform camera RGB to working space RGB")},
    {"CameraColorProfileIntent"    ,ptGT_Choice       ,1,1,1 ,INTENT_PERCEPTUAL           ,GuiOptions->CameraColorProfileIntent  ,tr("Intent of the profile")},
    {"CameraColorGamma"            ,ptGT_Choice       ,1,1,1 ,ptCameraColorGamma_None     ,GuiOptions->CameraColorGamma          ,tr("Gamma that was applied before this profile")},
    {"WorkColor"                   ,ptGT_Choice       ,1,1,1 ,ptSpace_sRGB_D65            ,GuiOptions->WorkColor                 ,tr("Working colorspace")},
    {"CMQuality"                   ,ptGT_Choice       ,1,1,0 ,ptCMQuality_HighResPreCalc  ,GuiOptions->CMQuality                 ,tr("Color management quality")},
    {"PreviewColorProfileIntent"   ,ptGT_Choice       ,1,0,1 ,INTENT_PERCEPTUAL           ,GuiOptions->PreviewColorProfileIntent ,tr("Intent of the profile")},
    {"OutputColorProfileIntent"    ,ptGT_Choice       ,1,1,1 ,INTENT_PERCEPTUAL           ,GuiOptions->OutputColorProfileIntent  ,tr("Intent of the profile")},
    {"SaveButtonMode"              ,ptGT_Choice       ,1,1,1 ,ptOutputMode_Pipe           ,GuiOptions->OutputMode                ,tr("Output mode of save button")},
    {"Style"                       ,ptGT_Choice       ,1,0,0 ,ptStyle_DarkGrey            ,GuiOptions->Style                     ,tr("Set the theme.")},
    {"StyleHighLight"              ,ptGT_Choice       ,1,0,0 ,ptStyleHighLight_Blue       ,GuiOptions->StyleHighLight            ,tr("Set the highlight color of the theme.")},
    {"PipeSize"                    ,ptGT_Choice       ,2,0,1 ,ptPipeSize_Quarter          ,GuiOptions->PipeSize                  ,tr("Size of image processed vs original.")},
    {"SpecialPreview"              ,ptGT_Choice       ,2,0,1 ,ptSpecialPreview_RGB        ,GuiOptions->SpecialPreview            ,tr("Special preview for image analysis")},
    {"BadPixels"                   ,ptGT_Choice       ,1,1,0 ,0                           ,GuiOptions->BadPixels                 ,tr("Bad pixels file")},
    {"DarkFrame"                   ,ptGT_Choice       ,1,1,0 ,0                           ,GuiOptions->DarkFrame                 ,tr("Darkframe file")},
    {"WhiteBalance"                ,ptGT_Choice       ,2,1,1 ,ptWhiteBalance_Camera       ,GuiOptions->WhiteBalance              ,tr("WhiteBalance")},
    {"Interpolation"               ,ptGT_Choice       ,2,1,1 ,ptInterpolation_DCB         ,GuiOptions->Interpolation             ,tr("Demosaicing algorithm")},
    {"BayerDenoise"                ,ptGT_Choice       ,2,1,1 ,ptBayerDenoise_None         ,GuiOptions->BayerDenoise              ,tr("Denosie on Bayer pattern")},
    {"CropRectangleMode"           ,ptGT_Choice       ,1,0,0 ,ptRectangleMode_GoldenRatio ,GuiOptions->CropRectangleMode         ,tr("Guide lines for crop")},
    {"ClipMode"                    ,ptGT_Choice       ,1,1,1 ,ptClipMode_Blend            ,GuiOptions->ClipMode                  ,tr("How to handle clipping")},
    {"LensfunCamera"               ,ptGT_Choice       ,9,0,0 ,0                           ,NULL                                  ,tr("Camera for lensfun")},
    {"LensfunLens"                 ,ptGT_Choice       ,9,0,0 ,0                           ,NULL                                  ,tr("Lens for lensfun")},
    {"LensfunGeometry"             ,ptGT_Choice       ,9,0,1 ,LF_RECTILINEAR              ,GuiOptions->LensfunGeometry           ,tr("Target geometry for lensfun")},
    {"ResizeFilter"                ,ptGT_Choice       ,1,1,1 ,ptIMFilter_Mitchell         ,GuiOptions->IMResizeFilter              ,tr("Filter to be used for resizing")},
    {"FlipMode"                    ,ptGT_Choice       ,2,1,1 ,ptFlipMode_None             ,GuiOptions->FlipMode                  ,tr("Flip mode")},
    {"AspectRatioW"                ,ptGT_Choice       ,2,0,0 ,3                           ,GuiOptions->AspectRatioW              ,tr("Aspect width")},
    {"AspectRatioH"                ,ptGT_Choice       ,2,0,0 ,2                           ,GuiOptions->AspectRatioH              ,tr("Aspect height")},
    {"ChannelMixer"                ,ptGT_Choice       ,2,1,1 ,ptChannelMixerChoice_None   ,GuiOptions->ChannelMixer              ,tr("ChannelMixer")},
    {"ExposureClipMode"            ,ptGT_Choice       ,1,1,1 ,ptExposureClipMode_Curve    ,GuiOptions->ExposureClipMode          ,tr("Clip mode")},
    {"AutoExposure"                ,ptGT_Choice       ,1,1,1 ,ptAutoExposureMode_Zero     ,GuiOptions->AutoExposureMode          ,tr("Auto exposure mode")},
    {"LABTransform"                ,ptGT_Choice       ,2,1,1 ,ptLABTransform_L            ,GuiOptions->LABTransformMode          ,tr("LAB Transform mode")},
    {"LMHLightRecovery1MaskType"   ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for recovery")},
    {"LMHLightRecovery2MaskType"   ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for recovery")},
    {"Microcontrast1MaskType"      ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for microcontrast")},
    {"Microcontrast2MaskType"      ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for microcontrast")},
    {"LabLMHLightRecovery1MaskType",ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for recovery")},
    {"LabLMHLightRecovery2MaskType",ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for recovery")},
    {"LabMicrocontrast1MaskType"   ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for microcontrast")},
    {"LabMicrocontrast2MaskType"   ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for microcontrast")},
    {"GREYCLab"                    ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->EnableGreyC        ,tr("Enable GreyCStoration on L")},
    {"GREYCLabMaskType"            ,ptGT_Choice       ,2,1,1 ,ptDenoiseMask_Shadows3      ,GuiOptions->DenoiseMask      ,tr("Shadow mask for denoising")},
    {"GREYCLabInterpolation"       ,ptGT_Choice       ,2,1,1 ,ptGREYCInterpolation_NearestNeighbour,GuiOptions->GREYCInterpolation, tr("GREYC Interpolation")},
    {"USM"                         ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable              ,tr("Enable USM sharpening")},
    {"Highpass"                    ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable              ,tr("Enable Highpass sharpening")},
    {"Grain1MaskType"              ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->GrainMaskType                  ,tr("Values for film grain")},
    {"Grain1Mode"                  ,ptGT_Choice       ,2,1,1 ,ptGrainMode_SoftGaussian    ,GuiOptions->GrainMode                 ,tr("Mode for film grain")},
    {"Grain2MaskType"              ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->GrainMaskType                  ,tr("Values for film grain")},
    {"Grain2Mode"                  ,ptGT_Choice       ,2,1,1 ,ptGrainMode_SoftGaussian    ,GuiOptions->GrainMode                 ,tr("Mode for film grain")},
    {"LabVignetteMode"             ,ptGT_Choice       ,2,1,1 ,ptVignetteMode_None         ,GuiOptions->VignetteMode           ,tr("Mode for Vignette")},
    {"CurveRGB"                    ,ptGT_Choice       ,1,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("RGB curve")},
    {"CurveR"                      ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("R curve")},
    {"CurveG"                      ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("G curve")},
    {"CurveB"                      ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("B curve")},
    {"CurveL"                      ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("L curve")},
    {"CurveLa"                     ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("a curve")},
    {"CurveLb"                     ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("b curve")},
    {"CurveLByHue"                 ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("L by hue curve")},
    {"CurveTexture"                ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("Texture curve")},
    {"CurveSaturation"             ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("Saturation curve")},
    {"BaseCurve"                   ,ptGT_Choice       ,1,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("Base curve")},
    {"BaseCurve2"                  ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("After gamma curve")},
    {"CurveShadowsHighlights"      ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("Shadows / Highlights curve")},
    {"CurveDenoise"                ,ptGT_Choice       ,9,1,1 ,ptCurveChoice_None          ,GuiOptions->Curve                     ,tr("Denoise curve")},
    //{"GREYCInterpolation"        ,ptGT_Choice       ,2,1,1 ,ptGREYCInterpolation_NearestNeighbour,GuiOptions->GREYCInterpolation, tr("GREYC Interpolation")},
    {"ViewLAB"                     ,ptGT_Choice       ,2,1,1 ,ptViewLAB_LAB               ,GuiOptions->ViewLAB              ,tr("View seperate LAB channels")},
    {"LABToneAdjust1MaskType"      ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for tone adjustment")},
    {"LABToneAdjust2MaskType"      ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->LMHLightRecoveryMaskType  ,tr("Values for tone adjustment")},
    {"BWStylerFilmType"            ,ptGT_Choice       ,2,1,1 ,ptFilmType_Luminance        ,GuiOptions->FilmType              ,tr("Film emulation")},
    {"BWStylerColorFilterType"     ,ptGT_Choice       ,2,1,1 ,ptColorFilterType_None      ,GuiOptions->ColorFilterType              ,tr("Color filter emulation")},
    {"Tone1MaskType"               ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->MaskType              ,tr("Values for Toning")},
    {"Tone2MaskType"               ,ptGT_Choice       ,2,1,1 ,ptMaskType_None             ,GuiOptions->MaskType              ,tr("Values for Toning")},
    {"CrossprocessingMode"         ,ptGT_Choice       ,2,1,1 ,ptCrossprocessMode_None     ,GuiOptions->CrossprocessMode    ,tr("Colors for cross processing")},
    {"GradualOverlay1"             ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode           ,tr("Mode for Gradual Overlay")},
    {"GradualOverlay2"             ,ptGT_Choice       ,2,1,1 ,ptOverlayMode_None          ,GuiOptions->OverlayMode           ,tr("Mode for Gradual Overlay")},
    {"VignetteMode"                ,ptGT_Choice       ,2,1,1 ,ptVignetteMode_None         ,GuiOptions->VignetteMode           ,tr("Mode for Vignette")},
    {"SoftglowMode"                ,ptGT_Choice       ,2,1,1 ,ptSoftglowMode_None         ,GuiOptions->SoftglowMode           ,tr("Mode for Softglow")},
    {"WebResize"                   ,ptGT_Choice       ,2,1,1 ,ptEnable_None               ,GuiOptions->Enable              ,tr("Enable web resizing")},
    {"WebResizeFilter"             ,ptGT_Choice       ,1,1,1 ,ptIMFilter_Lanczos          ,GuiOptions->IMResizeFilter              ,tr("Filter to be used for resizing")},
    {"SaveFormat"                  ,ptGT_Choice       ,1,1,1 ,ptSaveFormat_JPEG           ,GuiOptions->SaveFormat                ,tr("Output format")},
    {"SaveSampling"                ,ptGT_Choice       ,1,1,1 ,ptSaveSampling_211          ,GuiOptions->SaveSampling              ,tr("JPEG color sampling")},
    {"OutputMode"                  ,ptGT_Choice       ,1,1,1 ,ptOutputMode_Full           ,GuiOptions->OutputMode                ,tr("Output mode")}
  };

  // Load in the gui check elements
  const ptGuiCheckItem GuiCheckItems[] = {
    // Name, GuiType,InitLevel,InJobFile,Default,Label,Tip
    {"StartupSettings"            ,ptGT_Check ,1,0,1,tr("User settings")   ,tr("Load user settings on startup")},
    {"StartupSettingsReset"       ,ptGT_Check ,1,0,0,tr("Reset on new image") ,tr("Reset to user settings when new image is opened")},
    {"InputsAddPowerLaw"          ,ptGT_Check ,1,1,1,tr("Nonlinear slider response")   ,tr("Alter the slider behaviour")},
    {"ToolBoxMode"                ,ptGT_Check ,1,0,0,tr("Enabled")         ,tr("Show seperate toolboxes")},
    {"PreviewTabMode"             ,ptGT_Check ,1,0,0,tr("Tab mode")        ,tr("Show the preview after the active tab")},
    {"BackgroundColor"            ,ptGT_Check ,1,0,0,tr("Override default"),tr("Override the default color")},
    {"Translation"                ,ptGT_Check ,1,0,0,tr("Enabled")         ,tr("Translate photivo")},
    {"WriteBackupSettings"        ,ptGT_Check ,1,0,0,tr("Backup settings") ,tr("Write backup settings during processing")},
    {"RunMode"                    ,ptGT_Check ,1,0,0,tr("manual")          ,tr("manual or automatic pipe")},
    {"MultiplierEnhance"          ,ptGT_Check ,1,1,0,tr("Intensify")       ,tr("Normalize lowest channel to 1")},
    {"ManualBlackPoint"           ,ptGT_Check ,2,1,0,tr("Manual BP")       ,tr("Manual black point setting enabled")},
    {"ManualWhitePoint"           ,ptGT_Check ,2,1,0,tr("Manual WP")       ,tr("Manual white point setting enabled")},
    {"CaCorrect"                  ,ptGT_Check ,2,1,0,tr("CA correction")   ,tr("Automatic CA correction")},
    {"EeciRefine"                 ,ptGT_Check ,2,1,0,tr("Eeci refinement") ,tr("Eeci refinement")},
    {"EnableLensfun"              ,ptGT_Check ,9,0,0,tr("Enable automatic lensfun")   ,tr("Overall enabling or disabling lensfun")},
    {"LensfunTCAEnable"           ,ptGT_Check ,9,0,0,tr("")                ,tr("Enable TCA correction")},
    {"LensfunVignettingEnable"    ,ptGT_Check ,9,0,0,tr("")                ,tr("Enable vignetting correction")},
    {"LensfunDistortionEnable"    ,ptGT_Check ,9,0,0,tr("")                ,tr("Enable distortion correction")},
    {"LensfunGeometryEnable"      ,ptGT_Check ,9,0,0,tr("")                ,tr("Enable geometry correction")},
    {"Grid"                       ,ptGT_Check ,9,1,0,tr("Grid")            ,tr("Enable the overlay grid")},
    {"Crop"                       ,ptGT_Check ,9,1,0,tr("Crop")            ,tr("Enable to make a crop")},
    {"Resize"                     ,ptGT_Check ,9,1,0,tr("Resize")          ,tr("Enable resize")},
    {"AutomaticPipeSize"          ,ptGT_Check ,1,1,0,tr("Automatic pipe size") ,tr("Automatic pipe size")},
    {"GeometryBlock"              ,ptGT_Check ,9,0,0,tr("Block pipe")      ,tr("Disable the pipe")},
    {"Reinhard05"                 ,ptGT_Check ,2,1,0,tr("Enable")          ,tr("Enable Reinhard 05")},
    {"GREYCLabFast"               ,ptGT_Check ,2,1,1,tr("Enable 'fast'")   ,tr("Enable GREYC 'fast'")},
    {"WienerFilter"               ,ptGT_Check ,2,1,0,tr("Enable")          ,tr("Enable wiener filter")},
    {"WienerFilterUseEdgeMask"    ,ptGT_Check ,2,1,1,tr("Only edges")      ,tr("Sharpen only edges")},
    {"InverseDiffusionUseEdgeMask",ptGT_Check ,2,1,1,tr("Only edges")      ,tr("Sharpen only edges")},
    //{"GREYC"                    ,ptGT_Check ,2,1,0,tr("Enable")          ,tr("Enable GREYC restoration")},
    //{"GREYCFast"                ,ptGT_Check ,2,1,1,tr("Enable 'fast'")   ,tr("Enable GREYC 'fast'")},
    {"WebResizeBeforeGamma"       ,ptGT_Check ,1,1,0,tr("before gamma")    ,tr("Webresizing before gamma compensation")},
    {"OutputGammaCompensation"    ,ptGT_Check ,1,1,0,tr("sRGB gamma compensation")    ,tr("sRGB gamma compensation")},
    {"WienerFilter2"              ,ptGT_Check ,2,1,0,tr("Enable")          ,tr("Enable wiener filter")},
    {"WienerFilter2UseEdgeMask"   ,ptGT_Check ,2,1,1,tr("Only edges")      ,tr("Sharpen only edges")},
    {"IncludeExif"                ,ptGT_Check ,2,1,1,tr("Include metadata"),tr("Include metadata (only in jpeg and tiff)")},
    {"EraseExifThumbnail"         ,ptGT_Check ,2,1,1,tr("Erase thumbnail"),tr("Erase the exif thumbnail (only in jpeg and tiff)")}
  };

  // Load in the non gui elements
  const ptItem Items[] = {
    // Name                                 InitLevel  Default                                              JobFile
    {"PipeIsRunning"                        ,9         ,0                                                   ,0},
    {"BlockTools"                           ,9         ,0                                                   ,0},
    {"InputPowerFactor"                     ,0         ,2.2                                                 ,1},
    {"PreviewMode"                          ,1         ,ptPreviewMode_End                                   ,0},
    {"ZoomMode"                             ,9         ,ptZoomMode_Fit                                      ,0},
    {"Scaled"                               ,9         ,0                                                   ,0},
    {"IsRAW"                                ,9         ,1                                                   ,0},
    {"HaveImage"                            ,9         ,0                                                   ,0},
    {"RawsDirectory"                        ,0         ,""                                                  ,0},
    {"OutputDirectory"                      ,0         ,""                                                  ,1},
    {"MainDirectory"                        ,0         ,"@INSTALL@/"                             ,0},
    {"ShareDirectory"                       ,0         ,"@INSTALL@/"                             ,1},
    {"UserDirectory"                        ,0         ,"@INSTALL@/"                             ,1},
    {"TranslationsDirectory"                ,0         ,"@INSTALL@/Translations"                 ,0},
    {"CurvesDirectory"                      ,0         ,"@INSTALL@/Curves"                       ,1},
    {"ChannelMixersDirectory"               ,0         ,"@INSTALL@/ChannelMixers"                ,1},
    {"PresetDirectory"                      ,0         ,"@INSTALL@/Presets"                      ,0},
    {"CameraColorProfilesDirectory"         ,0         ,"@INSTALL@/Profiles/Camera"              ,1},
    {"PreviewColorProfilesDirectory"        ,0         ,"@INSTALL@/Profiles/Preview"             ,0},
    {"OutputColorProfilesDirectory"         ,0         ,"@INSTALL@/Profiles/Output"              ,1},
    {"StandardAdobeProfilesDirectory"       ,0         ,"@INSTALL@/Profiles/Camera/Standard"     ,1},
    {"LensfunDatabaseDirectory"             ,0         ,"@INSTALL@/LensfunDatabase"              ,1},
    {"PreviewColorProfile"                  ,1         ,"@INSTALL@/Profiles/Preview/sRGB.icc"    ,1},
    {"OutputColorProfile"                   ,1         ,"@INSTALL@/Profiles/Output/sRGB.icc"     ,1},
    {"GimpExecCommand"                      ,1         ,"gimp"                                   ,0},
    {"StartupSettingsFile"                  ,1         ,"@INSTALL@/Presets/MakeFancy.pts"        ,0},
    {"CameraMake"                           ,9         ,""                                                  ,0},
    {"CameraModel"                          ,9         ,""                                                  ,0},
    {"CameraColorProfile"                   ,1         ,""                                                  ,1},
    {"HaveBadPixels"                        ,1         ,0                                                   ,1},
    {"BadPixelsFileName"                    ,1         ,""                                                  ,1},
    {"HaveDarkFrame"                        ,1         ,0                                                   ,1},
    {"DarkFrameFileName"                    ,1         ,""                                                  ,1},
    {"VisualSelectionX"                     ,9         ,0                                                   ,1},
    {"VisualSelectionY"                     ,9         ,0                                                   ,1},
    {"VisualSelectionWidth"                 ,9         ,0                                                   ,1},
    {"VisualSelectionHeight"                ,9         ,0                                                   ,1},
    {"LensfunCameraIndex"                   ,9         ,0                                                   ,0},
    {"LensfunCameraUpdatedByProcessor"      ,9         ,0                                                   ,0},
    {"LensfunLensIndex"                     ,9         ,0                                                   ,0},
    {"LensfunHaveTCAModel"                  ,9         ,0                                                   ,0},
    {"LensfunTCAModel"                      ,9         ,tr("None")                                           ,0},
    {"LensfunHaveVignettingModel"           ,9         ,0                                                   ,0},
    {"LensfunVignettingModel"               ,9         ,tr("None")                                           ,0},
    {"LensfunHaveDistortionModel"           ,9         ,0                                                   ,0},
    {"LensfunDistortionModel"               ,9         ,tr("None")                                           ,0},
    {"LensfunCameraMake"                    ,9         ,""                                                  ,0},
    {"LensfunCameraModel"                   ,9         ,""                                                  ,0},
    {"ImageW"                               ,9         ,0                                                   ,0},
    {"ImageH"                               ,9         ,0                                                   ,0},
    {"PipeImageW"                           ,9         ,0                                                   ,0},
    {"PipeImageH"                           ,9         ,0                                                   ,0},
    {"CropX"                                ,9         ,0                                                   ,1},
    {"CropY"                                ,9         ,0                                                   ,1},
    {"CropW"                                ,9         ,0                                                   ,1},
    {"CropH"                                ,9         ,0                                                   ,1},
    {"RotateW"                              ,9         ,0                                                   ,1},
    {"RotateH"                              ,9         ,0                                                   ,1},
    {"ExposureNormalization"                ,9         ,0.0                                                 ,0},
    {"ChannelMixerFileNames"                ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesRGB"                    ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesR"                      ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesG"                      ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesB"                      ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesL"                      ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesLa"                     ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesLb"                     ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesLByHue"                 ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesTexture"                ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesSaturation"             ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesBase"                   ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesBase2"                  ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesShadowsHighlights"      ,0         ,QStringList()                                       ,1},
    {"CurveFileNamesDenoise"                ,0         ,QStringList()                                       ,1},
    {"OutputFileName"                       ,9         ,""                                                  ,0}, // Not in JobFile. Constructed.
    {"JobMode"                              ,9         ,0                                                   ,0}, // Not in JobFile !! Overwrites else.
    {"InputFileNameList"                    ,9         ,QStringList()                                       ,1},
    {"Tone1ColorRed"                        ,2         ,0                                                   ,1},
    {"Tone1ColorGreen"                      ,2         ,200                                                 ,1},
    {"Tone1ColorBlue"                       ,2         ,255                                                 ,1},
    {"Tone2ColorRed"                        ,2         ,255                                                 ,1},
    {"Tone2ColorGreen"                      ,2         ,200                                                 ,1},
    {"Tone2ColorBlue"                       ,2         ,0                                                   ,1},
    {"GradualOverlay1ColorRed"              ,2         ,0                                                   ,1},
    {"GradualOverlay1ColorGreen"            ,2         ,0                                                   ,1},
    {"GradualOverlay1ColorBlue"             ,2         ,0                                                   ,1},
    {"GradualOverlay2ColorRed"              ,2         ,255                                                 ,1},
    {"GradualOverlay2ColorGreen"            ,2         ,200                                                 ,1},
    {"GradualOverlay2ColorBlue"             ,2         ,0                                                   ,1},
    {"DigikamTagsList"                      ,9         ,QStringList()                                       ,1},
    {"TagsList"                             ,9         ,QStringList()                                       ,1},
    {"BackgroundRed"                        ,1         ,0                                                   ,0},
    {"BackgroundGreen"                      ,1         ,0                                                   ,0},
    {"BackgroundBlue"                       ,1         ,0                                                   ,0},
    {"HistogramChannel"                     ,1         ,ptHistogramChannel_RGB                              ,0},
    {"HistogramLogX"                        ,1         ,0                                                   ,0},
    {"HistogramLogY"                        ,1         ,1                                                   ,0},
    {"HistogramMode"                        ,1         ,ptHistogramMode_Preview                             ,0},
    {"HistogramCrop"                        ,9         ,0                                                   ,0},
    {"HistogramCropX"                       ,9         ,0                                                   ,0},
    {"HistogramCropY"                       ,9         ,0                                                   ,0},
    {"HistogramCropW"                       ,9         ,0                                                   ,0},
    {"HistogramCropH"                       ,9         ,0                                                   ,0},
    {"ExposureIndicator"                    ,1         ,0                                                   ,0},
    {"ExposureIndicatorSensor"              ,0         ,0                                                   ,0},
    {"ExposureIndicatorR"                   ,0         ,1                                                   ,0},
    {"ExposureIndicatorG"                   ,0         ,1                                                   ,0},
    {"ExposureIndicatorB"                   ,0         ,1                                                   ,0},
    {"ExposureIndicatorOver"                ,0         ,1                                                   ,0},
    {"ExposureIndicatorUnder"               ,0         ,1                                                   ,0},
    {"ShowExposureIndicatorSensor"          ,0         ,0                                                   ,0},
    {"ShowBottomContainer"                  ,1         ,1                                                   ,0},
    {"ShowToolContainer"                    ,9         ,1                                                   ,0},
    {"SatCurveMode"                         ,1         ,0                                                   ,1},
    {"SatCurveType"                         ,1         ,0                                                   ,1},
    {"TextureCurveType"                     ,1         ,0                                                   ,1},
    {"DenoiseCurveType"                     ,1         ,0                                                   ,1},
    {"FullOutput"                           ,9         ,0                                                   ,0},
    {"HiddenTools"                          ,0         ,QStringList()                                       ,1},
    {"BlockedTools"                         ,0         ,QStringList()                                       ,1},
    {"BlockUpdate"                          ,9         ,0                                                   ,0}
  };

   // Gui Numerical inputs. Copy them from the const array in ptSettingItem.
  short NrSettings = sizeof(GuiInputItems)/sizeof(ptGuiInputItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiInputItem Descridlion = GuiInputItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Descridlion.GuiType;
    SettingItem->InitLevel       = Descridlion.InitLevel;
    SettingItem->InJobFile       = Descridlion.InJobFile;
    SettingItem->HasDefaultValue = Descridlion.HasDefaultValue;
    SettingItem->DefaultValue    = Descridlion.DefaultValue;
    SettingItem->MinimumValue    = Descridlion.MinimumValue;
    SettingItem->MaximumValue    = Descridlion.MaximumValue;
    SettingItem->Step            = Descridlion.Step;
    SettingItem->NrDecimals      = Descridlion.NrDecimals;
    SettingItem->Label           = Descridlion.Label;
    SettingItem->ToolTip         = Descridlion.ToolTip;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Gui Choice inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiChoiceItems)/sizeof(ptGuiChoiceItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiChoiceItem Descridlion = GuiChoiceItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType         = Descridlion.GuiType;
    SettingItem->InitLevel       = Descridlion.InitLevel;
    SettingItem->InJobFile       = Descridlion.InJobFile;
    SettingItem->HasDefaultValue = Descridlion.HasDefaultValue;
    SettingItem->DefaultValue    = Descridlion.DefaultValue;
    SettingItem->Value           = Descridlion.DefaultValue;
    SettingItem->ToolTip         = Descridlion.ToolTip;
    SettingItem->InitialOptions  = Descridlion.InitialOptions;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Gui Check inputs. Copy them from the const array in ptSettingItem.
  NrSettings = sizeof(GuiCheckItems)/sizeof(ptGuiCheckItem);
  for (short i=0; i<NrSettings; i++) {
    ptGuiCheckItem Descridlion = GuiCheckItems[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = Descridlion.GuiType;
    SettingItem->InitLevel    = Descridlion.InitLevel;
    SettingItem->InJobFile    = Descridlion.InJobFile;
    SettingItem->DefaultValue = Descridlion.DefaultValue;
    SettingItem->Value        = Descridlion.DefaultValue;
    SettingItem->Label        = Descridlion.Label;
    SettingItem->ToolTip      = Descridlion.ToolTip;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
  }
  // Non gui elements
  NrSettings = sizeof(Items)/sizeof(ptItem);
  for (short i=0; i<NrSettings; i++) {
    ptItem Descridlion = Items[i];
    ptSettingItem* SettingItem = new ptSettingItem;
    SettingItem->GuiType      = ptGT_None;
    SettingItem->InitLevel    = Descridlion.InitLevel;
    if (Descridlion.DefaultValue.type() == QVariant::String) {
      QString Tmp = Descridlion.DefaultValue.toString();
      Tmp.replace(QString("@INSTALL@"),QCoreApplication::applicationDirPath());
      Descridlion.DefaultValue = Tmp;
    }
    SettingItem->DefaultValue = Descridlion.DefaultValue;
    SettingItem->Value        = Descridlion.DefaultValue;
    SettingItem->InJobFile    = Descridlion.InJobFile;
    M_InsertKeyIntoHash(Descridlion.KeyName,SettingItem);
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
        switch (TargetType) {
          case QVariant::Int:
          case QVariant::UInt:
            Setting->Value = Setting->Value.toInt();
            break;
          case QVariant::Double:
          case QMetaType::Float:
            Setting->Value = Setting->Value.toDouble();
            break;
          case QVariant::StringList:
            Setting->Value = Setting->Value.toStringList();
            break;
          default:
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

const QVariant ptSettings::GetValue(const QString Key) {
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "(%s,%d) Could not find key '%s'\n",
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  return m_Hash[Key]->Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// GetInt
//
////////////////////////////////////////////////////////////////////////////////

int ptSettings::GetInt(const QString Key) {
  // Remark : UInt and Int are mixed here.
  // The only settings related type where u is important is uint16_t
  // (dimensions). uint16_t fits in an integer which is 32 bit.
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::Int && Tmp.type() != QVariant::UInt) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::(U)Int' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    if (Tmp.type() == QVariant::String) {
      ptLogError(ptError_Argument,
                 "Additionally : it's a string '%s'\n",
                 Tmp.toString().toAscii().data());
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

double ptSettings::GetDouble(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (static_cast<QMetaType::Type>(Tmp.type()) == QMetaType::Float)
    Tmp.convert(QVariant::Double);
  if (Tmp.type() != QVariant::Double) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::Double' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    assert(Tmp.type() == QVariant::Double);
  }
  return Tmp.toDouble();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetString
//
////////////////////////////////////////////////////////////////////////////////

const QString ptSettings::GetString(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::String) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::String' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
    assert(Tmp.type() == QVariant::String);
  }
  return Tmp.toString();
}

////////////////////////////////////////////////////////////////////////////////
//
// GetStringList
//
////////////////////////////////////////////////////////////////////////////////

const QStringList ptSettings::GetStringList(const QString Key) {
  QVariant Tmp = GetValue(Key);
  if (Tmp.type() != QVariant::StringList) {
    ptLogError(ptError_Argument,
               "Expected 'QVariant::StringList' but got '%d' for key '%s'\n",
               Tmp.type(),Key.toAscii().data());
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
               __FILE__,__LINE__,Key.toAscii().data());
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
               __FILE__,__LINE__,Key.toAscii().data());
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
                 Key.toAscii().data());
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
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiInput) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiInput\n",
               Key.toAscii().data());
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
               __FILE__,__LINE__,Key.toAscii().data());
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
                 Key.toAscii().data());
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
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
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
  if (!m_Hash.contains(Key)) {
    ptLogError(ptError_Argument,
               "Could not find key '%s'\n",
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
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
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
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
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
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
               Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  if (!m_Hash[Key]->GuiChoice) {
    ptLogError(ptError_Argument,
               "Key '%s' has no initialized GuiChoice\n",
               Key.toAscii().data());
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
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiInput = Value;
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
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiChoice = Value;
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
               __FILE__,__LINE__,Key.toAscii().data());
    assert (m_Hash.contains(Key));
  }
  m_Hash[Key]->GuiCheck = Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// Transfer the settings from Settings To DcRaw UserSettings.
// This is sometimes not straightforward as DcRaw assumes certain
// combinations. That's taken care of here.
//
////////////////////////////////////////////////////////////////////////////////

void ptSettings::ToDcRaw(DcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Relying on m_PipeSize being as defined in the constants ! (1<<Size)
  TheDcRaw->m_UserSetting_HalfSize = GetInt("PipeSize");

  // Input file name

  QString InputFileName = GetStringList("InputFileNameList")[0];
  FREE(TheDcRaw->m_UserSetting_InputFileName);
  TheDcRaw->m_UserSetting_InputFileName =
    (char*) MALLOC(1+strlen(InputFileName.toAscii().data()));
  ptMemoryError(TheDcRaw->m_UserSetting_InputFileName,__FILE__,__LINE__);
  strcpy(TheDcRaw->m_UserSetting_InputFileName,InputFileName.toAscii().data());

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
             strlen(GetString("BadPixelsFileName").toAscii().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_BadPixelsFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_BadPixelsFileName,
             GetString("BadPixelsFileName").toAscii().data());
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
             strlen(GetString("DarkFrameFileName").toAscii().data()));
      ptMemoryError(TheDcRaw->m_UserSetting_DarkFrameFileName,
                    __FILE__,__LINE__);
      strcpy(TheDcRaw->m_UserSetting_DarkFrameFileName,
             GetString("DarkFrameFileName").toAscii().data());
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

void ptSettings::FromDcRaw(DcRaw* TheDcRaw) {

  if (!TheDcRaw) return;

  // Copy make and model to our gui settings (f.i.
  // white balances rely on it)
  SetValue("CameraMake",TheDcRaw->m_CameraMake);
  SetValue("CameraModel",TheDcRaw->m_CameraModel);

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
  if (TheDcRaw->m_RawColor) {
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

////////////////////////////////////////////////////////////////////////////////
//
// Tool Info
// IsActive contains, if the filter will be processed!
//
////////////////////////////////////////////////////////////////////////////////

struct sToolInfo {
  QString               Name;
  int                   IsActive;
  int                   IsHidden;
  int                   IsBlocked;
};

sToolInfo ToolInfo (const QString GuiName) {
  sToolInfo Info = {"N.N.",0,0,0};
  // Tab Geometry
  if (GuiName == "TabRotation") {
      Info.Name = "Rotation";
      Info.IsActive = Settings->GetDouble("Rotate")!=0.0?1:0;
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
  else if (GuiName == "TabChannelMixer") {
      Info.Name = "Channel mixer";
      Info.IsActive = Settings->GetInt("ChannelMixer");
  } else if (GuiName == "TabHighlights") {
      Info.Name = "RGB highlights";
      Info.IsActive = (Settings->GetDouble("HighlightsR")!=0.0 ||
                       Settings->GetDouble("HighlightsG")!=0.0 ||
                       Settings->GetDouble("HighlightsB")!=0.0)!=0?1:0;
  } else if (GuiName == "TabColorIntensity") {
      Info.Name = "RGB color intensity";
      Info.IsActive = (Settings->GetInt("Vibrance") ||
                       Settings->GetInt("IntensityRed") ||
                       Settings->GetInt("IntensityGreen") ||
                       Settings->GetInt("IntensityBlue"))!=0?1:0;
  } else if (GuiName == "TabBrightness") {
      Info.Name = "RGB brightness";
      Info.IsActive = (Settings->GetDouble("CatchWhite")!=0.0 ||
                       Settings->GetDouble("CatchBlack")!=0.0 ||
                       Settings->GetDouble("ExposureGain")!=0.0)!=0?1:0;
  } else if (GuiName == "TabExposure") {
      Info.Name = "Exposure";
      Info.IsActive = Settings->GetDouble("Exposure")!=0.0?1:0;
  } else if (GuiName == "TabReinhard05") {
      Info.Name = "Reinhard 05";
      Info.IsActive = Settings->GetInt("Reinhard05");
  } else if (GuiName == "TabGammaTool") {
      Info.Name = "RGB gamma tool";
      Info.IsActive = Settings->GetDouble("RGBGammaAmount") != 1.0?1:0;
  } else if (GuiName == "TabNormalization") {
      Info.Name = "Normalization";
      Info.IsActive = Settings->GetDouble("NormalizationOpacity")!=0.0?1:0;
  } else if (GuiName == "TabColorEnhance") {
      Info.Name = "RGB color enhancement";
      Info.IsActive = (Settings->GetDouble("ColorEnhanceShadows")!=0.0 ||
                       Settings->GetDouble("ColorEnhanceHighlights")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBRecovery") {
      Info.Name = "RGB Low/Mid/Highlight Recovery";
      Info.IsActive = Settings->GetInt("LMHLightRecovery1MaskType") ||
                      Settings->GetInt("LMHLightRecovery2MaskType");
  } else if (GuiName == "TabRGBTextureContrast") {
      Info.Name = "RGB texture contrast";
      Info.IsActive = (Settings->GetDouble("RGBTextureContrastAmount")!=0.0 &&
                       Settings->GetDouble("RGBTextureContrastOpacity")!=0.0)!=0?1:0;
  } else if (GuiName == "TabRGBLocalContrast1") {
      Info.Name = "RGB Local Contrast 1";
      Info.IsActive = Settings->GetInt("Microcontrast1MaskType");
  } else if (GuiName == "TabRGBLocalContrast2") {
      Info.Name = "RGB Local Contrast 2";
      Info.IsActive = Settings->GetInt("Microcontrast2MaskType");
  } else if (GuiName == "TabRGBContrast") {
      Info.Name = "RGB Contrast";
      Info.IsActive = Settings->GetDouble("RGBContrastAmount")!=0.0;
  } else if (GuiName == "TabRGBLevels") {
      Info.Name = "RGB Levels";
      Info.IsActive = (Settings->GetDouble("LevelsBlackPoint")!=0.0 ||
                      (Settings->GetDouble("LevelsWhitePoint")-1.0)!=0.0)?1:0;
  } else if (GuiName == "TabRGBCurve") {
      Info.Name = "RGB tone curve";
      Info.IsActive = Settings->GetInt("CurveRGB");
  }
  // Lab Color and Contrast
  else if (GuiName == "TabLABTransform") {
      Info.Name = "Lab Transform";
      Info.IsActive = Settings->GetInt("LABTransform");
  } else if (GuiName == "TabLABShadowsHighlights") {
      Info.Name = "Lab Shadows and Highlights";
      Info.IsActive = (Settings->GetDouble("ShadowsHighlightsFine")!=0.0 ||
                      Settings->GetDouble("ShadowsHighlightsCoarse")!=0.0 ||
                      Settings->GetInt("CurveShadowsHighlights"))?1:0;
  } else if (GuiName == "TabLABRecovery") {
      Info.Name = "Lab Low/Mid/Highlight Recovery";
      Info.IsActive = Settings->GetInt("LabLMHLightRecovery1MaskType") ||
                      Settings->GetInt("LabLMHLightRecovery2MaskType");
  } else if (GuiName == "TabLABDRC") {
      Info.Name = "Lab Dynamic Range Compression";
      Info.IsActive = Settings->GetDouble("DRCBeta")!=1.0?1:0;
  } else if (GuiName == "TabLABTextureCurve") {
      Info.Name = "Lab Texture Curve";
      Info.IsActive = Settings->GetInt("CurveTexture");
  } else if (GuiName == "TabLABTexture1") {
      Info.Name = "Lab Texture Contrast 1";
      Info.IsActive = (Settings->GetDouble("TextureContrast1Amount")!=0.0 &&
                      Settings->GetDouble("TextureContrast1Opacity")!=0.0)?1:0;
  } else if (GuiName == "TabLABTexture2") {
      Info.Name = "Lab Texture Contrast 2";
      Info.IsActive = (Settings->GetDouble("TextureContrast2Amount")!=0.0 &&
                      Settings->GetDouble("TextureContrast2Opacity")!=0.0)?1:0;
  } else if (GuiName == "TabLABLocalContrast1") {
      Info.Name = "Lab Local Contrast 1";
      Info.IsActive = Settings->GetInt("LabMicrocontrast1MaskType");
  } else if (GuiName == "TabLABLocalContrast2") {
      Info.Name = "Lab Local Contrast 2";
      Info.IsActive = Settings->GetInt("LabMicrocontrast2MaskType");
  } else if (GuiName == "TabLABLCStretch1") {
      Info.Name = "Lab Local Contrast Stretch 1";
      Info.IsActive = Settings->GetDouble("LC1Opacity")!=0.0?1:0;
  } else if (GuiName == "TabLABLCStretch2") {
      Info.Name = "Lab Local Contrast Stretch 2";
      Info.IsActive = Settings->GetDouble("LC2Opacity")!=0.0?1:0;
  } else if (GuiName == "TabLABContrast") {
      Info.Name = "Lab L* Contrast";
      Info.IsActive = Settings->GetDouble("ContrastAmount")!=0.0?1:0;
  } else if (GuiName == "TabLABSaturation") {
      Info.Name = "Lab Saturation";
      Info.IsActive = Settings->GetDouble("SaturationAmount")!=0.0?1:0;
  } else if (GuiName == "TabLABColorBoost") {
      Info.Name = "Lab Color Boost";
      Info.IsActive = ((Settings->GetDouble("ColorBoostValueA")-1)!=0.0 ||
                      (Settings->GetDouble("ColorBoostValueB")-1)!=0.0)?1:0;
  } else if (GuiName == "TabLABLevels") {
      Info.Name = "Lab Levels";
      Info.IsActive = (Settings->GetDouble("LabLevelsBlackPoint")!=0.0 ||
                      (Settings->GetDouble("LabLevelsWhitePoint")-1.0)!=0.0)?1:0;
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
  } else if (GuiName == "TabWaveletDenoise") {
      Info.Name = "Lab Wavelet denoise";
      Info.IsActive = (Settings->GetDouble("WaveletDenoiseL")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseA")!=0.0 ||
                      Settings->GetDouble("WaveletDenoiseB")!=0.0)?1:0;
  } else if (GuiName == "TabLuminanceDenoise") {
      Info.Name = "Lab Luminance denoise";
      Info.IsActive = Settings->GetDouble("BilateralLOpacity")!=0.0?1:0;
  } else if (GuiName == "TabPyramidDenoise") {
      Info.Name = "Lab Pyramid denoise";
      Info.IsActive = (Settings->GetInt("PyrDenoiseLAmount")!=0.0||
                       Settings->GetInt("PyrDenoiseABAmount"))!=0.0?1:0;
  } else if (GuiName == "TabColorDenoise") {
      Info.Name = "Lab Levels";
      Info.IsActive = (Settings->GetDouble("BilateralASigmaR")!=0.0 ||
                      Settings->GetDouble("BilateralBSigmaR")!=0.0)?1:0;
  } else if (GuiName == "TabDetailCurve") {
      Info.Name = "Lab detail curve";
      Info.IsActive = Settings->GetInt("CurveDenoise") ||
                      Settings->GetDouble("DetailCurveHotpixel")!=0.0;
  } else if (GuiName == "TabLABGradientSharpen") {
      Info.Name = "Lab Gradient Sharpen";
      Info.IsActive = Settings->GetInt("GradientSharpenPasses") ||
                      Settings->GetDouble("MLMicroContrastStrength")!=0.0 ||
                      Settings->GetDouble("LabHotpixel")!=0.0;
  } else if (GuiName == "TabLABWiener") {
      Info.Name = "Lab Wiener filter";
      Info.IsActive = Settings->GetInt("WienerFilter");
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
  else if (GuiName == "TabLbyHue") {
      Info.Name = "Lab luminance by hue curve";
      Info.IsActive = Settings->GetInt("CurveLByHue")!=0?1:0;
  } else if (GuiName == "TabSaturationCurve") {
      Info.Name = "Lab saturation curve";
      Info.IsActive = Settings->GetInt("CurveSaturation")!=0?1:0;
  } else if (GuiName == "TabLCurve") {
      Info.Name = "Lab luminance curve";
      Info.IsActive = Settings->GetInt("CurveL")!=0?1:0;
  } else if (GuiName == "TabABCurves") {
      Info.Name = "Lab color curves";
      Info.IsActive = (Settings->GetInt("CurveLa") ||
                       Settings->GetInt("CurveLb"))!=0?1:0;
  } else if (GuiName == "TabColorContrast") {
      Info.Name = "Lab color contrast";
      Info.IsActive = Settings->GetDouble("ColorcontrastOpacity")!=0.0?1:0;
  } else if (GuiName == "TabLABToneAdjust1") {
      Info.Name = "Lab tone adjustment 1";
      Info.IsActive = Settings->GetInt("LABToneAdjust1MaskType")!=0?1:0;
  } else if (GuiName == "TabLABToneAdjust2") {
      Info.Name = "Lab tone adjustment 2";
      Info.IsActive = Settings->GetInt("LABToneAdjust2MaskType")!=0?1:0;
  } else if (GuiName == "TabLAdjustment") {
      Info.Name = "Lab luminance adjustment";
      Info.IsActive = (Settings->GetDouble("LAdjustC1")!=0.0 ||
                      Settings->GetDouble("LAdjustC2")!=0.0 ||
                      Settings->GetDouble("LAdjustC3")!=0.0 ||
                      Settings->GetDouble("LAdjustC4")!=0.0 ||
                      Settings->GetDouble("LAdjustC5")!=0.0 ||
                      Settings->GetDouble("LAdjustC6")!=0.0 ||
                      Settings->GetDouble("LAdjustC7")!=0.0 ||
                      Settings->GetDouble("LAdjustC8")!=0.0)!=0?1:0;
  } else if (GuiName == "TabSaturationAdjustment") {
      Info.Name = "Lab saturation adjustment";
      Info.IsActive = (Settings->GetDouble("LAdjustSC1")!=0.0 ||
                      Settings->GetDouble("LAdjustSC2")!=0.0 ||
                      Settings->GetDouble("LAdjustSC3")!=0.0 ||
                      Settings->GetDouble("LAdjustSC4")!=0.0 ||
                      Settings->GetDouble("LAdjustSC5")!=0.0 ||
                      Settings->GetDouble("LAdjustSC6")!=0.0 ||
                      Settings->GetDouble("LAdjustSC7")!=0.0 ||
                      Settings->GetDouble("LAdjustSC8")!=0.0)!=0?1:0;
  } else if (GuiName == "TabLABTone") {
      Info.Name = "Lab toning";
      Info.IsActive = (Settings->GetDouble("LABToneAmount")!=0.0 ||
                       Settings->GetDouble("LABToneSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneSAmount") != 0.0 ||
                       Settings->GetDouble("LABToneSSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneMAmount")!=0.0 ||
                       Settings->GetDouble("LABToneMSaturation") != 1.0 ||
                       Settings->GetDouble("LABToneHAmount")!=0.0 ||
                       Settings->GetDouble("LABToneHSaturation") != 1.0)!=0?1:0;
  } else if (GuiName == "TabLABVignette") {
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
  } else if (GuiName == "TabRGBTone") {
      Info.Name = "RGB Toning";
      Info.IsActive = (Settings->GetInt("Tone1MaskType") ||
                       Settings->GetInt("Tone2MaskType"))!=0?1:0;
  } else if (GuiName == "TabCrossProcessing") {
      Info.Name = "Cross processing";
      Info.IsActive = Settings->GetInt("CrossprocessingMode")!=0?1:0;
  } else if (GuiName == "TabECContrast") {
      Info.Name = "Eyecandy contrast";
      Info.IsActive = Settings->GetDouble("RGBContrast2Amount")!=0.0?1:0;
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
  } else if (GuiName == "TabSoftglow") {
      Info.Name = "Softglow / Orton";
      Info.IsActive = (Settings->GetInt("SoftglowMode") &&
                       Settings->GetDouble("SoftglowAmount")!=0.0)!=0?1:0;
  } else if (GuiName == "TabECColorIntensity") {
      Info.Name = "Eyecandy color intensity";
      Info.IsActive = (Settings->GetInt("Vibrance2") ||
                       Settings->GetInt("Intensity2Red") ||
                       Settings->GetInt("Intensity2Green") ||
                       Settings->GetInt("Intensity2Blue"))!=0?1:0;
  } else if (GuiName == "TabRToneCurve") {
      Info.Name = "Eyecandy R tone curve";
      Info.IsActive = Settings->GetInt("CurveR");
  } else if (GuiName == "TabGToneCurve") {
      Info.Name = "Eyecandy G tone curve";
      Info.IsActive = Settings->GetInt("CurveG");
  } else if (GuiName == "TabBToneCurve") {
      Info.Name = "Eyecandy B tone curve";
      Info.IsActive = Settings->GetInt("CurveB");
  }
  // Tab Output
  else if (GuiName == "TabBaseCurve") {
      Info.Name = "Output base curve";
      Info.IsActive = Settings->GetInt("BaseCurve")!=0?1:0;
  } else if (GuiName == "TabGammaCompensation") {
      Info.Name = "Output gamma compensation";
      Info.IsActive = Settings->GetInt("OutputGammaCompensation")!=0?1:0;
  } else if (GuiName == "TabWebResize") {
      Info.Name = "Output webreisze";
      Info.IsActive = (Settings->GetInt("WebResize")==2 ||
                       (Settings->GetInt("FullOutput") &&
                        Settings->GetInt("WebResize")==1))!=0?1:0;
  } else if (GuiName == "TabAfterGammaCurve") {
      Info.Name = "Output after gamma curve";
      Info.IsActive = Settings->GetInt("BaseCurve2")!=0?1:0;
  } else if (GuiName == "TabOutContrast") {
      Info.Name = "Output sigmiodal contrast";
      Info.IsActive = Settings->GetDouble("RGBContrast3Amount")!=0.0?1:0;
  } else if (GuiName == "TabOutWiener") {
      Info.Name = "Output wiener filter";
      Info.IsActive = Settings->GetInt("WienerFilter2")!=0?1:0;
  }

  // tool blocked?
  Info.IsBlocked = (Settings->GetStringList("BlockedTools")).contains(GuiName)?1:0;

  // tool hidden?
  Info.IsHidden = (Settings->GetStringList("HiddenTools")).contains(GuiName)?1:0;
  return Info;
}

int ptSettings::ToolAlwaysVisible(const QString GuiName) {
  QStringList VisibleTools =
  (QStringList()
    // Settings tab
    << "TabWorkColorSpace"
    << "TabPreviewColorSpace"
    << "TabGimpCommand"
    << "TabRememberSettings"
    << "TabInputControl"
    << "TabToolBoxControl"
    << "TabTabStatusIndicator"
    << "TabPreviewControl"
    << "TabTheming"
    << "TabTranslation"
    << "TabMemoryTest"
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

QString ptSettings::ToolGetName (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.Name;
}

int ptSettings::ToolIsActive (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return (Info.IsHidden || Info.IsBlocked)?0:Info.IsActive;
}

int ptSettings::ToolIsBlocked (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsBlocked;
}

int ptSettings::ToolIsHidden (const QString GuiName) {
  sToolInfo Info = ToolInfo(GuiName);
  return Info.IsHidden;
}
///////////////////////////////////////////////////////////////////////////////
