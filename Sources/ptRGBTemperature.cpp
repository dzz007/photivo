/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include "ptDefines.h"
#include "ptConstants.h"

#include <lcms2.h>

#include <cassert>
#include <cstdio>

void TemperatureToRGB(int Temperature,double RGB[3]) {

  cmsCIExyY WhitePoint;
  double XYZ[3];

  if (Temperature>4000) {
    cmsWhitePointFromTemp(&WhitePoint,Temperature);
    XYZ[0] = WhitePoint.x/WhitePoint.y;
    XYZ[1] = 1.0;
    XYZ[2] = (1-WhitePoint.x-WhitePoint.y)/WhitePoint.y;
  } else {
    // ufraw code snippet. Not sure of origin.
    double T = Temperature;
    double xD = 0.27475e9/(T*T*T) - 0.98598e6/(T*T) + 1.17444e3/T + 0.145986;
    double yD = -3*xD*xD + 2.87*xD - 0.275;
    XYZ[0] = xD/yD;
    XYZ[1] = 1.0;
    XYZ[2] = (1-xD-yD)/yD;
  }

  double Maximum = 0;

  for (short c=0; c<3; c++) {
    RGB[c] = 0;
    for (short k=0;k<3;k++) {
      RGB[c] += MatrixXYZToRGB[ptSpace_sRGB_D65][c][k]*XYZ[k];
    }
    if (RGB[c]>Maximum) Maximum = RGB[c];
  }
  // Normalize to maximum 1.
  for(short c=0; c<3; c++) RGB[c] = RGB[c]/Maximum;
}

// Do the inverse lookup.

void RGBToTemperature(double RGB[3],int *Temperature,double *Green) {
  int Tmax = 15000;
  int Tmin = 2000;
  int Accuracy = (int)(0.001*(Tmax-Tmin));
  double TryRGB[3];
  for (*Temperature=(Tmax+Tmin)/2;
        Tmax-Tmin>Accuracy;
       *Temperature=(Tmax+Tmin)/2) {
    TemperatureToRGB(*Temperature,TryRGB);
    if (TryRGB[2]/TryRGB[0] > RGB[2]/RGB[0])
      // The tried temperature gives too red => decrease temp.
      Tmax = *Temperature;
    else
      Tmin = *Temperature;
  }
  // The remaining degree of freedom
  *Green = (TryRGB[1]/TryRGB[0]) / (RGB[1]/RGB[0]);
}

////////////////////////////////////////////////////////////////////////////////
