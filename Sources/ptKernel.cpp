////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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
#include <math.h>
#include <stdlib.h>
#include "ptKernel.h"
#include "ptError.h"

////////////////////////////////////////////////////////////////////////////////
//
// GetOptimalKernelWidth1D
//
////////////////////////////////////////////////////////////////////////////////

uint16_t ptGetOptimalKernelWidth1D(const double Radius,
                                 const double Sigma) {

  assert (Sigma != 0.0);

  uint16_t Width;

  if (Radius > 0.0) {
    return MAX ( 2*(uint16_t)(Radius)+1 ,3 );
  }

  for (Width=5;;) {
    double Normalize=0.0;
    int16_t u;
    for (u=(-Width/2); u <= (Width/2); u++) {
      Normalize += exp(-((double) u*u)/(2.0*Sigma*Sigma))/(ptSQ2PI*Sigma);
    }
    u=Width/2;
    double Value =
      exp(-((double) u*u)/(2.0*Sigma*Sigma))/(ptSQ2PI*Sigma)/Normalize;
    if ((uint16_t) (0xffff*Value) <= 0)
      break;
    Width+=2;
  }
  return((uint16_t) (Width-2));
}

////////////////////////////////////////////////////////////////////////////////
//
// GetBlurKernel
//
////////////////////////////////////////////////////////////////////////////////

double *GetBlurKernel(const uint16_t Width,
                      const double   Sigma) {

  double* Kernel;
  // Generate a 1-D convolution kernel.
  Kernel=(double *) CALLOC2(Width,sizeof(*Kernel));
  ptMemoryError(Kernel,__FILE__,__LINE__);

  const short KernelRank = 3;
  int32_t Bias= (KernelRank*Width)/2;
  for (int32_t i=(-Bias); i <= Bias; i++) {
    double Alpha =
      exp((-((double) (i*i))/(2.0*KernelRank*KernelRank*Sigma*Sigma)));
    Kernel[(i+Bias)/KernelRank] += (double) (Alpha/(ptSQ2PI*Sigma));
  }
  double Normalize=0.0;
  for (uint16_t i=0; i < Width; i++) Normalize += Kernel[i];
  for (uint16_t i=0; i < Width; i++) Kernel[i] /= Normalize;
  return(Kernel);
}

////////////////////////////////////////////////////////////////////////////////
