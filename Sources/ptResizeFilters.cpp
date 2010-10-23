////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
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

#include <math.h>
#include <stdint.h>
#include "ptConstants.h"

float BoxFunction(float x) {
  if (x < 0) x = - x;
  if (x <= 0.5) return 1;
  return 0;
}

float TriangleFunction(float x) {
  if (x < 0) x = - x;
  if (x < 1) return (1-x);
  return 0;
}

float QuadraticFunction(float x) {
  if (x < 0) x = - x;
  if (x <= 0.5) return (-2*x*x+1);
  if (x <= 1.5) return (x*x-2.5*x+1.5);
  return 0;
}

float CubicBSplineFunction(float x) {
  if (x < 0) x = - x;
  if (x < 1 ) {
    float temp = x*x;
    return (0.5*temp*x-temp+2.0/3.0);
  }
  if (x < 2) {
    x = 2.0 - x;
    return (pow(x,3)/6.0);
  }
  return 0;
}

float QuadraticBSplineFunction(float x) {
  if (x < 0) x = - x;
  if (x <= 0.5) return (-x*x+0.75);
  if (x <= 1.5) return (0.5*x*x-1.5*x+ 1.125);
  return 0;
}

float CubicConvolutionFunction(float x) {
  if (x < 0) x = - x;
  float temp = x*x;
  if (x <= 1) return ((4.0/3.0)*temp*x-(7.0/3.0)*temp+1);
  if (x <= 2) return (-(7.0/12.0)*temp*x+3*temp-(59.0/12.0)*x+2.5);
  if (x <= 3) return ((1.0/12.0)*temp*x-(2.0/3.0)*temp+1.75*x-1.5);
  return 0;
}

float sinc(float x) {
  if (x == 0) return 1;
  x *= ptPI;
  return sin(x)/x;
}

float Lanczos3Function(float x) {
  if (x < 0) x = - x;
  if (x < 3) return (sinc(x)*sinc(x/3.0));
  return 0;
}

float MitchellFunction(float x) {
  const float C = 1.0/3;
  if (x < 0) x = - x;
  float temp = x*x;
  if (x < 1) {
    x = (((12-9*C-6*C)* (x*temp)) +
         ((-18+12*C+6*C)*temp) +
         (6-2*C));
    return (x/6);
  }
  if (x < 2) {
    x = (((-C-6*C)*(x*temp)) +
         ((6*C+30*C)*temp) +
         ((-12*C-48*C)*x) +
         (8*C+24*C));
    return (x/6);
  }
  return 0;
}

float CatmullRomFunction(float x) {
  if (x < 0) x = - x;
  float temp = x*x;
  if (x <= 1) return (1.5*temp*x-2.5*temp+ 1);
  if (x <= 2) return (-0.5*temp*x+2.5*temp- 4*x+2);
  return 0;
}

float CosineFunction(float x) {
  if ((x >= -1) && (x <= 1)) return ((cos(x*ptPI)+1)/2.0);
  return 0;
}

float BellFunction(float x) {
  if (x < 0) x = - x;
  if (x < 0.5) return (0.75-x*x);
  if (x < 1.5) return (0.5*pow(x-1.5,2));
  return 0;
}

float HermiteFunction(float x) {
  if (x < 0) x = - x;
  if (x < 1) return ((2*x-3)*x*x+1);
  return 0;
}

uint16_t FilterTableSize[12];
short    FilterTableInited[12];
float*   FilterTable[12];

float (*FilterFunction[12])(float) = {
                               /* Box */               BoxFunction,
                               /* Triangle */          TriangleFunction,
                               /* Quadratic */         QuadraticFunction,
                               /* CubicBSpline */      CubicBSplineFunction,
                               /* QuadraticBSpline */  QuadraticBSplineFunction,
                               /* CubicConvolution */  CubicConvolutionFunction,
                               /* Lanczos3 */          Lanczos3Function,
                               /* Mitchell */          MitchellFunction,
                               /* CatmullRom */        CatmullRomFunction,
                               /* Cosine */            CosineFunction,
                               /* Bell */              BellFunction,
                               /* Hermite */           HermiteFunction};

////////////////////////////////////////////////////////////////////////////////
