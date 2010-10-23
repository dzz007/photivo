////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
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

#include <stdio.h>
#include <ctype.h>
#include <lcms2.h>
#include <math.h>
#include <assert.h>
#include <lcms2_plugin.h>


#include "ptConstants.h"

cmsHPROFILE MatrixToProfile(cmsMAT3 MatrixXYZToCam) {

  cmsMAT3 MatrixCamToXYZ;
  _cmsMAT3inverse(&MatrixXYZToCam, &MatrixCamToXYZ);

  cmsCIEXYZ RedXYZ   = { X : MatrixCamToXYZ.v[0].n[0],
                         Y : MatrixCamToXYZ.v[1].n[0],
                         Z : MatrixCamToXYZ.v[2].n[0]};
  cmsCIEXYZ GreenXYZ = { X : MatrixCamToXYZ.v[0].n[1],
                         Y : MatrixCamToXYZ.v[1].n[1],
                         Z : MatrixCamToXYZ.v[2].n[1]};
  cmsCIEXYZ BlueXYZ  = { X : MatrixCamToXYZ.v[0].n[2],
                         Y : MatrixCamToXYZ.v[1].n[2],
                         Z : MatrixCamToXYZ.v[2].n[2]};

  cmsCIExyY RedxyY;
  cmsCIExyY GreenxyY;
  cmsCIExyY BluexyY;

  cmsXYZ2xyY(&RedxyY,&RedXYZ);
  cmsXYZ2xyY(&GreenxyY,&GreenXYZ);
  cmsXYZ2xyY(&BluexyY,&BlueXYZ);

  cmsCIExyYTRIPLE Primaries = {Red : RedxyY, Green : GreenxyY, Blue : BluexyY};

  //double gamma  = 0.45;
  //double linear = 0.10;
  //double g = gamma*(1.0-linear)/(1.0-linear*gamma);
  //double a = 1.0/(1.0+linear*(g-1.0));
  //double b = linear*(g-1.0)*a;
  //double c = pow(a*linear+b,g)/linear;
  //double Params[] = {g,a,b,c,linear};
  // Gamma => None (=1)
  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE Profile = cmsCreateRGBProfile(cmsD50_xyY(),&Primaries,Gamma3);
  cmsFreeToneCurve(Gamma);

  return Profile;
}

#include "ptAdobeTable.h"

int main() {

  cmsMAT3 MatrixXYZToCam;

  for (int i=0; i < (int) sizeof AdobeTable / (int) sizeof *AdobeTable; i++){

    printf("%s\n",AdobeTable[i].Identification);

    // Might be impacting, but not sure yet how. TODO FIXME
    bool HaveBlackpointProblem = 0;
    if (AdobeTable[i].Blackpoint) {
      printf("Check me. Have blackpoint. '%s'\n",AdobeTable[i].Identification);
      HaveBlackpointProblem = 1;
    }

    for (short j = 0; j < 3; j++) {
      for (short k = 0; k < 3; k++) {
        MatrixXYZToCam.v[j].n[k] = AdobeTable[i].XYZToCam[j*3 + k] /10000.0;
      }
    }


    bool HaveFourColorProblem = 0;
    for (short j = 9; j < 12; j++) {
      if (AdobeTable[i].XYZToCam[j] != 0) {
        HaveFourColorProblem = 1;
        break;
      }
    }

    if (HaveFourColorProblem) {
      printf("Check me. Have 4 color. '%s'\n",AdobeTable[i].Identification);
    }

    cmsHPROFILE Profile = MatrixToProfile(MatrixXYZToCam);
    cmsSetDeviceClass(Profile,cmsSigInputClass);
    cmsSetColorSpace(Profile,cmsSigRgbData);
    cmsSetPCS(Profile,cmsSigXYZData);
    char OutputFileName[1024];
    snprintf(OutputFileName,1023,"Profiles/Camera/Standard/%s.icc",
             AdobeTable[i].Identification);
    for (unsigned i=0;i<strlen(OutputFileName);i++) {
      if (isspace(OutputFileName[i])) OutputFileName[i]='_';
      // Windows has issue with '*'
      if (OutputFileName[i]=='*') OutputFileName[i]='_';
    }

    cmsSaveProfileToFile(Profile,OutputFileName);

    cmsCloseProfile(Profile);
  }

  printf("%s\n","Flat Profile");

  for (short j = 0; j < 3; j++) {
    for (short k = 0; k < 3; k++) {
      MatrixXYZToCam.v[j].n[k] = MatrixXYZToRGB[ptSpace_sRGB_D65][j][k];
    }
  }

  cmsHPROFILE Profile = MatrixToProfile(MatrixXYZToCam);
  cmsSetDeviceClass(Profile,cmsSigInputClass);
  cmsSetColorSpace(Profile,cmsSigRgbData);
  cmsSetPCS(Profile,cmsSigXYZData);
  char OutputFileName[1024];
  snprintf(OutputFileName,1023,"Profiles/Camera/Flat/FlatProfile.icc");

  cmsSaveProfileToFile(Profile,OutputFileName);

  cmsCloseProfile(Profile);

  // linear sRGB profile
  printf("%s\n","Linear profiles");
  cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
  cmsToneCurve* Gamma3[3];
  Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

  cmsHPROFILE LinearProfile = 0;

  cmsCIExyY       D65;
  cmsCIExyY       D50;
  cmsWhitePointFromTemp(&D65, 6503);
  cmsWhitePointFromTemp(&D50, 5003);

  cmsCIExyY       DFromReference;

  DFromReference = D65;

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_sRGB_D65],
                                     Gamma3);

  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/sRGBlinear.icc");

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_AdobeRGB_D65],
                                     Gamma3);

  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/AdobeRGBlinear.icc");

  DFromReference = D50;

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_WideGamutRGB_D50],
                                     Gamma3);

  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/WideGamutRGBlinear.icc");

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_ProPhotoRGB_D50],
                                     Gamma3);

  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/ProPhotoRGBlinear.icc");

  cmsCloseProfile(LinearProfile);

  // sRGB profile
  printf("%s\n","sRGB profile");
  cmsHPROFILE sRGBProfile = cmsCreate_sRGBProfile();
  cmsSaveProfileToFile(sRGBProfile,
                       "Profiles/Output/sRGB.icc");
  cmsSaveProfileToFile(sRGBProfile,
                       "Profiles/Preview/sRGB.icc");
  cmsCloseProfile(sRGBProfile);
}
