/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLRESIZEFILTERS_H
#define DLRESIZEFILTERS_H

float BoxFunction(float x);
float TriangleFunction(float x);
float QuadraticFunction(float x);
float CubicBSplineFunction(float x);
float QuadraticBSplineFunction(float x);
float CubicConvolutionFunction(float x);
float sinc(float x);
float Lanczos3Function(float x);
float MitchellFunction(float x);
float CatmullRomFunction(float x);
float CosineFunction(float x);
float BellFunction(float x);
float HermiteFunction(float x);

// Keep ordered according to constants !!!

const float FilterLobes[12] = {/* Box */               0.5,
                               /* Triangle */          1.0,
                               /* Quadratic */         1.5,
                               /* CubicBSpline */      2.0,
                               /* QuadraticBSpline */  1.5,
                               /* CubicConvolution */  3.0,
                               /* Lanczos3 */          3.0,
                               /* Mitchell */          2.0,
                               /* CatmullRom */        2.0,
                               /* Cosine */            1.0,
                               /* Bell */              1.5,
                               /* Hermite */           1.0};
// Tables and its sizes, but those are only filled 'on request'.
const  int16_t  SamplesPerLobe = 4096;
extern uint16_t FilterTableSize[12];
extern short    FilterTableInited[12];
extern float*   FilterTable[12];
extern float    (*FilterFunction[12])(float);
#endif

////////////////////////////////////////////////////////////////////////////////
