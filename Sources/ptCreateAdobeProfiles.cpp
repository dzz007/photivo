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

#include <stdio.h>
#include <ctype.h>
#include <lcms2.h>
#include <math.h>
#include <assert.h>
#include <lcms2_plugin.h>


#include "ptConstants.h"

void create_darktable_profiles();

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

  cmsMLU *Description, *Model;
  Description = cmsMLUalloc(NULL, 1);
  Model = cmsMLUalloc(NULL, 1);

  DFromReference = D65;

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_sRGB_D65],
                                     Gamma3);
  cmsMLUsetASCII(Description,  "en", "US", "Linear sRGB");
  cmsMLUsetASCII(Model,    "en", "US", "LCMS2 linear sRGB");
  //~ cmsWriteTag(LinearProfile, cmsSigDeviceModelDescTag, Model);
  //~ cmsWriteTag(LinearProfile, cmsSigProfileDescriptionTag, Description);
  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/sRGBlinear.icc");

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_AdobeRGB_D65],
                                     Gamma3);
  cmsMLUsetASCII(Description,  "en", "US", "Linear Adobe RGB");
  cmsMLUsetASCII(Model,    "en", "US", "LCMS2 linear Adobe RGB");
  //~ cmsWriteTag(LinearProfile, cmsSigDeviceModelDescTag, Model);
  //~ cmsWriteTag(LinearProfile, cmsSigProfileDescriptionTag, Description);
  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/AdobeRGBlinear.icc");

  DFromReference = D50;

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_WideGamutRGB_D50],
                                     Gamma3);

  cmsMLUsetASCII(Description,  "en", "US", "Linear WideGamut RGB");
  cmsMLUsetASCII(Model,    "en", "US", "LCMS2 linear WirdeGamut RGB");
  //~ cmsWriteTag(LinearProfile, cmsSigDeviceModelDescTag, Model);
  //~ cmsWriteTag(LinearProfile, cmsSigProfileDescriptionTag, Description);
  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/WideGamutRGBlinear.icc");

  LinearProfile = cmsCreateRGBProfile(&DFromReference,
                                     (cmsCIExyYTRIPLE*)&RGBPrimaries[ptSpace_ProPhotoRGB_D50],
                                     Gamma3);

  cmsMLUsetASCII(Description,  "en", "US", "Linear ProPhoto RGB");
  cmsMLUsetASCII(Model,    "en", "US", "LCMS2 linear ProPhoto RGB");
  //~ cmsWriteTag(LinearProfile, cmsSigDeviceModelDescTag, Model);
  //~ cmsWriteTag(LinearProfile, cmsSigProfileDescriptionTag, Description);
  cmsSaveProfileToFile(LinearProfile,
                       "Profiles/Output/ProPhotoRGBlinear.icc");

  cmsCloseProfile(LinearProfile);

  // sRGB profile
  printf("%s\n","sRGB profile");
  cmsHPROFILE sRGBProfile = cmsCreate_sRGBProfile();
  cmsMLUsetASCII(Description,  "en", "US", "sRGB");
  cmsMLUsetASCII(Model,    "en", "US", "LCMS2 sRGB");
  //~ cmsWriteTag(LinearProfile, cmsSigDeviceModelDescTag, Model);
  //~ cmsWriteTag(LinearProfile, cmsSigProfileDescriptionTag, Description);
  cmsSaveProfileToFile(sRGBProfile,
                       "Profiles/Output/sRGB.icc");
  cmsSaveProfileToFile(sRGBProfile,
                       "Profiles/Preview/sRGB.icc");
  cmsCloseProfile(sRGBProfile);

  // create darttable enhanced profiles
  create_darktable_profiles();
}


/*
    This file is part of darktable,
    copyright (c) 2009--2010 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * this is a collection of custom measured color matrices, profiled
 * for darktable (darktable.sf.net), so far all calculated by Pascal de Bruijn.
 */
typedef struct dt_profiled_colormatrix_t
{
  const char *makermodel;
  int rXYZ[3], gXYZ[3], bXYZ[3], white[3];
}
dt_profiled_colormatrix_t;

  // image submitter, chart type, illuminant, comments
static dt_profiled_colormatrix_t dt_profiled_colormatrices[] = {

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "PENTAX K-x",                   { 821548, 337357,  42923}, { 247818, 1042969, -218735}, { -4105, -293045, 1085129}, {792206, 821823, 668640}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "PENTAX K-7",                   { 738541, 294037,  28061}, { 316025,  984482, -189682}, { 12543, -185852, 1075027}, {812683, 843994, 682587}},

  // Pascal de Bruijn, Homebrew ColorChecker, strobe, well lit (this is not a joke)
  { "PENTAX 645D",                  { 814209, 295822,  76019}, { 194641, 1101898, -541473}, { 83664, -313370, 1450531}, {740036, 767288, 629959}},

  // Sven Lindahl, Wolf Faust IT8, direct sunlight, well lit
  { "Canon EOS-1Ds Mark II",        {1078033, 378601, -31113}, { -15396, 1112045, -245743}, {166794, -252411, 1284531}, {681213, 705048, 590790}},

  // Xavier Besse, CMP Digital Target 3, direct sunlight, well lit
  { "Canon EOS 5D Mark II",         { 967590, 399139,  36026}, { -52094,  819046, -232071}, {144455, -143158, 1069305}, {864227, 899139, 741547}},

  // Alberto Ferrante, Wolf Faust IT8, direct sunlight, well lit
  { "Canon EOS 7D",                 { 977829, 294815, -44205}, { 154175, 1238007, -325684}, {103363, -297791, 1397461}, {707291, 741760, 626251}},

  // Wim Koorenneef, Wolf Faust IT8, direct sunlight, well lit
  { "Canon EOS 20D",                { 885468, 342117,  20798}, { 278702, 1194733, -164246}, { 42389, -302963, 1147125}, {741379, 771881, 664261}},

  // Roy Niswanger, ColorChecker DC, direct sunlight, experimental
  { "Canon EOS 30D",                { 840195, 148773, -67017}, { 112915, 1104553, -369720}, {240005,  -19562, 1468338}, {827255, 873337, 715317}},

  // Pascal de Bruijn, CMP Digital Target 3, strobe, well lit
  { "Canon EOS 40D",                { 845901, 325760, -13077}, { 110809,  960724, -213577}, { 82230, -218063, 1110229}, {837906, 868393, 705704}},

  // Pascal de Bruijn, CMP Digital Target 3, strobe, well lit
  { "Canon EOS 50D",                {1035110, 365005,  -8057}, {-192184,  930511, -477417}, {189545, -233353, 1360870}, {863983, 888763, 730026}},

  // Pascal de Bruijn, CMP Digital Target 3, strobe, well lit
  { "Canon EOS 350D DIGITAL",       { 784348, 329681, -18875}, { 227249, 1001602, -115692}, { 23834, -270844, 1011185}, {861252, 886368, 721420}},
  { "Canon EOS DIGITAL REBEL XT",   { 784348, 329681, -18875}, { 227249, 1001602, -115692}, { 23834, -270844, 1011185}, {861252, 886368, 721420}},
  { "Canon EOS Kiss Digital N",     { 784348, 329681, -18875}, { 227249, 1001602, -115692}, { 23834, -270844, 1011185}, {861252, 886368, 721420}},

  // Pascal de Bruijn, CMP Digital Target 3, strobe, well lit
  { "Canon EOS 400D DIGITAL",       { 743546, 283783, -16647}, { 256531, 1035355, -117432}, { 36560, -256836, 1013535}, {855698, 880066, 726181}},
  { "Canon EOS DIGITAL REBEL XTi",  { 743546, 283783, -16647}, { 256531, 1035355, -117432}, { 36560, -256836, 1013535}, {855698, 880066, 726181}},
  { "Canon EOS Kiss Digital X",     { 743546, 283783, -16647}, { 256531, 1035355, -117432}, { 36560, -256836, 1013535}, {855698, 880066, 726181}},

  // Pascal de Bruijn, CMP Digital Target 3, strobe, well lit
  { "Canon EOS 450D",               { 960098, 404968,  22842}, { -85114,  855072, -310928}, {159851, -194611, 1164276}, {851379, 871506, 711823}},
  { "Canon EOS DIGITAL REBEL XSi",  { 960098, 404968,  22842}, { -85114,  855072, -310928}, {159851, -194611, 1164276}, {851379, 871506, 711823}},
  { "Canon EOS Kiss Digital X2",    { 960098, 404968,  22842}, { -85114,  855072, -310928}, {159851, -194611, 1164276}, {851379, 871506, 711823}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "Canon EOS REBEL T1i",          { 956711, 314590,   1236}, {  27405, 1158569, -346283}, { 95444, -376572, 1260895}, {870087, 898087, 734146}},
  { "Canon EOS 500D",               { 956711, 314590,   1236}, {  27405, 1158569, -346283}, { 95444, -376572, 1260895}, {870087, 898087, 734146}},
  { "Canon EOS Kiss Digital X3",    { 956711, 314590,   1236}, {  27405, 1158569, -346283}, { 95444, -376572, 1260895}, {870087, 898087, 734146}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "Canon EOS REBEL T2i",          { 864960, 319305,  36880}, { 160904, 1113586, -251587}, { 68832, -334290, 1143463}, {848404, 883118, 718628}},
  { "Canon EOS 550D",               { 864960, 319305,  36880}, { 160904, 1113586, -251587}, { 68832, -334290, 1143463}, {848404, 883118, 718628}},
  { "Canon EOS Kiss Digital X4",    { 864960, 319305,  36880}, { 160904, 1113586, -251587}, { 68832, -334290, 1143463}, {848404, 883118, 718628}},

  // Artis Rozentals, Wolf Faust IT8, direct sunlight, well lit
  { "Canon PowerShot S60",          { 879990, 321808,  23041}, { 272324, 1104752, -410950}, { 75500, -184097, 1373230}, {702026, 740524, 622131}},

  // Pascal de Bruijn, CMP Digital Target 3, camera strobe, well lit
  { "Canon PowerShot S90",          { 866531, 231995,  55756}, {  76965, 1067474, -461502}, {106369, -243286, 1314529}, {807449, 855270, 690750}},

  // Henrik Andersson, Homebrew ColorChecker, strobe, well lit
  { "NIKON D60",                    { 746475, 318924,   9277}, { 254776,  946991, -130447}, { 63171, -166458, 1029190}, {753220, 787949, 652695}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "NIKON D3000",                  { 778854, 333221,  21927}, { 292007, 1031448,  -88516}, { 27664, -245956,  997391}, {714828, 740387, 601334}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "NIKON D5000",                  { 852386, 356232,  42389}, { 205353, 1026688, -220184}, {  6348, -292526, 1083313}, {822647, 849106, 688538}},

  // Henrik Andersson, Homebrew ColorChecker, strobe, well lit
  { "NIKON D90",                    { 855072, 361176,  22751}, { 177414,  963577, -241501}, { 28931, -229019, 1123062}, {751816, 781677, 650024}},

  // Rolf Steinort, Wolf Faust IT8, direct sunlight, well lit
  { "NIKON D200",                   { 878922, 352966,   2914}, { 273575, 1048141, -116302}, { 61661, -171021, 1126297}, {691483, 727142, 615204}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "NIKON D300s",                  { 813202, 327667,  31067}, { 248810, 1047043, -203049}, { -1160, -284607, 1075790}, {774872, 800415, 648727}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "NIKON D700",                   { 789261, 332016,  34149}, { 270386,  985748, -129135}, {  4074, -230209,  999008}, {798172, 826721, 673126}},

  // Alexander Rabtchevich, Wolf Faust IT8, direct sunlight, well lit
  { "SONY DSLR-A200",               { 846786, 366302, -22858}, { 311584, 1046249, -107056}, { 54596, -192993, 1191406}, {708405, 744507, 596771}},

  // Stephane Chauveau, Wolf Faust IT8, direct sunlight, well lit
  { "SONY DSLR-A550",               {1031235, 405899,   1572}, { 185623, 1122162, -272659}, {-25528, -329514, 1249969}, {729797, 753586, 633530}},

  // Mark Haun, Wolf Faust IT8, direct sunlight, well lit
  { "OLYMPUS E-PL1",                { 824387, 288086,  -7355}, { 299500, 1148865, -308929}, { 91858, -198425, 1346603}, {720139, 750717, 619751}},

  // Henrik Andersson, Homebrew ColorChecker, camera strobe, well lit
  { "OLYMPUS SP570UZ",              { 780991, 262283,  27969}, { 147522, 1135239, -422974}, {142731, -293610, 1316803}, {769669, 804474, 676895}},

  // Robert Park, ColorChecker Passport, camera strobe, well lit
  { "Panasonic DMC-FZ40",           { 833542, 259720,  35721}, { 129517, 1239594, -525848}, {117340, -405273, 1440384}, {825226, 863846, 688431}},

  // Robert Park, ColorChecker Passport, strobe, well lit
  { "Panasonic DMC-G1",             { 747467, 300064,  74265}, { 225922, 1028946, -310913}, { 91782, -229019, 1153793}, {846222, 864502, 694458}},

  // Pieter de Boer, CMP Digital Target 3, camera strobe, well lit
  { "KODAK EASYSHARE Z1015 IS",     { 716446, 157928, -39536}, { 288498, 1234573, -412460}, { 43045, -337677, 1385773}, {774048, 823563, 644012}}
};

static const int dt_profiled_colormatrix_cnt = sizeof(dt_profiled_colormatrices)/sizeof(dt_profiled_colormatrix_t);

void create_darktable_profiles() {
  dt_profiled_colormatrix_t *preset = NULL;
  cmsMLU *Description, *Model, *Mfg;
  Description = cmsMLUalloc(NULL, 1);
  Model = cmsMLUalloc(NULL, 1);
  Mfg = cmsMLUalloc(NULL, 1);

  for(int k=0;k<dt_profiled_colormatrix_cnt;k++)
  {
    preset = dt_profiled_colormatrices + k;

    const float wxyz = preset->white[0]+preset->white[1]+preset->white[2];
    const float rxyz = preset->rXYZ[0] +preset->rXYZ[1] +preset->rXYZ[2];
    const float gxyz = preset->gXYZ[0] +preset->gXYZ[1] +preset->gXYZ[2];
    const float bxyz = preset->bXYZ[0] +preset->bXYZ[1] +preset->bXYZ[2];
    cmsCIExyY       WP = {preset->white[0]/wxyz, preset->white[1]/wxyz, 1.0};
    cmsCIExyYTRIPLE XYZPrimaries   = {
                                     {preset->rXYZ[0]/rxyz, preset->rXYZ[1]/rxyz, 1.0},
                                     {preset->gXYZ[0]/gxyz, preset->gXYZ[1]/gxyz, 1.0},
                                     {preset->bXYZ[0]/bxyz, preset->bXYZ[1]/bxyz, 1.0}
                                     };
    cmsHPROFILE  hp;

    cmsToneCurve* Gamma = cmsBuildGamma(NULL, 1.0);
    cmsToneCurve* Gamma3[3];
    Gamma3[0] = Gamma3[1] = Gamma3[2] = Gamma;

    hp = cmsCreateRGBProfile(&WP, &XYZPrimaries, Gamma3);
    cmsFreeToneCurve(Gamma);

    char name[512];
    snprintf(name, 512, "Darktable profiled %s", preset->makermodel);

    cmsMLUsetASCII(Description,  "en", "US", "(Photivo -> dt enhanced)");
    cmsMLUsetASCII(Description,  "en", "US", name);
    cmsMLUsetASCII(Model,    "en", "US", name);
    //~ cmsWriteTag(hp, cmsSigDeviceMfgDescTag, Mfg);
    //~ cmsWriteTag(hp, cmsSigDeviceModelDescTag, Model);
    //~ cmsWriteTag(hp, cmsSigProfileDescriptionTag, Description);

    char OutputFileName[1024];
    snprintf(OutputFileName,1023,"Profiles/Camera/Enhanced/%s.icc",
             preset->makermodel);
    for (unsigned i=0;i<strlen(OutputFileName);i++) {
      if (isspace(OutputFileName[i])) OutputFileName[i]='_';
      // Windows has issue with '*'
      if (OutputFileName[i]=='*') OutputFileName[i]='_';
    }
    cmsSaveProfileToFile(hp,OutputFileName);
    cmsCloseProfile(hp);
  }
}
