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

#ifndef FLKERNEL_H
#define FLKERNEL_H

#include "ptDefines.h"
#include "ptConstants.h"

//  GetOptimalKernelWidth() computes kernel radius for a convolution.
//  Start with the value of 3 pixels and go further until below
//  1 bit accuracy.
//    Radius : Radius of the Gaussian in pixels not counting the center pixel.
//      If Radius is >0.0 it defines basically the radius.
//      If < 0.0 above search is done.
//    Sigma  : The standard deviation of the Gaussian, in pixels.
uint16_t ptGetOptimalKernelWidth1D(const double Radius,
                                 const double Sigma);

// Construct a 1D Blur Kernel.
//   Width : Width of the kernel.
//   Sigma : The standard deviation of the Gaussian, in pixels.
double *GetBlurKernel(const uint16_t Width,
                      const double   Sigma);

#endif
////////////////////////////////////////////////////////////////////////////////
