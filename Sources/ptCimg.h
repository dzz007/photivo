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

#ifndef DLGREYCSTORATION
#define DLGREYCSTORATION

#include "ptImage.h"
#include "ptError.h"
#include "ptConstants.h"

void ptGreycStoration(ptImage* Image,
                      void (*ReportProgress)(const QString Message),
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
                      short    Fast);

void ptGreycStorationLab(ptImage* Image,
                         void (*ReportProgress)(const QString Message),
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
       double   Opacity);

void ptCimgEdgeTensors(ptImage* Image,
           const double    Sharpness,
                       const double    Anisotropy,
                       const double    Alpha,
                       const double    Sigma,
           const double    Blur = 0,
           const short     MaskType = ptMaskType_All);

void ptCimgBlurBilateralChannel(ptImage* Image,
              const float Sigma_s,
        const float Sigma_r);

void ptCimgBlur(ptImage* Image,
          const short ChannelMask,
    const float Sigma);

void ptCimgBlurLayer(uint16_t *Layer,
         const uint16_t Width,
         const uint16_t Height,
         const float Sigma);

void ptCimgNoise(ptImage* Image,
     const double Sigma,
     const short NoiseType,
           const double Radius);

void ptCimgSharpen(ptImage* Image,
       const short ChannelMask,
       const float Amplitude,
       const short NrIterations,
       const short UseEdgeMask = 0);

float *ptGradientMask(const ptImage* Image, const double Radius, const double Threshold = 0);

void ptCimgEdgeDetection(ptImage* Image, const short ChannelMask);

void ptCimgEqualize(ptImage* Image, const double Opacity);

void ptCimgRotate(ptImage* Image, const double Angle, const short Interpolation);

#endif

////////////////////////////////////////////////////////////////////////////////
