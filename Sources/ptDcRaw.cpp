/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009-2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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
/*
** This is basically the translation into a more or less C++ object of
** dcraw.c -- Dave Coffin's raw photo decoder
** Copyright 1997-2008 by Dave Coffin, dcoffin a cybercom o net
**
*******************************************************************************/

#include "ptDcRaw.h"

#include <cassert>

#include "ptDefines.h"
#include "ptError.h"
#include "ptConstants.h"
#include "ptCalloc.h"

#define NO_JASPER
#ifndef NO_JASPER
#include <jasper/jasper.h>
#endif

// Macro fix for explicit fread returnvalue check.
#define ptfread(ptr,size,n,stream)              \
{                                               \
size_t RV = fread(ptr,size,n,stream);           \
if (RV != (size_t) n) assert(!ferror(stream));  \
}
#define ptfwrite(ptr,size,n,stream)    \
{                                      \
size_t RV = fwrite(ptr,size,n,stream); \
assert(RV == (size_t) n);              \
}
#define ptfscanf(file,format,arg)      \
{                                      \
int RV = fscanf(file,format,arg);      \
assert (RV == 1);                      \
}
#define ptfgets(str,num,file)   \
{                               \
char* RV = fgets(str,num,file); \
assert (RV);                    \
}

inline void VAppend(std::vector<char> &AVector, char* AArray, const int ALength) {
  char* hEnd = AArray + ALength * sizeof(char);
  AVector.insert(AVector.end(), AArray, hEnd);
}

// The class.
#define CLASS ptDcRaw::
CLASS ptDcRaw() {
  //printf("(%s,%d) '%s'\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);

  // This were the original global variables initialized.
  // Now moved into constructor.
  // All m_UserSetting* are obviously ,uh, usersettings
  // that were done via the command line parameters.
  m_UserSetting_ShotSelect=0;
  m_UserSetting_MaxMultiplier=0;
  m_UserSetting_Multiplier[0]=0;
  m_UserSetting_Multiplier[1]=0;
  m_UserSetting_Multiplier[2]=0;
  m_UserSetting_Multiplier[3]=0;
  m_UserSetting_HalfSize=0;
  m_UserSetting_HotpixelReduction=0;
  m_UserSetting_BayerDenoise=0;
  m_UserSetting_CfaLineDn=0;
  m_UserSetting_GreenEquil=0;
  m_UserSetting_CaCorrect=0;
  m_UserSetting_CaRed=0;
  m_UserSetting_CaBlue=0;
  m_UserSetting_AutoWb=0;
  m_UserSetting_CameraWb=0;
  m_UserSetting_CameraMatrix=-1;
  m_UserSetting_GreyBox[0] = 0;
  m_UserSetting_GreyBox[1] = 0;
  m_UserSetting_GreyBox[2] = 0xFFFF;
  m_UserSetting_GreyBox[3] = 0xFFFF;
  m_UserSetting_photivo_ClipMode = ptClipMode_Clip;
  m_UserSetting_photivo_ClipParameter = 0;
  m_UserSetting_Quality = 3;
  m_UserSetting_BlackPoint = -1;
  m_UserSetting_Saturation = -1;
  m_UserSetting_InputFileName       = NULL;
  m_UserSetting_DetailView          = 0;
  m_UserSetting_DetailViewCropX     = 0;
  m_UserSetting_DetailViewCropY     = 0;
  m_UserSetting_DetailViewCropW     = 0;
  m_UserSetting_DetailViewCropH     = 0;
  m_UserSetting_BadPixelsFileName   = NULL;
  m_UserSetting_DarkFrameFileName   = NULL;
  m_UserSetting_AdjustMaximum       = 0;
  m_UserSetting_DenoiseThreshold    = 0;
  m_UserSetting_InterpolationPasses = 0;
  m_UserSetting_MedianPasses        = 0;
  m_UserSetting_ESMedianPasses      = 0;


  // Safety settings to have NULL on uninitialized images.
  m_Image = NULL;
  m_Image_AfterPhase1 = NULL;
  m_Image_AfterPhase2 = NULL;
  m_Image_AfterPhase3 = NULL;
  m_Image_AfterPhase4 = NULL;

  // Some other pointers that are in a dynamic environment better NULL.
  m_MetaData    = NULL;
  m_InputFile   = NULL;

  m_Thumb.clear();
  ResetNonUserSettings();
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor
// Deallocate everything dynamic.
//
////////////////////////////////////////////////////////////////////////////////

CLASS ~ptDcRaw() {

//printf("(%s,%d) '%s'\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);

  FREE(m_UserSetting_InputFileName);
  FREE(m_UserSetting_BadPixelsFileName);
  FREE(m_UserSetting_DarkFrameFileName);
  FREE(m_Image);
  FREE(m_Image_AfterPhase1);
  FREE(m_Image_AfterPhase2);
  FREE(m_Image_AfterPhase3);
  FCLOSE(m_InputFile);
  FREE(m_MetaData);
}

////////////////////////////////////////////////////////////////////////////////
//
// ResetNonUserSettings
// Reset all variables except user settings.
// This is for second entry support.
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ResetNonUserSettings() {
  // Safety settings to have NULL on uninitialized images.
  // And freeing the underlying memory (which was long time a leak !)
  // FREE(NULL) is safe, so the beginsituation is fine too.
  // FREE implies setting of the pointer to NULL
  FREE(m_Image);
  FREE(m_Image_AfterPhase1);
  FREE(m_Image_AfterPhase2);
  FREE(m_Image_AfterPhase3);
  FREE(m_Image_AfterPhase4);

  // Some other pointers that are in a dynamic environment better NULL.
  // Same remarks as above.
  FREE(m_MetaData);
  FCLOSE(m_InputFile);

  // This was originally in the identify code, but which is called
  // anyway in the beginning. So this is simply global initialization like
  // anything else.

  m_Tiff_Flip = m_Flip = -1; /* 0 is valid, so -1 is unknown */
  m_Filters = (unsigned)(-1); // hack not to change dcraw.
  m_RawHeight = m_RawWidth = m_Fuji_Width = m_IsFuji = fuji_layout = cr2_slice[0] = 0;
  m_WhiteLevel = m_Height = m_Width = m_TopMargin = m_LeftMargin = 0;
  m_ColorDescriptor[0] = m_Description[0] = m_Artist[0] = m_CameraMake[0] = m_CameraModel[0] = m_CameraModelBis[0] = 0;
  m_IsoSpeed = m_Shutter = m_Aperture = m_FocalLength = unique_id = 0;
  m_Tiff_NrIFDs = 0;
  memset (m_Tiff_IFD, 0, sizeof m_Tiff_IFD);
  memset (white, 0, sizeof white);
  m_ThumbOffset = m_ThumbLength = m_ThumbWidth = m_ThumbHeight = 0;
  m_LoadRawFunction = m_ThumbLoadRawFunction = 0;
  m_WriteThumb = &CLASS jpeg_thumb;
  m_Data_Offset = m_MetaLength = m_Tiff_bps = m_Tiff_Compress = 0;
  m_Kodak_cbpp = zero_after_ff = m_DNG_Version = m_Load_Flags = 0;
  m_TimeStamp = m_ShotOrder = m_Tiff_Samples = m_BlackLevel = m_IsFoveon = 0;
  for (int k=0; k<8; k++) m_CBlackLevel[k] = 0;
  m_MixGreen = m_ProfileLength = data_error = m_ZeroIsBad = 0;
  m_PixelAspect = m_IsRaw = m_RawColor = 1; m_RawColorPhotivo = 0;
  m_TileWidth = m_TileLength = 0;
  m_Raw_Image = 0;
  memset (m_Mask, 0, sizeof m_Mask);
  for (int i=0; i < 4; i++) {
    short c;
    ASSIGN(m_CameraMultipliers[i], i == 1);
    ASSIGN(m_PreMultipliers[i], i < 3);
    ASSIGN(m_D65Multipliers[i], i < 3);
    for (c=0; c<3; c++) m_cmatrix[c][i] = 0;
    for (c=0; c<3; c++) m_MatrixCamRGBToSRGB[c][i] = c == i;
  }
  m_Colors = 3;
  for (int i=0; i < 0x10000; i++) m_Curve[i] = i;

  m_Gamma[0] = 0.45;
  m_Gamma[1] = 4.50;
  m_Gamma[2] = 0;
  m_Gamma[3] = 0;
  m_Gamma[4] = 0;
  m_Gamma[5] = 0;

  m_getbithuff_bitbuf=0;
  m_getbithuff_reset=0;
  m_getbithuff_vbits=0;
  m_ph1_bithuffbitbuf=0;
  m_ph1_bithuffvbits=0;
  for (int i = 0; i < 0x4000; i++) m_pana_bits_buf[i] = 0;
  m_pana_bits_vbits = 0;
  for (int i = 0; i < 4096; i++) jpeg_buffer[i] = 0;
  for (int i = 0; i < 128; i++) m_sony_decrypt_pad[i] = 0;
  m_sony_decrypt_p = 0;
  for (int i = 0; i < 1024; i++) m_foveon_decoder_huff[i] = 0;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      MatrixXYZToCam[i][j] = 0.0;
    }
  }

  ToCamFunctionInited = 0;
  for (int i = 0; i < 0x20000; i++) ToLABFunctionTable[i] = 0.0;
  ToLABFunctionInited = 0;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      MatrixCamToXYZ[i][j] = 0.0;
    }
  }

}

/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Do not use the FC or BAYER macros with the Leaf CatchLight,
   because its pattern is 16x16, not 2x8.

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2

  PowerShot 600 PowerShot A50 PowerShot Pro70 Pro90 & G1
  0xe1e4e1e4: 0x1b4e4b1e: 0x1e4b4e1b: 0xb4b4b4b4:

    0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5
  0 G M G M G M 0 C Y C Y C Y 0 Y C Y C Y C 0 G M G M G M
  1 C Y C Y C Y 1 M G M G M G 1 M G M G M G 1 Y C Y C Y C
  2 M G M G M G 2 Y C Y C Y C 2 C Y C Y C Y
  3 C Y C Y C Y 3 G M G M G M 3 G M G M G M
      4 C Y C Y C Y 4 Y C Y C Y C
  PowerShot A5  5 G M G M G M 5 G M G M G M
  0x1e4e1e4e: 6 Y C Y C Y C 6 C Y C Y C Y
      7 M G M G M G 7 M G M G M G
    0 1 2 3 4 5
  0 C Y C Y C Y
  1 G M G M G M
  2 C Y C Y C Y
  3 M G M G M G

   All RGB cameras use one of these Bayer grids:

  0x16161616: 0x61616161: 0x49494949: 0x94949494:

    0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5
  0 B G B G B G 0 G R G R G R 0 G B G B G B 0 R G R G R G
  1 G R G R G R 1 B G B G B G 1 R G R G R G 1 G B G B G B
  2 B G B G B G 2 G R G R G R 2 G B G B G B 2 R G R G R G
  3 G R G R G R 3 B G B G B G 3 R G R G R G 3 G B G B G B
 */

#define RAW(row,col) \
  m_Raw_Image[(row)*m_RawWidth+(col)]

#define FC(row,col) \
  (m_Filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define BAYER(row,col) \
  m_Image[(row)*m_Width + (col)][FC(row,col)]
  // old: m_Image[((row) >> m_Shrink)*m_OutWidth + ((col) >> m_Shrink)][FC(row,col)]

#define BAYER2(row,col) \
  m_Image[(row)*m_Width + (col)][fcol(row,col)]

int CLASS fcol (int row, int col)
{
  static const char filter[16][16] =
  { { 2,1,1,3,2,3,2,0,3,2,3,0,1,2,1,0 },
    { 0,3,0,2,0,1,3,1,0,1,1,2,0,3,3,2 },
    { 2,3,3,2,3,1,1,3,3,1,2,1,2,0,0,3 },
    { 0,1,0,1,0,2,0,2,2,0,3,0,1,3,2,1 },
    { 3,1,1,2,0,1,0,2,1,3,1,3,0,1,3,0 },
    { 2,0,0,3,3,2,3,1,2,0,2,0,3,2,2,1 },
    { 2,3,3,1,2,1,2,1,2,1,1,2,3,0,0,1 },
    { 1,0,0,2,3,0,0,3,0,3,0,3,2,1,2,3 },
    { 2,3,3,1,1,2,1,0,3,2,3,0,2,3,1,3 },
    { 1,0,2,0,3,0,3,2,0,1,1,2,0,1,0,2 },
    { 0,1,1,3,3,2,2,1,1,3,3,0,2,1,3,2 },
    { 2,3,2,0,0,1,3,0,2,0,1,2,3,0,1,0 },
    { 1,3,1,2,3,2,3,2,0,2,0,1,1,0,3,0 },
    { 0,2,0,3,1,0,0,1,1,3,3,2,3,2,2,1 },
    { 2,1,3,2,3,1,2,1,0,3,0,2,0,2,0,2 },
    { 0,3,1,0,0,2,0,3,2,1,3,1,1,3,1,3 } };
  static const char filter2[6][6] =
  { { 1,1,0,1,1,2 },
    { 1,1,2,1,1,0 },
    { 2,0,1,0,2,1 },
    { 1,1,2,1,1,0 },
    { 1,1,0,1,1,2 },
    { 0,2,1,2,0,1 } };

  if (m_Filters == 1) return filter[(row+m_TopMargin)&15][(col+m_LeftMargin)&15];
  if (m_Filters == 2) return filter2[(row+6) % 6][(col+6) % 6];
  return FC(row,col);
}

#ifndef __GLIBC__
char* CLASS my_memmem (char *haystack, size_t haystacklen,
        char *neepte, size_t neeptelen)
{
  char *c;
  for (c = haystack; c <= haystack + haystacklen - neeptelen; c++)
    if (!memcmp (c, neepte, neeptelen))
      return c;
  return 0;
}
#define memmem my_memmem
#endif

float rgb_cam[3][4];
const double xyz_rgb[3][3] = {      /* XYZ from RGB */
  { 0.412453, 0.357580, 0.180423 },
  { 0.212671, 0.715160, 0.072169 },
  { 0.019334, 0.119193, 0.950227 } };
const float d65_white[3] = { 0.950456, 1, 1.088754 };

// Now everything importent is set up, so we can include external demosaicers
#include "dcb/dcb_demosaicing.c"
#include "dcb/dcb_demosaicing_old.c"
#include "vcd/ahd_interpolate_mod.c"
#include "vcd/ahd_partial_interpolate.c"
#include "vcd/refinement.c"
#include "vcd/vcd_interpolate.c"
#include "vcd/es_median_filter.c"
#include "vcd/median_filter_new.c"
#include "perfectraw/lmmse_interpolate.c"
#include "rawtherapee/amaze_interpolate.c"
#include "rawtherapee/cfa_line_dn.c"
#include "rawtherapee/ca_correct.c"
#include "rawtherapee/green_equil.c"

void CLASS merror (void *ptr, const char *where)
{
  if (ptr) return;
  fprintf (stderr,_("%s: Out of memory in %s\n"), m_UserSetting_InputFileName, where);
  longjmp (m_Failure, 1);
}

void CLASS derror()
{
  if (!data_error) {
    fprintf (stderr, "%s: ", m_UserSetting_InputFileName);
    if (feof(m_InputFile))
      fprintf (stderr,_("Unexpected end of file\n"));
    else
      //fprintf (stderr,_("Corrupt data near 0x%lx\n"), (int64_t) ftell(m_InputFile));
      fprintf (stderr,_("Corrupt data near 0x%lx\n"), (long unsigned int) ftell(m_InputFile));
  }
  data_error++;
}

uint16_t CLASS sget2 (uint8_t *s)
{
  if (m_ByteOrder == 0x4949)    /* "II" means little-endian */
    return s[0] | s[1] << 8;
  else        /* "MM" means big-endian */
    return s[0] << 8 | s[1];
}

uint16_t CLASS get2()
{
  uint8_t str[2] = { 0xff,0xff };
  ptfread (str, 1, 2, m_InputFile);
  return sget2(str);
}

unsigned CLASS sget4 (uint8_t *s)
{
  if (m_ByteOrder == 0x4949)
    return s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
  else
    return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}
#define sget4(s) sget4((uint8_t *)s)

unsigned CLASS get4()
{
  uint8_t str[4] = { 0xff,0xff,0xff,0xff };
  ptfread (str, 1, 4, m_InputFile);
  return sget4(str);
}

unsigned CLASS getint (int type)
{
  return type == 3 ? get2() : get4();
}

float CLASS int_to_float (int i)
{
  union { int i; float f; } u;
  u.i = i;
  return u.f;
}

double CLASS getreal (int type)
{
  union { char c[8]; double d; } u;
  int i, rev;

  switch (type) {
    case 3: return (unsigned short) get2();
    case 4: return (unsigned int) get4();
    case 5:  u.d = (unsigned int) get4();
      return u.d / (unsigned int) get4();
    case 8: return (signed short) get2();
    case 9: return (signed int) get4();
    case 10: u.d = (signed int) get4();
      return u.d / (signed int) get4();
    case 11: return int_to_float (get4());
    case 12:
      rev = 7 * ((m_ByteOrder == 0x4949) == (ntohs(0x1234) == 0x1234));
      for (i=0; i < 8; i++)
  u.c[i ^ rev] = fgetc(m_InputFile);
      return u.d;
    default: return fgetc(m_InputFile);
  }
}

void CLASS read_shorts (uint16_t *pixel, int count)
{
  if (fread (pixel, 2, count, m_InputFile) < (size_t) count) derror();
  if ((m_ByteOrder == 0x4949) == (ntohs(0x1234) == 0x1234))
    swab ((char *)pixel, (char *)pixel, count*2);
}

/* -> 1438
void CLASS canon_black (double dark[2], int nblack)
{
  int c, diff, row, col;

  if (!nblack) return;
  for (c=0; c<2; c++) dark[c] /= nblack >> 1;
  if ((diff = (int)(dark[0] - dark[1])))
    for (row=0; row < m_Height; row++)
      for (col=1; col < m_Width; col+=2)
  BAYER(row,col) += diff;
  dark[1] += diff;
  m_BlackLevel = (unsigned)((dark[0] + dark[1] + 1) / 2);
}
*/

void CLASS canon_600_fixed_wb (int temp)
{
  static const short mul[4][5] = {
    {  667, 358,397,565,452 },
    {  731, 390,367,499,517 },
    { 1119, 396,348,448,537 },
    { 1399, 485,431,508,688 } };
  int lo, hi, i;
  float frac=0;

  for (lo=4; --lo; )
    if (*mul[lo] <= temp) break;
  for (hi=0; hi < 3; hi++)
    if (*mul[hi] >= temp) break;
  if (lo != hi)
    frac = (float) (temp - *mul[lo]) / (*mul[hi] - *mul[lo]);
  for (i=1; i < 5; i++)
    ASSIGN(m_D65Multipliers[i-1],1/(frac * mul[hi][i] + (1-frac) * mul[lo][i]));
}

/* Return values:  0 = white  1 = near white  2 = not white */
int CLASS canon_600_color (int ratio[2], int mar)
{
  int clipped=0, target, miss;

  if (flash_used) {
    if (ratio[1] < -104)
      { ratio[1] = -104; clipped = 1; }
    if (ratio[1] >   12)
      { ratio[1] =   12; clipped = 1; }
  } else {
    if (ratio[1] < -264 || ratio[1] > 461) return 2;
    if (ratio[1] < -50)
      { ratio[1] = -50; clipped = 1; }
    if (ratio[1] > 307)
      { ratio[1] = 307; clipped = 1; }
  }
  target = flash_used || ratio[1] < 197
  ? -38 - (398 * ratio[1] >> 10)
  : -123 + (48 * ratio[1] >> 10);
  if (target - mar <= ratio[0] &&
      target + 20  >= ratio[0] && !clipped) return 0;
  miss = target - ratio[0];
  if (abs(miss) >= mar*4) return 2;
  if (miss < -20) miss = -20;
  if (miss > mar) miss = mar;
  ratio[0] = target - miss;
  return 1;
}

void CLASS canon_600_auto_wb()
{
  int mar, row, col, i, j, st, count[] = { 0,0 };
  int test[8], total[2][8], ratio[2][2], stat[2];

  memset (&total, 0, sizeof total);
  i = int ( canon_ev + 0.5 );
  if      (i < 10) mar = 150;
  else if (i > 12) mar = 20;
  else mar = 280 - 20 * i;
  if (flash_used) mar = 80;
  for (row=14; row < m_Height-14; row+=4)
    for (col=10; col < m_Width; col+=2) {
      for (i=0; i < 8; i++)
  test[(i & 4) + FC(row+(i >> 1),col+(i & 1))] =
        BAYER(row+(i >> 1),col+(i & 1));
      for (i=0; i < 8; i++)
  if (test[i] < 150 || test[i] > 1500) goto next;
      for (i=0; i < 4; i++)
  if (abs(test[i] - test[i+4]) > 50) goto next;
      for (i=0; i < 2; i++) {
  for (j=0; j < 4; j+=2)
    ratio[i][j >> 1] = ((test[i*4+j+1]-test[i*4+j]) << 10) / test[i*4+j];
  stat[i] = canon_600_color (ratio[i], mar);
      }
      if ((st = stat[0] | stat[1]) > 1) goto next;
      for (i=0; i < 2; i++)
  if (stat[i])
    for (j=0; j < 2; j++)
      test[i*4+j*2+1] = test[i*4+j*2] * (0x400 + ratio[i][j]) >> 10;
      for (i=0; i < 8; i++)
  total[st][i] += test[i];
      count[st]++;
next: ;
    }
  if (count[0] | count[1]) {
    st = count[0]*200 < count[1];
    for (i=0; i < 4; i++)
      ASSIGN(m_D65Multipliers[i], 1.0 / (total[st][i] + total[st][i+4]));
  }
}

void CLASS canon_600_coeff()
{
  static const short table[6][12] = {
    { -190,702,-1878,2390,   1861,-1349,905,-393, -432,944,2617,-2105  },
    { -1203,1715,-1136,1648, 1388,-876,267,245,  -1641,2153,3921,-3409 },
    { -615,1127,-1563,2075,  1437,-925,509,3,     -756,1268,2519,-2007 },
    { -190,702,-1886,2398,   2153,-1641,763,-251, -452,964,3040,-2528  },
    { -190,702,-1878,2390,   1861,-1349,905,-393, -432,944,2617,-2105  },
    { -807,1319,-1785,2297,  1388,-876,769,-257,  -230,742,2067,-1555  } };
  int t=0, i, c;
  float mc, yc;

  mc = VALUE(m_D65Multipliers[1]) / VALUE(m_D65Multipliers[2]);
  yc = VALUE(m_D65Multipliers[3]) / VALUE(m_D65Multipliers[2]);
  if (mc > 1 && mc <= 1.28 && yc < 0.8789) t=1;
  if (mc > 1.28 && mc <= 2) {
    if  (yc < 0.8789) t=3;
    else if (yc <= 2) t=4;
  }
  if (flash_used) t=5;
  for (m_RawColor = i=0; i < 3; i++)
    for (c=0; c < m_Colors; c++)
      m_MatrixCamRGBToSRGB[i][c] = table[t][i*4 + c] / 1024.0;
}

void CLASS canon_600_load_raw()
{
  uint8_t  data[1120], *dp;
  uint16_t *pix;
  int irow, row;

  for (irow=row=0; irow < m_Height; irow++) {
    if (fread (data, 1, 1120, m_InputFile) < 1120) derror();
    pix = m_Raw_Image + row*m_RawWidth;
    for (dp=data; dp < data+1120;  dp+=10, pix+=8) {
      pix[0] = (dp[0] << 2) + (dp[1] >> 6    );
      pix[1] = (dp[2] << 2) + (dp[1] >> 4 & 3);
      pix[2] = (dp[3] << 2) + (dp[1] >> 2 & 3);
      pix[3] = (dp[4] << 2) + (dp[1]      & 3);
      pix[4] = (dp[5] << 2) + (dp[9]      & 3);
      pix[5] = (dp[6] << 2) + (dp[9] >> 2 & 3);
      pix[6] = (dp[7] << 2) + (dp[9] >> 4 & 3);
      pix[7] = (dp[8] << 2) + (dp[9] >> 6    );
    }
    if ((row+=2) > m_Height) row = 1;
  }
}

void CLASS canon_600_correct()
{
  int row, col, val;
  static const short mul[4][2] =
  { { 1141,1145 }, { 1128,1109 }, { 1178,1149 }, { 1128,1109 } };

  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++) {
      if ((val = BAYER(row,col) - m_BlackLevel) < 0) val = 0;
      val = val * mul[row & 3][col & 1] >> 9;
      BAYER(row,col) = val;
    }
  canon_600_fixed_wb(1311);
  canon_600_auto_wb();
  canon_600_coeff();
  m_WhiteLevel = (0x3ff - m_BlackLevel) * 1109 >> 9;
  m_BlackLevel = 0;
}

int CLASS canon_s2is()
{
  unsigned row;

  for (row=0; row < 100; row++) {
    fseek (m_InputFile, row*3340 + 3284, SEEK_SET);
    if (getc(m_InputFile) > 15) return 1;
  }
  return 0;
}

/*
   getbits(-1) initializes the buffer
   getbits(n) where 0 <= n <= 25 returns an n-bit integer
 */
unsigned CLASS getbithuff(int nbits,uint16_t *huff)
{
  // unsigned c;
  int c;

  if (nbits == -1)
    return m_getbithuff_bitbuf = m_getbithuff_vbits = m_getbithuff_reset = 0;
  if (nbits == 0 || m_getbithuff_vbits < 0) return 0;
  while (!m_getbithuff_reset && m_getbithuff_vbits < nbits && (c = fgetc(m_InputFile)) != EOF &&
    !(m_getbithuff_reset = zero_after_ff && c == 0xff && fgetc(m_InputFile))) {
    m_getbithuff_bitbuf = (m_getbithuff_bitbuf << 8) + (uint8_t) c;
    m_getbithuff_vbits += 8;
  }
  c = m_getbithuff_bitbuf << (32-m_getbithuff_vbits) >> (32-nbits);
  if (huff) {
    m_getbithuff_vbits -= huff[c] >> 8;
    c = (uint8_t) huff[c];
  } else
    m_getbithuff_vbits -= nbits;
  if (m_getbithuff_vbits < 0) derror();
  return c;
}

#define getbits(n) getbithuff(n,0)
#define gethuff(h) getbithuff(*h,h+1)

/*
   Construct a decode tree according the specification in *source.
   The first 16 bytes specify how many codes should be 1-bit, 2-bit
   3-bit, etc.  Bytes after that are the leaf values.

   For example, if the source is

    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

   then the code is

  00    0x04
  010   0x03
  011   0x05
  100   0x06
  101   0x02
  1100    0x07
  1101    0x01
  11100   0x08
  11101   0x09
  11110   0x00
  111110    0x0a
  1111110   0x0b
  1111111   0xff
 */
uint16_t * CLASS make_decoder_ref (const uint8_t **source)
{
  int max, len, h, i, j;
  const uint8_t *count;
  uint16_t *huff;

  count = (*source += 16) - 17;
  for (max=16; max && !count[max]; max--) {} ;
  huff = (uint16_t *) CALLOC (1 + (1 << max), sizeof *huff);
  merror (huff, "make_decoder()");
  huff[0] = max;
  for (h=len=1; len <= max; len++)
    for (i=0; i < count[len]; i++, ++*source)
      for (j=0; j < 1 << (max-len); j++)
  if (h <= 1 << max)
    huff[h++] = len << 8 | **source;
  return huff;
}


uint16_t * CLASS make_decoder (const uint8_t *source)
{
  return make_decoder_ref (&source);
}

void CLASS crw_init_tables (unsigned table, uint16_t *huff[2])
{
  static const uint8_t first_tree[3][29] = {
    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },
    { 0,2,2,3,1,1,1,1,2,0,0,0,0,0,0,0,
      0x03,0x02,0x04,0x01,0x05,0x00,0x06,0x07,0x09,0x08,0x0a,0x0b,0xff  },
    { 0,0,6,3,1,1,2,0,0,0,0,0,0,0,0,0,
      0x06,0x05,0x07,0x04,0x08,0x03,0x09,0x02,0x00,0x0a,0x01,0x0b,0xff  },
  };
  static const uint8_t second_tree[3][180] = {
    { 0,2,2,2,1,4,2,1,2,5,1,1,0,0,0,139,
      0x03,0x04,0x02,0x05,0x01,0x06,0x07,0x08,
      0x12,0x13,0x11,0x14,0x09,0x15,0x22,0x00,0x21,0x16,0x0a,0xf0,
      0x23,0x17,0x24,0x31,0x32,0x18,0x19,0x33,0x25,0x41,0x34,0x42,
      0x35,0x51,0x36,0x37,0x38,0x29,0x79,0x26,0x1a,0x39,0x56,0x57,
      0x28,0x27,0x52,0x55,0x58,0x43,0x76,0x59,0x77,0x54,0x61,0xf9,
      0x71,0x78,0x75,0x96,0x97,0x49,0xb7,0x53,0xd7,0x74,0xb6,0x98,
      0x47,0x48,0x95,0x69,0x99,0x91,0xfa,0xb8,0x68,0xb5,0xb9,0xd6,
      0xf7,0xd8,0x67,0x46,0x45,0x94,0x89,0xf8,0x81,0xd5,0xf6,0xb4,
      0x88,0xb1,0x2a,0x44,0x72,0xd9,0x87,0x66,0xd4,0xf5,0x3a,0xa7,
      0x73,0xa9,0xa8,0x86,0x62,0xc7,0x65,0xc8,0xc9,0xa1,0xf4,0xd1,
      0xe9,0x5a,0x92,0x85,0xa6,0xe7,0x93,0xe8,0xc1,0xc6,0x7a,0x64,
      0xe1,0x4a,0x6a,0xe6,0xb3,0xf1,0xd3,0xa5,0x8a,0xb2,0x9a,0xba,
      0x84,0xa4,0x63,0xe5,0xc5,0xf3,0xd2,0xc4,0x82,0xaa,0xda,0xe4,
      0xf2,0xca,0x83,0xa3,0xa2,0xc3,0xea,0xc2,0xe2,0xe3,0xff,0xff  },
    { 0,2,2,1,4,1,4,1,3,3,1,0,0,0,0,140,
      0x02,0x03,0x01,0x04,0x05,0x12,0x11,0x06,
      0x13,0x07,0x08,0x14,0x22,0x09,0x21,0x00,0x23,0x15,0x31,0x32,
      0x0a,0x16,0xf0,0x24,0x33,0x41,0x42,0x19,0x17,0x25,0x18,0x51,
      0x34,0x43,0x52,0x29,0x35,0x61,0x39,0x71,0x62,0x36,0x53,0x26,
      0x38,0x1a,0x37,0x81,0x27,0x91,0x79,0x55,0x45,0x28,0x72,0x59,
      0xa1,0xb1,0x44,0x69,0x54,0x58,0xd1,0xfa,0x57,0xe1,0xf1,0xb9,
      0x49,0x47,0x63,0x6a,0xf9,0x56,0x46,0xa8,0x2a,0x4a,0x78,0x99,
      0x3a,0x75,0x74,0x86,0x65,0xc1,0x76,0xb6,0x96,0xd6,0x89,0x85,
      0xc9,0xf5,0x95,0xb4,0xc7,0xf7,0x8a,0x97,0xb8,0x73,0xb7,0xd8,
      0xd9,0x87,0xa7,0x7a,0x48,0x82,0x84,0xea,0xf4,0xa6,0xc5,0x5a,
      0x94,0xa4,0xc6,0x92,0xc3,0x68,0xb5,0xc8,0xe4,0xe5,0xe6,0xe9,
      0xa2,0xa3,0xe3,0xc2,0x66,0x67,0x93,0xaa,0xd4,0xd5,0xe7,0xf8,
      0x88,0x9a,0xd7,0x77,0xc4,0x64,0xe2,0x98,0xa5,0xca,0xda,0xe8,
      0xf3,0xf6,0xa9,0xb2,0xb3,0xf2,0xd2,0x83,0xba,0xd3,0xff,0xff  },
    { 0,0,6,2,1,3,3,2,5,1,2,2,8,10,0,117,
      0x04,0x05,0x03,0x06,0x02,0x07,0x01,0x08,
      0x09,0x12,0x13,0x14,0x11,0x15,0x0a,0x16,0x17,0xf0,0x00,0x22,
      0x21,0x18,0x23,0x19,0x24,0x32,0x31,0x25,0x33,0x38,0x37,0x34,
      0x35,0x36,0x39,0x79,0x57,0x58,0x59,0x28,0x56,0x78,0x27,0x41,
      0x29,0x77,0x26,0x42,0x76,0x99,0x1a,0x55,0x98,0x97,0xf9,0x48,
      0x54,0x96,0x89,0x47,0xb7,0x49,0xfa,0x75,0x68,0xb6,0x67,0x69,
      0xb9,0xb8,0xd8,0x52,0xd7,0x88,0xb5,0x74,0x51,0x46,0xd9,0xf8,
      0x3a,0xd6,0x87,0x45,0x7a,0x95,0xd5,0xf6,0x86,0xb4,0xa9,0x94,
      0x53,0x2a,0xa8,0x43,0xf5,0xf7,0xd4,0x66,0xa7,0x5a,0x44,0x8a,
      0xc9,0xe8,0xc8,0xe7,0x9a,0x6a,0x73,0x4a,0x61,0xc7,0xf4,0xc6,
      0x65,0xe9,0x72,0xe6,0x71,0x91,0x93,0xa6,0xda,0x92,0x85,0x62,
      0xf3,0xc5,0xb2,0xa4,0x84,0xba,0x64,0xa5,0xb3,0xd2,0x81,0xe5,
      0xd3,0xaa,0xc4,0xca,0xf2,0xb1,0xe4,0xd1,0x83,0x63,0xea,0xc3,
      0xe2,0x82,0xf1,0xa3,0xc2,0xa1,0xc1,0xe3,0xa2,0xe1,0xff,0xff  }
  };
  if (table > 2) table = 2;
  huff[0] = make_decoder ( first_tree[table]);
  huff[1] = make_decoder (second_tree[table]);
}

/*
   Return 0 if the image starts with compressed data,
   1 if it starts with uncompressed low-order bits.

   In Canon compressed data, 0xff is always followed by 0x00.
 */
int CLASS canon_has_lowbits()
{
  uint8_t test[0x4000];
  int ret=1, i;

  fseek (m_InputFile, 0, SEEK_SET);
  ptfread (test, 1, sizeof test, m_InputFile);
  for (i=540; i < (int)(sizeof test) - 1; i++)
    if (test[i] == 0xff) {
      if (test[i+1]) return 1;
      ret=0;
    }
  return ret;
}

void CLASS canon_load_raw()
{
  uint16_t *pixel, *prow, *huff[2];
  int nblocks, lowbits, i, c, row, r, save, val;
  int block, diffbuf[64], leaf, len, diff, carry=0, pnum=0, base[2];

  crw_init_tables (m_Tiff_Compress,huff);
  lowbits = canon_has_lowbits();
  if (!lowbits) m_WhiteLevel = 0x3ff;
  fseek (m_InputFile, 540 + lowbits*m_RawHeight*m_RawWidth/4, SEEK_SET);
  zero_after_ff = 1;
  getbits(-1);
  for (row=0; row < m_RawHeight; row+=8) {
    pixel = m_Raw_Image + row*m_RawWidth;
    nblocks = MIN (8, m_RawHeight-row) * m_RawWidth >> 6;
    for (block=0; block < nblocks; block++) {
      memset (diffbuf, 0, sizeof diffbuf);
      for (i=0; i < 64; i++ ) {
        leaf = gethuff(huff[i>0]);
  if (leaf == 0 && i) break;
  if (leaf == 0xff) continue;
  i  += leaf >> 4;
  len = leaf & 15;
  if (len == 0) continue;
  diff = getbits(len);
  if ((diff & (1 << (len-1))) == 0)
    diff -= (1 << len) - 1;
  if (i < 64) diffbuf[i] = diff;
      }
      diffbuf[0] += carry;
      carry = diffbuf[0];
      for (i=0; i < 64; i++ ) {
  if (pnum++ % m_RawWidth == 0)
    base[0] = base[1] = 512;
  if ((pixel[(block << 6) + i] = base[i & 1] += diffbuf[i]) >> 10)
    derror();
      }
    }
    if (lowbits) {
      save = ftell(m_InputFile);
      fseek (m_InputFile, 26 + row*m_RawWidth/4, SEEK_SET);
      for (prow=pixel, i=0; i < m_RawWidth*2; i++) {
        c = fgetc(m_InputFile);
        for (r=0; r < 8; r+=2, prow++) {
          val = (*prow << 2) + ((c >> r) & 3);
          if (m_RawWidth == 2672 && val < 512) val += 2;
          *prow = val;
        }
      }
      fseek (m_InputFile, save, SEEK_SET);
    }
  }
  FREE(huff[0]);
  FREE(huff[1]);
}

/*
   Not a full implementation of Lossless JPEG, just
   enough to decode Canon, Kodak and Adobe DNG images.
 */
struct jhead {
  int bits, high, wide, clrs, sraw, psv, restart, vpred[6];
  uint16_t *huff[6], *free[4], *row;
};

int CLASS ljpeg_start (struct jhead *jh, int info_only)
{
  int c, tag, len;
  uint8_t data[0x10000];
  const uint8_t *dp;

  memset (jh, 0, sizeof *jh);
  jh->restart = INT_MAX;
  ptfread (data, 2, 1, m_InputFile);
  if (data[1] != 0xd8) return 0;
  do {
    ptfread (data, 2, 2, m_InputFile);
    tag =  data[0] << 8 | data[1];
    len = (data[2] << 8 | data[3]) - 2;
    if (tag <= 0xff00) return 0;
    ptfread (data, 1, len, m_InputFile);
    switch (tag) {
      case 0xffc3:
        jh->sraw = ((data[7] >> 4) * (data[7] & 15) - 1) & 3;
      case 0xffc0:
  jh->bits = data[0];
  jh->high = data[1] << 8 | data[2];
  jh->wide = data[3] << 8 | data[4];
  jh->clrs = data[5] + jh->sraw;
  if (len == 9 && !m_DNG_Version) getc(m_InputFile);
  break;
      case 0xffc4:
  if (info_only) break;
  for (dp = data; dp < data+len && (c = *dp++) < 4; )
    jh->free[c] = jh->huff[c] = make_decoder_ref (&dp);
  break;
      case 0xffda:
  jh->psv = data[1+data[0]*2];
        jh->bits -= data[3+data[0]*2] & 15;
  break;
      case 0xffdd:
  jh->restart = data[0] << 8 | data[1];
    }
  } while (tag != 0xffda);
  if (info_only) return 1;
  for (c=0;c<5;c++) if (!jh->huff[c+1]) jh->huff[c+1] = jh->huff[c];
  if (jh->sraw) {
    for (c=0;c<4;c++) jh->huff[2+c] = jh->huff[1];
    for (c=0;c<(jh->sraw);c++) jh->huff[1+c] = jh->huff[0];
  }
  jh->row = (uint16_t *) CALLOC (jh->wide*jh->clrs, 4);
  merror (jh->row, "ljpeg_start()");
  return zero_after_ff = 1;
}

void CLASS ljpeg_end (struct jhead *jh)
{
  int c;
  for(c=0;c<4;c++) if (jh->free[c]) FREE (jh->free[c]);
  FREE (jh->row);
}

int CLASS ljpeg_diff (uint16_t *huff)
{
  int len, diff;

  len = gethuff(huff);

  if (len == 16 && (!m_DNG_Version || m_DNG_Version >= 0x1010000))
    return -32768;
  diff = getbits(len);
  if ((diff & (1 << (len-1))) == 0)
    diff -= (1 << len) - 1;
  return diff;
}

uint16_t * CLASS ljpeg_row (int jrow, struct jhead *jh)
{
  int col, c, diff, pred, spred=0;
  uint16_t mark=0, *row[3];

  if (jrow * jh->wide % jh->restart == 0) {
    for (c=0;c<6;c++) jh->vpred[c] = 1 << (jh->bits-1);
    if (jrow) {
      fseek(m_InputFile,-2,SEEK_CUR);
      do mark = (mark << 8) + (c = fgetc(m_InputFile));
      while (c != EOF && mark >> 4 != 0xffd);
    }
    getbits(-1);
  }
  for (c=0; c<3; c++) row[c] = jh->row + jh->wide*jh->clrs*((jrow+c) & 1);
  for (col=0; col < jh->wide; col++)
    for (c=0; c < jh->clrs; c++) {
      diff = ljpeg_diff (jh->huff[c]);
      if (jh->sraw && c <= jh->sraw && (col | c))
        pred = spred;
      else if (col) pred = row[0][-jh->clrs];
      else      pred = (jh->vpred[c] += diff) - diff;
      if (jrow && col) switch (jh->psv) {
  case 1: break;
  case 2: pred = row[1][0];         break;
  case 3: pred = row[1][-jh->clrs];       break;
  case 4: pred = pred +   row[1][0] - row[1][-jh->clrs];    break;
  case 5: pred = pred + ((row[1][0] - row[1][-jh->clrs]) >> 1); break;
  case 6: pred = row[1][0] + ((pred - row[1][-jh->clrs]) >> 1); break;
  case 7: pred = (pred + row[1][0]) >> 1;       break;
  default: pred = 0;
      }
      if ((**row = pred + diff) >> jh->bits) derror();
      if (c <= jh->sraw) spred = **row;
      row[0]++; row[1]++;
    }
  return row[2];
}

void CLASS lossless_jpeg_load_raw()
{
  int jwide, jrow, jcol, val, jidx, i, j, row=0, col=0;
  struct jhead jh;
  uint16_t *rp;

  if (!ljpeg_start (&jh, 0)) return;
  jwide = jh.wide * jh.clrs;

  TRACEKEYVALS("jh.high","%d",jh.high);
  TRACEKEYVALS("jwide","%d",jwide);
  for (jrow=0; jrow < jh.high; jrow++) {
    rp = ljpeg_row (jrow, &jh);
    if (m_Load_Flags & 1)
      row = jrow & 1 ? m_Height-1-jrow/2 : jrow/2;
    for (jcol=0; jcol < jwide; jcol++) {
      val = m_Curve[*rp++];
      if (cr2_slice[0]) {
  jidx = jrow*jwide + jcol;
  i = jidx / (cr2_slice[1]*jh.high);
  if ((j = i >= cr2_slice[0]))
     i  = cr2_slice[0];
  jidx -= i * (cr2_slice[1]*jh.high);
  row = jidx / cr2_slice[1+j];
  col = jidx % cr2_slice[1+j] + i*cr2_slice[1];
      }
      if (m_RawWidth == 3984 && (col -= 2) < 0)
        col += (row--,m_RawWidth);
      if (row >= 0) RAW(row,col) = val;
      if (++col >= m_RawWidth)
  col = (row++,0);
    }
  }
  ljpeg_end(&jh);
// printf("TIEDELIE dark[0]:%f dark[1]:%f nblack:%d\n",dark[0],dark[1],nblack);
}

void CLASS canon_sraw_load_raw()
{
  struct jhead jh;
  short *rp=0, (*ip)[4];
  int jwide, slice, scol, ecol, row, col, jrow=0, jcol=0, pix[3], c;
  int v[3]={0,0,0}, ver, hue;
  char *cp;

  if (!ljpeg_start (&jh, 0)) return;
  jwide = (jh.wide >>= 1) * jh.clrs;

  for (ecol=slice=0; slice <= cr2_slice[0]; slice++) {
    scol = ecol;
    ecol += cr2_slice[1] * 2 / jh.clrs;
    if (!cr2_slice[0] || ecol > m_RawWidth-1) ecol = m_RawWidth & -2;
    for (row=0; row < m_Height; row += (jh.clrs >> 1) - 1) {
      ip = (short (*)[4]) m_Image + row*m_Width;
      for (col=scol; col < ecol; col+=2, jcol+=jh.clrs) {
  if ((jcol %= jwide) == 0)
    rp = (short *) ljpeg_row (jrow++, &jh);
  if (col >= m_Width) continue;
  for (c=0; c< (jh.clrs-2); c++)
    ip[col + (c >> 1)*m_Width + (c & 1)][0] = rp[jcol+c];
  ip[col][1] = rp[jcol+jh.clrs-2] - 16384;
  ip[col][2] = rp[jcol+jh.clrs-1] - 16384;
      }
    }
  }
  for (cp=m_CameraModelBis; *cp && !isdigit(*cp); cp++) {};
  sscanf (cp, "%d.%d.%d", v, v+1, v+2);
  ver = (v[0]*1000 + v[1])*1000 + v[2];
  hue = (jh.sraw+1) << 2;
  if (unique_id >= 0x80000281 || (unique_id == 0x80000218 && ver > 1000006))
    hue = jh.sraw << 1;
  ip = (short (*)[4]) m_Image;
  rp = ip[0];
  for (row=0; row < m_Height; row++, ip+=m_Width) {
    if (row & (jh.sraw >> 1))
      for (col=0; col < m_Width; col+=2)
  for (c=1; c < 3; c++)
    if (row == m_Height-1) {
      ip[col][c] =  ip[col-m_Width][c];
    } else {
      ip[col][c] = (ip[col-m_Width][c] + ip[col+m_Width][c] + 1) >> 1;
    }
    for (col=1; col < m_Width; col+=2)
      for (c=1; c < 3; c++)
  if (col == m_Width-1)
       ip[col][c] =  ip[col-1][c];
  else ip[col][c] = (ip[col-1][c] + ip[col+1][c] + 1) >> 1;
  }
  for ( ; rp < ip[0]; rp+=4) {
    if (unique_id == 0x80000218 ||
	unique_id == 0x80000250 ||
	unique_id == 0x80000261 ||
	unique_id == 0x80000281 ||
	unique_id == 0x80000287) {
      rp[1] = (rp[1] << 2) + hue;
      rp[2] = (rp[2] << 2) + hue;
      pix[0] = rp[0] + ((  200*rp[1] + 22929*rp[2]) >> 14);
      pix[1] = rp[0] + ((-5640*rp[1] - 11751*rp[2]) >> 14);
      pix[2] = rp[0] + ((29040*rp[1] -   101*rp[2]) >> 14);
    } else {
      if (unique_id < 0x80000218) rp[0] -= 512;
      pix[0] = rp[0] + rp[2];
      pix[2] = rp[0] + rp[1];
      pix[1] = rp[0] + ((-778*rp[1] - (rp[2] << 11)) >> 12);
    }
    for (c=0;c<3;c++) rp[c] = CLIP(pix[c] * sraw_mul[c] >> 10);
  }
  ljpeg_end(&jh);
  m_WhiteLevel = 0x3fff;
}

void CLASS adobe_copy_pixel (unsigned row, unsigned col, uint16_t **rp)
{
  int c;

  if (m_IsRaw == 2 && m_UserSetting_ShotSelect) (*rp)++;
  if (m_Raw_Image) {
    if (row < m_RawHeight && col < m_RawWidth)
      RAW(row,col) = m_Curve[**rp];
    *rp += m_IsRaw;
  } else {
    if (row < m_Height && col < m_Width)
      for (c=0; c < (int32_t)m_Tiff_Samples; c++)
  m_Image[row*m_Width+col][c] = m_Curve[(*rp)[c]];
    *rp += m_Tiff_Samples;
  }
  if (m_IsRaw == 2 && m_UserSetting_ShotSelect) (*rp)--;
}

void CLASS lossless_dng_load_raw()
{
  unsigned save, trow=0, tcol=0, jwide, jrow, jcol, row, col;
  struct jhead jh;
  uint16_t *rp;

  while (trow < m_RawHeight) {
    save = ftell(m_InputFile);
    if (m_TileLength < INT_MAX)
      fseek (m_InputFile, get4(), SEEK_SET);
    if (!ljpeg_start (&jh, 0)) break;
    jwide = jh.wide;
    if (m_Filters) jwide *= jh.clrs;
    jwide /= m_IsRaw;
    for (row=col=jrow=0; jrow < (unsigned) jh.high; jrow++) {
      rp = ljpeg_row (jrow, &jh);
      for (jcol=0; jcol < jwide; jcol++) {
  adobe_copy_pixel (trow+row, tcol+col, &rp);
  if (++col >= m_TileWidth || col >= m_RawWidth)
    row += 1 + (col = 0);
      }
    }
    fseek (m_InputFile, save+4, SEEK_SET);
    if ((tcol += m_TileWidth) >= m_RawWidth)
      trow += m_TileLength + (tcol = 0);
    ljpeg_end(&jh);
  }
}

void CLASS packed_dng_load_raw()
{
  uint16_t *pixel, *rp;
  // int row, col;
  unsigned row, col;

  pixel = (uint16_t *) CALLOC (m_RawWidth * m_Tiff_Samples, sizeof *pixel);
  merror (pixel, "packed_dng_load_raw()");
  for (row=0; row < m_RawHeight; row++) {
    if (m_Tiff_bps == 16)
      read_shorts (pixel, m_RawWidth * m_Tiff_Samples);
    else {
      getbits(-1);
      for (col=0; col < m_RawWidth * m_Tiff_Samples; col++)
  pixel[col] = getbits(m_Tiff_bps);
    }
    for (rp=pixel, col=0; col < m_RawWidth; col++)
      adobe_copy_pixel (row, col, &rp);
  }
  FREE (pixel);
}

void CLASS pentax_load_raw()
{
  uint16_t bit[2][15], huff[4097];
  int dep, row, col, diff, c, i;
  uint16_t vpred[2][2] = {{0,0},{0,0}}, hpred[2];

  fseek (m_InputFile, meta_offset, SEEK_SET);
  dep = (get2() + 12) & 15;
  fseek (m_InputFile, 12, SEEK_CUR);
  for(c=0;c<dep;c++) bit[0][c] = get2();
  for(c=0;c<dep;c++) bit[1][c] = fgetc(m_InputFile);
  for(c=0;c<dep;c++)
    for (i=bit[0][c]; i <= ((bit[0][c]+(4096 >> bit[1][c])-1) & 4095); )
      huff[++i] = bit[1][c] << 8 | c;
  huff[0] = 12;
  fseek (m_InputFile, m_Data_Offset, SEEK_SET);
  getbits(-1);
  for (row=0; row < m_RawHeight; row++)
    for (col=0; col < m_RawWidth; col++) {
      diff = ljpeg_diff (huff);
      if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
      else     hpred[col & 1] += diff;
      RAW(row,col) = hpred[col & 1];
      if (hpred[col & 1] >> m_Tiff_bps) derror();
    }
}

void CLASS nikon_load_raw()
{
  static const uint8_t nikon_tree[][32] = {
    { 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,  /* 12-bit lossy */
      5,4,3,6,2,7,1,0,8,9,11,10,12 },
    { 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,  /* 12-bit lossy after split */
      0x39,0x5a,0x38,0x27,0x16,5,4,3,2,1,0,11,12,12 },
    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,  /* 12-bit lossless */
      5,4,6,3,7,2,8,1,9,0,10,11,12 },
    { 0,1,4,3,1,1,1,1,1,2,0,0,0,0,0,0,  /* 14-bit lossy */
      5,6,4,7,8,3,9,2,1,0,10,11,12,13,14 },
    { 0,1,5,1,1,1,1,1,1,1,2,0,0,0,0,0,  /* 14-bit lossy after split */
      8,0x5c,0x4b,0x3a,0x29,7,6,5,4,3,2,1,0,13,14 },
    { 0,1,4,2,2,3,1,2,0,0,0,0,0,0,0,0,  /* 14-bit lossless */
      7,6,8,5,9,4,10,3,11,12,2,0,1,13,14 } };
  uint16_t *huff, ver0, ver1, vpred[2][2], hpred[2], csize;
  int i, min, max, step=0, tree=0, split=0, row, col, len, shl, diff;

  fseek (m_InputFile, meta_offset, SEEK_SET);
  ver0 = fgetc(m_InputFile);
  ver1 = fgetc(m_InputFile);
  if (ver0 == 0x49 || ver1 == 0x58)
    fseek (m_InputFile, 2110, SEEK_CUR);
  if (ver0 == 0x46) tree = 2;
  if (m_Tiff_bps == 14) tree += 3;
  read_shorts (vpred[0], 4);
  max = 1 << m_Tiff_bps & 0x7fff;
  if ((csize = get2()) > 1)
    step = max / (csize-1);
  if (ver0 == 0x44 && ver1 == 0x20 && step > 0) {
    for (i=0; i < csize; i++)
      m_Curve[i*step] = get2();
    for (i=0; i < max; i++)
      m_Curve[i] = ( m_Curve[i-i%step]*(step-i%step) +
       m_Curve[i-i%step+step]*(i%step) ) / step;
    fseek (m_InputFile, meta_offset+562, SEEK_SET);
    split = get2();
  } else if (ver0 != 0x46 && csize <= 0x4001)
    read_shorts (m_Curve, max=csize);
  while (m_Curve[max-2] == m_Curve[max-1]) max--;
  huff = make_decoder(nikon_tree[tree]);
  fseek (m_InputFile, m_Data_Offset, SEEK_SET);
  getbits(-1);
  for (min=row=0; row < m_Height; row++) {
    if (split && row == split) {
      FREE(huff);
      huff = make_decoder(nikon_tree[tree+1]);
      max += (min = 16) << 1;
    }
    for (col=0; col < m_RawWidth; col++) {
      i = gethuff(huff);
      len = i & 15;
      shl = i >> 4;
      diff = ((getbits(len-shl) << 1) + 1) << shl >> 1;
      if ((diff & (1 << (len-1))) == 0)
  diff -= (1 << len) - !shl;
      if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
      else     hpred[col & 1] += diff;
      if ((uint16_t)(hpred[col & 1] + min) >= max) derror();
      RAW(row,col) = m_Curve[LIM((int16_t)hpred[col & 1],(int16_t)0,(int16_t)0x3fff)];
    }
  }
  FREE(huff);
}

/*
   Returns 1 for a Coolpix 995, 0 for anything else.
 */
int CLASS nikon_e995()
{
  int i, histo[256];
  const uint8_t often[] = { 0x00, 0x55, 0xaa, 0xff };

  memset (histo, 0, sizeof histo);
  fseek (m_InputFile, -2000, SEEK_END);
  for (i=0; i < 2000; i++)
    histo[fgetc(m_InputFile)]++;
  for (i=0; i < 4; i++)
    if (histo[often[i]] < 200)
      return 0;
  return 1;
}

/*
   Returns 1 for a Coolpix 2100, 0 for anything else.
 */
int CLASS nikon_e2100()
{
  uint8_t t[12];
  int i;

  fseek (m_InputFile, 0, SEEK_SET);
  for (i=0; i < 1024; i++) {
    ptfread (t, 1, 12, m_InputFile);
    if (((t[2] & t[4] & t[7] & t[9]) >> 4
  & t[1] & t[6] & t[8] & t[11] & 3) != 3)
      return 0;
  }
  return 1;
}

void CLASS nikon_3700()
{
  int bits; // i;
  uint8_t dp[24];
  static const struct {
    int bits;
    char make[12], model[15];
  } table[] = {
    { 0x00, "PENTAX",  "Optio 33WR" },
    { 0x03, "NIKON",   "E3200" },
    { 0x32, "NIKON",   "E3700" },
    { 0x33, "OLYMPUS", "C740UZ" } };

  fseek (m_InputFile, 3072, SEEK_SET);
  ptfread (dp, 1, 24, m_InputFile);
  bits = (dp[8] & 3) << 4 | (dp[20] & 3);
  for (unsigned i=0; i < sizeof table / sizeof *table; i++)
    if (bits == table[i].bits) {
      strcpy (m_CameraMake,  table[i].make );
      strcpy (m_CameraModel, table[i].model);
    }
}

/*
   Separates a Minolta DiMAGE Z2 from a Nikon E4300.
 */
int CLASS minolta_z2()
{
  unsigned i,nz;
  char tail[424];

  fseek (m_InputFile, (long)(-sizeof tail), SEEK_END);
  ptfread (tail, 1, sizeof tail, m_InputFile);
  for (nz=i=0; i < sizeof tail; i++)
    if (tail[i]) nz++;
  return nz > 20;
}

//void CLASS jpeg_thumb ();

void CLASS ppm_thumb ()
/* Remember: This function is modified to write the raw’s thumbnail to the
   m_Thumb instead of a file on disk. Always access thumbnails via DcRaw::thumbnail()!
*/
{
  char *thumb;
  m_ThumbLength = m_ThumbWidth*m_ThumbHeight*3;
  thumb = (char *) MALLOC (m_ThumbLength);
  merror (thumb, "ppm_thumb()");
  //fprintf (m_OutputFile, "P6\n%d %d\n255\n", m_ThumbWidth, m_ThumbHeight);
  QString dummy = QString("P6\n%1 %2\n255\n").arg(m_ThumbWidth).arg(m_ThumbHeight);
  VAppend(m_Thumb, dummy.toLocal8Bit().data(), dummy.length());
  ptfread  (thumb, 1, m_ThumbLength, m_InputFile);
  VAppend(m_Thumb, thumb, m_ThumbLength);
  FREE (thumb);
}

void CLASS ppm16_thumb()
{
  int i;
  char *thumb;
  m_ThumbLength = m_ThumbWidth*m_ThumbHeight*3;
  thumb = (char *) CALLOC (m_ThumbLength,2);
  merror (thumb, "ppm16_thumb()");
  read_shorts ((uint16_t *) thumb, m_ThumbLength);
  for (i=0; i < (int32_t)m_ThumbLength; i++)
    thumb[i] = ((uint16_t *) thumb)[i] >> 8;
  QString dummy = QString("P6\n%1 %2\n255\n").arg(m_ThumbWidth).arg(m_ThumbHeight);
  VAppend(m_Thumb, dummy.toLocal8Bit().data(), dummy.length());
  //fprintf (ofp, "P6\n%d %d\n255\n", m_ThumbWidth, m_ThumbHeight);
  //fwrite (thumb, 1, m_ThumbLength, ofp);
  VAppend(m_Thumb, thumb, m_ThumbLength);
  FREE (thumb);
}

void CLASS layer_thumb ()
/* Remember: This function is modified to write the raw’s thumbnail to the
   m_Thumb instead of a file on disk. Always access thumbnails via DcRaw::thumbnail()!
*/
// TODO: Tests needed: What format is this? Can it be read by QPixmap?
{
  // int i, c;
  int c;
  char *thumb, map[][4] = { "012","102" };

  m_Colors = m_ThumbMisc >> 5 & 7;
  m_ThumbLength = m_ThumbWidth*m_ThumbHeight;
  thumb = (char *) CALLOC (m_Colors, m_ThumbLength);
  merror (thumb, "layer_thumb()");
  //fprintf (m_OutputFile, "P%d\n%d %d\n255\n", 5 + (m_Colors >> 1), m_ThumbWidth, m_ThumbHeight);
  QString dummy = QString("P%1\n%2 %3\n255\n")
      .arg(5 + (m_Colors >> 1)).arg(m_ThumbWidth).arg(m_ThumbHeight);
  VAppend(m_Thumb, dummy.toLocal8Bit().data(), dummy.length());
  ptfread (thumb, m_ThumbLength, m_Colors, m_InputFile);
  for (unsigned i=0; i < m_ThumbLength; i++) {
    for (c=0; c < m_Colors; c++) {
      //putc (thumb[i+m_ThumbLength*(map[m_ThumbMisc >> 8][c]-'0')], m_OutputFile);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
      m_Thumb.push_back(thumb[i+m_ThumbLength*(map[m_ThumbMisc >> 8][c]-'0')]);
#pragma GCC diagnostic pop
    }
  }
  FREE (thumb);
}

void CLASS rollei_thumb ()
/* Remember: This function is modified to write the raw’s thumbnail to the
   m_Thumb instead of a file on disk. Always access thumbnails via DcRaw::thumbnail()!
*/
// TODO: Tests needed: What format is this? Can it be read by QPixmap?
{
  unsigned i;
  uint16_t *thumb;

  m_ThumbLength = m_ThumbWidth * m_ThumbHeight;
  thumb = (uint16_t *) CALLOC (m_ThumbLength, 2);
  merror (thumb, "rollei_thumb()");
  //fprintf (m_OutputFile, "P6\n%d %d\n255\n", m_ThumbWidth, m_ThumbHeight);
  QString dummy = QString("P6\n%1 %2\n255\n").arg(m_ThumbWidth).arg(m_ThumbHeight);
  VAppend(m_Thumb, dummy.toLocal8Bit().data(), dummy.length());
  read_shorts (thumb, m_ThumbLength);
  for (i=0; i < m_ThumbLength; i++) {
    //putc (thumb[i] << 3, m_OutputFile);
    //putc (thumb[i] >> 5  << 2, m_OutputFile);
    //putc (thumb[i] >> 11 << 3, m_OutputFile);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    m_Thumb.push_back(thumb[i] << 3);
    m_Thumb.push_back(thumb[i] >> 5 << 2);
    m_Thumb.push_back(thumb[i] >> 11 << 3);
#pragma GCC diagnostic pop
  }
  FREE (thumb);
}

void CLASS rollei_load_raw()
{
  uint8_t pixel[10];
  unsigned iten=0, isix, i, buffer=0, todo[16];

  isix = m_RawWidth * m_RawHeight * 5 / 8;
  while (fread (pixel, 1, 10, m_InputFile) == 10) {
    for (i=0; i < 10; i+=2) {
      todo[i]   = iten++;
      todo[i+1] = pixel[i] << 8 | pixel[i+1];
      buffer    = pixel[i] >> 2 | buffer << 6;
    }
    for (   ; i < 16; i+=2) {
      todo[i]   = isix++;
      todo[i+1] = buffer >> (14-i)*5;
    }
    for (i=0; i < 16; i+=2)
      m_Raw_Image[todo[i]] = (todo[i+1] & 0x3ff);
  }
  m_WhiteLevel = 0x3ff;
}

int CLASS raw (unsigned row, unsigned col)
{
  return (row < m_RawHeight && col < m_RawWidth) ? RAW(row,col) : 0;
}

void CLASS phase_one_flat_field (int is_float, int nc)
{
  uint16_t head[8];
  unsigned wide, y, x, c, rend, cend, row, col;
  float *mrow, num, mult[4];

  read_shorts (head, 8);
  wide = head[2] / head[4];
  mrow = (float *) CALLOC (nc*wide, sizeof *mrow);
  merror (mrow, "phase_one_flat_field()");
  for (y=0; y < (unsigned) (head[3] / head[5]); y++) {
    for (x=0; x < wide; x++)
      for (c=0; c < (unsigned) nc; c+=2) {
  num = is_float ? getreal(11) : get2()/32768.0;
  if (y==0) mrow[c*wide+x] = num;
  else mrow[(c+1)*wide+x] = (num - mrow[c*wide+x]) / head[5];
      }
    if (y==0) continue;
    rend = head[1] + y*head[5];
    for (row = rend-head[5]; row < m_RawHeight && row < rend; row++) {
      for (x=1; x < wide; x++) {
  for (c=0; c < (unsigned) nc; c+=2) {
    mult[c] = mrow[c*wide+x-1];
    mult[c+1] = (mrow[c*wide+x] - mult[c]) / head[4];
  }
  cend = head[0] + x*head[4];
  for (col = cend-head[4]; col < m_RawWidth && col < cend; col++) {
    c = nc > 2 ? FC(row-m_TopMargin,col-m_LeftMargin) : 0;
    if (!(c & 1)) {
      c = unsigned(RAW(row,col) * mult[c]);
      RAW(row,col) = MIN((int32_t)c,65535);
    }
    for (c=0; c < (unsigned) nc; c+=2)
      mult[c] += mult[c+1];
  }
      }
      for (x=0; x < wide; x++)
  for (c=0; c < (unsigned) nc; c+=2)
    mrow[c*wide+x] += mrow[(c+1)*wide+x];
    }
  }
  FREE (mrow);
}

void CLASS phase_one_correct()
{
  unsigned entries, tag, data, save, col, row, type;
  int len, i, j, k, cip, val[4], dev[4], sum, max;
  int head[9], diff, mindiff=INT_MAX, off_412=0;
  static const signed char dir[12][2] =
    { {-1,-1}, {-1,1}, {1,-1}, {1,1}, {-2,0}, {0,-2}, {0,2}, {2,0},
      {-2,-2}, {-2,2}, {2,-2}, {2,2} };
  float poly[8], num, cfrac, frac, mult[2], *yval[2];
  uint16_t *xval[2];

  if (m_UserSetting_HalfSize || !m_MetaLength) return;
  TRACEKEYVALS("Phase One correction","%s","");
  fseek (m_InputFile, meta_offset, SEEK_SET);
  m_ByteOrder = get2();
  fseek (m_InputFile, 6, SEEK_CUR);
  fseek (m_InputFile, meta_offset+get4(), SEEK_SET);
  entries = get4();  get4();
  while (entries--) {
    tag  = get4();
    len  = get4();
    data = get4();
    save = ftell(m_InputFile);
    fseek (m_InputFile, meta_offset+data, SEEK_SET);
    if (tag == 0x419) {       /* Polynomial curve */
      for (get4(), i=0; i < 8; i++)
  poly[i] = getreal(11);
      poly[3] += (ph1.tag_210 - poly[7]) * poly[6] + 1;
      for (i=0; i < 0x10000; i++) {
  num = (poly[5]*i + poly[3])*i + poly[1];
  m_Curve[i] = (uint16_t) LIM((int32_t)num,0,65535);
      } goto apply;       /* apply to right half */
    } else if (tag == 0x41a) {      /* Polynomial curve */
      for (i=0; i < 4; i++)
  poly[i] = getreal(11);
      for (i=0; i < 0x10000; i++) {
  for (num=0, j=4; j--; )
    num = num * i + poly[j];
  m_Curve[i] = (uint16_t) LIM((int32_t)(num+i),0,65535);
      } apply:          /* apply to whole image */
      for (row=0; row < m_RawHeight; row++)
  for (col = (tag & 1)*ph1.split_col; col < m_RawWidth; col++)
    RAW(row,col) = m_Curve[RAW(row,col)];
    } else if (tag == 0x400) {      /* Sensor defects */
      while ((len -= 8) >= 0) {
  col  = get2();
  row  = get2();
  type = get2(); get2();
  if (col >= m_RawWidth) continue;
  if (type == 131)      /* Bad column */
    for (row=0; row < m_RawHeight; row++)
      if (FC(row-m_TopMargin,col-m_LeftMargin) == 1) {
        for (sum=i=0; i < 4; i++)
    sum += val[i] = raw (row+dir[i][0], col+dir[i][1]);
        for (max=i=0; i < 4; i++) {
    dev[i] = abs((val[i] << 2) - sum);
    if (dev[max] < dev[i]) max = i;
        }
        RAW(row,col) = (uint16_t) ((sum - val[max])/3.0 + 0.5);
      } else {
        for (sum=0, i=8; i < 12; i++)
    sum += raw (row+dir[i][0], col+dir[i][1]);
        RAW(row,col) = (uint16_t) (0.5 + sum * 0.0732233 +
    (raw(row,col-2) + raw(row,col+2)) * 0.3535534);
      }
  else if (type == 129) {     /* Bad pixel */
    if (row >= m_RawHeight) continue;
    j = (FC(row-m_TopMargin,col-m_LeftMargin) != 1) * 4;
    for (sum=0, i=j; i < j+8; i++)
      sum += raw (row+dir[i][0], col+dir[i][1]);
    RAW(row,col) = (sum + 4) >> 3;
  }
      }
    } else if (tag == 0x401) {      /* All-color flat fields */
      phase_one_flat_field (1, 2);
    } else if (tag == 0x416 || tag == 0x410) {
      phase_one_flat_field (0, 2);
    } else if (tag == 0x40b) {      /* Red+blue flat field */
      phase_one_flat_field (0, 4);
    } else if (tag == 0x412) {
      fseek (m_InputFile, 36, SEEK_CUR);
      diff = abs (get2() - ph1.tag_21a);
      if (mindiff > diff) {
  mindiff = diff;
  off_412 = ftell(m_InputFile) - 38;
      }
    }
    fseek (m_InputFile, save, SEEK_SET);
  }
  if (off_412) {
    fseek (m_InputFile, off_412, SEEK_SET);
    for (i=0; i < 9; i++) head[i] = get4() & 0x7fff;
    yval[0] = (float *) CALLOC (head[1]*head[3] + head[2]*head[4], 6);
    merror (yval[0], "phase_one_correct()");
    yval[1] = (float  *) (yval[0] + head[1]*head[3]);
    xval[0] = (uint16_t *) (yval[1] + head[2]*head[4]);
    xval[1] = (uint16_t *) (xval[0] + head[1]*head[3]);
    get2();
    for (i=0; i < 2; i++)
      for (j=0; j < head[i+1]*head[i+3]; j++)
  yval[i][j] = getreal(11);
    for (i=0; i < 2; i++)
      for (j=0; j < head[i+1]*head[i+3]; j++)
  xval[i][j] = get2();
    for (row=0; row < m_RawHeight; row++)
      for (col=0; col < m_RawWidth; col++) {
  cfrac = (float) col * head[3] / m_RawWidth;
  cfrac -= (cip = (int) cfrac);
  num = RAW(row,col) * 0.5;
  for (i=cip; i < cip+2; i++) {
    for (k=j=0; j < head[1]; j++)
      if (num < xval[0][k = head[1]*i+j]) break;
    frac = (j == 0 || j == head[1]) ? 0 :
    (xval[0][k] - num) / (xval[0][k] - xval[0][k-1]);
    mult[i-cip] = yval[0][k-1] * frac + yval[0][k] * (1-frac);
  }
  i = (int) (((mult[0] * (1-cfrac) + mult[1] * cfrac) * row + num) * 2);
  RAW(row,col) = LIM(i,0,65535);
      }
    FREE (yval[0]);
  }
}

void CLASS phase_one_load_raw()
{
  int a, b, i;
  uint16_t akey, bkey, m_Mask;

  fseek (m_InputFile, ph1.key_off, SEEK_SET);
  akey = get2();
  bkey = get2();
  m_Mask = ph1.format == 1 ? 0x5555:0x1354;
  fseek (m_InputFile, m_Data_Offset, SEEK_SET);
  read_shorts (m_Raw_Image, m_RawWidth*m_RawHeight);
  if (ph1.format)
    for (i=0; i < m_RawWidth*m_RawHeight; i+=2) {
      a = m_Raw_Image[i+0] ^ akey;
      b = m_Raw_Image[i+1] ^ bkey;
      m_Raw_Image[i+0] = (a & m_Mask) | (b & ~m_Mask);
      m_Raw_Image[i+1] = (b & m_Mask) | (a & ~m_Mask);
    }
}

unsigned CLASS ph1_bithuff (int nbits, uint16_t *huff)
{
  unsigned c;

  if (nbits == -1)
    return m_ph1_bithuffbitbuf = m_ph1_bithuffvbits = 0;
  if (nbits == 0) return 0;
  if (m_ph1_bithuffvbits < nbits) {
    m_ph1_bithuffbitbuf = m_ph1_bithuffbitbuf << 32 | get4();
    m_ph1_bithuffvbits += 32;
  }
  c = m_ph1_bithuffbitbuf << (64-m_ph1_bithuffvbits) >> (64-nbits);
  if (huff) {
    m_ph1_bithuffvbits -= huff[c] >> 8;
    return (unsigned char) huff[c];
  }
  m_ph1_bithuffvbits -= nbits;
  return c;
}
#define ph1_bits(n) ph1_bithuff(n,0)
#define ph1_huff(h) ph1_bithuff(*h,h+1)

void CLASS phase_one_load_raw_c()
{
  static const int length[] = { 8,7,6,9,11,10,5,12,14,13 };
  int *offset, len[2], pred[2], row, col, i, j;
  uint16_t *pixel;
  short (*black)[2];

  pixel = (uint16_t *) CALLOC (m_RawWidth + m_RawHeight*4, 2);
  merror (pixel, "phase_one_load_raw_c()");
  offset = (int *) (pixel + m_RawWidth);
  fseek (m_InputFile, strip_offset, SEEK_SET);
  for (row=0; row < m_RawHeight; row++)
    offset[row] = get4();
  black = (short (*)[2]) offset + m_RawHeight;
  fseek (m_InputFile, ph1.black_off, SEEK_SET);
  if (ph1.black_off)
    read_shorts ((uint16_t *) black[0], m_RawHeight*2);
  for (i=0; i < 256; i++)
    m_Curve[i] = (uint16_t) (i*i / 3.969 + 0.5);
  for (row=0; row < m_RawHeight; row++) {
    fseek (m_InputFile, m_Data_Offset + offset[row], SEEK_SET);
    ph1_bits(-1);
    pred[0] = pred[1] = 0;
    for (col=0; col < m_RawWidth; col++) {
      if (col >= (m_RawWidth & -8))
  len[0] = len[1] = 14;
      else if ((col & 7) == 0)
  for (i=0; i < 2; i++) {
    for (j=0; j < 5 && !ph1_bits(1); j++) {};
    if (j--) len[i] = length[j*2 + ph1_bits(1)];
  }
      if ((i = len[col & 1]) == 14)
  pixel[col] = pred[col & 1] = ph1_bits(16);
      else
  pixel[col] = pred[col & 1] += ph1_bits(i) + 1 - (1 << (i - 1));
      if (pred[col & 1] >> 16) derror();
      if (ph1.format == 5 && pixel[col] < 256)
  pixel[col] = m_Curve[pixel[col]];
    }
    for (col=0; col < m_RawWidth; col++) {
      i = (pixel[col] << 2) - ph1.black + black[row][col >= ph1.split_col];
  if (i > 0) RAW(row,col) = i;
    }
  }
  FREE (pixel);
  m_WhiteLevel = 0xfffc - ph1.black;
}

void CLASS hasselblad_load_raw()
{
  struct jhead jh;
  int row, col, pred[2], len[2], diff, c;

  if (!ljpeg_start (&jh, 0)) return;
  m_ByteOrder = 0x4949;
  ph1_bits(-1);
  for (row=0; row < m_RawHeight; row++) {
    pred[0] = pred[1] = 0x8000 + m_Load_Flags;
    for (col=0; col < m_RawWidth; col+=2) {
      for(c=0;c<2;c++) len[c] = ph1_huff(jh.huff[0]);
      for(c=0;c<2;c++) {
  diff = ph1_bits(len[c]);
  if ((diff & (1 << (len[c]-1))) == 0)
    diff -= (1 << len[c]) - 1;
  if (diff == 65535) diff = -32768;
	RAW(row,col+c) = pred[c] += diff;
      }
    }
  }
  ljpeg_end (&jh);
  m_WhiteLevel = 0xffff;
}

void CLASS leaf_hdr_load_raw()
{
  uint16_t *pixel=0;
  unsigned tile=0, r, c, row, col;

  if (!m_Filters) {
    pixel = (uint16_t *) CALLOC (m_RawWidth, sizeof *pixel);
    merror (pixel, "leaf_hdr_load_raw()");
  }
  for (c=0; c < m_Tiff_Samples; c++) {
    for (r=0; r < m_RawHeight; r++) {
      if (r % m_TileLength == 0) {
  fseek (m_InputFile, m_Data_Offset + 4*tile++, SEEK_SET);
  fseek (m_InputFile, get4(), SEEK_SET);
      }
      if (m_Filters && c != m_UserSetting_ShotSelect) continue;
      if (m_Filters) pixel = m_Raw_Image + r*m_RawWidth;
      read_shorts (pixel, m_RawWidth);
      if (!m_Filters && (row = r - m_TopMargin) < m_Height)
  for (col=0; col < m_Width; col++)
    m_Image[row*m_Width+col][c] = pixel[col+m_LeftMargin];
    }
}
  if (!m_Filters) {
    m_WhiteLevel = 0xffff;
    m_RawColor = 1;
    FREE (pixel);
  }
}

void CLASS unpacked_load_raw()
{
  int row, col, bits=0;

  while ((1 << ++bits) < (int32_t)m_WhiteLevel);
  read_shorts (m_Raw_Image, m_RawWidth*m_RawHeight);
  for (row=0; row < m_RawHeight; row++)
    for (col=0; col < m_RawWidth; col++)
      if ((RAW(row,col) >>= m_Load_Flags) >> bits
  && (unsigned) (row-m_TopMargin) < m_Height
  && (unsigned) (col-m_LeftMargin) < m_Width) derror();
}

void CLASS sinar_4shot_load_raw()
{
  uint16_t *pixel;
  unsigned shot, row, col, r, c;

  if ((shot = m_UserSetting_ShotSelect) || m_UserSetting_HalfSize) {
    if (shot) shot--;
    if (shot > 3) shot = 3;
    fseek (m_InputFile, m_Data_Offset + shot*4, SEEK_SET);
    fseek (m_InputFile, get4(), SEEK_SET);
    unpacked_load_raw();
    return;
  }
  FREE (m_Raw_Image);
  m_Raw_Image = 0;
  m_Image = (uint16_t (*)[4])
  CALLOC ((m_OutHeight=m_Height)*(m_OutWidth=m_Width), sizeof *m_Image);
  merror (m_Image, "sinar_4shot_load_raw()");
  pixel = (uint16_t *) CALLOC (m_RawWidth, sizeof *pixel);
  merror (pixel, "sinar_4shot_load_raw()");
  for (shot=0; shot < 4; shot++) {
    fseek (m_InputFile, m_Data_Offset + shot*4, SEEK_SET);
    fseek (m_InputFile, get4(), SEEK_SET);
    for (row=0; row < m_RawHeight; row++) {
      read_shorts (pixel, m_RawWidth);
      if ((r = row-m_TopMargin - (shot >> 1 & 1)) >= m_Height) continue;
      for (col=0; col < m_RawWidth; col++) {
  if ((c = col-m_LeftMargin - (shot & 1)) >= m_Width) continue;
        m_Image[r*m_Width+c][FC(row,col)] = pixel[col];
      }
    }
  }
  FREE (pixel);
  m_Shrink = m_Filters = 0;
}

void CLASS imacon_full_load_raw()
{
  int row, col;

  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++)
      read_shorts (m_Image[row*m_Width+col], 3);
}

void CLASS packed_load_raw()
{
  int vbits=0, bwide, pwide, rbits, bite, half, irow, row, col, val, i;
  uint64_t bitbuf=0;

  if (m_RawWidth * 8U >= m_Width * m_Tiff_bps)  /* Is m_RawWidth in bytes? */
       pwide = (bwide = m_RawWidth) * 8 / m_Tiff_bps;
  else bwide = (pwide = m_RawWidth) * m_Tiff_bps / 8;
  rbits = bwide * 8 - pwide * m_Tiff_bps;
  if (m_Load_Flags & 1) bwide = bwide * 16 / 15;
  bite = 8 + (m_Load_Flags & 24);
  half = (m_RawHeight+1) >> 1;
  for (irow=0; irow < m_RawHeight; irow++) {
    row = irow;
    if (m_Load_Flags & 2 &&
  (row = irow % half * 2 + irow / half) == 1 &&
  m_Load_Flags & 4) {
      if (vbits=0, m_Tiff_Compress)
  fseek (m_InputFile, m_Data_Offset - (-half*bwide & -2048), SEEK_SET);
      else {
  fseek (m_InputFile, 0, SEEK_END);
  fseek (m_InputFile, ftell(m_InputFile) >> 3 << 2, SEEK_SET);
      }
    }
    for (col=0; col < pwide; col++) {
      for (vbits -= m_Tiff_bps; vbits < 0; vbits += bite) {
  bitbuf <<= bite;
  for (i=0; i < bite; i+=8)
    bitbuf |= (unsigned) (fgetc(m_InputFile) << i);
      }
      val = bitbuf << (64-m_Tiff_bps-vbits) >> (64-m_Tiff_bps);
      RAW(row,col ^ (m_Load_Flags >> 6)) = val;
      if (m_Load_Flags & 1 && (col % 10) == 9 &&
  fgetc(m_InputFile) && col < m_Width+m_LeftMargin) derror();
    }
    vbits -= rbits;
  }
}

void CLASS nokia_load_raw()
{
  uint8_t  *data,  *dp;
  int rev, dwide, row, col, c;

  rev = 3 * (m_ByteOrder == 0x4949);
  dwide = m_RawWidth * 5 / 4;
  data = (uint8_t *) MALLOC (dwide*2);
  merror (data, "nokia_load_raw()");
  for (row=0; row < m_RawHeight; row++) {
    if (fread (data+dwide, 1, dwide, m_InputFile) < (size_t) dwide) derror();
    for(c=0;c<dwide;c++) data[c] = data[dwide+(c ^ rev)];
    for (dp=data, col=0; col < m_RawWidth; dp+=5, col+=4)
      for (c=0; c<4; c++) RAW(row,col+c) = (dp[c] << 2) | (dp[4] >> (c << 1) & 3);
  }
  FREE (data);
  m_WhiteLevel = 0x3ff;
}

unsigned CLASS pana_bits (int nbits)
{
  int byte;



  if (!nbits) return m_pana_bits_vbits=0;
  if (!m_pana_bits_vbits) {
    ptfread (m_pana_bits_buf+m_Load_Flags, 1, 0x4000-m_Load_Flags, m_InputFile);
    ptfread (m_pana_bits_buf, 1, m_Load_Flags, m_InputFile);
  }
  m_pana_bits_vbits = (m_pana_bits_vbits - nbits) & 0x1ffff;
  byte = m_pana_bits_vbits >> 3 ^ 0x3ff0;
  return (m_pana_bits_buf[byte] | m_pana_bits_buf[byte+1] << 8) >> (m_pana_bits_vbits & 7) & ~(-1 << nbits);
}

void CLASS panasonic_load_raw()
{
  int row, col, i, j, sh=0, pred[2], nonz[2];

  pana_bits(0);
  for (row=0; row < m_Height; row++)
    for (col=0; col < m_RawWidth; col++) {
      if ((i = col % 14) == 0)
  pred[0] = pred[1] = nonz[0] = nonz[1] = 0;
      if (i % 3 == 2) sh = 4 >> (3 - pana_bits(2));
      if (nonz[i & 1]) {
  if ((j = pana_bits(8))) {
    if ((pred[i & 1] -= 0x80 << sh) < 0 || sh == 4)
         pred[i & 1] &= ~(-1 << sh);
    pred[i & 1] += j << sh;
  }
      } else if ((nonz[i & 1] = pana_bits(8)) || i > 11)
  pred[i & 1] = nonz[i & 1] << 4 | pana_bits(4);
      if ((RAW(row,col) = pred[col & 1]) > 4098 && col < m_Width) derror();
    }
}

void CLASS olympus_load_raw()
{
  uint16_t huff[4096];
  int row, col, nbits, sign, low, high, i, c, w, n, nw;
  int acarry[2][3], *carry, pred, diff;

  huff[n=0] = 0xc0c;
  for (i=12; i--; )
    for (c=0;c<(2048>>i);c++) huff[++n] = (i+1) << 8 | i;
  fseek (m_InputFile, 7, SEEK_CUR);
  getbits(-1);
  for (row=0; row < m_Height; row++) {
    memset (acarry, 0, sizeof acarry);
    for (col=0; col < m_RawWidth; col++) {
      carry = acarry[col & 1];
      i = 2 * (carry[2] < 3);
      for (nbits=2+i; (uint16_t) carry[0] >> (nbits+i); nbits++){} ;
      low = (sign = getbits(3)) & 3;
      sign = sign << 29 >> 31;
      if ((high = getbithuff(12,huff)) == 12)
        high = getbits(16-nbits) >> 1;
      carry[0] = (high << nbits) | getbits(nbits);
      diff = (carry[0] ^ sign) + carry[1];
      carry[1] = (diff*3 + carry[1]) >> 5;
      carry[2] = carry[0] > 16 ? 0 : carry[2]+1;
      if (col >= m_Width) continue;
      if (row < 2 && col < 2) pred = 0;
      else if (row < 2) pred = RAW(row,col-2);
      else if (col < 2) pred = RAW(row-2,col);
      else {
  w  = RAW(row,col-2);
  n  = RAW(row-2,col);
  nw = RAW(row-2,col-2);
  if ((w < nw && nw < n) || (n < nw && nw < w)) {
    if (ABS(w-nw) > 32 || ABS(n-nw) > 32)
      pred = w + n - nw;
    else pred = (w + n) >> 1;
  } else pred = ABS(w-nw) > ABS(n-nw) ? w : n;
      }
      if ((RAW(row,col) = pred + ((diff << 2) | low)) >> 12) derror();
    }
  }
}

void CLASS olympus_cseries_load_raw()
{
  int irow, row, col;

  for (irow=0; irow < m_Height; irow++) {
    row = irow * 2 % m_Height + irow / (m_Height/2);
    if (row < 2) {
      fseek (m_InputFile, m_Data_Offset - row*(-m_Width*m_Height*3/4 & -2048), SEEK_SET);
      getbits(-1);
    }
    for (col=0; col < m_Width; col++)
      BAYER(row,col) = getbits(12);
  }
  m_BlackLevel >>= 4;
}

void CLASS minolta_rd175_load_raw()
{
  uint8_t pixel[768];
  unsigned irow, box, row, col;

  for (irow=0; irow < 1481; irow++) {
    if (fread (pixel, 1, 768, m_InputFile) < 768) derror();
    box = irow / 82;
    row = irow % 82 * 12 + ((box < 12) ? box | 1 : (box-12)*2);
    switch (irow) {
      case 1477: case 1479: continue;
      case 1476: row = 984; break;
      case 1480: row = 985; break;
      case 1478: row = 985; box = 1;
    }
    if ((box < 12) && (box & 1)) {
      for (col=0; col < 1533; col++, row ^= 1)
  if (col != 1) RAW(row,col) = (col+1) & 2 ?
       pixel[col/2-1] + pixel[col/2+1] : pixel[col/2] << 1;
      RAW(row,1)    = pixel[1]   << 1;
      RAW(row,1533) = pixel[765] << 1;
    } else
      for (col=row & 1; col < 1534; col+=2)
  RAW(row,col) = pixel[col/2] << 1;
  }
  m_WhiteLevel = 0xff << 1;
}

void CLASS quicktake_100_load_raw()
{
  uint8_t pixel[484][644];
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  static const short rstep[6][4] =
  { {  -3,-1,1,3  }, {  -5,-1,1,5  }, {  -8,-2,2,8  },
    { -13,-3,3,13 }, { -19,-4,4,19 }, { -28,-6,6,28 } };
  int rb, row, col, sharp, val=0;

  getbits(-1);
  memset (pixel, 0x80, sizeof pixel);
  for (row=2; row < m_Height+2; row++) {
    for (col=2+(row & 1); col < m_Width+2; col+=2) {
      val = ((pixel[row-1][col-1] + 2*pixel[row-1][col+1] +
    pixel[row][col-2]) >> 2) + gstep[getbits(4)];
      pixel[row][col] = val = LIM(val,0,255);
      if (col < 4)
  pixel[row][col-2] = pixel[row+1][~row & 1] = val;
      if (row == 2)
  pixel[row-1][col+1] = pixel[row-1][col+3] = val;
    }
    pixel[row][col] = val;
  }
  for (rb=0; rb < 2; rb++)
    for (row=2+rb; row < m_Height+2; row+=2)
      for (col=3-(row & 1); col < m_Width+2; col+=2) {
  if (row < 4 || col < 4) sharp = 2;
  else {
    val = ABS(pixel[row-2][col] - pixel[row][col-2])
        + ABS(pixel[row-2][col] - pixel[row-2][col-2])
        + ABS(pixel[row][col-2] - pixel[row-2][col-2]);
    sharp = val <  4 ? 0 : val <  8 ? 1 : val < 16 ? 2 :
      val < 32 ? 3 : val < 48 ? 4 : 5;
  }
  val = ((pixel[row-2][col] + pixel[row][col-2]) >> 1)
        + rstep[sharp][getbits(2)];
  pixel[row][col] = val = LIM(val,0,255);
  if (row < 4) pixel[row-2][col+2] = val;
  if (col < 4) pixel[row+2][col-2] = val;
      }
  for (row=2; row < m_Height+2; row++)
    for (col=3-(row & 1); col < m_Width+2; col+=2) {
      val = ((pixel[row][col-1] + (pixel[row][col] << 2) +
        pixel[row][col+1]) >> 1) - 0x100;
      pixel[row][col] = LIM(val,0,255);
    }
  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++)
      RAW(row,col) = m_Curve[pixel[row+2][col+2]];
  m_WhiteLevel = 0x3ff;
}

#define radc_token(tree) ((signed char) getbithuff(8,huff[tree]))

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

void CLASS kodak_radc_load_raw()
{
  static const char src[] = {
    1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
    1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
    2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
    2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
    2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
    2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
    2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
    2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
    2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
    2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
    1,0, 2,2, 2,-2,
    1,-3, 1,3,
    2,-17, 2,-5, 2,5, 2,17,
    2,-7, 2,2, 2,9, 2,18,
    2,-18, 2,-9, 2,-2, 2,7,
    2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
    2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
    2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
  };
  uint16_t huff[19][256];
  int row, col, tree, nreps, rep, step, /* i,*/ c, s, r, x, y, val;
  constexpr unsigned int bufi1 = 3;
  constexpr unsigned int bufi2 = 386;
  short last[3] = { 16,16,16 }, mul[3], buf[3][bufi1][bufi2];
  static const uint16_t pt[] =
    { 0,0, 1280,1344, 2320,3616, 3328,8000, 4095,16383, 65535,16383 };
  int i;

  for (i=2; i < 12; i+=2)
    for (c=pt[i-2]; c <= pt[i]; c++)
      m_Curve[c] = (float)
  (c-pt[i-2]) / (pt[i]-pt[i-2]) * (pt[i+1]-pt[i-1]) + pt[i-1] + 0.5;
  for (s=i=0; i < (int)(sizeof(src)); i+=2)
    for(c=0;c<(256>>src[i]);c++)
      huff[0][s++] = src[i] << 8 | (unsigned char) src[i+1];
  s = m_Kodak_cbpp == 243 ? 2 : 3;
  for(c=0;c<256;c++) huff[18][c] = (8-s) << 8 | c >> s << s | 1 << (s-1);
  getbits(-1);
  memset(buf, 2048, sizeof(buf)/sizeof(short));
//  for (unsigned i=0; i < sizeof(buf)/sizeof(short); i++)
//    buf[0][0][i] = 2048;
  for (row=0; row < m_Height; row+=4) {
    for (c=0; c<3; c++) mul[c] = getbits(6);
    for (c=0; c<3; c++) {
      val = ((0x1000000/last[c] + 0x7ff) >> 12) * mul[c];
      s = val > 65564 ? 10:12;
      x = ~(-1 << (s-1));
      val <<= 12-s;
      for (unsigned i=0; i < bufi1; i++)
        for (unsigned j=0; j < bufi2; j++)
          buf[c][i][j] = (buf[c][i][j] * val + x) >> s;
//      for (unsigned i=0; i < sizeof(buf[0])/sizeof(short); i++)
//        buf[c][0][i] = (buf[c][0][i] * val + x) >> s;
      last[c] = mul[c];
      for (r=0; r <= !c; r++) {
  buf[c][1][m_Width/2] = buf[c][2][m_Width/2] = mul[c] << 7;
  for (tree=1, col=m_Width/2; col > 0; ) {
    if ((tree = radc_token(tree))) {
      col -= 2;
      if (tree == 8)
        FORYX buf[c][y][x] = (uint8_t) radc_token(18) * mul[c];
      else
        FORYX buf[c][y][x] = radc_token(tree+10) * 16 + PREDICTOR;
    } else
      do {
        nreps = (col > 2) ? radc_token(9) + 1 : 1;
        for (rep=0; rep < 8 && rep < nreps && col > 0; rep++) {
    col -= 2;
    FORYX buf[c][y][x] = PREDICTOR;
    if (rep & 1) {
      step = radc_token(10) << 4;
      FORYX buf[c][y][x] += step;
    }
        }
      } while (nreps == 9);
  }
  for (y=0; y < 2; y++)
    for (x=0; x < m_Width/2; x++) {
      val = (buf[c][y+1][x] << 4) / mul[c];
      if (val < 0) val = 0;
      if (c) RAW(row+y*2+c-1,x*2+2-c) = val;
      else   RAW(row+r*2+y,x*2+y) = val;
    }
  memcpy (buf[c][0]+!c, buf[c][2], sizeof buf[c][0]-2*!c);
      }
    }
    for (y=row; y < row+4; y++)
      for (x=0; x < m_Width; x++)
  if ((x+y) & 1) {
    r = x ? x-1 : x+1;
    s = x+1 < m_Width ? x+1 : x-1;
    val = (RAW(y,x)-2048)*2 + (RAW(y,r)+RAW(y,s))/2;
    if (val < 0) val = 0;
    RAW(y,x) = val;
  }
  }
  for (i=0; i < m_Height*m_Width; i++)
    m_Raw_Image[i] = m_Curve[m_Raw_Image[i]];

  m_WhiteLevel = 0xfff;
}

#undef FORYX
#undef PREDICTOR

#ifdef NO_JPEG
void CLASS kodak_jpeg_load_raw() {}
void CLASS lossy_dng_load_raw() {}
#else

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
  static uint8_t jpeg_buffer[4096];
  size_t nbytes;
  ptDcRaw *data = (ptDcRaw*) cinfo->client_data;

  nbytes = fread (jpeg_buffer, 1, 4096, data->m_InputFile);
  swab ((char *)jpeg_buffer, (char *)jpeg_buffer, nbytes);
  cinfo->src->next_input_byte = jpeg_buffer;
  cinfo->src->bytes_in_buffer = nbytes;
  return TRUE;
}

void CLASS kodak_jpeg_load_raw()
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPARRAY buf;
  JSAMPLE (*pixel)[3];
  int row, col;

  cinfo.client_data = this;
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  jpeg_stdio_src (&cinfo, m_InputFile);
  cinfo.src->fill_input_buffer = fill_input_buffer;
  jpeg_read_header (&cinfo, TRUE);
  jpeg_start_decompress (&cinfo);
  if ((cinfo.output_width      != m_Width  ) ||
      (cinfo.output_height*2   != m_Height ) ||
      (cinfo.output_components != 3      )) {
    fprintf (stderr,_("%s: incorrect JPEG dimensions\n"), m_UserSetting_InputFileName);
    jpeg_destroy_decompress (&cinfo);
    longjmp (m_Failure, 3);
  }
  buf = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, m_Width*3, 1);

  while (cinfo.output_scanline < cinfo.output_height) {
    row = cinfo.output_scanline * 2;
    jpeg_read_scanlines (&cinfo, buf, 1);
    pixel = (JSAMPLE (*)[3]) buf[0];
    for (col=0; col < m_Width; col+=2) {
      RAW(row+0,col+0) = pixel[col+0][1] << 1;
      RAW(row+1,col+1) = pixel[col+1][1] << 1;
      RAW(row+0,col+1) = pixel[col][0] + pixel[col+1][0];
      RAW(row+1,col+0) = pixel[col][2] + pixel[col+1][2];
    }
  }
  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);
  m_WhiteLevel = 0xff << 1;
}

void CLASS lossy_dng_load_raw()
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPARRAY buf;
  JSAMPLE (*pixel)[3];
  unsigned sm_ByteOrder=m_ByteOrder, ntags, opcode, deg, i, j, c;
  unsigned save=m_Data_Offset-4, trow=0, tcol=0, row, col;
  ushort curve[3][256];
  double coeff[9], tot;

  fseek (m_InputFile, meta_offset, SEEK_SET);
  m_ByteOrder = 0x4d4d;
  ntags = get4();
  while (ntags--) {
    opcode = get4(); get4(); get4();
    if (opcode != 8)
    { fseek (m_InputFile, get4(), SEEK_CUR); continue; }
    fseek (m_InputFile, 20, SEEK_CUR);
    if ((c = get4()) > 2) break;
    fseek (m_InputFile, 12, SEEK_CUR);
    if ((deg = get4()) > 8) break;
    for (i=0; i <= deg && i < 9; i++)
      coeff[i] = getreal(12);
    for (i=0; i < 256; i++) {
      for (tot=j=0; j <= deg; j++)
  tot += coeff[j] * pow(i/255.0, j);
      curve[c][i] = tot*0xffff;
    }
  }
  m_ByteOrder = sm_ByteOrder;
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  while (trow < m_RawHeight) {
    fseek (m_InputFile, save+=4, SEEK_SET);
    if (m_TileLength < INT_MAX)
      fseek (m_InputFile, get4(), SEEK_SET);
    jpeg_stdio_src (&cinfo, m_InputFile);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    buf = (*cinfo.mem->alloc_sarray)
  ((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width*3, 1);
    while (cinfo.output_scanline < cinfo.output_height &&
  (row = trow + cinfo.output_scanline) < m_Height) {
      jpeg_read_scanlines (&cinfo, buf, 1);
      pixel = (JSAMPLE (*)[3]) buf[0];
      for (col=0; col < cinfo.output_width && tcol+col < m_Width; col++) {
  for (c=0; c<3; c++) m_Image[row*m_Width+tcol+col][c] = curve[c][pixel[col][c]];
      }
    }
    jpeg_abort_decompress (&cinfo);
    if ((tcol += m_TileWidth) >= m_RawWidth)
      trow += m_TileLength + (tcol = 0);
  }
  jpeg_destroy_decompress (&cinfo);
  m_WhiteLevel = 0xffff;
}
#endif

void CLASS kodak_dc120_load_raw()
{
  static const int mul[4] = { 162, 192, 187,  92 };
  static const int add[4] = {   0, 636, 424, 212 };
  uint8_t pixel[848];
  int row, shift, col;

  for (row=0; row < m_Height; row++) {
    if (fread (pixel, 1, 848, m_InputFile) < 848) derror();
    shift = row * mul[row & 3] + add[row & 3];
    for (col=0; col < m_Width; col++)
      RAW(row,col) = (uint16_t) pixel[(col + shift) % 848];
  }
  m_WhiteLevel = 0xff;
}

void CLASS eight_bit_load_raw()
{
  uint8_t *pixel;
  unsigned row, col;

  pixel = (uint8_t *) CALLOC (m_RawWidth, sizeof *pixel);
  merror (pixel, "eight_bit_load_raw()");
  for (row=0; row < m_RawHeight; row++) {
    if (fread (pixel, 1, m_RawWidth, m_InputFile) < m_RawWidth) derror();
    for (col=0; col < m_RawWidth; col++)
      RAW(row,col) = m_Curve[pixel[col]];
  }
  FREE (pixel);
  m_WhiteLevel = m_Curve[0xff];
}

void CLASS kodak_yrgb_load_raw()
{
  uint8_t *pixel;
  int row, col, y, cb, cr, rgb[3], c;

  pixel = (uint8_t *) CALLOC (m_RawWidth, 3*sizeof *pixel);
  merror (pixel, "kodak_yrgb_load_raw()");
  for (row=0; row < m_Height; row++) {
    if (~row & 1)
      if (fread (pixel, m_RawWidth, 3, m_InputFile) < 3) derror();
    for (col=0; col < m_RawWidth; col++) {
      y  = pixel[m_Width*2*(row & 1) + col];
      cb = pixel[m_Width + (col & -2)]   - 128;
      cr = pixel[m_Width + (col & -2)+1] - 128;
      rgb[1] = y-((cb + cr + 2) >> 2);
      rgb[2] = rgb[1] + cb;
      rgb[0] = rgb[1] + cr;
      for (c=0;c<3;c++) m_Image[row*m_Width+col][c] = m_Curve[LIM(rgb[c],0,255)];
    }
  }
  FREE (pixel);
  m_WhiteLevel = m_Curve[0xff];
}

void CLASS kodak_262_load_raw()
{
  static const uint8_t kodak_tree[2][26] =
  { { 0,1,5,1,1,2,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9 },
    { 0,3,1,1,1,1,1,2,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9 } };
  uint16_t *huff[2];
  uint8_t *pixel;
  int *strip, ns, c, row, col, chess, pi=0, pi1, pi2, pred, val;
  huff[0] = make_decoder (kodak_tree[0]);
  huff[1] = make_decoder (kodak_tree[1]);
  ns = (m_RawHeight+63) >> 5;
  pixel = (uint8_t *) MALLOC (m_RawWidth*32 + ns*4);
  merror (pixel, "kodak_262_load_raw()");
  strip = (int *) (pixel + m_RawWidth*32);
  m_ByteOrder = 0x4d4d;
  for(c=0;c<ns;c++) strip[c] = get4();
  for (row=0; row < m_RawHeight; row++) {
    if ((row & 31) == 0) {
      fseek (m_InputFile, strip[row >> 5], SEEK_SET);
      getbits(-1);
      pi = 0;
    }
    for (col=0; col < m_RawWidth; col++) {
      chess = (row + col) & 1;
      pi1 = chess ? pi-2           : pi-m_RawWidth-1;
      pi2 = chess ? pi-2*m_RawWidth : pi-m_RawWidth+1;
      if (col <= chess) pi1 = -1;
      if (pi1 < 0) pi1 = pi2;
      if (pi2 < 0) pi2 = pi1;
      if (pi1 < 0 && col > 1) pi1 = pi2 = pi-2;
      pred = (pi1 < 0) ? 0 : (pixel[pi1] + pixel[pi2]) >> 1;
      pixel[pi] = val = pred + ljpeg_diff (huff[chess]);
      if (val >> 8) derror();
      val = m_Curve[pixel[pi++]];
      RAW(row,col) = val;
    }
  }
  FREE (pixel);
  FREE (huff[0]);
  FREE (huff[1]);
}

int CLASS kodak_65000_decode (short *out, int bsize)
{
  uint8_t c, blen[768];
  uint16_t raw[6];
  int64_t bitbuf=0;
  int save, bits=0, i, j, len, diff;

  save = ftell(m_InputFile);
  bsize = (bsize + 3) & -4;
  for (i=0; i < bsize; i+=2) {
    c = fgetc(m_InputFile);
    if ((blen[i  ] = c & 15) > 12 ||
  (blen[i+1] = c >> 4) > 12 ) {
      fseek (m_InputFile, save, SEEK_SET);
      for (i=0; i < bsize; i+=8) {
  read_shorts (raw, 6);
  out[i  ] = raw[0] >> 12 << 8 | raw[2] >> 12 << 4 | raw[4] >> 12;
  out[i+1] = raw[1] >> 12 << 8 | raw[3] >> 12 << 4 | raw[5] >> 12;
  for (j=0; j < 6; j++)
    out[i+2+j] = raw[j] & 0xfff;
      }
      return 1;
    }
  }
  if ((bsize & 7) == 4) {
    bitbuf  = fgetc(m_InputFile) << 8;
    bitbuf += fgetc(m_InputFile);
    bits = 16;
  }
  for (i=0; i < bsize; i++) {
    len = blen[i];
    if (bits < len) {
      for (j=0; j < 32; j+=8)
  bitbuf += (int64_t) fgetc(m_InputFile) << (bits+(j^8));
      bits += 32;
    }
    diff = bitbuf & (0xffff >> (16-len));
    bitbuf >>= len;
    bits -= len;
    if ((diff & (1 << (len-1))) == 0)
      diff -= (1 << len) - 1;
    out[i] = diff;
  }
  return 0;
}

void CLASS kodak_65000_load_raw()
{
  short buf[256];
  int row, col, len, pred[2], ret, i;

  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col+=256) {
      pred[0] = pred[1] = 0;
      len = MIN (256, m_Width-col);
      ret = kodak_65000_decode (buf, len);
      for (i=0; i < len; i++)
  if ((RAW(row,col+i) =	m_Curve[ret ? buf[i] :
    (pred[i & 1] += buf[i])]) >> 12) derror();
    }
}

void CLASS kodak_ycbcr_load_raw()
{
  short buf[384], *bp;
  int row, col, len, c, i, j, k, y[2][2], cb, cr, rgb[3];
  uint16_t *ip;

  for (row=0; row < m_Height; row+=2)
    for (col=0; col < m_Width; col+=128) {
      len = MIN (128, m_Width-col);
      kodak_65000_decode (buf, len*3);
      y[0][1] = y[1][1] = cb = cr = 0;
      for (bp=buf, i=0; i < len; i+=2, bp+=2) {
  cb += bp[4];
  cr += bp[5];
  rgb[1] = -((cb + cr + 2) >> 2);
  rgb[2] = rgb[1] + cb;
  rgb[0] = rgb[1] + cr;
  for (j=0; j < 2; j++)
    for (k=0; k < 2; k++) {
      if ((y[j][k] = y[j][k^1] + *bp++) >> 10) derror();
      ip = m_Image[(row+j)*m_Width + col+i+k];
      for (c=0; c<3; c++) ip[c] = m_Curve[LIM(y[j][k]+rgb[c], 0, 0xfff)];
    }
      }
    }
}

void CLASS kodak_rgb_load_raw()
{
  short buf[768], *bp;
  int row, col, len, c, i, rgb[3];
  uint16_t *ip=m_Image[0];

  if (m_Raw_Image) FREE (m_Raw_Image);
  m_Raw_Image = 0;
  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col+=256) {
      len = MIN (256, m_Width-col);
      kodak_65000_decode (buf, len*3);
      memset (rgb, 0, sizeof rgb);
      for (bp=buf, i=0; i < len; i++, ip+=4)
  for (c=0; c<3; c++) if ((ip[c] = rgb[c] += *bp++) >> 12) derror();
    }
}

void CLASS kodak_thumb_load_raw()
{
  int row, col;
  m_Colors = m_ThumbMisc >> 5;
  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++)
      read_shorts (m_Image[row*m_Width+col], m_Colors);
  m_WhiteLevel = (1 << (m_ThumbMisc & 31)) - 1;
}

void CLASS sony_decrypt (unsigned *data, int len, int start, int key)
{
  if (start) {
    for (m_sony_decrypt_p=0; m_sony_decrypt_p < 4; m_sony_decrypt_p++)
      m_sony_decrypt_pad[m_sony_decrypt_p] = key = key * 48828125 + 1;
    m_sony_decrypt_pad[3] = m_sony_decrypt_pad[3] << 1 | (m_sony_decrypt_pad[0]^m_sony_decrypt_pad[2]) >> 31;
    for (m_sony_decrypt_p=4; m_sony_decrypt_p < 127; m_sony_decrypt_p++)
      m_sony_decrypt_pad[m_sony_decrypt_p] = (m_sony_decrypt_pad[m_sony_decrypt_p-4]^m_sony_decrypt_pad[m_sony_decrypt_p-2]) << 1 | (m_sony_decrypt_pad[m_sony_decrypt_p-3]^m_sony_decrypt_pad[m_sony_decrypt_p-1]) >> 31;
    for (m_sony_decrypt_p=0; m_sony_decrypt_p < 127; m_sony_decrypt_p++)
      m_sony_decrypt_pad[m_sony_decrypt_p] = htonl(m_sony_decrypt_pad[m_sony_decrypt_p]);
  }
  while (len--)
    *data++ ^= m_sony_decrypt_pad[m_sony_decrypt_p++ & 127] = m_sony_decrypt_pad[(m_sony_decrypt_p+1) & 127] ^ m_sony_decrypt_pad[(m_sony_decrypt_p+65) & 127];
}

void CLASS sony_load_raw()
{
  uint8_t head[40];
  uint16_t *pixel;
  unsigned i, key; // row, col;
  uint16_t row,col;

  fseek (m_InputFile, 200896, SEEK_SET);
  fseek (m_InputFile, (unsigned) fgetc(m_InputFile)*4 - 1, SEEK_CUR);
  m_ByteOrder = 0x4d4d;
  key = get4();
  fseek (m_InputFile, 164600, SEEK_SET);
  ptfread (head, 1, 40, m_InputFile);
  sony_decrypt ((unsigned int *) head, 10, 1, key);
  for (i=26; i-- > 22; )
    key = key << 8 | head[i];
  fseek (m_InputFile, m_Data_Offset, SEEK_SET);
  for (row=0; row < m_RawHeight; row++) {
    pixel = m_Raw_Image + row*m_RawWidth;
    if (fread (pixel, 2, m_RawWidth, m_InputFile) < m_RawWidth) derror();
    sony_decrypt ((unsigned int *) pixel, m_RawWidth/2, !row, key);
    for (col=0; col < m_RawWidth; col++)
      if ((pixel[col] = ntohs(pixel[col])) >> 14) derror();
  }
  m_WhiteLevel = 0x3ff0;
}

void CLASS sony_arw_load_raw()
{
  uint16_t huff[32768];
  static const uint16_t tab[18] =
  { 0xf11,0xf10,0xe0f,0xd0e,0xc0d,0xb0c,0xa0b,0x90a,0x809,
    0x708,0x607,0x506,0x405,0x304,0x303,0x300,0x202,0x201 };
  int i, c, n, col, row, len, diff, sum=0;

  for (n=i=0; i < 18; i++)
    for (c=0;c<(32768 >> (tab[i] >> 8));c++) huff[n++] = tab[i];

  getbits(-1);
  for (col = m_RawWidth; col--; )
    for (row=0; row < m_RawHeight+1; row+=2) {
      if (row == m_RawHeight) row = 1;
      len = getbithuff(15,huff);
      diff = getbits(len);
      if ((diff & (1 << (len-1))) == 0)
  diff -= (1 << len) - 1;
      if ((sum += diff) >> 12) derror();
      if (row < m_Height) RAW(row,col) = sum;
    }
}

void CLASS sony_arw2_load_raw()
{
  uint8_t *data, *dp;
  uint16_t pix[16];
  int row, col, val, max, min, imax, imin, sh, bit, i;

  data = (uint8_t *) MALLOC (m_RawWidth);
  merror (data, "sony_arw2_load_raw()");
  for (row=0; row < m_Height; row++) {
    ptfread (data, 1, m_RawWidth, m_InputFile);
    for (dp=data, col=0; col < m_RawWidth-30; dp+=16) {
      max = 0x7ff & (val = sget4(dp));
      min = 0x7ff & val >> 11;
      imax = 0x0f & val >> 22;
      imin = 0x0f & val >> 26;
      for (sh=0; sh < 4 && 0x80 << sh <= max-min; sh++) {};
      for (bit=30, i=0; i < 16; i++)
  if      (i == imax) pix[i] = max;
  else if (i == imin) pix[i] = min;
  else {
    pix[i] = ((sget2(dp+(bit >> 3)) >> (bit & 7) & 0x7f) << sh) + min;
    if (pix[i] > 0x7ff) pix[i] = 0x7ff;
    bit += 7;
  }
      for (i=0; i < 16; i++, col+=2)
	RAW(row,col) = m_Curve[pix[i] << 1] >> 2;
      col -= col & 1 ? 1:31;
    }
  }
  FREE (data);
}

#define HOLE(row) ((holes >> (((row) - m_RawHeight) & 7)) & 1)

/* Kudos to Rich Taylor for figuring out SMaL's compression algorithm. */
void CLASS smal_decode_segment (unsigned seg[2][2], int holes)
{
  uint8_t hist[3][13] = {
    { 7, 7, 0, 0, 63, 55, 47, 39, 31, 23, 15, 7, 0 },
    { 7, 7, 0, 0, 63, 55, 47, 39, 31, 23, 15, 7, 0 },
    { 3, 3, 0, 0, 63,     47,     31,     15,    0 } };
  int low, high=0xff, carry=0, nbits=8;
  int pix, s, count, bin, next, i, sym[3];
  uint8_t diff, pred[]={0,0};
  uint16_t data=0, range=0;

  fseek (m_InputFile, seg[0][1]+1, SEEK_SET);
  getbits(-1);
  for (pix=seg[0][0]; pix < (int32_t)seg[1][0]; pix++) {
    for (s=0; s < 3; s++) {
      data = data << nbits | getbits(nbits);
      if (carry < 0)
  carry = (nbits += carry+1) < 1 ? nbits-1 : 0;
      while (--nbits >= 0)
  if ((data >> nbits & 0xff) == 0xff) break;
      if (nbits > 0)
    data = ((data & ((1 << (nbits-1)) - 1)) << 1) |
  ((data + (((data & (1 << (nbits-1)))) << 1)) & (-1 << nbits));
      if (nbits >= 0) {
  data += getbits(1);
  carry = nbits - 8;
      }
      count = ((((data-range+1) & 0xffff) << 2) - 1) / (high >> 4);
      for (bin=0; hist[s][bin+5] > count; bin++) {} ;
    low = hist[s][bin+5] * (high >> 4) >> 2;
      if (bin) high = hist[s][bin+4] * (high >> 4) >> 2;
      high -= low;
      for (nbits=0; high << nbits < 128; nbits++) {} ;
      range = (range+low) << nbits;
      high <<= nbits;
      next = hist[s][1];
      if (++hist[s][2] > hist[s][3]) {
        next = (next+1) & hist[s][0];
        hist[s][3] = (hist[s][next+4] - hist[s][next+5]) >> 2;
        hist[s][2] = 1;
      }
      if (hist[s][hist[s][1]+4] - hist[s][hist[s][1]+5] > 1) {
  if (bin < hist[s][1]) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    for (i=bin; i < hist[s][1]; i++) hist[s][i+5]--;
#pragma GCC diagnostic pop
  } else if (next <= bin)
    for (i=hist[s][1]; i < bin; i++) hist[s][i+5]++;
      }
      hist[s][1] = next;
      sym[s] = bin;
    }
    diff = sym[2] << 5 | sym[1] << 2 | (sym[0] & 3);
    if (sym[0] & 4)
      diff = diff ? -diff : 0x80;
    if (unsigned(ftell(m_InputFile) + 12) >= seg[1][1])
      diff = 0;
    m_Raw_Image[pix] = pred[pix & 1] += diff;
    if (!(pix & 1) && HOLE(pix / m_RawWidth)) pix += 2;
  }
  m_WhiteLevel = 0xff;
}

void CLASS smal_v6_load_raw()
{
  unsigned seg[2][2];

  fseek (m_InputFile, 16, SEEK_SET);
  seg[0][0] = 0;
  seg[0][1] = get2();
  seg[1][0] = m_RawWidth * m_RawHeight;
  seg[1][1] = INT_MAX;
  smal_decode_segment (seg, 0);
}

int CLASS median4 (int *p)
{
  int min, max, sum, i;

  min = max = sum = p[0];
  for (i=1; i < 4; i++) {
    sum += p[i];
    if (min > p[i]) min = p[i];
    if (max < p[i]) max = p[i];
  }
  return (sum - min - max) >> 1;
}

void CLASS fill_holes (int holes)
{
  int row, col, val[4];

  for (row=2; row < m_Height-2; row++) {
    if (!HOLE(row)) continue;
    for (col=1; col < m_Width-1; col+=4) {
      val[0] = RAW(row-1,col-1);
      val[1] = RAW(row-1,col+1);
      val[2] = RAW(row+1,col-1);
      val[3] = RAW(row+1,col+1);
      RAW(row,col) = median4(val);
    }
    for (col=2; col < m_Width-2; col+=4)
      if (HOLE(row-2) || HOLE(row+2))
  RAW(row,col) = (RAW(row,col-2) + RAW(row,col+2)) >> 1;
      else {
  val[0] = RAW(row,col-2);
  val[1] = RAW(row,col+2);
  val[2] = RAW(row-2,col);
  val[3] = RAW(row+2,col);
  RAW(row,col) = median4(val);
      }
  }
}

void CLASS smal_v9_load_raw()
{
  unsigned seg[256][2], offset, nseg, holes, i;

  fseek (m_InputFile, 67, SEEK_SET);
  offset = get4();
  nseg = fgetc(m_InputFile);
  fseek (m_InputFile, offset, SEEK_SET);
  for (i=0; i < nseg*2; i++)
    seg[0][i] = get4() + m_Data_Offset*(i & 1);
  fseek (m_InputFile, 78, SEEK_SET);
  holes = fgetc(m_InputFile);
  fseek (m_InputFile, 88, SEEK_SET);
  seg[nseg][0] = m_RawHeight * m_RawWidth;
  seg[nseg][1] = get4() + m_Data_Offset;
  for (i=0; i < nseg; i++)
    smal_decode_segment (seg+i, holes);
  if (holes) fill_holes (holes);
}

void CLASS redcine_load_raw()
{
#ifndef NO_JASPER
  int c, row, col;
  jas_stream_t *in;
  jas_m_Image_t *jimg;
  jas_matrix_t *jmat;
  jas_seqent_t *data;
  uint16_t *img, *pix;

  jas_init();
  in = jas_stream_fopen (ifname, "rb");
  jas_stream_seek (in, m_Data_Offset+20, SEEK_SET);
  jimg = jas_m_Image_decode (in, -1, 0);
  if (!jimg) longjmp (failure, 3);
  jmat = jas_matrix_create (m_Height/2, m_Width/2);
  merror (jmat, "redcine_m_load_raw()");
  img = (uint16_t *) calloc ((m_Height+2)*(m_Width+2), 2);
  merror (img, "redcine_m_load_raw()");
  for (c=0; c<4; c++) {
    jas_m_Image_readcmpt (jimg, c, 0, 0, m_Width/2, m_Height/2, jmat);
    data = jas_matrix_getref (jmat, 0, 0);
    for (row = c >> 1; row < m_Height; row+=2)
      for (col = c & 1; col < m_Width; col+=2)
  img[(row+1)*(m_Width+2)+col+1] = data[(row/2)*(m_Width/2)+col/2];
  }
  for (col=1; col <= m_Width; col++) {
    img[col] = img[2*(m_Width+2)+col];
    img[(m_Height+1)*(m_Width+2)+col] = img[(m_Height-1)*(m_Width+2)+col];
  }
  for (row=0; row < m_Height+2; row++) {
    img[row*(m_Width+2)] = img[row*(m_Width+2)+2];
    img[(row+1)*(m_Width+2)-1] = img[(row+1)*(m_Width+2)-3];
  }
  for (row=1; row <= m_Height; row++) {
    pix = img + row*(m_Width+2) + (col = 1 + (FC(row,1) & 1));
    for (   ; col <= m_Width; col+=2, pix+=2) {
      c = (((pix[0] - 0x800) << 3) +
  pix[-(m_Width+2)] + pix[m_Width+2] + pix[-1] + pix[1]) >> 2;
      pix[0] = LIM(c,0,4095);
    }
  }
  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++)
      RAW(row,col) = m_Curve[img[(row+1)*(m_Width+2)+col+1]];
  free (img);
  jas_matrix_destroy (jmat);
  jas_m_Image_destroy (jimg);
  jas_stream_close (in);
#endif
}

/* RESTRICTED code starts here */

void CLASS foveon_decoder (unsigned size, unsigned code)
{
  struct decode *cur;
  // int i, len;
  unsigned i,len;

  if (!code) {
    for (unsigned i=0; i < size; i++)
      m_foveon_decoder_huff[i] = get4();
    memset (first_decode, 0, sizeof first_decode);
    free_decode = first_decode;
  }
  cur = free_decode++;
  if (free_decode > first_decode+2048) {
    fprintf (stderr,_("%s: decoder table overflow\n"), m_UserSetting_InputFileName);
    longjmp (m_Failure, 2);
  }
  if (code)
    for (i=0; i < size; i++)
      if (m_foveon_decoder_huff[i] == code) {
  cur->leaf = i;
  return;
      }
  if ((len = code >> 27) > 26) return;
  code = (len+1) << 27 | (code & 0x3ffffff) << 1;

  cur->branch[0] = free_decode;
  foveon_decoder (size, code);
  cur->branch[1] = free_decode;
  foveon_decoder (size, code+1);
}

void CLASS foveon_thumb ()
/* Remember: This function is modified to write the raw’s thumbnail to the
   m_Thumb instead of a file on disk. Always access thumbnails via DcRaw::thumbnail()!
*/
// TODO: Tests needed: What format is this? Can it be read by QPixmap?
{
  unsigned bwide, row, col, bitbuf=0, bit=1, c, i;
  char *buf;
  struct decode *dindex;
  short pred[3];

  bwide = get4();
  //fprintf (m_OutputFile, "P6\n%d %d\n255\n", m_ThumbWidth, m_ThumbHeight);
  QString dummy = QString("P6\n%1 %2\n255\n").arg(m_ThumbWidth).arg(m_ThumbHeight);
  VAppend(m_Thumb, dummy.toLocal8Bit().data(), dummy.length());
  if (bwide > 0) {
    if (bwide < (unsigned)(m_ThumbWidth*3)) return;
    buf = (char *) MALLOC (bwide);
    merror (buf, "foveon_thumb()");
    for (row=0; row < m_ThumbHeight; row++) {
      ptfread  (buf, 1, bwide, m_InputFile);
      //ptfwrite (buf, 3, m_ThumbWidth, m_OutputFile);
      VAppend(m_Thumb, buf, 3 * m_ThumbWidth);
    }
    FREE (buf);
    return;
  }
  foveon_decoder (256, 0);

  for (row=0; row < m_ThumbHeight; row++) {
    memset (pred, 0, sizeof pred);
    if (!bit) get4();
    for (bit=col=0; col < m_ThumbWidth; col++)
      for (c=0; c<3; c++) {
  for (dindex=first_decode; dindex->branch[0]; ) {
    if ((bit = (bit-1) & 31) == 31)
      for (i=0; i < 4; i++)
        bitbuf = (bitbuf << 8) + fgetc(m_InputFile);
    dindex = dindex->branch[bitbuf >> bit & 1];
  }
  pred[c] += dindex->leaf;
  //fputc (pred[c], m_OutputFile);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  m_Thumb.push_back(pred[c]);
#pragma GCC diagnostic pop
      }
  }
}

void CLASS foveon_sd_load_raw()
{
  struct decode *dindex;
  short diff[1024];
  unsigned bitbuf=0;
  int pred[3], row, col, bit=-1, c, i;

  read_shorts ((uint16_t *) diff, 1024);
  if (!m_Load_Flags) foveon_decoder (1024, 0);

  for (row=0; row < m_Height; row++) {
    memset (pred, 0, sizeof pred);
    if (!bit && !m_Load_Flags && atoi(m_CameraModel+2) < 14) get4();
    for (col=bit=0; col < m_Width; col++) {
      if (m_Load_Flags) {
  bitbuf = get4();
  for (c=0; c<3; c++) pred[2-c] += diff[bitbuf >> c*10 & 0x3ff];
      }
      else for (c=0; c<3; c++) {
  for (dindex=first_decode; dindex->branch[0]; ) {
    if ((bit = (bit-1) & 31) == 31)
      for (i=0; i < 4; i++)
        bitbuf = (bitbuf << 8) + fgetc(m_InputFile);
    dindex = dindex->branch[bitbuf >> bit & 1];
  }
  pred[c] += diff[dindex->leaf];
  if (pred[c] >> 16 && ~pred[c] >> 16) derror();
      }
      for (c=0; c<3; c++) m_Image[row*m_Width+col][c] = pred[c];
    }
  }
}

void CLASS foveon_huff (uint16_t *huff)
{
  int i, j, clen, code;

  huff[0] = 8;
  for (i=0; i < 13; i++) {
    clen = getc(m_InputFile);
    code = getc(m_InputFile);
    for (j=0; j < 256 >> clen; )
      huff[code+ ++j] = clen << 8 | i;
  }
  get2();
}

void CLASS foveon_dp_load_raw()
{
  unsigned c, roff[4], row, col, diff;
  uint16_t huff[258], vpred[2][2], hpred[2];

  fseek (m_InputFile, 8, SEEK_CUR);
  foveon_huff (huff);
  roff[0] = 48;
  for (c=0; c<3; c++) roff[c+1] = -(-(roff[c] + get4()) & -16);
  for (c=0; c<3; c++) {
    fseek (m_InputFile, m_Data_Offset+roff[c], SEEK_SET);
    getbits(-1);
    vpred[0][0] = vpred[0][1] = vpred[1][0] = vpred[1][1] = 512;
    for (row=0; row < m_Height; row++) {
      for (col=0; col < m_Width; col++) {
  diff = ljpeg_diff(huff);
  if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
  else hpred[col & 1] += diff;
  m_Image[row*m_Width+col][c] = hpred[col & 1];
      }
    }
  }
}

void CLASS foveon_load_camf()
{
  unsigned type, wide, high, i, j, row, col, diff;
  uint16_t huff[258], vpred[2][2] = {{512,512},{512,512}}, hpred[2];

  fseek (m_InputFile, meta_offset, SEEK_SET);
  type = get4();  get4();  get4();
  wide = get4();
  high = get4();
  if (type == 2) {
    ptfread (m_MetaData, 1, m_MetaLength, m_InputFile);
    for (i=0; i < m_MetaLength; i++) {
      high = (high * 1597 + 51749) % 244944;
      wide = high * (INT64) 301593171 >> 24;
      m_MetaData[i] ^= ((((high << 8) - wide) >> 1) + wide) >> 17;
    }
  } else if (type == 4) {
    FREE (m_MetaData);
    m_MetaData = (char *) MALLOC (m_MetaLength = wide*high*3/2);
    merror (m_MetaData, "foveon_load_camf()");
    foveon_huff (huff);
    get4();
    getbits(-1);
    for (j=row=0; row < high; row++) {
      for (col=0; col < wide; col++) {
  diff = ljpeg_diff(huff);
  if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
  else         hpred[col & 1] += diff;
  if (col & 1) {
    m_MetaData[j++] = hpred[0] >> 4;
    m_MetaData[j++] = hpred[0] << 4 | hpred[1] >> 8;
    m_MetaData[j++] = hpred[1];
        }
      }
    }
  } else
    fprintf (stderr,_("%s has unknown CAMF type %d.\n"), m_UserSetting_InputFileName, type);
}

const char * CLASS foveon_camf_param (const char *block, const char *param)
{
  unsigned idx, num;
  char *pos, *cp, *dp;

  for (idx=0; idx < m_MetaLength; idx += sget4(pos+8)) {
    pos = m_MetaData + idx;
    if (strncmp (pos, "CMb", 3)) break;
    if (pos[3] != 'P') continue;
    if (strcmp (block, pos+sget4(pos+12))) continue;
    cp = pos + sget4(pos+16);
    num = sget4(cp);
    dp = pos + sget4(cp+4);
    while (num--) {
      cp += 8;
      if (!strcmp (param, dp+sget4(cp)))
  return dp+sget4(cp+4);
    }
  }
  return 0;
}

void * CLASS foveon_camf_matrix (unsigned dim[3], const char *name)
{
  unsigned i, idx, type, ndim, size, *mat;
  char *pos, *cp, *dp;
  double dsize;

  for (idx=0; idx < m_MetaLength; idx += sget4(pos+8)) {
    pos = m_MetaData + idx;
    if (strncmp (pos, "CMb", 3)) break;
    if (pos[3] != 'M') continue;
    if (strcmp (name, pos+sget4(pos+12))) continue;
    dim[0] = dim[1] = dim[2] = 1;
    cp = pos + sget4(pos+16);
    type = sget4(cp);
    if ((ndim = sget4(cp+4)) > 3) break;
    dp = pos + sget4(cp+8);
    for (i=ndim; i--; ) {
      cp += 12;
      dim[i] = sget4(cp);
    }
    if ((dsize = (double) dim[0]*dim[1]*dim[2]) > m_MetaLength/4) break;
    mat = (unsigned *) MALLOC ((size = (unsigned) dsize) * 4);
    merror (mat, "foveon_camf_matrix()");
    for (i=0; i < size; i++)
      if (type && type != 6)
  mat[i] = sget4(dp + i*4);
      else
  mat[i] = sget4(dp + i*2) & 0xffff;
    return mat;
  }
  fprintf (stderr,_("%s: \"%s\" matrix not found!\n"), m_UserSetting_InputFileName, name);
  return 0;
}

int CLASS foveon_fixed (void *ptr, int size, const char *name)
{
  void *dp;
  unsigned dim[3];

  if (!name) return 0;
  dp = foveon_camf_matrix (dim, name);
  if (!dp) return 0;
  memcpy (ptr, dp, size*4);
  FREE (dp);
  return 1;
}

float CLASS foveon_avg (short *pix, int range[2], float cfilt)
{
  int i;
  float val, min=FLT_MAX, max=-FLT_MAX, sum=0;

  for (i=range[0]; i <= range[1]; i++) {
    sum += val = pix[i*4] + (pix[i*4]-pix[(i-1)*4]) * cfilt;
    if (min > val) min = val;
    if (max < val) max = val;
  }
  if (range[1] - range[0] == 1) return sum/2;
  return (sum - min - max) / (range[1] - range[0] - 1);
}

short * CLASS foveon_make_curve (double max, double mul, double filt)
{
  short *curve;
  unsigned i, size;
  double x;

  if (!filt) filt = 0.8;
  size = (unsigned) (4*ptPI*max / filt);
  if (size == UINT_MAX) size--;
  curve = (short *) CALLOC (size+1, sizeof *curve);
  merror (curve, "foveon_make_curve()");
  curve[0] = size;
  for (i=0; i < size; i++) {
    x = i*filt/max/4;
    curve[i+1] = (short) ((cos(x)+1)/2 * tanh(i*filt/mul) * mul + 0.5);
  }
  return curve;
}

void CLASS foveon_make_curves
  (short **curvep, float dq[3], float div[3], float filt)
{
  double mul[3], max=0;
  int c;

  for (c=0; c<3; c++) mul[c] = dq[c]/div[c];
  for (c=0; c<3; c++) if (max < mul[c]) max = mul[c];
  for (c=0; c<3; c++) curvep[c] = foveon_make_curve (max, mul[c], filt);
}

int CLASS foveon_apply_curve (short *l_Curve, int i)
{
  if (abs(i) >= l_Curve[0]) return 0;
  return i < 0 ? -l_Curve[1-i] : l_Curve[1+i];
}

#define m_Image ((short (*)[4]) m_Image)

void CLASS foveon_interpolate()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
  static const short hood[] = { -1,-1, -1,0, -1,1, 0,-1, 0,1, 1,-1, 1,0, 1,1 };
  short *pix, prev[3], *l_curve[8], (*shrink)[3];
  float cfilt=0, ddft[3][3][2], ppm[3][3][3];
  float cam_xyz[3][3], correct[3][3], last[3][3], trans[3][3];
  float chroma_dq[3], color_dq[3], diag[3][3], div[3];
  float (*black)[3], (*sgain)[3], (*sgrow)[3];
  float fsum[3], val, frow, num;
  int row, col, c, i, j, diff, sgx, irow, sum, min, max, limit;
  int dscr[2][2], dstb[4], (*smrow[7])[3], total[4], ipix[3];
  int work[3][3], smlast, smred, smred_p=0, dev[3];
  int satlev[3], keep[4], active[4];
  unsigned dim[3], *badpix;
  double dsum=0, trsum[3];
  char str[128];
  const char* cp;

  memset(cam_xyz, 0, sizeof(cam_xyz));
  memset(dscr,    0, sizeof(dscr));
  memset(dstb,    0, sizeof(dstb));
  memset(active,  0, sizeof(active));
  memset(keep,    0, sizeof(keep));

  TRACEKEYVALS("Foveon interpolation","%s","");

  foveon_load_camf();
  foveon_fixed (dscr, 4, "DarkShieldColRange");
  foveon_fixed (ppm[0][0], 27, "PostPolyMatrix");
  foveon_fixed (satlev, 3, "SaturationLevel");
  foveon_fixed (keep, 4, "KeepImageArea");
  foveon_fixed (active, 4, "ActiveImageArea");
  foveon_fixed (chroma_dq, 3, "ChromaDQ");
  foveon_fixed (color_dq, 3,
  foveon_camf_param ("IncludeBlocks", "ColorDQ") ?
    "ColorDQ" : "ColorDQCamRGB");
  if (foveon_camf_param ("IncludeBlocks", "ColumnFilter"))
       foveon_fixed (&cfilt, 1, "ColumnFilter");

  memset (ddft, 0, sizeof ddft);
  if (!foveon_camf_param ("IncludeBlocks", "DarkDrift")
   || !foveon_fixed (ddft[1][0], 12, "DarkDrift"))
    for (i=0; i < 2; i++) {
      foveon_fixed (dstb, 4, i ? "DarkShieldBottom":"DarkShieldTop");
      for (row = dstb[1]; row <= dstb[3]; row++)
  for (col = dstb[0]; col <= dstb[2]; col++)
    for (c=0; c<3; c++) ddft[i+1][c][1] += (short) m_Image[row*m_Width+col][c];
      for (c=0; c<3; c++) ddft[i+1][c][1] /= (dstb[3]-dstb[1]+1) * (dstb[2]-dstb[0]+1);
    }

  if (!(cp = foveon_camf_param ("WhiteBalanceIlluminants", m_CameraModelBis)))
  { fprintf (stderr,_("%s: Invalid white balance \"%s\"\n"), m_UserSetting_InputFileName, m_CameraModelBis);
    return; }
  foveon_fixed (cam_xyz, 9, cp);
  foveon_fixed (correct, 9,
  foveon_camf_param ("WhiteBalanceCorrections", m_CameraModelBis));
  memset (last, 0, sizeof last);
  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      for (c=0; c<3; c++) last[i][j] += correct[i][c] * cam_xyz[c][j];

  #define LAST(x,y) last[(i+x)%3][(c+y)%3]
  for (i=0; i < 3; i++)
    for (c=0; c<3; c++) diag[c][i] = LAST(1,1)*LAST(2,2) - LAST(1,2)*LAST(2,1);
  #undef LAST
  for (c=0; c<3; c++) div[c] = diag[c][0]*0.3127 + diag[c][1]*0.329 + diag[c][2]*0.3583;
  sprintf (str, "%sRGBNeutral", m_CameraModelBis);
  if (foveon_camf_param ("IncludeBlocks", str))
    foveon_fixed (div, 3, str);
  num = 0;
  for (c=0; c<3; c++) if (num < div[c]) num = div[c];
  for (c=0; c<3; c++) div[c] /= num;

  memset (trans, 0, sizeof trans);
  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      for (c=0; c<3; c++) trans[i][j] += m_MatrixCamRGBToSRGB[i][c] * last[c][j] * div[j];
  for (c=0; c<3; c++) trsum[c] = trans[c][0] + trans[c][1] + trans[c][2];
  dsum = (6*trsum[0] + 11*trsum[1] + 3*trsum[2]) / 20;
  for (i=0; i < 3; i++)
    for (c=0; c<3; c++) last[i][c] = trans[i][c] * dsum / trsum[i];
  memset (trans, 0, sizeof trans);
  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      for (c=0; c<3; c++) trans[i][j] += (i==c ? 32 : -1) * last[c][j] / 30;

  foveon_make_curves (l_curve, color_dq, div, cfilt);
  for (c=0; c<3; c++) chroma_dq[c] /= 3;
  foveon_make_curves (l_curve+3, chroma_dq, div, cfilt);
  for (c=0; c<3; c++) dsum += chroma_dq[c] / div[c];
  l_curve[6] = foveon_make_curve (dsum, dsum, cfilt);
  l_curve[7] = foveon_make_curve (dsum*2, dsum*2, cfilt);

  sgain = (float (*)[3]) foveon_camf_matrix (dim, "SpatialGain");
  if (!sgain) return;
  sgrow = (float (*)[3]) CALLOC (dim[1], sizeof *sgrow);
  sgx = (m_Width + dim[1]-2) / (dim[1]-1);

  black = (float (*)[3]) CALLOC (m_Height, sizeof *black);
  for (row=0; row < m_Height; row++) {
    for (i=0; i < 6; i++)
      ddft[0][0][i] = ddft[1][0][i] +
  row / (m_Height-1.0) * (ddft[2][0][i] - ddft[1][0][i]);
    for (c=0; c<3; c++) black[row][c] =
  ( foveon_avg (m_Image[row*m_Width]+c, dscr[0], cfilt) +
    foveon_avg (m_Image[row*m_Width]+c, dscr[1], cfilt) * 3
    - ddft[0][c][0] ) / 4 - ddft[0][c][1];
  }
  memcpy (black, black+8, sizeof *black*8);
  memcpy (black+m_Height-11, black+m_Height-22, 11*sizeof *black);
  memcpy (last, black, sizeof last);

  for (row=1; row < m_Height-1; row++) {
    for (c=0; c<3; c++) if (last[1][c] > last[0][c]) {
  if (last[1][c] > last[2][c])
    black[row][c] = (last[0][c] > last[2][c]) ? last[0][c]:last[2][c];
      } else
  if (last[1][c] < last[2][c])
    black[row][c] = (last[0][c] < last[2][c]) ? last[0][c]:last[2][c];
    memmove (last, last+1, 2*sizeof last[0]);
    memcpy (last[2], black[row+1], sizeof last[2]);
  }
  for (c=0; c<3; c++) black[row][c] = (last[0][c] + last[1][c])/2;
  for (c=0; c<3; c++) black[0][c] = (black[1][c] + black[3][c])/2;

  val = 1 - exp(-1/24.0);
  memcpy (fsum, black, sizeof fsum);
  for (row=1; row < m_Height; row++)
    for (c=0; c<3; c++) fsum[c] += black[row][c] =
  (black[row][c] - black[row-1][c])*val + black[row-1][c];
  memcpy (last[0], black[m_Height-1], sizeof last[0]);
  for (c=0; c<3; c++) fsum[c] /= m_Height;
  for (row = m_Height; row--; )
    for (c=0; c<3; c++) last[0][c] = black[row][c] =
  (black[row][c] - fsum[c] - last[0][c])*val + last[0][c];

  memset (total, 0, sizeof total);
  for (row=2; row < m_Height; row+=4)
    for (col=2; col < m_Width; col+=4) {
      for (c=0; c<3; c++) total[c] += (short) m_Image[row*m_Width+col][c];
      total[3]++;
    }
  for (row=0; row < m_Height; row++)
    for (c=0; c<3; c++) black[row][c] += fsum[c]/2 + total[c]/(total[3]*100.0);

  for (row=0; row < m_Height; row++) {
    for (i=0; i < 6; i++)
      ddft[0][0][i] = ddft[1][0][i] +
  row / (m_Height-1.0) * (ddft[2][0][i] - ddft[1][0][i]);
    pix = m_Image[row*m_Width];
    memcpy (prev, pix, sizeof prev);
    frow = row / (m_Height-1.0) * (dim[2]-1);
    if ((unsigned)(irow = (int)(frow)) == dim[2]-1) irow--;
    frow -= irow;
    for (i=0; (unsigned) i < dim[1]; i++)
      for (c=0; c<3; c++) sgrow[i][c] = sgain[ irow   *dim[1]+i][c] * (1-frow) +
        sgain[(irow+1)*dim[1]+i][c] *    frow;
    for (col=0; col < m_Width; col++) {
      for (c=0; c<3; c++) {
  diff = pix[c] - prev[c];
  prev[c] = pix[c];
  ipix[c] = (int)(pix[c] + floor ((diff + (diff*diff >> 14)) * cfilt
    - ddft[0][c][1] - ddft[0][c][0] * ((float) col/m_Width - 0.5)
    - black[row][c] ));
      }
      for (c=0; c<3; c++) {
  work[0][c] = ipix[c] * ipix[c] >> 14;
  work[2][c] = ipix[c] * work[0][c] >> 14;
  work[1][2-c] = ipix[(c+1) % 3] * ipix[(c+2) % 3] >> 14;
      }
      for (c=0; c<3; c++) {
  for (val=i=0; i < 3; i++)
    for (  j=0; j < 3; j++)
      val += ppm[c][i][j] * work[i][j];
  ipix[c] = (int) (floor ((ipix[c] + floor(val)) *
    ( sgrow[col/sgx  ][c] * (sgx - col%sgx) +
      sgrow[col/sgx+1][c] * (col%sgx) ) / sgx / div[c]));
  if (ipix[c] > 32000) ipix[c] = 32000;
  pix[c] = ipix[c];
      }
      pix += 4;
    }
  }
  FREE (black);
  FREE (sgrow);
  FREE (sgain);

  if ((badpix = (unsigned int *) foveon_camf_matrix (dim, "BadPixels"))) {
    for (i=0; (unsigned) i < dim[0]; i++) {
      col = (badpix[i] >> 8 & 0xfff) - keep[0];
      row = (badpix[i] >> 20       ) - keep[1];
      if ((unsigned)(row-1) > (unsigned)(m_Height-3) || (unsigned)(col-1) > (unsigned)(m_Width-3))
  continue;
      memset (fsum, 0, sizeof fsum);
      for (sum=j=0; j < 8; j++)
  if (badpix[i] & (1 << j)) {
    for (c=0; c<3; c++) fsum[c] += (short)
    m_Image[(row+hood[j*2])*m_Width+col+hood[j*2+1]][c];
    sum++;
  }
      if (sum) for (c=0; c<3; c++) m_Image[row*m_Width+col][c] = (short) (fsum[c]/sum);
    }
    FREE (badpix);
  }

  /* Array for 5x5 Gaussian averaging of red values */
  smrow[6] = (int (*)[3]) CALLOC (m_Width*5, sizeof **smrow);
  merror (smrow[6], "foveon_interpolate()");
  for (i=0; i < 5; i++)
    smrow[i] = smrow[6] + i*m_Width;

  /* Sharpen the reds against these Gaussian averages */
  for (smlast=-1, row=2; row < m_Height-2; row++) {
    while (smlast < row+2) {
      for (i=0; i < 6; i++)
  smrow[(i+5) % 6] = smrow[i];
      pix = m_Image[++smlast*m_Width+2];
      for (col=2; col < m_Width-2; col++) {
  smrow[4][col][0] =
    (pix[0]*6 + (pix[-4]+pix[4])*4 + pix[-8]+pix[8] + 8) >> 4;
  pix += 4;
      }
    }
    pix = m_Image[row*m_Width+2];
    for (col=2; col < m_Width-2; col++) {
      smred = ( 6 *  smrow[2][col][0]
        + 4 * (smrow[1][col][0] + smrow[3][col][0])
        +      smrow[0][col][0] + smrow[4][col][0] + 8 ) >> 4;
      if (col == 2)
  smred_p = smred;
      i = pix[0] + ((pix[0] - ((smred*7 + smred_p) >> 3)) >> 3);
      if (i > 32000) i = 32000;
      pix[0] = i;
      smred_p = smred;
      pix += 4;
    }
  }

  /* Adjust the brighter pixels for better linearity */
  min = 0xffff;
  for (c=0; c<3; c++) {
    i = (int)(satlev[c] / div[c]);
    if (min > i) min = i;
  }
  limit = min * 9 >> 4;
  for (pix=m_Image[0]; pix < m_Image[m_Height*m_Width]; pix+=4) {
    if (pix[0] <= limit || pix[1] <= limit || pix[2] <= limit)
      continue;
    min = max = pix[0];
    for (c=1; c < 3; c++) {
      if (min > pix[c]) min = pix[c];
      if (max < pix[c]) max = pix[c];
    }
    if (min >= limit*2) {
      pix[0] = pix[1] = pix[2] = max;
    } else {
      i = 0x4000 - ((min - limit) << 14) / limit;
      i = 0x4000 - (i*i >> 14);
      i = i*i >> 14;
      for (c=0; c<3; c++) pix[c] += (max - pix[c]) * i >> 14;
    }
  }
/*
   Because photons that miss one detector often hit another,
   the sum R+G+B is much less noisy than the individual colors.
   So smooth the hues without smoothing the total.
 */
  for (smlast=-1, row=2; row < m_Height-2; row++) {
    while (smlast < row+2) {
      for (i=0; i < 6; i++)
  smrow[(i+5) % 6] = smrow[i];
      pix = m_Image[++smlast*m_Width+2];
      for (col=2; col < m_Width-2; col++) {
  for (c=0; c<3; c++) smrow[4][col][c] = (pix[c-4]+2*pix[c]+pix[c+4]+2) >> 2;
  pix += 4;
      }
    }
    pix = m_Image[row*m_Width+2];
    for (col=2; col < m_Width-2; col++) {
      for (c=0; c<3; c++) dev[c] = -foveon_apply_curve (l_curve[7], pix[c] -
  ((smrow[1][col][c] + 2*smrow[2][col][c] + smrow[3][col][c]) >> 2));
      sum = (dev[0] + dev[1] + dev[2]) >> 3;
      for (c=0; c<3; c++) pix[c] += dev[c] - sum;
      pix += 4;
    }
  }
  for (smlast=-1, row=2; row < m_Height-2; row++) {
    while (smlast < row+2) {
      for (i=0; i < 6; i++)
  smrow[(i+5) % 6] = smrow[i];
      pix = m_Image[++smlast*m_Width+2];
      for (col=2; col < m_Width-2; col++) {
  for (c=0; c<3; c++) smrow[4][col][c] =
    (pix[c-8]+pix[c-4]+pix[c]+pix[c+4]+pix[c+8]+2) >> 2;
  pix += 4;
      }
    }
    pix = m_Image[row*m_Width+2];
    for (col=2; col < m_Width-2; col++) {
      for (total[3]=375, sum=60, c=0; c < 3; c++) {
  for (total[c]=i=0; i < 5; i++)
    total[c] += smrow[i][col][c];
  total[3] += total[c];
  sum += pix[c];
      }
      if (sum < 0) sum = 0;
      j = total[3] > 375 ? (sum << 16) / total[3] : sum * 174;
      for (c=0; c<3; c++) pix[c] += foveon_apply_curve (l_curve[6],
    ((j*total[c] + 0x8000) >> 16) - pix[c]);
      pix += 4;
    }
  }

  /* Transform the image to a different colorspace */
  for (pix=m_Image[0]; pix < m_Image[m_Height*m_Width]; pix+=4) {
    for (c=0; c<3; c++) pix[c] -= foveon_apply_curve (l_curve[c], pix[c]);
    sum = (pix[0]+pix[1]+pix[1]+pix[2]) >> 2;
    for (c=0; c<3; c++) pix[c] -= foveon_apply_curve (l_curve[c], pix[c]-sum);
    for (c=0; c<3; c++) {
      for (dsum=i=0; i < 3; i++)
  dsum += trans[c][i] * pix[i];
      if (dsum < 0)  dsum = 0;
      if (dsum > 24000) dsum = 24000;
      ipix[c] = (int)(dsum + 0.5);
    }
    for (c=0; c<3; c++) pix[c] = ipix[c];
  }

  /* Smooth the image bottom-to-top and save at 1/4 scale */
  shrink = (short (*)[3]) CALLOC ((m_Width/4) * (m_Height/4), sizeof *shrink);
  merror (shrink, "foveon_interpolate()");
  for (row = m_Height/4; row--; )
    for (col=0; col < m_Width/4; col++) {
      ipix[0] = ipix[1] = ipix[2] = 0;
      for (i=0; i < 4; i++)
  for (j=0; j < 4; j++)
    for (c=0; c<3; c++) ipix[c] += m_Image[(row*4+i)*m_Width+col*4+j][c];
      for (c=0; c<3; c++)
  if (row+2 > m_Height/4)
    shrink[row*(m_Width/4)+col][c] = ipix[c] >> 4;
  else
    shrink[row*(m_Width/4)+col][c] =
      (shrink[(row+1)*(m_Width/4)+col][c]*1840 + ipix[c]*141 + 2048) >> 12;
    }
  /* From the 1/4-scale image, smooth right-to-left */
  for (row=0; row < (m_Height & ~3); row++) {
    ipix[0] = ipix[1] = ipix[2] = 0;
    if ((row & 3) == 0)
      for (col = m_Width & ~3 ; col--; )
  for (c=0; c<3; c++) smrow[0][col][c] = ipix[c] =
    (shrink[(row/4)*(m_Width/4)+col/4][c]*1485 + ipix[c]*6707 + 4096) >> 13;

  /* Then smooth left-to-right */
    ipix[0] = ipix[1] = ipix[2] = 0;
    for (col=0; col < (m_Width & ~3); col++)
      for (c=0; c<3; c++) smrow[1][col][c] = ipix[c] =
  (smrow[0][col][c]*1485 + ipix[c]*6707 + 4096) >> 13;

  /* Smooth top-to-bottom */
    if (row == 0)
      memcpy (smrow[2], smrow[1], sizeof **smrow * m_Width);
    else
      for (col=0; col < (m_Width & ~3); col++)
  for (c=0; c<3; c++) smrow[2][col][c] =
    (smrow[2][col][c]*6707 + smrow[1][col][c]*1485 + 4096) >> 13;

  /* Adjust the chroma toward the smooth values */
    for (col=0; col < (m_Width & ~3); col++) {
      for (i=j=30, c=0; c < 3; c++) {
  i += smrow[2][col][c];
  j += m_Image[row*m_Width+col][c];
      }
      j = (j << 16) / i;
      for (sum=c=0; c < 3; c++) {
  ipix[c] = foveon_apply_curve (l_curve[c+3],
    ((smrow[2][col][c] * j + 0x8000) >> 16) - m_Image[row*m_Width+col][c]);
  sum += ipix[c];
      }
      sum >>= 3;
      for (c=0; c<3; c++) {
  i = m_Image[row*m_Width+col][c] + ipix[c] - sum;
  if (i < 0) i = 0;
  m_Image[row*m_Width+col][c] = i;
      }
    }
  }
  FREE (shrink);
  FREE (smrow[6]);
  for (i=0; i < 8; i++)
    FREE (l_curve[i]);

  /* Trim off the black border */
  active[1] -= keep[1];
  active[3] -= 2;
  i = active[2] - active[0];
  for (row=0; row < active[3]-active[1]; row++)
    memcpy (m_Image[row*i], m_Image[(row+active[1])*m_Width+active[0]],
   i * sizeof *m_Image);
  m_Width = i;
  m_Height = row;
#pragma GCC diagnostic pop
}
#undef m_Image

/* RESTRICTED code ends here */

void CLASS crop_masked_pixels()
{
  int row, col;
  unsigned r, c, m, mm_BlackLevel[8], zero, val;

  if (m_LoadRawFunction == &CLASS phase_one_load_raw ||
      m_LoadRawFunction == &CLASS phase_one_load_raw_c)
    phase_one_correct();
  if (m_Fuji_Width) {
    for (row=0; row < m_RawHeight-m_TopMargin*2; row++) {
      for (col=0; col < m_Fuji_Width << !fuji_layout; col++) {
  if (fuji_layout) {
    r = m_Fuji_Width - 1 - col + (row >> 1);
    c = col + ((row+1) >> 1);
  } else {
    r = m_Fuji_Width - 1 + row - (col >> 1);
    c = row + ((col+1) >> 1);
  }
  if (r < m_Height && c < m_Width)
    BAYER(r,c) = RAW(row+m_TopMargin,col+m_LeftMargin);
      }
    }
  } else {
    for (row=0; row < m_Height; row++)
      for (col=0; col < m_Width; col++)
  BAYER2(row,col) = RAW(row+m_TopMargin,col+m_LeftMargin);
  }
  if (m_Mask[0][3]) goto mask_set;
  if (m_LoadRawFunction == &CLASS canon_load_raw ||
      m_LoadRawFunction == &CLASS lossless_jpeg_load_raw) {
    m_Mask[0][1] = m_Mask[1][1] = 2;
    m_Mask[0][3] = -2;
    goto sides;
  }
  if (m_LoadRawFunction == &CLASS canon_600_load_raw ||
      m_LoadRawFunction == &CLASS sony_load_raw ||
     (m_LoadRawFunction == &CLASS eight_bit_load_raw && strncmp(m_CameraModel,"DC2",3)) ||
      m_LoadRawFunction == &CLASS kodak_262_load_raw ||
     (m_LoadRawFunction == &CLASS packed_load_raw && (m_Load_Flags & 32))) {
sides:
    m_Mask[0][0] = m_Mask[1][0] = m_TopMargin;
    m_Mask[0][2] = m_Mask[1][2] = m_TopMargin+m_Height;
    m_Mask[0][3] += m_LeftMargin;
    m_Mask[1][1] += m_LeftMargin+m_Width;
    m_Mask[1][3] += m_RawWidth;
  }
  if (m_LoadRawFunction == &CLASS nokia_load_raw) {
    m_Mask[0][2] = m_TopMargin;
    m_Mask[0][3] = m_Width;
  }
mask_set:
  memset (mm_BlackLevel, 0, sizeof mm_BlackLevel);
  for (zero=m=0; m < 8; m++)
    for (row=m_Mask[m][0]; row < m_Mask[m][2]; row++)
      for (col=m_Mask[m][1]; col < m_Mask[m][3]; col++) {
  c = FC(row-m_TopMargin,col-m_LeftMargin);
  mm_BlackLevel[c] += val = RAW(row,col);
  mm_BlackLevel[4+c]++;
  zero += !val;
      }
  if (m_LoadRawFunction == &CLASS canon_600_load_raw && m_Width < m_RawWidth) {
    m_BlackLevel = (mm_BlackLevel[0]+mm_BlackLevel[1]+mm_BlackLevel[2]+mm_BlackLevel[3]) /
      (mm_BlackLevel[4]+mm_BlackLevel[5]+mm_BlackLevel[6]+mm_BlackLevel[7]) - 4;
    canon_600_correct();
  } else if (zero < mm_BlackLevel[4] && mm_BlackLevel[5] && mm_BlackLevel[6] && mm_BlackLevel[7])
    for (c=0; c<4; c++) m_CBlackLevel[c] = mm_BlackLevel[c] / mm_BlackLevel[4+c];
}

void CLASS remove_zeroes()
{
  unsigned row, col, tot, n, r, c;

  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++)
      if (BAYER(row,col) == 0) {
  tot = n = 0;
  for (r = row-2; r <= row+2; r++)
    for (c = col-2; c <= col+2; c++)
      if (r < m_Height && c < m_Width &&
    FC(r,c) == FC(row,col) && BAYER(r,c))
        tot += (n++,BAYER(r,c));
  if (n) BAYER(row,col) = tot/n;
      }
}

/*
   Seach from the current directory up to the root looking for
   a ".badpixels" file, and fix those pixels now.
 */
void CLASS bad_pixels (const char *cfname)
{
  FILE *fp=0;
  char *cp, line[128];
  int /* len, */ time, row, col, r, c, rad, tot, n;

  if (!m_Filters) return;
  if (cfname)
    fp = fopen (cfname, "r");
/* MASK AWAY IN dcRaw
  else {
    for (len=32 ; ; len *= 2) {
      fname = (char *) MALLOC (len);
      if (!fname) return;
      if (getcwd (fname, len-16)) break;
      FREE (fname);
      if (errno != ERANGE) return;
    }
#if defined(WIN32) || defined(DJGPP)
    if (fname[1] == ':')
      memmove (fname, fname+2, len-2);
    for (cp=fname; *cp; cp++)
      if (*cp == '\\') *cp = '/';
#endif
    cp = fname + strlen(fname);
    if (cp[-1] == '/') cp--;
    while (*fname == '/') {
      strcpy (cp, "/.badpixels");
      if ((fp = fopen (fname, "r"))) break;
      if (cp == fname) break;
      while (*--cp != '/');
    }
    FREE (fname);
  }
*/
  if (!fp) return;
  while (fgets (line, 128, fp)) {
    cp = strchr (line, '#');
    if (cp) *cp = 0;
    if (sscanf (line, "%d %d %d", &col, &row, &time) != 3) continue;
    if ((unsigned) col >= m_Width || (unsigned) row >= m_Height) continue;
    if (time > m_TimeStamp) continue;
    for (tot=n=0, rad=1; rad < 3 && n==0; rad++)
      for (r = row-rad; r <= row+rad; r++)
  for (c = col-rad; c <= col+rad; c++)
    if ((unsigned) r < m_Height && (unsigned) c < m_Width &&
    (r != row || c != col) && fcol(r,c) == fcol(row,col)) {
      tot += BAYER2(r,c);
      n++;
    }
    BAYER2(row,col) = tot/n;
    TRACEKEYVALS("Fixed dead pixel at column","%d",col);
    TRACEKEYVALS("Fixed dead pixel at row","%d",row);
  }
  FCLOSE (fp);
}

void CLASS subtract (const char *fname)
{
  FILE *fp;
  int dim[3]={0,0,0}, comment=0, number=0, error=0, nd=0, c, row, col;
  uint16_t *pixel;

  if (!(fp = fopen (fname, "rb"))) {
    perror (fname);  return;
  }
  if (fgetc(fp) != 'P' || fgetc(fp) != '5') error = 1;
  while (!error && nd < 3 && (c = fgetc(fp)) != EOF) {
    if (c == '#')  comment = 1;
    if (c == '\n') comment = 0;
    if (comment) continue;
    if (isdigit(c)) number = 1;
    if (number) {
      if (isdigit(c)) dim[nd] = dim[nd]*10 + c -'0';
      else if (isspace(c)) {
  number = 0;  nd++;
      } else error = 1;
    }
  }
  if (error || nd < 3) {
    fprintf (stderr,_("%s is not a valid PGM file!\n"), fname);
    FCLOSE (fp);  return;
  } else if (dim[0] != m_Width || dim[1] != m_Height || dim[2] != 65535) {
    fprintf (stderr,_("%s has the wrong dimensions!\n"), fname);
    FCLOSE (fp);  return;
  }
  pixel = (uint16_t*) CALLOC (m_Width, sizeof *pixel);
  merror (pixel, "subtract()");
  for (row=0; row < m_Height; row++) {
    ptfread (pixel, 2, m_Width, fp);
    for (col=0; col < m_Width; col++)
      BAYER(row,col) = MAX (BAYER(row,col) - ntohs(pixel[col]), 0);
  }
  FREE (pixel);
  FCLOSE (fp);
  memset (m_CBlackLevel, 0, sizeof m_CBlackLevel);
  m_BlackLevel = 0;
}

void CLASS gamma_curve (double pwr, double ts, int mode, int imax)
{
  int i;
  double g[6], bnd[2]={0,0}, r;

  g[0] = pwr;
  g[1] = ts;
  g[2] = g[3] = g[4] = 0;
  bnd[g[1] >= 1] = 1;
  if (g[1] && (g[1]-1)*(g[0]-1) <= 0) {
    for (i=0; i < 48; i++) {
      g[2] = (bnd[0] + bnd[1])/2;
      if (g[0]) bnd[(pow(g[2]/g[1],-g[0]) - 1)/g[0] - 1/g[2] > -1] = g[2];
      else  bnd[g[2]/exp(1-1/g[2]) < g[1]] = g[2];
    }
    g[3] = g[2] / g[1];
    if (g[0]) g[4] = g[2] * (1/g[0] - 1);
  }
  if (g[0]) g[5] = 1 / (g[1]*SQR(g[3])/2 - g[4]*(1 - g[3]) +
    (1 - pow(g[3],1+g[0]))*(1 + g[4])/(1 + g[0])) - 1;
  else      g[5] = 1 / (g[1]*SQR(g[3])/2 + 1
    - g[2] - g[3] - g[2]*g[3]*(log(g[3]) - 1)) - 1;
  if (!mode--) {
    memcpy (m_Gamma, g, sizeof m_Gamma);
    return;
  }
  for (i=0; i < 0x10000; i++) {
    m_Curve[i] = 0xffff;
    if ((r = (double) i / imax) < 1)
      m_Curve[i] = 0x10000 * ( mode
  ? (r < g[3] ? r*g[1] : (g[0] ? pow( r,g[0])*(1+g[4])-g[4]    : log(r)*g[2]+1))
  : (r < g[2] ? r/g[1] : (g[0] ? pow((r+g[4])/(1+g[4]),1/g[0]) : exp((r-1)/g[2]))));
  }
}

void CLASS pseudoinverse (double (*in)[3], double (*out)[3], int size)
{
  double work[3][6], num;
  int i, j, k;

  for (i=0; i < 3; i++) {
    for (j=0; j < 6; j++)
      work[i][j] = j == i+3;
    for (j=0; j < 3; j++)
      for (k=0; k < size; k++)
  work[i][j] += in[k][i] * in[k][j];
  }
  for (i=0; i < 3; i++) {
    num = work[i][i];
    for (j=0; j < 6; j++)
      work[i][j] /= num;
    for (k=0; k < 3; k++) {
      if (k==i) continue;
      num = work[k][i];
      for (j=0; j < 6; j++)
  work[k][j] -= work[i][j] * num;
    }
  }
  for (i=0; i < size; i++)
    for (j=0; j < 3; j++)
      for (out[i][j]=k=0; k < 3; k++)
  out[i][j] += work[j][k+3] * in[i][k];
}

void CLASS cam_xyz_coeff (double MatrixXYZToCamRGB[4][3])
{
  double MatrixSRGBToCamRGB[4][3], inverse[4][3], num;
  int i, j, k;

  for (i=0; i < m_Colors; i++)    /* Multiply out XYZ colorspace */
    for (j=0; j < 3; j++)
      for (MatrixSRGBToCamRGB[i][j] = k=0; k < 3; k++)
  MatrixSRGBToCamRGB[i][j] +=
          MatrixXYZToCamRGB[i][k] * MatrixRGBToXYZ[ptSpace_sRGB_D65][k][j];

  // Normalize MatrixSRGBToCamRGB such that
  // (1,1,1) in SRGB maps onto (1,1,1,1) in CamRGB.
  for (i=0; i < m_Colors; i++) {
    for (num=j=0; j < 3; j++)
      num += MatrixSRGBToCamRGB[i][j];
    for (j=0; j < 3; j++)
      MatrixSRGBToCamRGB[i][j] /= num;

    // TODO More profound explanation.
    // But somehow it is clear that the factors to bring
    // (1,1,1)=>(1,1,1,1) are the multipliers at the reference point D65.
    ASSIGN(m_D65Multipliers[i], 1 / num);
  }

  pseudoinverse (MatrixSRGBToCamRGB, inverse, m_Colors);
  m_RawColorPhotivo = m_RawColor;
  for (m_RawColor = i=0; i < 3; i++)
    for (j=0; j < m_Colors; j++)
      m_MatrixCamRGBToSRGB[i][j] = inverse[j][i];
}

void CLASS hat_transform (float *temp, float *base, int st, int size, int sc)
{
  int i;
  for (i=0; i < sc; i++)
    temp[i] = 2*base[st*i] + base[st*(sc-i)] + base[st*(i+sc)];
  for (; i+sc < size; i++)
    temp[i] = 2*base[st*i] + base[st*(i-sc)] + base[st*(i+sc)];
  for (; i < size; i++)
    temp[i] = 2*base[st*i] + base[st*(i-sc)] + base[st*(2*size-2-(i+sc))];
}

////////////////////////////////////////////////////////////////////////////////
//
// ptAdjustMaximum
// This is modelled after the adjust_maximum of libraw.
// It's purpose is false color suppression in the highlights.
// Typically "Threshold" should be between 0.75 and 1.
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptAdjustMaximum(double Threshold) {
  m_WhiteLevel = LIM((int32_t)(Threshold*(double)m_WhiteLevel),0, 4095);
}

////////////////////////////////////////////////////////////////////////////////
//
// ptScaleColors
// Be aware : ptWaveletDenoising has as side effect that
// m_Blacklevel and m_Whitelevel change.
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptScaleColors() {
  unsigned bottom, right, row, col, x, y, c, sum[8];
  int val;
  double dsum[8], dmin, dmax;


  // Use the UserSetting_Multipliers if they are defined.
  // Remark that they will be overwritten if we have a AutoWb on CameraWb on !

  if (m_UserSetting_Multiplier[0]) {
    TRACEKEYVALS("Setting PreMultipliers due to UserSetting","%s","");
    ASSIGN(m_PreMultipliers[0],m_UserSetting_Multiplier[0]);
    ASSIGN(m_PreMultipliers[1],m_UserSetting_Multiplier[1]);
    ASSIGN(m_PreMultipliers[2],m_UserSetting_Multiplier[2]);
    ASSIGN(m_PreMultipliers[3],m_UserSetting_Multiplier[3]);
  }

  // AutoWb calculation, also fallback if no CameraWb found and
  // CameraWb is on.
  // Also the GreyBox whitesetting is done here, so AutoWb should be on for
  // GreyBox calculation !

  if ( m_UserSetting_AutoWb ||
      (m_UserSetting_CameraWb && VALUE(m_CameraMultipliers[0]) == -1)) {
    memset (dsum, 0, sizeof dsum);
    bottom = MIN (m_UserSetting_GreyBox[1]+m_UserSetting_GreyBox[3], (int32_t)m_Height);
    right  = MIN (m_UserSetting_GreyBox[0]+m_UserSetting_GreyBox[2], (int32_t)m_Width);
    TRACEKEYVALS("AutoWB GreyBox[0]","%d",m_UserSetting_GreyBox[0]);
    TRACEKEYVALS("AutoWB GreyBox[1]","%d",m_UserSetting_GreyBox[1]);
    TRACEKEYVALS("AutoWB GreyBox[2]","%d",m_UserSetting_GreyBox[2]);
    TRACEKEYVALS("AutoWB GreyBox[3]","%d",m_UserSetting_GreyBox[3]);
    TRACEKEYVALS("AutoWB m_Width","%d",m_Width);
    TRACEKEYVALS("AutoWB m_Height","%d",m_Height);
    TRACEKEYVALS("AutoWB right","%d",right);
    TRACEKEYVALS("AutoWB bottom","%d",bottom);
    // Auto whitebalance done over the greybox but resulting
    // in the whole image if no greybox given.
    for (row=m_UserSetting_GreyBox[1]; row < bottom; row += 8)
      for (col=m_UserSetting_GreyBox[0]; col < right; col += 8) {
        // Apparently the image is scanned in 8*8 pixel blocks.
        memset (sum, 0, sizeof sum);
        for (y=row; y < row+8 && y < bottom; y++)
          for (x=col; x < col+8 && x < right; x++)
            for (c=0; c < 4; c++) {
              if (m_Filters) {
            c = fcol(y,x);
            val = BAYER2(y,x);
              } else
                val = m_Image[y*m_Width+x][c];
              if ((unsigned) val > m_WhiteLevel-25) goto skip_block;
              if ((val -= m_CBlackLevel[c]) < 0) val = 0;
              sum[c] += val;
              sum[c+4]++;
              if (m_Filters) break;
            }
        for (c=0; c < 8; c++) dsum[c] += sum[c];
      skip_block: ;
      }
    // How intenser the blocks , how smaller the multipliers.
    // In terms of ratio's it means they will be close to each other
    // (neutral color balance) if the image has equally distributed RGB
    // (that's grey :))
    for (c=0; c < 4; c++) if (dsum[c]) ASSIGN(m_PreMultipliers[c], dsum[c+4] / dsum[c]);
    TRACEKEYVALS("Setting PreMultipliers due to AutoWB","%s","");

  }

  if (m_UserSetting_CameraWb && VALUE(m_CameraMultipliers[0]) != -1) {
    memset (sum, 0, sizeof sum);
    for (row=0; row < 8; row++)
      for (col=0; col < 8; col++) {
        c = FC(row,col);
        if ((val = white[row][col] - m_CBlackLevel[c]) > 0)
          sum[c] += val;
        sum[c+4]++;
      }
    if (sum[0] && sum[1] && sum[2] && sum[3]) {
      for (c=0; c < 4; c++) ASSIGN(m_PreMultipliers[c], (float) sum[c+4] / sum[c]);
      TRACEKEYVALS("Setting PreMultipliers due to CameraWB","%s","");
    }
    else if (VALUE(m_CameraMultipliers[0]) && VALUE(m_CameraMultipliers[2])) {
      ASSIGN(m_PreMultipliers[0],VALUE(m_CameraMultipliers[0]));
      ASSIGN(m_PreMultipliers[1],VALUE(m_CameraMultipliers[1]));
      ASSIGN(m_PreMultipliers[2],VALUE(m_CameraMultipliers[2]));
      ASSIGN(m_PreMultipliers[3],VALUE(m_CameraMultipliers[3]));
      TRACEKEYVALS("Setting PreMultipliers due to CameraMultipliers","%s","");
    }
    else
      fprintf (stderr,_("%s: Cannot use camera white balance.\n"), m_UserSetting_InputFileName);
  }

  if (VALUE(m_PreMultipliers[3]) == 0) {
    ASSIGN(m_PreMultipliers[3],
           (m_Colors < 4) ? VALUE(m_PreMultipliers[1]) : 1);
  }

  // Denoising before color scaling and interpolation.
  // Remark m_BlackLevel and m_WhiteLevel migth be changed.
  if (m_UserSetting_DenoiseThreshold) ptWaveletDenoise();

  for (dmin=DBL_MAX, dmax=c=0; c < 4; c++) {
    if (dmin > VALUE(m_PreMultipliers[c]))
      dmin = VALUE(m_PreMultipliers[c]);
    if (dmax < VALUE(m_PreMultipliers[c]))
      dmax = VALUE(m_PreMultipliers[c]);
  }

  TRACEKEYVALS("PreMult[0]","%f",VALUE(m_PreMultipliers[0]));
  TRACEKEYVALS("PreMult[1]","%f",VALUE(m_PreMultipliers[1]));
  TRACEKEYVALS("PreMult[2]","%f",VALUE(m_PreMultipliers[2]));
  TRACEKEYVALS("PreMult[3]","%f",VALUE(m_PreMultipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("PreMult File","%s",m_PreMultipliers[0].File);
  TRACEKEYVALS("PreMult Line","%d",m_PreMultipliers[0].Line);
#endif

  for (c=0;c<4;c++) {
    if (m_UserSetting_MaxMultiplier==0)
      ASSIGN(m_PreMultipliers[c],VALUE(m_PreMultipliers[c]) / dmax);
    else
      ASSIGN(m_PreMultipliers[c],VALUE(m_PreMultipliers[c]) / dmin);
    ASSIGN(m_Multipliers[c],
           VALUE(m_PreMultipliers[c])*0xffff/(m_WhiteLevel-m_CBlackLevel[c]));
  }
  if (m_UserSetting_MaxMultiplier==0)
    m_MinPreMulti = dmin/dmax; // And the maximum is per construction 1.0
  else
    m_MinPreMulti = 1.0; // TODO Mike: max is not 1.0 anymore...

  TRACEKEYVALS("dmax","%f",dmax);
  TRACEKEYVALS("dmin","%f",dmin);
  TRACEKEYVALS("m_MinPreMulti","%f",m_MinPreMulti);
  TRACEKEYVALS("PreMult[0]","%f",VALUE(m_PreMultipliers[0]));
  TRACEKEYVALS("PreMult[1]","%f",VALUE(m_PreMultipliers[1]));
  TRACEKEYVALS("PreMult[2]","%f",VALUE(m_PreMultipliers[2]));
  TRACEKEYVALS("PreMult[3]","%f",VALUE(m_PreMultipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("PreMult File","%s",m_PreMultipliers[0].File);
  TRACEKEYVALS("PreMult Line","%d",m_PreMultipliers[0].Line);
#endif
  TRACEKEYVALS("Mult[0]","%f",VALUE(m_Multipliers[0]));
  TRACEKEYVALS("Mult[1]","%f",VALUE(m_Multipliers[1]));
  TRACEKEYVALS("Mult[2]","%f",VALUE(m_Multipliers[2]));
  TRACEKEYVALS("Mult[3]","%f",VALUE(m_Multipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("Mult File","%s",m_Multipliers[0].File);
  TRACEKEYVALS("Mult Line","%d",m_Multipliers[0].Line);
#endif

  TRACEKEYVALS("Scaling colors","%s","");

  uint16_t LUT[0x10000][4];
#pragma omp parallel for schedule(static)
  for (uint32_t i = 0; i < 0xffff; i++) {
    for (short c = 0; c < 4; c++) {
      LUT[i][c] = i>m_CBlackLevel[c]?
                    MIN((int32_t)((i - m_CBlackLevel[c])*VALUE(m_Multipliers[c])),0xffff):0;
    }
  }

  uint32_t Size = m_OutHeight*m_OutWidth;
#pragma omp parallel for schedule(static)
  for (uint32_t i = 0; i < Size; i++) {
    for (short Color = 0; Color < 4; Color++) {
      m_Image[i][Color] = LUT[m_Image[i][Color]][Color];
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// ptHighlight
// Clipping is determined by Clip... FIXME
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptHighlight(const short  ClipMode,
                       const short  ClipParameter) {

  TRACEKEYVALS("ClipMode","%d",ClipMode);
  TRACEKEYVALS("ClipParameter","%d",ClipParameter);

  TRACEKEYVALS("m_Multi[0]","%f",VALUE(m_Multipliers[0]));
  TRACEKEYVALS("m_Multi[1]","%f",VALUE(m_Multipliers[1]));
  TRACEKEYVALS("m_Multi[2]","%f",VALUE(m_Multipliers[2]));
  TRACEKEYVALS("m_Multi[3]","%f",VALUE(m_Multipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("Mult File","%s",m_Multipliers[0].File);
  TRACEKEYVALS("Mult Line","%d",m_Multipliers[0].Line);
#endif

  TRACEKEYVALS("m_MinPreMulti","%f",m_MinPreMulti);
  short ColorRGB[4]={0,1,2,1};
#pragma omp parallel for schedule(static) default(shared)
  for (uint16_t Row = 0; Row < m_OutHeight; Row++) {
    for (uint16_t Column = 0; Column < m_OutWidth; Column++) {
      uint32_t Pos = Row*m_OutWidth+Column;

      // Do take into account however that we scaled the image here
      // already !

      // Clip occurs if the sensor reached its max value. Full stop.
      // m_Image[Pos] stands for value after application of the
      // m_Multipliers, which are per construction so that the saturation
      // value of the sensor maps onto 0XFFFF. It is the unclipped pixel.

      short Clipped = 0;
      for (short Color = 0; Color < m_Colors; Color++) {
        if (m_Image[Pos][Color] >=
            (uint16_t)
                ((m_WhiteLevel-m_CBlackLevel[ColorRGB[Color]])*VALUE(m_Multipliers[Color]))) {
          Clipped = 1;
        }
      }

      if (Clipped) {
        // Here it becomes fun. The sensor reached a clipped value.

        // 'ClippedPixel' stands for the pixel which is multiplied
        // by a value that guarantees the saturation value of the
        // least scaled channel maps onto 0XFFFF.
        // That value is obtained by observing that normally the
        // least scaled channel maps on saturation*minimum_multiplier
        // So since maximum_multipliers brings us per definition on 0XFFFF
        // upscaling with maximum_multiplier/minimum_multipier is that
        // value. Via the equivalence with the pre_multipliers where the
        // maximum is per construction 1 , it means that we have to upscale
        // with 1/m_MinPreMulti.
        // That way all saturated pixels are definitely mapped onto 0xFFFF
        uint16_t ClippedPixel[4];
        for (short Color = 0; Color < m_Colors; Color++) {
          // This ensures that the channel with the smallest multiplier
          // is clipped at its saturation level.
          ClippedPixel[Color] =
            MIN(0xFFFF,
                (int32_t)(m_Image[Pos][Color]/m_MinPreMulti));
          // And now we correct it back for the increased exposure.
          // (but clipped stays clipped !)
          ClippedPixel[Color] = (uint16_t)(ClippedPixel[Color]* m_MinPreMulti);
        }

        // From here on there are now different strategies with respect
        // to clipping.

  // Try to remove purple highlights
  if (0)
    if (m_Image[Pos][2]>m_Image[Pos][1] && m_Image[Pos][0]==0xffff) m_Image[Pos][2]=m_Image[Pos][1];

        // Simply use the clipped value.
        if (ptClipMode_Clip == ClipMode) {
          for (short Color = 0; Color < m_Colors; Color++) {
            m_Image[Pos][Color] = ClippedPixel[Color];
          }

        // Or use a value starting from Unclipped->Clipped ,
        // defined by the ClipParameter.
        } else if (ptClipMode_NoClip == ClipMode) {
          for (short Color = 0; Color < m_Colors; Color++) {
            m_Image[Pos][Color] =
              m_Image[Pos][Color] +
               (uint16_t) (ClipParameter/100.0*
                           (ClippedPixel[Color]-m_Image[Pos][Color]));
          }

        // Or restore via Lab, which is basically the same as
        // used in ufraw (and a simplification of Cyril Guyots LCH blending.
        } else if (ptClipMode_Lab == ClipMode) {
          double ClippedLab[3];
          double UnclippedLab[3];
          double FinalLab[3];
          CamToLab(ClippedPixel,ClippedLab);
          CamToLab(m_Image[Pos],UnclippedLab);
          // FIXME / TODO : Clarify and explain.
          // This is a bit bizar in fact (but also in ufraw ...)
          // The result seems to be better when taking the clipped
          // alternatives for ab and unclipped for L.
          // Wouldn't we expect it the other way around ?
          FinalLab[0] = UnclippedLab[0] +
                        ClipParameter/100.0*
                        (ClippedLab[0]-UnclippedLab[0]);
          FinalLab[1] = ClippedLab[1];
          FinalLab[2] = ClippedLab[2];
          LabToCam(FinalLab,m_Image[Pos]);

        // Or restore via HSV, as in ufraw.
        } else if (ptClipMode_HSV == ClipMode) {
          // FIXME / TODO : can this not break in some 4 colour modes ?
          short MaxChannel,MidChannel,MinChannel;
          if (m_Image[Pos][0] > m_Image[Pos][1] &&
              m_Image[Pos][0] > m_Image[Pos][2]) {
            MaxChannel = 0;
      if (m_Image[Pos][1] > m_Image[Pos][2]) {
              MidChannel = 1;
              MinChannel = 2;
            } else {
              MidChannel = 2;
              MinChannel = 1;
            }
          } else if (m_Image[Pos][1] > m_Image[Pos][2]) {
            MaxChannel = 1;
      if (m_Image[Pos][0] > m_Image[Pos][2]) {
              MidChannel = 0;
              MinChannel = 2;
            } else {
              MidChannel = 2;
              MinChannel = 0;
            }
          } else {
            MaxChannel = 2;
      if (m_Image[Pos][0] > m_Image[Pos][1]) {
              MidChannel = 0;
              MinChannel = 1;
            } else {
              MidChannel = 1;
              MinChannel = 0;
            }
          }

          uint16_t UnclippedLuminance = m_Image[Pos][MaxChannel];
          uint16_t ClippedLuminance   = ClippedPixel[MaxChannel];
          double   ClippedSaturation;
          if ( ClippedPixel[MaxChannel]<ClippedPixel[MinChannel] ||
               ClippedPixel[MaxChannel]==0) {
            ClippedSaturation = 0;
    } else {
            ClippedSaturation =
              1.0 - (double)ClippedPixel[MinChannel] / ClippedPixel[MaxChannel];
          }
//warning: variable 'ClippedHue' set but not used [-Wunused-but-set-variable]
//          double ClippedHue;
//    if ( ClippedPixel[MaxChannel]==ClippedPixel[MinChannel] ) {
//            ClippedHue = 0;
//    } else {
//            ClippedHue =
//              ((double)ClippedPixel[MidChannel]-ClippedPixel[MinChannel]) /
//              ((double)ClippedPixel[MaxChannel]-ClippedPixel[MinChannel]);
//          }
          double UnclippedHue;
    if ( m_Image[Pos][MaxChannel]==m_Image[Pos][MinChannel] ) {
            UnclippedHue = 0;
    } else {
            UnclippedHue =
              ((double)m_Image[Pos][MidChannel]-m_Image[Pos][MinChannel]) /
              ((double)m_Image[Pos][MaxChannel]-m_Image[Pos][MinChannel]);
          }
          uint16_t Luminance =
            UnclippedLuminance +
            (uint16_t)(ClipParameter/100.0 *
                       (ClippedLuminance-UnclippedLuminance));
          double Saturation = ClippedSaturation;
          double Hue = UnclippedHue;
    m_Image[Pos][MaxChannel] = Luminance;
    m_Image[Pos][MinChannel] = (uint16_t)(Luminance * (1-Saturation));
    m_Image[Pos][MidChannel] =
            (uint16_t)(Luminance * (1-Saturation + Saturation*Hue));

        } else if (ptClipMode_Blend == ClipMode) {
          // Do nothing at this stage, keep the unclipped image.
          ;
        } else if (ptClipMode_Rebuild == ClipMode) {
          // Do nothing at this stage, keep the unclipped image.
          ;
        } else {
          assert(0); // should not occur.
        }
      }
    }
  }

  if (ptClipMode_Rebuild == ClipMode) {
    ptRebuildHighlights(ClipParameter);
  }
  if (ptClipMode_Blend == ClipMode) {
    ptBlendHighlights();
  }
}

void CLASS pre_interpolate()
{
  int row, col;

  TRACEKEYVALS("pre_interpolate","%s","");

  if (m_Shrink) {
    if (m_UserSetting_HalfSize) {
      // No interpolation will be needed as
      // m_Shrink has caused a 2X2 Bayer area mapped onto one pixel.
      m_Height = m_OutHeight;
      m_Width  = m_OutWidth;
    } else {
      // in photivo we assume that m_Shrink is only set due
      // to m_UserSetting_HalfSize.
      assert(0);
    }
  }
  if (m_Filters > 1000 && m_Colors == 3) {
    if (m_MixGreen) { // 4 color demosaicer will follow
      m_Colors++;
      // Change from dcraw 1.445 to 1.447 wanted "m_MixGreen = !m_UserSetting_HalfSize;"
      // but this doesn't work in Photivo, since we don't run the full pipe of dcraw,
      // since most of the time we start with phase2
    } else {
      // RG1BG2 -> RGB
#pragma omp parallel for schedule(static) default(shared) private(row, col)
      for (row = FC(1,0) >> 1; row < m_Height; row+=2)
        for (col = FC(row,1) & 1; col < m_Width; col+=2)
          m_Image[row*m_Width+col][1] = m_Image[row*m_Width+col][3];
      m_Filters &= ~((m_Filters & 0x55555555) << 1);
    }
  }

  // If m_UserSetting_HalfSize is set no interpolation will
  // be needed. This is registered by m_Filters = 0.
  // (no Bayer array anymore, this means)
  if (m_UserSetting_HalfSize && m_Filters != 2) {
    m_Filters = 0;
  }
}

void CLASS border_interpolate (int border)
{
  unsigned row, col, y, x, f, c, sum[8];

  for (row=0; row < m_Height; row++)
    for (col=0; col < m_Width; col++) {
      if (col==(unsigned) border && row >= (unsigned) border && row < (unsigned) (m_Height-border))
  col = m_Width-border;
      memset (sum, 0, sizeof sum);
      for (y=row-1; y != row+2; y++)
  for (x=col-1; x != col+2; x++)
    if (y < m_Height && x < m_Width) {
    f = fcol(y,x);
      sum[f] += m_Image[y*m_Width+x][f];
      sum[f+4]++;
    }
      f = fcol(row,col);
      for (c=0; c < (unsigned) m_Colors; c++) if (c != f && sum[c+4])
  m_Image[row*m_Width+col][c] = sum[c] / sum[c+4];
    }
}


void CLASS lin_interpolate()
{
  int code[16][16][32], size=16, *ip, sum[4];
  int f, c, i, x, y, row, col, shift, color;
  uint16_t *pix;

  TRACEKEYVALS("Bilinear interpolation","%s","");
  if (m_Filters == 2) size = 6;
  border_interpolate(1);
  for (row=0; row < size; row++)
    for (col=0; col < size; col++) {
      ip = code[row][col]+1;
      f = fcol(row,col);
      memset (sum, 0, sizeof sum);
      for (y=-1; y <= 1; y++)
  for (x=-1; x <= 1; x++) {
    shift = (y==0) + (x==0);
    color = fcol(row+y,col+x);
    if (color == f) continue;
    *ip++ = (m_Width*y + x)*4 + color;
    *ip++ = shift;
    *ip++ = color;
    sum[color] += 1 << shift;
  }
      code[row][col][0] = (ip - code[row][col]) / 3;
      for (c=0; c < m_Colors; c++)
  if (c != f) {
    *ip++ = c;
    *ip++ = 256 / sum[c];
  }
    }
#pragma omp parallel for schedule(static) default(shared) private(row,col,pix,ip,sum,i)
  for (row=1; row < m_Height-1; row++)
    for (col=1; col < m_Width-1; col++) {
      pix = m_Image[row*m_Width+col];
      ip = code[row % size][col % size];
      memset (sum, 0, sizeof sum);
      for (i=*ip++; i--; ip+=3)
  sum[ip[2]] += pix[ip[0]] << ip[1];
      for (i=m_Colors; --i; ip+=2)
  pix[ip[0]] = sum[ip[0]] * ip[1] >> 8;
    }
}

/*
   This algorithm is officially called:

   "Interpolation using a Threshold-based variable number of gradients"

   described in http://scien.stanford.edu/pages/labsite/1999/psych221/projects/99/tingchen/algodep/vargra.html

   I've extended the basic idea to work with non-Bayer filter arrays.
   Gradients are numbered clockwise from NW=0 to W=7.
 */
void CLASS vng_interpolate()
{
  static const int16_t *cp, terms[] = {
    -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
    -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
    -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
    -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
    -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
    -1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
    -1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
    -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
    -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
    -1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
    -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
    -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
    -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
    +0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
    +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
    +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
    +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
    +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
    +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
    +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
    +1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
    +1,+0,+2,+1,0,0x10
  }, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
  uint16_t (*brow[5])[4], *pix;
  int prow=8, pcol=2, *ip, *code[16][16], gval[8], gmin, gmax, sum[4];
  int row, col, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;

  lin_interpolate();

  TRACEKEYVALS("VNG interpolation","%s","");

  if (m_Filters == 1) prow = pcol = 16;
  if (m_Filters == 2) prow = pcol =  6;
  ip = (int *) CALLOC (prow*pcol, 1280);
  merror (ip, "vng_interpolate()");
  for (row=0; row < prow; row++)		/* Precalculate for VNG */
    for (col=0; col < pcol; col++) {
      code[row][col] = ip;
      for (cp=terms, t=0; t < 64; t++) {
  y1 = *cp++;  x1 = *cp++;
  y2 = *cp++;  x2 = *cp++;
  weight = *cp++;
  grads = *cp++;
  color = fcol(row+y1,col+x1);
  if (fcol(row+y2,col+x2) != color) continue;
  diag = (fcol(row,col+1) == color && fcol(row+1,col) == color) ? 2:1;
  if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
  *ip++ = (y1*m_Width + x1)*4 + color;
  *ip++ = (y2*m_Width + x2)*4 + color;
  *ip++ = weight;
  for (g=0; g < 8; g++)
    if (grads & 1<<g) *ip++ = g;
  *ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=chood, g=0; g < 8; g++) {
  y = *cp++;  x = *cp++;
  *ip++ = (y*m_Width + x) * 4;
  color = fcol(row,col);
  if (fcol(row+y,col+x) != color && fcol(row+y*2,col+x*2) == color)
    *ip++ = (y*m_Width + x) * 8 + color;
  else
    *ip++ = 0;
      }
    }

  brow[4] = (uint16_t (*)[4]) CALLOC (m_Width*3, sizeof **brow);
  merror (brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*m_Width;

  for (row=2; row < m_Height-2; row++) {    /* Do VNG interpolation */
    for (col=2; col < m_Width-2; col++) {
      pix = m_Image[row*m_Width+col];
      ip = code[row % prow][col % pcol];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {    /* Calculate gradients */
  diff = ABS(pix[g] - pix[ip[1]]) << ip[2];
  gval[ip[3]] += diff;
  ip += 5;
  if ((g = ip[-1]) == -1) continue;
  gval[g] += diff;
  while ((g = *ip++) != -1)
    gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];      /* Choose a threshold */
      for (g=1; g < 8; g++) {
  if (gmin > gval[g]) gmin = gval[g];
  if (gmax < gval[g]) gmax = gval[g];
      }
      if (gmax == 0) {
  memcpy (brow[2][col], pix, sizeof *m_Image);
  continue;
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = fcol(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {   /* Average the neighbors */
  if (gval[g] <= thold) {
    for (c=0; c < m_Colors; c++)
      if (c == color && ip[1])
        sum[c] += (pix[c] + pix[ip[1]]) >> 1;
      else
        sum[c] += pix[ip[0] + c];
    num++;
  }
      }
      for (c=0; c < m_Colors; c++) {          /* Save to buffer */
  t = pix[color];
  if (c != color)
    t += (sum[c] - sum[color]) / num;
  brow[2][col][c] = CLIP(t);
      }
    }
    if (row > 3)        /* Write buffer to image */
      memcpy (m_Image[(row-2)*m_Width+2], brow[0]+2, (m_Width-4)*sizeof *m_Image);
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (m_Image[(row-2)*m_Width+2], brow[0]+2, (m_Width-4)*sizeof *m_Image);
  memcpy (m_Image[(row-1)*m_Width+2], brow[1]+2, (m_Width-4)*sizeof *m_Image);
  FREE (brow[4]);
  FREE (code[0][0]);
}

/*
   Patterned Pixel Grouping Interpolation by Alain Desbiolles
*/
void CLASS ppg_interpolate()
{
  const int dir[5] = { 1, m_Width, -1, -m_Width, 1 };
  int row, col, diff[2], guess[2], c, d, i;
  uint16_t (*pix)[4];

  border_interpolate(3);

  TRACEKEYVALS("PPG interpolation","%s","");

#pragma omp parallel private(row,c,col,i,guess,diff,d,pix)
/*  Fill in the green layer with gradients and pattern recognition: */
#pragma omp for
  for (row=3; row < m_Height-3; row++)
    for (col=3+(FC(row,3) & 1), c=FC(row,col); col < m_Width-3; col+=2) {
      pix = m_Image + row*m_Width+col;
      for (i=0; i<2; i++) {
        d = dir[i];
  guess[i] = (pix[-d][1] + pix[0][c] + pix[d][1]) * 2
          - pix[-2*d][c] - pix[2*d][c];
  diff[i] = ( ABS(pix[-2*d][c] - pix[ 0][c]) +
        ABS(pix[ 2*d][c] - pix[ 0][c]) +
        ABS(pix[  -d][1] - pix[ d][1]) ) * 3 +
      ( ABS(pix[ 3*d][1] - pix[ d][1]) +
        ABS(pix[-3*d][1] - pix[-d][1]) ) * 2;
      }
      d = dir[i = diff[0] > diff[1]];
      pix[0][1] = ULIM(guess[i] >> 2, (int32_t)pix[d][1], (int32_t)pix[-d][1]);
    }
/*  Calculate red and blue for each green pixel:    */
#pragma omp for
  for (row=1; row < m_Height-1; row++)
    for (col=1+(FC(row,2) & 1), c=FC(row,col+1); col < m_Width-1; col+=2) {
      pix = m_Image + row*m_Width+col;
      for (i=0; (d=dir[i]) > 0; c=2-c, i++)
  pix[0][c] = CLIP((pix[-d][c] + pix[d][c] + 2*pix[0][1]
      - pix[-d][1] - pix[d][1]) >> 1);
    }
/*  Calculate blue for red pixels and vice versa:   */
#pragma omp for
  for (row=1; row < m_Height-1; row++)
    for (col=1+(FC(row,1) & 1), c=2-FC(row,col); col < m_Width-1; col+=2) {
      pix = m_Image + row*m_Width+col;
      for (i=0; i < 2; i++) {
        d = dir[i]+dir[i+1];
        diff[i] = ABS(pix[-d][c] - pix[d][c]) +
      ABS(pix[-d][1] - pix[0][1]) +
      ABS(pix[ d][1] - pix[0][1]);
  guess[i] = pix[-d][c] + pix[d][c] + 2*pix[0][1]
     - pix[-d][1] - pix[d][1];
      }
      if (diff[0] != diff[1])
  pix[0][c] = CLIP(guess[diff[0] > diff[1]] >> 1);
      else
  pix[0][c] = CLIP((guess[0]+guess[1]) >> 2);
    }
}

/*
   Adaptive Homogeneity-Directed interpolation is based on
   the work of Keigo Hirakawa, Thomas Parks, and Paul Lee.
 */
#define TS 256    /* Tile Size */

void CLASS ahd_interpolate()
{
  int i, j, k, top, left, row, col, tr, tc, c, d, val, hm[2];
  uint16_t (*pix)[4], (*rix)[3];
  static const int dir[4] = { -1, 1, -TS, TS };
  unsigned ldiff[2][4], abdiff[2][4], leps, abeps;
  float r, cbrt[0x10000], xyz[3], xyz_cam[3][4];
  uint16_t (*rgb)[TS][TS][3];
   short (*lab)[TS][TS][3], (*lix)[3];
   char (*homo)[TS][TS], *buffer;

  TRACEKEYVALS("AHD interpolation","%s","");

  for (i=0; i < 0x10000; i++) {
    r = i / 65535.0;
    cbrt[i] = r > 0.008856 ? pow(r,1/3.0) : 7.787*r + 16/116.0;
  }
  for (i=0; i < 3; i++)
    for (j=0; j < m_Colors; j++)
      for (xyz_cam[i][j] = k=0; k < 3; k++)
  xyz_cam[i][j] += MatrixRGBToXYZ[ptSpace_sRGB_D65][i][k] * m_MatrixCamRGBToSRGB[k][j] / D65Reference[i];

  border_interpolate(5);

#pragma omp parallel private(buffer,rgb,lab,homo,top,left,row,c,col,pix,val,d,rix,xyz,lix,tc,tr,ldiff,abdiff,leps,abeps,hm,i,j) firstprivate(cbrt) shared(xyz_cam)
  {
      buffer = (char *) MALLOC (26*TS*TS);    /* 1664 kB */
      merror (buffer, "ahd_interpolate()");
      rgb  = (uint16_t(*)[TS][TS][3]) buffer;
      lab  = (short (*)[TS][TS][3])(buffer + 12*TS*TS);
      homo = (char  (*)[TS][TS])   (buffer + 24*TS*TS);

#pragma omp for schedule(static)
      for (top=2; top < m_Height-5; top += TS-6)
          for (left=2; left < m_Width-5; left += TS-6) {

          /*  Interpolate green horizontally and vertically:    */
          for (row = top; row < top+TS && row < m_Height-2; row++) {
              col = left + (FC(row,left) & 1);
              for (c = FC(row,col); col < left+TS && col < m_Width-2; col+=2) {
                  pix = m_Image + row*m_Width+col;
                  val = ((pix[-1][1] + pix[0][c] + pix[1][1]) * 2
                         - pix[-2][c] - pix[2][c]) >> 2;
                  rgb[0][row-top][col-left][1] = ULIM(val,(int32_t)pix[-1][1],(int32_t)pix[1][1]);
                  val = ((pix[-m_Width][1] + pix[0][c] + pix[m_Width][1]) * 2
                         - pix[-2*m_Width][c] - pix[2*m_Width][c]) >> 2;
                  rgb[1][row-top][col-left][1] = ULIM(val,(int32_t)pix[-m_Width][1],(int32_t)pix[m_Width][1]);
              }
          }
          /*  Interpolate red and blue, and convert to CIELab:    */
          for (d=0; d < 2; d++)
              for (row=top+1; row < top+TS-1 && row < m_Height-3; row++)
                  for (col=left+1; col < left+TS-1 && col < m_Width-3; col++) {
              pix = m_Image + row*m_Width+col;
              rix = &rgb[d][row-top][col-left];
              lix = &lab[d][row-top][col-left];
              if ((c = 2 - FC(row,col)) == 1) {
                  c = FC(row+1,col);
                  val = pix[0][1] + (( pix[-1][2-c] + pix[1][2-c]
                                       - rix[-1][1] - rix[1][1] ) >> 1);
                  rix[0][2-c] = CLIP(val);
                  val = pix[0][1] + (( pix[-m_Width][c] + pix[m_Width][c]
                                       - rix[-TS][1] - rix[TS][1] ) >> 1);
              } else
                  val = rix[0][1] + (( pix[-m_Width-1][c] + pix[-m_Width+1][c]
                                       + pix[+m_Width-1][c] + pix[+m_Width+1][c]
                                       - rix[-TS-1][1] - rix[-TS+1][1]
                                       - rix[+TS-1][1] - rix[+TS+1][1] + 1) >> 2);
              rix[0][c] = CLIP(val);
              c = FC(row,col);
              rix[0][c] = pix[0][c];
              xyz[0] = xyz[1] = xyz[2] = 0.5;
              for (c=0; c < m_Colors; c++) {
                  xyz[0] += xyz_cam[0][c] * rix[0][c];
                  xyz[1] += xyz_cam[1][c] * rix[0][c];
                  xyz[2] += xyz_cam[2][c] * rix[0][c];
              }
              xyz[0] = cbrt[CLIP((int) xyz[0])];
              xyz[1] = cbrt[CLIP((int) xyz[1])];
              xyz[2] = cbrt[CLIP((int) xyz[2])];
              lix[0][0] = (short) (64 * (116 * xyz[1] - 16));
              lix[0][1] = (short) (64 * 500 * (xyz[0] - xyz[1]));
              lix[0][2] = (short) (64 * 200 * (xyz[1] - xyz[2]));
          }
          /*  Build homogeneity maps from the CIELab images:    */
          memset (homo, 0, 2*TS*TS);
          for (row=top+2; row < top+TS-2 && row < m_Height-4; row++) {
              tr = row-top;
              for (col=left+2; col < left+TS-2 && col < m_Width-4; col++) {
                  tc = col-left;
                  for (d=0; d < 2; d++) {
                      lix = &lab[d][tr][tc];
                      for (i=0; i < 4; i++) {
                          ldiff[d][i] = ABS(lix[0][0]-lix[dir[i]][0]);
                          abdiff[d][i] = SQR(lix[0][1]-lix[dir[i]][1])
                                         + SQR(lix[0][2]-lix[dir[i]][2]);
                      }
                  }
                  leps = MIN(MAX(ldiff[0][0],ldiff[0][1]),
                             MAX(ldiff[1][2],ldiff[1][3]));
                  abeps = MIN(MAX(abdiff[0][0],abdiff[0][1]),
                              MAX(abdiff[1][2],abdiff[1][3]));
                  for (d=0; d < 2; d++)
                      for (i=0; i < 4; i++)
                          if (ldiff[d][i] <= leps && abdiff[d][i] <= abeps)
                             homo[d][tr][tc]++;
              }
          }
          /*  Combine the most homogenous pixels for the final result:  */
          for (row=top+3; row < top+TS-3 && row < m_Height-5; row++) {
              tr = row-top;
              for (col=left+3; col < left+TS-3 && col < m_Width-5; col++) {
                  tc = col-left;
                  for (d=0; d < 2; d++)
                      for (hm[d]=0, i=tr-1; i <= tr+1; i++)
                          for (j=tc-1; j <= tc+1; j++)
                              hm[d] += homo[d][i][j];
                  if (hm[0] != hm[1])
                      for (c=0; c<3; c++) m_Image[row*m_Width+col][c] = rgb[hm[1] > hm[0]][tr][tc][c];
                  else
                      for (c=0; c<3; c++) m_Image[row*m_Width+col][c] =
                              (rgb[0][tr][tc][c] + rgb[1][tr][tc][c]) >> 1;
              }
          }
      }
      FREE (buffer);
  }
}
#undef TS

void CLASS tiff_get (unsigned base,
  unsigned *tag, unsigned *type, unsigned *len, unsigned *save)
{
  *tag  = get2();
  *type = get2();
  *len  = get4();
  *save = ftell(m_InputFile) + 4;
  if (*len * ("11124811248488"[*type < 14 ? *type:0]-'0') > 4)
    fseek (m_InputFile, get4()+base, SEEK_SET);
}

void CLASS parse_thumb_note (int base, unsigned toff, unsigned tlen)
{
  unsigned entries, tag, type, len, save;

  entries = get2();
  while (entries--) {
    tiff_get (base, &tag, &type, &len, &save);
    if (tag == toff) m_ThumbOffset = get4()+base;
    if (tag == tlen) m_ThumbLength = get4();
    fseek (m_InputFile, save, SEEK_SET);
  }
}

//int CLASS parse_tiff_ifd (int base);

void CLASS parse_makernote (int base, int uptag)
{
  static const uint8_t xlat[2][256] = {
  { 0xc1,0xbf,0x6d,0x0d,0x59,0xc5,0x13,0x9d,0x83,0x61,0x6b,0x4f,0xc7,0x7f,0x3d,0x3d,
    0x53,0x59,0xe3,0xc7,0xe9,0x2f,0x95,0xa7,0x95,0x1f,0xdf,0x7f,0x2b,0x29,0xc7,0x0d,
    0xdf,0x07,0xef,0x71,0x89,0x3d,0x13,0x3d,0x3b,0x13,0xfb,0x0d,0x89,0xc1,0x65,0x1f,
    0xb3,0x0d,0x6b,0x29,0xe3,0xfb,0xef,0xa3,0x6b,0x47,0x7f,0x95,0x35,0xa7,0x47,0x4f,
    0xc7,0xf1,0x59,0x95,0x35,0x11,0x29,0x61,0xf1,0x3d,0xb3,0x2b,0x0d,0x43,0x89,0xc1,
    0x9d,0x9d,0x89,0x65,0xf1,0xe9,0xdf,0xbf,0x3d,0x7f,0x53,0x97,0xe5,0xe9,0x95,0x17,
    0x1d,0x3d,0x8b,0xfb,0xc7,0xe3,0x67,0xa7,0x07,0xf1,0x71,0xa7,0x53,0xb5,0x29,0x89,
    0xe5,0x2b,0xa7,0x17,0x29,0xe9,0x4f,0xc5,0x65,0x6d,0x6b,0xef,0x0d,0x89,0x49,0x2f,
    0xb3,0x43,0x53,0x65,0x1d,0x49,0xa3,0x13,0x89,0x59,0xef,0x6b,0xef,0x65,0x1d,0x0b,
    0x59,0x13,0xe3,0x4f,0x9d,0xb3,0x29,0x43,0x2b,0x07,0x1d,0x95,0x59,0x59,0x47,0xfb,
    0xe5,0xe9,0x61,0x47,0x2f,0x35,0x7f,0x17,0x7f,0xef,0x7f,0x95,0x95,0x71,0xd3,0xa3,
    0x0b,0x71,0xa3,0xad,0x0b,0x3b,0xb5,0xfb,0xa3,0xbf,0x4f,0x83,0x1d,0xad,0xe9,0x2f,
    0x71,0x65,0xa3,0xe5,0x07,0x35,0x3d,0x0d,0xb5,0xe9,0xe5,0x47,0x3b,0x9d,0xef,0x35,
    0xa3,0xbf,0xb3,0xdf,0x53,0xd3,0x97,0x53,0x49,0x71,0x07,0x35,0x61,0x71,0x2f,0x43,
    0x2f,0x11,0xdf,0x17,0x97,0xfb,0x95,0x3b,0x7f,0x6b,0xd3,0x25,0xbf,0xad,0xc7,0xc5,
    0xc5,0xb5,0x8b,0xef,0x2f,0xd3,0x07,0x6b,0x25,0x49,0x95,0x25,0x49,0x6d,0x71,0xc7 },
  { 0xa7,0xbc,0xc9,0xad,0x91,0xdf,0x85,0xe5,0xd4,0x78,0xd5,0x17,0x46,0x7c,0x29,0x4c,
    0x4d,0x03,0xe9,0x25,0x68,0x11,0x86,0xb3,0xbd,0xf7,0x6f,0x61,0x22,0xa2,0x26,0x34,
    0x2a,0xbe,0x1e,0x46,0x14,0x68,0x9d,0x44,0x18,0xc2,0x40,0xf4,0x7e,0x5f,0x1b,0xad,
    0x0b,0x94,0xb6,0x67,0xb4,0x0b,0xe1,0xea,0x95,0x9c,0x66,0xdc,0xe7,0x5d,0x6c,0x05,
    0xda,0xd5,0xdf,0x7a,0xef,0xf6,0xdb,0x1f,0x82,0x4c,0xc0,0x68,0x47,0xa1,0xbd,0xee,
    0x39,0x50,0x56,0x4a,0xdd,0xdf,0xa5,0xf8,0xc6,0xda,0xca,0x90,0xca,0x01,0x42,0x9d,
    0x8b,0x0c,0x73,0x43,0x75,0x05,0x94,0xde,0x24,0xb3,0x80,0x34,0xe5,0x2c,0xdc,0x9b,
    0x3f,0xca,0x33,0x45,0xd0,0xdb,0x5f,0xf5,0x52,0xc3,0x21,0xda,0xe2,0x22,0x72,0x6b,
    0x3e,0xd0,0x5b,0xa8,0x87,0x8c,0x06,0x5d,0x0f,0xdd,0x09,0x19,0x93,0xd0,0xb9,0xfc,
    0x8b,0x0f,0x84,0x60,0x33,0x1c,0x9b,0x45,0xf1,0xf0,0xa3,0x94,0x3a,0x12,0x77,0x33,
    0x4d,0x44,0x78,0x28,0x3c,0x9e,0xfd,0x65,0x57,0x16,0x94,0x6b,0xfb,0x59,0xd0,0xc8,
    0x22,0x36,0xdb,0xd2,0x63,0x98,0x43,0xa1,0x04,0x87,0x86,0xf7,0xa6,0x26,0xbb,0xd6,
    0x59,0x4d,0xbf,0x6a,0x2e,0xaa,0x2b,0xef,0xe6,0x78,0xb6,0x4e,0xe0,0x2f,0xdc,0x7c,
    0xbe,0x57,0x19,0x32,0x7e,0x2a,0xd0,0xb8,0xba,0x29,0x00,0x3c,0x52,0x7d,0xa8,0x49,
    0x3b,0x2d,0xeb,0x25,0x49,0xfa,0xa3,0xaa,0x39,0xa7,0xc5,0xa7,0x50,0x11,0x36,0xfb,
    0xc6,0x67,0x4a,0xf5,0xa5,0x12,0x65,0x7e,0xb0,0xdf,0xaf,0x4e,0xb3,0x61,0x7f,0x2f } };
  unsigned offset=0, entries, tag, type, len, save, c;
  unsigned ver97=0, serial=0, i, wbi=0, wb[4]={0,0,0,0};
  uint8_t buf97[324], ci, cj, ck;
  short mm_ByteOrder, sm_ByteOrder=m_ByteOrder;
  char buf[10];
/*
   The MakerNote might have its own TIFF header (possibly with
   its own byte-order!), or it might just be a table.
 */
  if (!strcmp(m_CameraMake,"Nokia")) return;
  ptfread (buf, 1, 10, m_InputFile);
  if (!strncmp (buf,"KDK" ,3) ||  /* these aren't TIFF tables */
      !strncmp (buf,"VER" ,3) ||
      !strncmp (buf,"IIII",4) ||
      !strncmp (buf,"MMMM",4)) return;
  if (!strncmp (buf,"KC"  ,2) ||  /* Konica KD-400Z, KD-510Z */
      !strncmp (buf,"MLY" ,3)) {  /* Minolta DiMAGE G series */
    m_ByteOrder = 0x4d4d;
    while ((i=ftell(m_InputFile)) < (unsigned) m_Data_Offset && i < (unsigned) 16384) {
      wb[0] = wb[2];  wb[2] = wb[1];  wb[1] = wb[3];
      wb[3] = get2();
      if (wb[1] == 256 && wb[3] == 256 &&
    wb[0] > 256 && wb[0] < 640 && wb[2] > 256 && wb[2] < 640)
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c], wb[c]);
    }
    goto quit;
  }
  if (!strcmp (buf,"Nikon")) {
      base = ftell(m_InputFile);
      m_ByteOrder = get2();
      if (get2() != 42) goto quit;
      offset = get4();
      fseek (m_InputFile, offset-8, SEEK_CUR);
    } else if (!strcmp (buf,"OLYMPUS")) {
      base = ftell(m_InputFile)-10;
      fseek (m_InputFile, -2, SEEK_CUR);
      m_ByteOrder = get2();  get2();
    } else if (!strncmp (buf,"SONY",4) ||
         !strcmp  (buf,"Panasonic")) {
      goto nf;
    } else if (!strncmp (buf,"FUJIFILM",8)) {
      base = ftell(m_InputFile)-10;
  nf: m_ByteOrder = 0x4949;
      fseek (m_InputFile,  2, SEEK_CUR);
    } else if (!strcmp (buf,"OLYMP") ||
         !strcmp (buf,"LEICA") ||
         !strcmp (buf,"Ricoh") ||
         !strcmp (buf,"EPSON"))
      fseek (m_InputFile, -2, SEEK_CUR);
    else if (!strcmp (buf,"AOC") ||
       !strcmp (buf,"QVC"))
      fseek (m_InputFile, -4, SEEK_CUR);
    else {
      fseek (m_InputFile, -10, SEEK_CUR);
      if (!strncmp(m_CameraMake,"SAMSUNG",7))
        base = ftell(m_InputFile);
    }
  entries = get2();
  if (entries > 1000) return;
  mm_ByteOrder = m_ByteOrder;
  while (entries--) {
    m_ByteOrder = mm_ByteOrder;
    tiff_get (base, &tag, &type, &len, &save);
    tag |= uptag << 16;
    if (tag == 2 && strstr(m_CameraMake,"NIKON") && !m_IsoSpeed)
      m_IsoSpeed = (get2(),get2());
    if (tag == 4 && len > 26 && len < 35) {
      if ((i=(get4(),get2())) != 0x7fff && !m_IsoSpeed)
        m_IsoSpeed =  50 * pow (2, i/32.0 - 4);
      if ((i=(get2(),get2())) != 0x7fff && !m_Aperture)
  m_Aperture = pow (2, i/64.0);
      if ((i=get2()) != 0xffff && !m_Shutter)
  m_Shutter = pow (2, (short) i/-32.0);
      wbi = (get2(),get2());
      m_ShotOrder = (get2(),get2());
    }
   if ((tag == 4 || tag == 0x114) && !strncmp(m_CameraMake,"KONICA",6)) {
      fseek (m_InputFile, tag == 4 ? 140:160, SEEK_CUR);
      switch (get2()) {
  case 72:  m_Flip = 0;  break;
  case 76:  m_Flip = 6;  break;
  case 82:  m_Flip = 5;  break;
      }
    }
    if (tag == 7 && type == 2 && len > 20)
      ptfgets (m_CameraModelBis, 64, m_InputFile);
    if (tag == 8 && type == 4)
      m_ShotOrder = get4();
    if (tag == 9 && !strcmp(m_CameraMake,"Canon"))
      ptfread (m_Artist, 64, 1, m_InputFile);
    if (tag == 0xc && len == 4) {
      ASSIGN(m_CameraMultipliers[0], getreal(type));
      ASSIGN(m_CameraMultipliers[2], getreal(type));
    }
    if (tag == 0xd && type == 7 && get2() == 0xaaaa) {
      for (c=i=2; (uint16_t) c != 0xbbbb && i < len; i++)
  c = c << 8 | fgetc(m_InputFile);
      while ((i+=4) < len-5)
  if (get4() == 257 && (i=len) && (c = (get4(),fgetc(m_InputFile))) < 3)
    m_Flip = "065"[c]-'0';
    }
    if (tag == 0x10 && type == 4)
      unique_id = get4();
    if (tag == 0x11 && m_IsRaw && !strncmp(m_CameraMake,"NIKON",5)) {
      fseek (m_InputFile, get4()+base, SEEK_SET);
      parse_tiff_ifd (base);
    }
    if (tag == 0x14 && type == 7) {
      if (len == 2560) {
        fseek (m_InputFile, 1248, SEEK_CUR);
        goto get2_256;
      }
      ptfread (buf, 1, 10, m_InputFile);
      if (!strncmp(buf,"NRW ",4)) {
        fseek (m_InputFile, strcmp(buf+4,"0100") ? 46:1546, SEEK_CUR);
        ASSIGN(m_CameraMultipliers[0], get4() << 2);
        ASSIGN(m_CameraMultipliers[1], get4() + get4());
        ASSIGN(m_CameraMultipliers[2], get4() << 2);
      }
    }
    if (tag == 0x15 && type == 2 && m_IsRaw)
      ptfread (m_CameraModel, 64, 1, m_InputFile);
    if (strstr(m_CameraMake,"PENTAX")) {
      if (tag == 0x1b) tag = 0x1018;
      if (tag == 0x1c) tag = 0x1017;
    }
    if (tag == 0x1d)
      while ((c = fgetc(m_InputFile)) && c != (unsigned) EOF)
  serial = serial*10 + (isdigit(c) ? c - '0' : c % 10);
    if (tag == 0x81 && type == 4) {
      m_Data_Offset = get4();
      fseek (m_InputFile, m_Data_Offset + 41, SEEK_SET);
      m_RawHeight = get2() * 2;
      m_RawWidth  = get2();
      m_Filters = 0x61616161;
    }
    if (tag == 0x29 && type == 1) {
      c = wbi < 18 ? "012347800000005896"[wbi]-'0' : 0;
      fseek (m_InputFile, 8 + c*32, SEEK_CUR);
      for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1) ^ 1], get4());
    }
    if ((tag == 0x81  && type == 7) ||
  (tag == 0x100 && type == 7) ||
  (tag == 0x280 && type == 1)) {
      m_ThumbOffset = ftell(m_InputFile);
      m_ThumbLength = len;
    }
    if (tag == 0x88 && type == 4 && (m_ThumbOffset = get4()))
      m_ThumbOffset += base;
    if (tag == 0x89 && type == 4)
      m_ThumbLength = get4();
    if (tag == 0x8c || tag == 0x96)
      meta_offset = ftell(m_InputFile);
    if (tag == 0x97) {
      for (i=0; i < 4; i++)
  ver97 = ver97 * 10  + fgetc(m_InputFile)-'0';
      switch (ver97) {
  case 100:
    fseek (m_InputFile, 68, SEEK_CUR);
    for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[(c >> 1) | ((c & 1) << 1)], get2());
    break;
  case 102:
    fseek (m_InputFile, 6, SEEK_CUR);
    goto get2_rggb;
  case 103:
    fseek (m_InputFile, 16, SEEK_CUR);
    for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c], get2());
      }
      if (ver97 >= 200) {
  if (ver97 != 205) fseek (m_InputFile, 280, SEEK_CUR);
  ptfread (buf97, 324, 1, m_InputFile);
      }
    }
    if (tag == 0xa1 && type == 7) {
      type = m_ByteOrder;
      m_ByteOrder = 0x4949;
      fseek (m_InputFile, 140, SEEK_CUR);
      for (c=0; c < 3; c++) ASSIGN(m_CameraMultipliers[c], get4());
      m_ByteOrder = type;
    }
    if (tag == 0xa4 && type == 3) {
      fseek (m_InputFile, wbi*48, SEEK_CUR);
      for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], get2());
    }
    if (tag == 0xa7 && (unsigned) (ver97-200) < 12 && !VALUE(m_CameraMultipliers[0])) {
      ci = xlat[0][serial & 0xff];
      cj = xlat[1][fgetc(m_InputFile)^fgetc(m_InputFile)^fgetc(m_InputFile)^fgetc(m_InputFile)];
      ck = 0x60;
      for (i=0; i < 324; i++)
  buf97[i] ^= (cj += ci * ck++);
      i = "66666>666;6A;:;55"[ver97-200] - '0';
      for (c=0; c < 4; c++)
        ASSIGN(m_CameraMultipliers[c ^ (c >> 1) ^ (i &1)],
               sget2 (buf97 + (i & -2) + c*2));
    }
    if (tag == 0x200 && len == 3)
      m_ShotOrder = (get4(),get4());
    if (tag == 0x200 && len == 4)
      for (c=0; c<4; c++) m_CBlackLevel[c ^ c >> 1] = get2();
    if (tag == 0x201 && len == 4)
      goto get2_rggb;
    if (tag == 0x220 && type == 7)
      meta_offset = ftell(m_InputFile);
    if (tag == 0x401 && type == 4 && len == 4) {
      for (c=0; c<4; c++) m_CBlackLevel[c ^ c >> 1] = get4();
    }
    if (tag == 0xe01) {   /* Nikon Capture Note */
      type = m_ByteOrder;
      m_ByteOrder = 0x4949;
      fseek (m_InputFile, 22, SEEK_CUR);
      for (offset=22; offset+22 < len; offset += 22+i) {
  tag = get4();
  fseek (m_InputFile, 14, SEEK_CUR);
  i = get4()-4;
  if (tag == 0x76a43207) m_Flip = get2();
  else fseek (m_InputFile, i, SEEK_CUR);
      }
      m_ByteOrder = type;
    }
    if (tag == 0xe80 && len == 256 && type == 7) {
      fseek (m_InputFile, 48, SEEK_CUR);
      ASSIGN(m_CameraMultipliers[0], get2() * 508 * 1.078 / 0x10000);
      ASSIGN(m_CameraMultipliers[2], get2() * 382 * 1.173 / 0x10000);
    }
    if (tag == 0xf00 && type == 7) {
      if (len == 614)
  fseek (m_InputFile, 176, SEEK_CUR);
      else if (len == 734 || len == 1502)
  fseek (m_InputFile, 148, SEEK_CUR);
      else goto next;
      goto get2_256;
    }
    if ((tag == 0x1011 && len == 9) || tag == 0x20400200)
      for (i=0; i < 3; i++)
  for (c=0; c<3; c++) m_cmatrix[i][c] = ((short) get2()) / 256.0;
    if ((tag == 0x1012 || tag == 0x20400600) && len == 4)
      for (c=0; c<4; c++) m_CBlackLevel[c ^ c >> 1] = get2();
    if (tag == 0x1017 || tag == 0x20400100)
      ASSIGN(m_CameraMultipliers[0], get2() / 256.0);
    if (tag == 0x1018 || tag == 0x20400100)
      ASSIGN(m_CameraMultipliers[2], get2() / 256.0);
    if (tag == 0x2011 && len == 2) {
get2_256:
      m_ByteOrder = 0x4d4d;
      ASSIGN(m_CameraMultipliers[0], get2() / 256.0);
      ASSIGN(m_CameraMultipliers[2], get2() / 256.0);
    }
    if ((tag | 0x70) == 0x2070 && type == 4)
      fseek (m_InputFile, get4()+base, SEEK_SET);
    if (tag == 0x2010 && type != 7)
      m_LoadRawFunction = &CLASS olympus_load_raw;
    if (tag == 0x2020)
      parse_thumb_note (base, 257, 258);
    if (tag == 0x2040)
      parse_makernote (base, 0x2040);
    if (tag == 0xb028) {
      fseek (m_InputFile, get4()+base, SEEK_SET);
      parse_thumb_note (base, 136, 137);
    }
    if (tag == 0x4001 && len > 500) {
      i = len == 582 ? 50 : len == 653 ? 68 : len == 5120 ? 142 : 126;
      fseek (m_InputFile, i, SEEK_CUR);
get2_rggb:
      for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], get2());
      i = len >> 3 == 164 ? 112:22;
      fseek (m_InputFile, i, SEEK_CUR);
      for (c=0; c < 4; c++) sraw_mul[c ^ (c >> 1)] = get2();
    }
    if (tag == 0xa021)
      for (c=0; c<4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], get4());
    if (tag == 0xa028)
      for (c=0; c<4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], m_CameraMultipliers[c ^ (c >> 1)] - get4());
next:
    fseek (m_InputFile, save, SEEK_SET);
  }
quit:
  m_ByteOrder = sm_ByteOrder;
}

/*
   Since the TIFF DateTime string has no timezone information,
   assume that the camera's clock was set to Universal Time.
 */
void CLASS get_timestamp (int reversed)
{
  struct tm t;
  char str[20];
  int i;

  str[19] = 0;
  if (reversed)
    for (i=19; i--; ) str[i] = fgetc(m_InputFile);
  else
    ptfread (str, 19, 1, m_InputFile);
  memset (&t, 0, sizeof t);
  if (sscanf (str, "%d:%d:%d %d:%d:%d", &t.tm_year, &t.tm_mon,
  &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
    return;
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  t.tm_isdst = -1;
  if (mktime(&t) > 0)
    m_TimeStamp = mktime(&t);
}

void CLASS parse_exif (int base)
{
  unsigned kodak, entries, tag, type, len, save, c;
  double expo;

  kodak = !strncmp(m_CameraMake,"EASTMAN",7) && m_Tiff_NrIFDs < 3;
  entries = get2();
  while (entries--) {
    tiff_get (base, &tag, &type, &len, &save);
    switch (tag) {
      case 33434:  m_Shutter = getreal(type);   break;
      case 33437:  m_Aperture = getreal(type);    break;
      case 34855:  m_IsoSpeed = get2();     break;
      case 36867:
      case 36868:  get_timestamp(0);      break;
      case 37377:  if ((expo = -getreal(type)) < 128)
         m_Shutter = pow (2, expo);   break;
      case 37378:  m_Aperture = pow (2, getreal(type)/2); break;
      case 37386:  m_FocalLength = getreal(type);   break;
      case 37500:  parse_makernote (base, 0);   break;
      case 40962:  if (kodak) m_RawWidth  = get4(); break;
      case 40963:  if (kodak) m_RawHeight = get4(); break;
      case 41730:
  if (get4() == 0x20002)
    for (exif_cfa=c=0; c < 8; c+=2)
      exif_cfa |= fgetc(m_InputFile) * 0x01010101 << c;
    }
    fseek (m_InputFile, save, SEEK_SET);
  }
}

void CLASS romm_coeff (float romm_cam[3][3])
{
  static const float rgb_romm[3][3] = /* ROMM == Kodak ProPhoto */
  { {  2.034193, -0.727420, -0.306766 },
    { -0.228811,  1.231729, -0.002922 },
    { -0.008565, -0.153273,  1.161839 } };
  int i, j, k;

  for (i=0; i < 3; i++)
    for (j=0; j < 3; j++)
      for (m_cmatrix[i][j] = k=0; k < 3; k++)
  m_cmatrix[i][j] += rgb_romm[i][k] * romm_cam[k][j];
}

void CLASS parse_mos (int offset)
{
  char data[40];
  int skip, from, i, c, neut[4], planes=0, frot=0;
  static const char *mod[] =
  { "","DCB2","Volare","Cantare","CMost","Valeo 6","Valeo 11","Valeo 22",
    "Valeo 11p","Valeo 17","","Aptus 17","Aptus 22","Aptus 75","Aptus 65",
    "Aptus 54S","Aptus 65S","Aptus 75S","AFi 5","AFi 6","AFi 7",
        "","","","","","","","","","","","","","","","","","AFi-II 12" };
  float romm_cam[3][3];

  fseek (m_InputFile, offset, SEEK_SET);
  while (1) {
    if (get4() != 0x504b5453) break;
    get4();
    ptfread (data, 1, 40, m_InputFile);
    skip = get4();
    from = ftell(m_InputFile);
    if (!strcmp(data,"JPEG_preview_data")) {
      m_ThumbOffset = from;
      m_ThumbLength = skip;
    }
    if (!strcmp(data,"icc_camera_profile")) {
      m_ProfileOffset = from;
      m_ProfileLength = skip;
    }
    if (!strcmp(data,"ShootObj_back_type")) {
      ptfscanf (m_InputFile, "%d", &i);
      if ((unsigned) i < sizeof mod / sizeof (*mod))
  strcpy (m_CameraModel, mod[i]);
    }
    if (!strcmp(data,"icc_camera_to_tone_matrix")) {
      for (i=0; i < 9; i++)
  romm_cam[0][i] = int_to_float(get4());
      romm_coeff (romm_cam);
    }
    if (!strcmp(data,"CaptProf_color_matrix")) {
      for (i=0; i < 9; i++)
  ptfscanf (m_InputFile, "%f", &romm_cam[0][i]);
      romm_coeff (romm_cam);
    }
    if (!strcmp(data,"CaptProf_number_of_planes"))
      ptfscanf (m_InputFile, "%d", &planes);
    if (!strcmp(data,"CaptProf_raw_data_rotation"))
      ptfscanf (m_InputFile, "%d", &m_Flip);
    if (!strcmp(data,"CaptProf_mosaic_pattern"))
      for (c=0; c < 4; c++) {
  ptfscanf (m_InputFile, "%d", &i);
  if (i == 1) frot = c ^ (c >> 1);
      }
    if (!strcmp(data,"ImgProf_rotation_angle")) {
      ptfscanf (m_InputFile, "%d", &i);
      m_Flip = i - m_Flip;
    }
    if (!strcmp(data,"NeutObj_neutrals") && !VALUE(m_CameraMultipliers[0])) {
      for (c=0; c < 4; c++) ptfscanf (m_InputFile, "%d", neut+c);
      for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], (float) neut[0] / neut[c+1]);
    }
    if (!strcmp(data,"Rows_data"))
      m_Load_Flags = get4();
    parse_mos (from);
    fseek (m_InputFile, skip+from, SEEK_SET);
  }
  if (planes)
    m_Filters = (planes == 1) * 0x01010101 *
  (uint8_t) "\x94\x61\x16\x49"[(m_Flip/90 + frot) & 3];
}

void CLASS linear_table (unsigned len)
{
  int i;
  if (len > 0x1000) len = 0x1000;
  read_shorts (m_Curve, len);
  for (i=len; i < 0x1000; i++)
    m_Curve[i] = m_Curve[i-1];
  m_WhiteLevel = m_Curve[0xfff];
}

void CLASS parse_kodak_ifd (int base)
{
  unsigned entries, tag, type, len, save;
  int i, c, wbi=-2, wbtemp=6500;
  float mul[3] = {1,1,1}, num = 0;
  static const int wbtag[] = { 64037,64040,64039,64041,-1,-1,64042 };

  entries = get2();
  if (entries > 1024) return;
  while (entries--) {
    tiff_get (base, &tag, &type, &len, &save);
    if (tag == 1020) wbi = getint(type);
    if (tag == 1021 && len == 72) {   /* WB set in software */
      fseek (m_InputFile, 40, SEEK_CUR);
      for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], 2048.0 / get2());
      wbi = -2;
    }
    if (tag == 2118) wbtemp = getint(type);
    if (tag == (unsigned)(2130 + wbi))
      for (c=0; c<3; c++) mul[c] = getreal(type);
    if (tag == (unsigned)(2140 + wbi) && wbi >= 0)
      for (c=0; c<3; c++) {
  for (num=i=0; i < 4; i++)
    num += getreal(type) * pow (wbtemp/100.0, i);
  ASSIGN(m_CameraMultipliers[c], 2048 / (num * mul[c]));
      }
    if (tag == 2317) linear_table (len);
    if (tag == 6020) m_IsoSpeed = getint(type);
    if (tag == 64013) wbi = fgetc(m_InputFile);
    if (wbi < 7 && (int) tag == wbtag[wbi])
      for(c=0;c<3;c++) ASSIGN(m_CameraMultipliers[c],get4());
    if (tag == 64019) m_Width = getint(type);
    if (tag == 64020) m_Height = (getint(type)+1) & -2;

    fseek (m_InputFile, save, SEEK_SET);
  }
}

//void CLASS parse_minolta (int base);

int CLASS parse_tiff_ifd (int l_Base) {
  unsigned    l_Entries;
  unsigned    l_Tag;
  unsigned    l_Type;
  unsigned    l_Length;
  unsigned    l_plen=16;
  unsigned    l_Save;
  int         l_ifd;
  int         l_use_cm=0;
  int         l_cfa;
  int         i, j, c;
  int         l_ImageLength=0;
  int blrr=1, blrc=1, dblack[] = { 0,0,0,0 };
  char        l_Software[64];
  char        *l_cbuf;
  char        *cp;
  uint8_t       l_cfa_pat[16];
  uint8_t       l_cfa_pc[] = { 0,1,2,3 };
  uint8_t       l_tab[256];
  double      l_cc[4][4];
  double      l_cm[4][3];
  double      l_cam_xyz[4][3];
  double      l_num;
  double      l_ab[]={ 1,1,1,1 };
  double      l_asn[] = { 0,0,0,0 };
  double      l_xyz[] = { 1,1,1 };
  unsigned    l_sony_curve[] = { 0,0,0,0,0,4095 };
  unsigned*   l_buf;
  unsigned    l_sony_offset=0;
  unsigned    l_sony_length=0;
  unsigned    l_sony_key=0;
  FILE*       l_sfp;

  struct jhead l_JHead;

  if (m_Tiff_NrIFDs >= sizeof m_Tiff_IFD / sizeof m_Tiff_IFD[0])
    return 1;
  l_ifd = m_Tiff_NrIFDs++;
  for (j=0; j < 4; j++)
    for (i=0; i < 4; i++)
      l_cc[j][i] = (i == j);
  l_Entries = get2();
  if (l_Entries > 512) return 1;
  while (l_Entries--) {
    tiff_get (l_Base, &l_Tag, &l_Type, &l_Length, &l_Save);
    switch (l_Tag) {
      case 5:   m_Width  = get2();  break;
      case 6:   m_Height = get2();  break;
      case 7:   m_Width += get2();  break;
      case 9:  m_Filters = get2();  break;
      case 14: case 15: case 16:
        m_WhiteLevel = get2();
        break;
      case 17: case 18:
  if (l_Type == 3 && l_Length == 1)
    ASSIGN(m_CameraMultipliers[(l_Tag-17)*2], get2() / 256.0);
  break;
      case 23:
  if (l_Type == 3) m_IsoSpeed = get2();
  break;
      case 36: case 37: case 38:
  ASSIGN(m_CameraMultipliers[l_Tag-0x24], get2());
  break;
      case 39:
  if (l_Length < 50 || VALUE(m_CameraMultipliers[0])) break;
  fseek (m_InputFile, 12, SEEK_CUR);
  for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], get2());
  break;
      case 46:
  if (l_Type != 7 || fgetc(m_InputFile) != 0xff || fgetc(m_InputFile) != 0xd8) break;
  m_ThumbOffset = ftell(m_InputFile) - 2;
  m_ThumbLength = l_Length;
  break;
      case 61440:     /* Fuji HS10 table */
  parse_tiff_ifd (l_Base);
  break;
      case 2: case 256: case 61441: /* ImageWidth */
  m_Tiff_IFD[l_ifd].width = getint(l_Type);
  break;
      case 3: case 257: case 61442: /* ImageHeight */
  m_Tiff_IFD[l_ifd].height = getint(l_Type);
  break;
      case 258:       /* BitsPerSample */
      case 61443:
  m_Tiff_IFD[l_ifd].samples = l_Length & 7;
  m_Tiff_IFD[l_ifd].bps = getint(l_Type);
  break;
      case 61446:
  m_RawHeight = 0;
  m_LoadRawFunction = &CLASS packed_load_raw;
  m_Load_Flags = get4() && (m_Filters=0x16161616) ? 24:80;
  break;
      case 259:       /* Compression */
  m_Tiff_IFD[l_ifd].comp = getint(l_Type);
  break;
      case 262:       /* PhotometricInterpretation */
  m_Tiff_IFD[l_ifd].phint = get2();
  break;
      case 270:       /* ImageDescription */
  ptfread (m_Description, 512, 1, m_InputFile);
  break;
      case 271:       /* Make */
  ptfgets (m_CameraMake, 64, m_InputFile);
  break;
      case 272:       /* Model */
  ptfgets (m_CameraModel, 64, m_InputFile);
  break;
      case 280:       /* Panasonic RW2 offset */
  if (l_Type != 4) break;
  m_LoadRawFunction = &CLASS panasonic_load_raw;
  m_Load_Flags = 0x2008;

      case 273:       /* StripOffset */
      case 513:       /* JpegIFOffset */
      case 61447:
  m_Tiff_IFD[l_ifd].offset = get4()+l_Base;
  if (!m_Tiff_IFD[l_ifd].bps && m_Tiff_IFD[l_ifd].offset > 0) {
    fseek (m_InputFile, m_Tiff_IFD[l_ifd].offset, SEEK_SET);
    if (ljpeg_start (&l_JHead, 1)) {
      m_Tiff_IFD[l_ifd].comp    = 6;
      m_Tiff_IFD[l_ifd].width   = l_JHead.wide;
      m_Tiff_IFD[l_ifd].height  = l_JHead.high;
      m_Tiff_IFD[l_ifd].bps     = l_JHead.bits;
      m_Tiff_IFD[l_ifd].samples = l_JHead.clrs;
      if (!(l_JHead.sraw || (l_JHead.clrs & 1)))
        m_Tiff_IFD[l_ifd].width *= l_JHead.clrs;
      i = m_ByteOrder;
      parse_tiff (m_Tiff_IFD[l_ifd].offset + 12);
      m_ByteOrder = i;
    }
  }
  break;
      case 274:       /* Orientation */
  m_Tiff_IFD[l_ifd].flip = "50132467"[get2() & 7]-'0';
  break;
      case 277:       /* SamplesPerPixel */
  m_Tiff_IFD[l_ifd].samples = getint(l_Type) & 7;
  break;
      case 279:       /* StripByteCounts */
      case 514:
      case 61448:
  m_Tiff_IFD[l_ifd].bytes = get4();
  break;
      case 61454:
  for (c=0; c < 3; c++) ASSIGN(m_CameraMultipliers[(4-c) % 3],getint(l_Type));
  break;
      case 305: case 11:    /* Software */
  ptfgets (l_Software, 64, m_InputFile);
  if (!strncmp(l_Software,"Adobe",5) ||
      !strncmp(l_Software,"dcraw",5) ||
      !strncmp(l_Software,"UFRaw",5) ||
      !strncmp(l_Software,"Bibble",6) ||
      !strncmp(l_Software,"Nikon Scan",10) ||
      !strcmp (l_Software,"Digital Photo Professional"))
    m_IsRaw = 0;
  break;
      case 306:       /* DateTime */
  get_timestamp(0);
  break;
      case 315:       /* Artist */
  ptfread (m_Artist, 64, 1, m_InputFile);
  break;
      case 322:       /* TileWidth */
  m_Tiff_IFD[l_ifd].tile_width = getint(l_Type);
  break;
      case 323:       /* TileLength */
  m_Tiff_IFD[l_ifd].tile_length = getint(l_Type);
  break;
      case 324:       /* TileOffsets */
  m_Tiff_IFD[l_ifd].offset = l_Length > 1 ? ftell(m_InputFile) : get4();
  if (l_Length == 4) {
    m_LoadRawFunction = &CLASS sinar_4shot_load_raw;
    m_IsRaw = 5;
  }
  break;
      case 330:       /* SubIFDs */
  if (!strcmp(m_CameraModel,"DSLR-A100") && m_Tiff_IFD[l_ifd].width == 3872) {
          m_LoadRawFunction = &CLASS sony_arw_load_raw;
    m_Data_Offset = get4()+l_Base;
    l_ifd++;  break;
  }
  while (l_Length--) {
    i = ftell(m_InputFile);
    fseek (m_InputFile, get4()+l_Base, SEEK_SET);
    if (parse_tiff_ifd (l_Base)) break;
    fseek (m_InputFile, i+4, SEEK_SET);
  }
  break;
      case 400:
  strcpy (m_CameraMake, "Sarnoff");
  m_WhiteLevel = 0xfff;
  break;
      case 28688:
  for (c=0; c < 4; c++) l_sony_curve[c+1] = get2() >> 2 & 0xfff;
  for (i=0; i < 5; i++)
    for (j = l_sony_curve[i]+1; (unsigned) j <= l_sony_curve[i+1]; j++)
      m_Curve[j] = m_Curve[j-1] + (1 << i);
  break;
      case 29184: l_sony_offset = get4();  break;
      case 29185: l_sony_length = get4();  break;
      case 29217: l_sony_key    = get4();  break;
      case 29264:
  parse_minolta (ftell(m_InputFile));
  m_RawWidth = 0;
  break;
      case 29443:
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c < 2)], get2());
  break;
      case 29459:
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c], get2());
  i = (m_CameraMultipliers[1] == 1024 && m_CameraMultipliers[2] == 1024) << 1;
  SWAP (m_CameraMultipliers[i],m_CameraMultipliers[i+1])
  break;
      case 33405:     /* Model2 */
  ptfgets (m_CameraModelBis, 64, m_InputFile);
  break;
      case 33422:     /* CFAPattern */
      case 64777:     /* Kodak P-series */
  if ((l_plen=l_Length) > 16) l_plen = 16;
  ptfread (l_cfa_pat, 1, l_plen, m_InputFile);
  for (m_Colors=l_cfa=i=0; (unsigned) i < l_plen; i++) {
    m_Colors += !(l_cfa & (1 << l_cfa_pat[i]));
    l_cfa |= 1 << l_cfa_pat[i];
  }
  if (l_cfa == 070) memcpy (l_cfa_pc,"\003\004\005",3); /* CMY */
  if (l_cfa == 072) memcpy (l_cfa_pc,"\005\003\004\001",4); /* GMCY */
  goto guess_l_cfa_pc;
      case 33424:
      case 65024:
  fseek (m_InputFile, get4()+l_Base, SEEK_SET);
  parse_kodak_ifd (l_Base);
  break;
      case 33434:     /* ExposureTime */
  m_Shutter = getreal(l_Type);
  break;
      case 33437:     /* FNumber */
  m_Aperture = getreal(l_Type);
  break;
      case 34306:     /* Leaf white balance */
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ 1], 4096.0 / get2());
  break;
      case 34307:     /* Leaf CatchLight color matrix */
  ptfread (l_Software, 1, 7, m_InputFile);
  if (strncmp(l_Software,"MATRIX",6)) break;
  m_Colors = 4;
  for (m_RawColor = i=0; i < 3; i++) {
    for (c=0; c < 4; c++) ptfscanf (m_InputFile, "%f", &m_MatrixCamRGBToSRGB[i][c^1]);
    if (!m_UserSetting_CameraWb) continue;
    l_num = 0;
    for (c=0; c < 4; c++) l_num += m_MatrixCamRGBToSRGB[i][c];
    for (c=0; c < 4; c++) m_MatrixCamRGBToSRGB[i][c] /= l_num;
  }
  break;
      case 34310:     /* Leaf metadata */
  parse_mos (ftell(m_InputFile));
      case 34303:
  strcpy (m_CameraMake, "Leaf");
  break;
      case 34665:     /* EXIF tag */
  fseek (m_InputFile, get4()+l_Base, SEEK_SET);
  parse_exif (l_Base);
  break;
      case 34675:     /* InterColorProfile */
      case 50831:     /* AsShotICCProfile */
  m_ProfileOffset = ftell(m_InputFile);
  m_ProfileLength = l_Length;
  break;
      case 37122:     /* CompressedBitsPerPixel */
  m_Kodak_cbpp = get4();
  break;
      case 37386:     /* FocalLength */
  m_FocalLength = getreal(l_Type);
  break;
      case 37393:     /* ImageNumber */
  m_ShotOrder = getint(l_Type);
  break;
      case 37400:     /* old Kodak KDC tag */
  for (m_RawColor = i=0; i < 3; i++) {
    getreal(l_Type);
    for (c=0; c<3; c++) m_MatrixCamRGBToSRGB[i][c] = getreal(l_Type);
  }
  break;
      case 46275:     /* Imacon tags */
  strcpy (m_CameraMake, "Imacon");
  m_Data_Offset = ftell(m_InputFile);
  l_ImageLength = l_Length;
  break;
      case 46279:
        if (l_ImageLength) break;
  fseek (m_InputFile, 38, SEEK_CUR);
      case 46274:
  fseek (m_InputFile, 40, SEEK_CUR);
  m_RawWidth  = get4();
  m_RawHeight = get4();
  m_LeftMargin = get4() & 7;
  m_Width = m_RawWidth - m_LeftMargin - (get4() & 7);
  m_TopMargin = get4() & 7;
  m_Height = m_RawHeight - m_TopMargin - (get4() & 7);
  if (m_RawWidth == 7262) {
    m_Height = 5444;
    m_Width  = 7244;
    m_LeftMargin = 7;
  }
  fseek (m_InputFile, 52, SEEK_CUR);
  for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], getreal(11));
  fseek (m_InputFile, 114, SEEK_CUR);
  m_Flip = (get2() >> 7) * 90;
  if (m_Width * m_Height * 6 == l_ImageLength) {
    if (m_Flip % 180 == 90) SWAP(m_Width,m_Height);
    m_RawWidth = m_Width;
    m_RawHeight = m_Height;
    m_LeftMargin = m_TopMargin = m_Filters = m_Flip = 0;
  }
  sprintf (m_CameraModel, "Ixpress %d-Mp", m_Height*m_Width/1000000);
  m_LoadRawFunction = &CLASS imacon_full_load_raw;
  if (m_Filters) {
    if (m_LeftMargin & 1) m_Filters = 0x61616161;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
  }
  m_WhiteLevel = 0xffff;
  break;
      case 50454:     /* Sinar tag */
      case 50455:
  if (!(l_cbuf = (char *) MALLOC(l_Length))) break;
  ptfread (l_cbuf, 1, l_Length, m_InputFile);
  for (cp = l_cbuf-1; cp && cp < l_cbuf+l_Length; cp = strchr(cp,'\n'))
    if (!strncmp (++cp,"Neutral ",8))
      sscanf (cp+8, "%f %f %f",
                    &VALUE(m_CameraMultipliers[0]),
                    &VALUE(m_CameraMultipliers[1]),
                    &VALUE(m_CameraMultipliers[2]));
  FREE (l_cbuf);
  break;
      case 50458:
        if(!m_CameraMake[0]) strcpy(m_CameraMake,"Hasselblad");
        break;
      case 50459:     /* Hasselblad tag */
  i = m_ByteOrder;
  j = ftell(m_InputFile);
  c = m_Tiff_NrIFDs;
  m_ByteOrder = get2();
  fseek (m_InputFile, j+(get2(),get4()), SEEK_SET);
  parse_tiff_ifd (j);
  m_WhiteLevel = 0xffff;
  m_Tiff_NrIFDs = c;
  m_ByteOrder = i;
  break;
      case 50706:     /* DNGVersion */
  for (c=0; c < 4; c++) m_DNG_Version = (m_DNG_Version << 8) + fgetc(m_InputFile);
        if (!m_CameraMake[0]) strcpy (m_CameraMake,"DNG");
        m_IsRaw = 1;
  break;
      case 50710:     /* CFAPlaneColor */
  if (l_Length > 4) l_Length = 4;
  m_Colors = l_Length;
  ptfread (l_cfa_pc, 1, m_Colors, m_InputFile);
guess_l_cfa_pc:
  for (c=0; c < m_Colors; c++) l_tab[l_cfa_pc[c]] = c;
  m_ColorDescriptor[c] = 0;
  for (i=16; i--; )
    m_Filters = m_Filters << 2 | l_tab[l_cfa_pat[i % l_plen]];
  break;
      case 50711:     /* CFALayout */
  if (get2() == 2) {
    m_Fuji_Width = 1;
    m_Filters = 0x49494949;
  }
  break;
      case 291:
      case 50712:     /* LinearizationTable */
  linear_table (l_Length);
  break;
        case 50713:     /* BlackLevelRepeatDim */
  blrr = get2();
  blrc = get2();
  break;
      case 61450:
  blrr = blrc = 2;
      case 50714:     /* BlackLevel */
  m_BlackLevel = getreal(l_Type);
  if (!m_Filters || !~m_Filters) break;
  dblack[0] = m_BlackLevel;
  dblack[1] = (blrc == 2) ? getreal(l_Type):dblack[0];
  dblack[2] = (blrr == 2) ? getreal(l_Type):dblack[0];
  dblack[3] = (blrc == 2 && blrr == 2) ? getreal(l_Type):dblack[1];
  if (m_Colors == 3)
    m_Filters |= ((m_Filters >> 2 & 0x22222222) |
          (m_Filters << 2 & 0x88888888)) & m_Filters << 1;
  for (c=0; c<4; c++) m_CBlackLevel[m_Filters >> (c << 1) & 3] = dblack[c];
  m_BlackLevel = 0;
  break;
      case 50715:     /* BlackLevelDeltaH */
      case 50716:     /* BlackLevelDeltaV */
  for (l_num=i=0; (unsigned)i < l_Length; i++)
    l_num += getreal(l_Type);
  m_BlackLevel += l_num/l_Length + 0.5;
  break;
      case 50717:     /* WhiteLevel */
  m_WhiteLevel = getint(l_Type);
  break;
      case 50718:     /* DefaultScale */
  m_PixelAspect  = getreal(l_Type);
  m_PixelAspect /= getreal(l_Type);
  break;
      case 50721:     /* ColorMatrix1 */
      case 50722:     /* ColorMatrix2 */
  for (c=0; c < m_Colors; c++) for (j=0; j < 3; j++)
    l_cm[c][j] = getreal(l_Type);
  l_use_cm = 1;
  break;
      case 50723:     /* CameraCalibration1 */
      case 50724:     /* CameraCalibration2 */
  for (i=0; i < m_Colors; i++)
    for (c=0; c < m_Colors; c++) l_cc[i][c] = getreal(l_Type);
  break;
      case 50727:     /* AnalogBalance */
  for (c=0; c < m_Colors; c++) l_ab[c] = getreal(l_Type);
  break;
      case 50728:     /* AsShotNeutral */
  for (c=0; c < m_Colors; c++) l_asn[c] = getreal(l_Type);
  break;
      case 50729:     /* AsShotWhiteXY */
  l_xyz[0] = getreal(l_Type);
  l_xyz[1] = getreal(l_Type);
  l_xyz[2] = 1 - l_xyz[0] - l_xyz[1];
  for (c=0; c<3; c++) l_xyz[c] /= D65Reference[c];
  break;
      case 50740:     /* DNGPrivateData */
  if (m_DNG_Version) break;
  parse_minolta (j = get4()+l_Base);
  fseek (m_InputFile, j, SEEK_SET);
  parse_tiff_ifd (l_Base);
  break;
      case 50752:
  read_shorts (cr2_slice, 3);
  break;
      case 50829:     /* ActiveArea */
  m_TopMargin = getint(l_Type);
  m_LeftMargin = getint(l_Type);
  m_Height = getint(l_Type) - m_TopMargin;
  m_Width = getint(l_Type) - m_LeftMargin;
  break;
      case 50830:			/* MaskedAreas */
        for (i=0; i < (int32_t)l_Length && i < 32; i++)
    m_Mask[0][i] = getint(l_Type);
  m_BlackLevel = 0;
  break;
      case 51009:			/* OpcodeList2 */
  meta_offset = ftell(m_InputFile);
  break;
      case 64772:     /* Kodak P-series */
        if (l_Length < 13) break;
  fseek (m_InputFile, 16, SEEK_CUR);
  m_Data_Offset = get4();
  fseek (m_InputFile, 28, SEEK_CUR);
  m_Data_Offset += get4();
  m_LoadRawFunction = &CLASS packed_load_raw;
        break;
      case 65026:
        if (l_Type == 2) ptfgets(m_CameraModelBis,64,m_InputFile);
    }
    fseek (m_InputFile, l_Save, SEEK_SET);
  }
  if (l_sony_length && (l_buf = (unsigned *) MALLOC(l_sony_length))) {
    fseek (m_InputFile, l_sony_offset, SEEK_SET);
    ptfread (l_buf, l_sony_length, 1, m_InputFile);
    sony_decrypt (l_buf, l_sony_length/4, 1, l_sony_key);
    l_sfp = m_InputFile;
    if ((m_InputFile = tmpfile())) {
      ptfwrite (l_buf, l_sony_length, 1, m_InputFile);
      fseek (m_InputFile, 0, SEEK_SET);
      parse_tiff_ifd (-l_sony_offset);
      FCLOSE (m_InputFile);
    }
    m_InputFile = l_sfp;
    FREE (l_buf);
  }
  for (i=0; i < m_Colors; i++)
    for (c=0; c < m_Colors; c++) l_cc[i][c] *= l_ab[i];
  if (l_use_cm) {
    for (c=0; c < m_Colors; c++) for (i=0; i < 3; i++)
      for (l_cam_xyz[c][i]=j=0; j < m_Colors; j++)
  l_cam_xyz[c][i] += l_cc[c][j] * l_cm[j][i] * l_xyz[i];
    cam_xyz_coeff (l_cam_xyz);
  }
  if (l_asn[0]) {
    ASSIGN(m_CameraMultipliers[3], 0);
    for (c=0; c < m_Colors; c++) ASSIGN(m_CameraMultipliers[c], 1 / l_asn[c]);
  }

  if (!l_use_cm)
    for (c=0; c < m_Colors; c++)
      ASSIGN(m_D65Multipliers[c],VALUE(m_D65Multipliers[c]) / l_cc[c][c]);

  return 0;
}

// To get out of the mess what is global and what is local
// I started here with l_ tagging of the variables.
// (but the most simple like i or c which are evidently , hopefully , local)
// What remains must be a global.
// (jpta) This is what my EOS400D would do.

int CLASS parse_tiff (int l_Base) {
  // Set to start of tiff
  fseek (m_InputFile, l_Base, SEEK_SET);

  // Check byte order and return if not OK
  m_ByteOrder = get2();
  if (m_ByteOrder != 0x4949 && m_ByteOrder != 0x4d4d) return 0;

  get2();

  int l_doff;
  while ((l_doff = get4())) {
    fseek (m_InputFile, l_doff+l_Base, SEEK_SET);
    if (parse_tiff_ifd (l_Base)) break;
  }
  return 1;
}

void CLASS apply_tiff()
{
  int l_MaxSample=0;
  int l_Raw=-1;
  int l_Thumb=-1;
  int i;

  struct jhead l_Jhead;

  m_ThumbMisc = 16;
  if (m_ThumbOffset) {
    fseek (m_InputFile,m_ThumbOffset,SEEK_SET);
    if (ljpeg_start (&l_Jhead, 1)) {
      m_ThumbMisc   = l_Jhead.bits;
      m_ThumbWidth  = l_Jhead.wide;
      m_ThumbHeight = l_Jhead.high;
    }
  }

  for (i=0; (unsigned) i < m_Tiff_NrIFDs; i++) {
    if (l_MaxSample < m_Tiff_IFD[i].samples)
  l_MaxSample = m_Tiff_IFD[i].samples;
    if (l_MaxSample > 3) l_MaxSample = 3;
    if ((m_Tiff_IFD[i].comp != 6 || m_Tiff_IFD[i].samples != 3) &&
        (m_Tiff_IFD[i].width | m_Tiff_IFD[i].height) < 0x10000 &&
         m_Tiff_IFD[i].width*m_Tiff_IFD[i].height > m_RawWidth*m_RawHeight) {
      m_RawWidth      = m_Tiff_IFD[i].width;
      m_RawHeight     = m_Tiff_IFD[i].height;
      m_Tiff_bps      = m_Tiff_IFD[i].bps;
      m_Tiff_Compress = m_Tiff_IFD[i].comp;
      m_Data_Offset   = m_Tiff_IFD[i].offset;
      m_Tiff_Flip     = m_Tiff_IFD[i].flip;
      m_Tiff_Samples  = m_Tiff_IFD[i].samples;
      m_TileWidth     = m_Tiff_IFD[i].tile_width;
      m_TileLength    = m_Tiff_IFD[i].tile_length;
      l_Raw           = i;
    }
  }
  if (!m_TileWidth ) m_TileWidth  = INT_MAX;
  if (!m_TileLength) m_TileLength = INT_MAX;
  for (i=m_Tiff_NrIFDs; i--; )
    if (m_Tiff_IFD[i].flip) m_Tiff_Flip = m_Tiff_IFD[i].flip;

  if (l_Raw >= 0 && !m_LoadRawFunction)
    switch (m_Tiff_Compress) {
      case 32767:
        if (m_Tiff_IFD[l_Raw].bytes == m_RawWidth*m_RawHeight) {
    m_Tiff_bps = 12;
    m_LoadRawFunction = &CLASS sony_arw2_load_raw;
    break;
  }
  if (m_Tiff_IFD[l_Raw].bytes*8U != m_RawWidth*m_RawHeight*m_Tiff_bps) {
    m_RawHeight += 8;
    m_LoadRawFunction = &CLASS sony_arw_load_raw;
    break;
  }
  m_Load_Flags = 79;
      case 32769:
        m_Load_Flags++;
      case 32770:
      case 32773: goto slr;
      case 0:  case 1:
  if (m_Tiff_IFD[l_Raw].bytes*5 == m_RawWidth*m_RawHeight*8) {
    m_Load_Flags = 81;
    m_Tiff_bps = 12;
  } slr:
  switch (m_Tiff_bps) {
    case  8: m_LoadRawFunction = &CLASS eight_bit_load_raw;	break;
    case 12: if (m_Tiff_IFD[l_Raw].phint == 2)
         m_Load_Flags = 6;
       m_LoadRawFunction = &CLASS packed_load_raw;		break;
    case 14: m_Load_Flags = 0;
    case 16: m_LoadRawFunction = &CLASS unpacked_load_raw;		break;
  }
  break;
      case 6:  case 7:  case 99:
  m_LoadRawFunction = &CLASS lossless_jpeg_load_raw;		break;
      case 262:
  m_LoadRawFunction = &CLASS kodak_262_load_raw;			break;
      case 34713:
	if ((m_RawWidth+9)/10*16*m_RawHeight == m_Tiff_IFD[l_Raw].bytes) {
	  m_LoadRawFunction = &CLASS packed_load_raw;
	  m_Load_Flags = 1;
	} else if (m_RawWidth*m_RawHeight*2 == m_Tiff_IFD[l_Raw].bytes) {
	  m_LoadRawFunction = &CLASS unpacked_load_raw;
	  m_Load_Flags = 4;
	  m_ByteOrder = 0x4d4d;
	} else
	  m_LoadRawFunction = &CLASS nikon_load_raw;			break;
      case 34892:
  m_LoadRawFunction = &CLASS lossy_dng_load_raw;		break;
      case 65535:
  m_LoadRawFunction = &CLASS pentax_load_raw;
        break;
      case 65000:
  switch (m_Tiff_IFD[l_Raw].phint) {
    case 2: m_LoadRawFunction = &CLASS kodak_rgb_load_raw;
                  m_Filters = 0;
                  break;
    case 6: m_LoadRawFunction = &CLASS kodak_ycbcr_load_raw;
                  m_Filters = 0;
                  break;
    case 32803: m_LoadRawFunction = &CLASS kodak_65000_load_raw;
  }
      case 32867: break;
      default: m_IsRaw = 0;
    }
  if (!m_DNG_Version)
    if ( (m_Tiff_Samples == 3 && m_Tiff_IFD[l_Raw].bytes &&
	  m_Tiff_bps != 14 && m_Tiff_bps != 2048 && 
	  m_Tiff_Compress != 32769 && m_Tiff_Compress != 32770)
      || (m_Tiff_bps == 8 && !strstr(m_CameraMake,"KODAK") &&
          !strstr(m_CameraMake,"Kodak") &&
    !strstr(m_CameraModelBis,"DEBUG RAW")))
      m_IsRaw = 0;

  for (i=0; (unsigned) i < m_Tiff_NrIFDs; i++) {
    auto l_denom1 = SQR(m_Tiff_IFD[i].bps+1);
    auto l_denom2 = SQR(m_ThumbMisc+1);
    if (l_denom1 == 0 || l_denom2 == 0)
      continue;   // prevent divide by zero crash

    if (i != l_Raw &&
        m_Tiff_IFD[i].samples == l_MaxSample &&
        (unsigned)(m_Tiff_IFD[i].width * m_Tiff_IFD[i].height / l_denom1) >
            m_ThumbWidth * m_ThumbHeight / l_denom2 &&
        m_Tiff_IFD[i].comp != 34892)
    {
      m_ThumbWidth  = m_Tiff_IFD[i].width;
      m_ThumbHeight = m_Tiff_IFD[i].height;
      m_ThumbOffset = m_Tiff_IFD[i].offset;
      m_ThumbLength = m_Tiff_IFD[i].bytes;
      m_ThumbMisc   = m_Tiff_IFD[i].bps;
      l_Thumb = i;
    }
  }

  if (l_Thumb >= 0) {
    m_ThumbMisc |= m_Tiff_IFD[l_Thumb].samples << 5;
    switch (m_Tiff_IFD[l_Thumb].comp) {
      case 0:
  m_WriteThumb = &CLASS layer_thumb;
  break;
      case 1:
  if (m_Tiff_IFD[l_Thumb].bps <= 8)
    m_WriteThumb = &CLASS ppm_thumb;
  else if (!strcmp(m_CameraMake,"Imacon"))
    m_WriteThumb = &CLASS ppm16_thumb;
  else
    m_ThumbLoadRawFunction = &CLASS kodak_thumb_load_raw;
  break;
      case 65000:
  m_ThumbLoadRawFunction = m_Tiff_IFD[l_Thumb].phint == 6 ?
    &CLASS kodak_ycbcr_load_raw : &CLASS kodak_rgb_load_raw;
    }
  }
}

void CLASS parse_minolta (int base)
{
  int save, tag, len, offset, high=0, wide=0, i, c;
  short sorder=m_ByteOrder;

  fseek (m_InputFile, base, SEEK_SET);
  if (fgetc(m_InputFile) || fgetc(m_InputFile)-'M' || fgetc(m_InputFile)-'R') return;
  m_ByteOrder = fgetc(m_InputFile) * 0x101;
  offset = base + get4() + 8;
  while ((save=ftell(m_InputFile)) < offset) {
    for (tag=i=0; i < 4; i++)
      tag = tag << 8 | fgetc(m_InputFile);
    len = get4();
    switch (tag) {
      case 0x505244:        /* PRD */
  fseek (m_InputFile, 8, SEEK_CUR);
  high = get2();
  wide = get2();
  break;
      case 0x574247:        /* WBG */
  get4();
  i = strcmp(m_CameraModel,"DiMAGE A200") ? 0:3;
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1) ^ i], get2());
  break;
      case 0x545457:        /* TTW */
  parse_tiff (ftell(m_InputFile));
  m_Data_Offset = offset;
    }
    fseek (m_InputFile, save+len+8, SEEK_SET);
  }
  m_RawHeight = high;
  m_RawWidth  = wide;
  m_ByteOrder = sorder;
}

/*
   Many cameras have a "debug mode" that writes JPEG and raw
   at the same time.  The raw file has no header, so try to
   to open the matching JPEG file and read its metadata.
 */
void CLASS parse_external_jpeg()
{
  const char *file, *ext;
  char *jname, *jfile, *jext;
  FILE *save=m_InputFile;

  ext  = strrchr (m_UserSetting_InputFileName, '.');
  file = strrchr (m_UserSetting_InputFileName, '/');
  if (!file) file = strrchr (m_UserSetting_InputFileName, '\\');
  if (!file) file = m_UserSetting_InputFileName-1;
  file++;
  if (!ext || strlen(ext) != 4 || ext-file != 8) return;
  jname = (char *) MALLOC (strlen(m_UserSetting_InputFileName) + 1);
  merror (jname, "parse_external_jpeg()");
  strcpy (jname, m_UserSetting_InputFileName);
  jfile = file - m_UserSetting_InputFileName + jname;
  jext  = ext  - m_UserSetting_InputFileName + jname;
  if (strcasecmp (ext, ".jpg")) {
    strcpy (jext, isupper(ext[1]) ? ".JPG":".jpg");
    if (isdigit(*file)) {
      memcpy (jfile, file+4, 4);
      memcpy (jfile+4, file, 4);
    }
  } else
    while (isdigit(*--jext)) {
      if (*jext != '9') {
        (*jext)++;
  break;
      }
      *jext = '0';
    }
  if (strcmp (jname, m_UserSetting_InputFileName)) {
    if ((m_InputFile = fopen (jname, "rb"))) {
      TRACEKEYVALS("Reading metadata from","%s",jname);
      parse_tiff (12);
      m_ThumbOffset = 0;
      m_IsRaw = 1;
      FCLOSE (m_InputFile);
    }
  }
  if (!m_TimeStamp)
    fprintf (stderr,_("Failed to read metadata from %s\n"), jname);
  FREE (jname);
  m_InputFile = save;
}

/*
   CIFF block 0x1030 contains an 8x8 white sample.
   Load this into white[][] for use in scale_colors().
 */
void CLASS ciff_block_1030()
{
  static const uint16_t key[] = { 0x410, 0x45f3 };
  int i, bpp, row, col, vbits=0;
  unsigned long bitbuf=0;

  if ((get2(),get4()) != 0x80008 || !get4()) return;
  bpp = get2();
  if (bpp != 10 && bpp != 12) return;
  for (i=row=0; row < 8; row++)
    for (col=0; col < 8; col++) {
      if (vbits < bpp) {
  bitbuf = bitbuf << 16 | (get2() ^ key[i++ & 1]);
  vbits += 16;
      }
      white[row][col] =
  bitbuf << (LONG_BIT - vbits) >> (LONG_BIT - bpp);
      vbits -= bpp;
    }
}

/*
   Parse a CIFF file, better known as Canon CRW format.
 */
void CLASS parse_ciff (int offset, int length)
{
  int tboff, nrecs, c, type, len, save, wbi=-1;
  uint16_t key[] = { 0x410, 0x45f3 };

  fseek (m_InputFile, offset+length-4, SEEK_SET);
  tboff = get4() + offset;
  fseek (m_InputFile, tboff, SEEK_SET);
  nrecs = get2();
  if (nrecs > 100) return;
  while (nrecs--) {
    type = get2();
    len  = get4();
    save = ftell(m_InputFile) + 4;
    fseek (m_InputFile, offset+get4(), SEEK_SET);
    if ((((type >> 8) + 8) | 8) == 0x38)
      parse_ciff (ftell(m_InputFile), len); /* Parse a sub-table */

    if (type == 0x0810)
      ptfread (m_Artist, 64, 1, m_InputFile);
    if (type == 0x080a) {
      ptfread (m_CameraMake, 64, 1, m_InputFile);
      fseek (m_InputFile, strlen(m_CameraMake) - 63, SEEK_CUR);
      ptfread (m_CameraModel, 64, 1, m_InputFile);
    }
    if (type == 0x1810) {
      fseek (m_InputFile, 12, SEEK_CUR);
      m_Flip = get4();
    }
    if (type == 0x1835)     /* Get the decoder table */
      m_Tiff_Compress = get4();
    if (type == 0x2007) {
      m_ThumbOffset = ftell(m_InputFile);
      m_ThumbLength = len;
    }
    if (type == 0x1818) {
      m_Shutter = pow (2, -int_to_float((get4(),get4())));
      m_Aperture = pow (2, int_to_float(get4())/2);
    }
    if (type == 0x102a) {
      m_IsoSpeed = pow (2, (get4(),get2())/32.0 - 4) * 50;
      m_Aperture  = pow (2, (get2(),(short)get2())/64.0);
      m_Shutter   = pow (2,-((short)get2())/32.0);
      wbi = (get2(),get2());
      if (wbi > 17) wbi = 0;
      fseek (m_InputFile, 32, SEEK_CUR);
      if (m_Shutter > 1e6) m_Shutter = get2()/10.0;
    }
    if (type == 0x102c) {
      if (get2() > 512) {   /* Pro90, G1 */
  fseek (m_InputFile, 118, SEEK_CUR);
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ 2], get2());
      } else {        /* G2, S30, S40 */
  fseek (m_InputFile, 98, SEEK_CUR);
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1) ^ 1], get2());
      }
    }
    if (type == 0x0032) {
      if (len == 768) {     /* EOS D30 */
  fseek (m_InputFile, 72, SEEK_CUR);
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], 1024.0 / get2());
  if (!wbi) ASSIGN(m_CameraMultipliers[0], -1); /* use my auto white balance */
      } else if (!VALUE(m_CameraMultipliers[0])) {
  if (get2() == key[0])   /* Pro1, G6, S60, S70 */
    c = (strstr(m_CameraModel,"Pro1") ?
        "012346000000000000":"01345:000000006008")[wbi]-'0'+ 2;
  else {        /* G3, G5, S45, S50 */
    c = "023457000000006000"[wbi]-'0';
    key[0] = key[1] = 0;
  }
  fseek (m_InputFile, 78 + c*8, SEEK_CUR);
  for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1) ^ 1], get2() ^ key[c & 1]);
  if (!wbi) ASSIGN(m_CameraMultipliers[0], -1);
      }
    }
    if (type == 0x10a9) {   /* D60, 10D, 300D, and clones */
      if (len > 66) wbi = "0134567028"[wbi]-'0';
      fseek (m_InputFile, 2 + wbi*8, SEEK_CUR);
      for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], get2());
    }
    if (type == 0x1030 && (0x18040 >> wbi & 1))
      ciff_block_1030();    /* all that don't have 0x10a9 */
    if (type == 0x1031) {
      m_RawWidth = (get2(),get2());
      m_RawHeight = get2();
    }
    if (type == 0x5029) {
      m_FocalLength = len >> 16;
      if ((len & 0xffff) == 2) m_FocalLength /= 32;
    }
    if (type == 0x5813) flash_used = int_to_float(len);
    if (type == 0x5814) canon_ev   = int_to_float(len);
    if (type == 0x5817) m_ShotOrder = len;
    if (type == 0x5834) unique_id  = len;
    if (type == 0x580e) m_TimeStamp  = len;
    if (type == 0x180e) m_TimeStamp  = get4();
#ifdef LOCALTIME
    if ((type | 0x4000) == 0x580e)
      m_TimeStamp = mktime (gmtime (&m_TimeStamp));
#endif
    fseek (m_InputFile, save, SEEK_SET);
  }
}

void CLASS parse_rollei()
{
  char line[128], *val;
  struct tm t;

  fseek (m_InputFile, 0, SEEK_SET);
  memset (&t, 0, sizeof t);
  do {
    ptfgets (line, 128, m_InputFile);
    if ((val = strchr(line,'=')))
      *val++ = 0;
    else
      val = line + strlen(line);
    if (!strcmp(line,"DAT"))
      sscanf (val, "%d.%d.%d", &t.tm_mday, &t.tm_mon, &t.tm_year);
    if (!strcmp(line,"TIM"))
      sscanf (val, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
    if (!strcmp(line,"HDR"))
      m_ThumbOffset = atoi(val);
    if (!strcmp(line,"X  "))
      m_RawWidth = atoi(val);
    if (!strcmp(line,"Y  "))
      m_RawHeight = atoi(val);
    if (!strcmp(line,"TX "))
      m_ThumbWidth = atoi(val);
    if (!strcmp(line,"TY "))
      m_ThumbHeight = atoi(val);
  } while (strncmp(line,"EOHD",4));
  m_Data_Offset = m_ThumbOffset + m_ThumbWidth * m_ThumbHeight * 2;
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  if (mktime(&t) > 0)
    m_TimeStamp = mktime(&t);
  strcpy (m_CameraMake, "Rollei");
  strcpy (m_CameraModel,"d530flex");
  m_WriteThumb = &CLASS rollei_thumb;
}

void CLASS parse_sinar_ia()
{
  int entries, off;
  char str[8], *cp;

  m_ByteOrder = 0x4949;
  fseek (m_InputFile, 4, SEEK_SET);
  entries = get4();
  fseek (m_InputFile, get4(), SEEK_SET);
  while (entries--) {
    off = get4(); get4();
    ptfread (str, 8, 1, m_InputFile);
    if (!strcmp(str,"META"))   meta_offset = off;
    if (!strcmp(str,"THUMB")) m_ThumbOffset = off;
    if (!strcmp(str,"RAW0"))   m_Data_Offset = off;
  }
  fseek (m_InputFile, meta_offset+20, SEEK_SET);
  ptfread (m_CameraMake, 64, 1, m_InputFile);
  m_CameraMake[63] = 0;
  if ((cp = strchr(m_CameraMake,' '))) {
    strcpy (m_CameraModel, cp+1);
    *cp = 0;
  }
  m_RawWidth  = get2();
  m_RawHeight = get2();
  m_LoadRawFunction = &CLASS unpacked_load_raw;
  m_ThumbWidth = (get4(),get2());
  m_ThumbHeight = get2();
  m_WriteThumb = &CLASS ppm_thumb;
  m_WhiteLevel = 0x3fff;
}

void CLASS parse_phase_one (int base)
{
//warning: variable 'type' set but not used [-Wunused-but-set-variable]
  unsigned entries, tag, /*type,*/ len, data, save, i, c;
  float romm_cam[3][3];
  char *cp;

  memset (&ph1, 0, sizeof ph1);
  fseek (m_InputFile, base, SEEK_SET);
  m_ByteOrder = get4() & 0xffff;
  if (get4() >> 8 != 0x526177) return;    /* "Raw" */
  fseek (m_InputFile, get4()+base, SEEK_SET);
  entries = get4();
  get4();
  while (entries--) {
    tag  = get4();
//    type = get4();
    len  = get4();
    data = get4();
    save = ftell(m_InputFile);
    fseek (m_InputFile, base+data, SEEK_SET);
    switch (tag) {
      case 0x100:  m_Flip = "0653"[data & 3]-'0';  break;
      case 0x106:
  for (i=0; i < 3; i++)
          for (short j=0; j<3; j++)
      romm_cam[i][j] = getreal(11);
  romm_coeff (romm_cam);
  break;
      case 0x107:
  for (c=0; c<3; c++) ASSIGN(m_CameraMultipliers[c], getreal(11));
  break;
      case 0x108:  m_RawWidth     = data; break;
      case 0x109:  m_RawHeight    = data; break;
      case 0x10a:  m_LeftMargin   = data; break;
      case 0x10b:  m_TopMargin    = data; break;
      case 0x10c:  m_Width         = data;  break;
      case 0x10d:  m_Height        = data;  break;
      case 0x10e:  ph1.format    = data;  break;
      case 0x10f:  m_Data_Offset   = data+base; break;
      case 0x110:  meta_offset   = data+base;
       m_MetaLength   = len;      break;
      case 0x112:  ph1.key_off   = save - 4;    break;
      case 0x210:  ph1.tag_210   = int_to_float(data);  break;
      case 0x21a:  ph1.tag_21a   = data;    break;
      case 0x21c:  strip_offset  = data+base;   break;
      case 0x21d:  ph1.black     = data;    break;
      case 0x222:  ph1.split_col = data;		break;
      case 0x223:  ph1.black_off = data+base;   break;
      case 0x301:
  m_CameraModel[63] = 0;
  ptfread (m_CameraModel, 1, 63, m_InputFile);
  if ((cp = strstr(m_CameraModel," camera"))) *cp = 0;
    }
    fseek (m_InputFile, save, SEEK_SET);
  }
  m_LoadRawFunction = ph1.format < 3 ?
  &CLASS phase_one_load_raw : &CLASS phase_one_load_raw_c;
  m_WhiteLevel = 0xffff;
  strcpy (m_CameraMake, "Phase One");
  if (m_CameraModel[0]) return;
  switch (m_RawHeight) {
    case 2060: strcpy (m_CameraModel,"LightPhase"); break;
    case 2682: strcpy (m_CameraModel,"H 10");   break;
    case 4128: strcpy (m_CameraModel,"H 20");   break;
    case 5488: strcpy (m_CameraModel,"H 25");   break;
  }
}

void CLASS parse_fuji (int offset)
{
  unsigned entries, tag, len, save, c;

  fseek (m_InputFile, offset, SEEK_SET);
  entries = get4();
  if (entries > 255) return;
  while (entries--) {
    tag = get2();
    len = get2();
    save = ftell(m_InputFile);
    if (tag == 0x100) {
      m_RawHeight = get2();
      m_RawWidth  = get2();
    } else if (tag == 0x121) {
      m_Height = get2();
      if ((m_Width = get2()) == 4284) m_Width += 3;
    } else if (tag == 0x130) {
      fuji_layout = fgetc(m_InputFile) >> 7;
      m_Fuji_Width = !(fgetc(m_InputFile) & 8);
    } else if (tag == 0x2ff0) {
      for (c=0; c<4; c++) m_CameraMultipliers[c ^ 1] = get2();
    } else if (tag == 0xc000) {
      c = m_ByteOrder;
      m_ByteOrder = 0x4949;
      if ((m_Width = get4()) > 10000) m_Width = get4();
      m_Height = get4();
      m_ByteOrder = c;
    }
    fseek (m_InputFile, save+len, SEEK_SET);

  }
  if (!m_RawHeight) {
    m_Filters = 0x16161616;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 24;
  }
  m_Height <<= fuji_layout;
  m_Width  >>= fuji_layout;
}

int CLASS parse_jpeg (int offset)
{
  int len, save, hlen, mark;

  fseek (m_InputFile, offset, SEEK_SET);
  if (fgetc(m_InputFile) != 0xff || fgetc(m_InputFile) != 0xd8) return 0;

  while (fgetc(m_InputFile) == 0xff && (mark = fgetc(m_InputFile)) != 0xda) {
    m_ByteOrder = 0x4d4d;
    len   = get2() - 2;
    save  = ftell(m_InputFile);
    if (mark == 0xc0 || mark == 0xc3) {
      fgetc(m_InputFile);
      m_RawHeight = get2();
      m_RawWidth  = get2();
    }
    m_ByteOrder = get2();
    hlen  = get4();
    if (get4() == 0x48454150)   /* "HEAP" */
      parse_ciff (save+hlen, len-hlen);
    if (parse_tiff (save+6)) apply_tiff();
    fseek (m_InputFile, save+len, SEEK_SET);
  }
  return 1;
}

void CLASS parse_riff()
{
  unsigned i, size, end;
  char tag[4], date[64], month[64];
  static const char mon[12][4] =
  { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
  struct tm t;

  m_ByteOrder = 0x4949;
  ptfread (tag, 4, 1, m_InputFile);
  size = get4();
  end = ftell(m_InputFile) + size;
  if (!memcmp(tag,"RIFF",4) || !memcmp(tag,"LIST",4)) {
    get4();
    while (ftell(m_InputFile)+7 < (long) end)
      parse_riff();
  } else if (!memcmp(tag,"nctg",4)) {
    while ((unsigned) ftell(m_InputFile)+7 < end) {
      i = get2();
      size = get2();
      if ((i+1) >> 1 == 10 && size == 20)
  get_timestamp(0);
      else fseek (m_InputFile, size, SEEK_CUR);
    }
  } else if (!memcmp(tag,"IDIT",4) && size < 64) {
    ptfread (date, 64, 1, m_InputFile);
    date[size] = 0;
    memset (&t, 0, sizeof t);
    if (sscanf (date, "%*s %s %d %d:%d:%d %d", month, &t.tm_mday,
  &t.tm_hour, &t.tm_min, &t.tm_sec, &t.tm_year) == 6) {
      for (i=0; i < 12 && strcasecmp(mon[i],month); i++) {};
      t.tm_mon = i;
      t.tm_year -= 1900;
      if (mktime(&t) > 0)
  m_TimeStamp = mktime(&t);
    }
  } else
    fseek (m_InputFile, size, SEEK_CUR);
}

void CLASS parse_smal (int offset, int fsize)
{
  int ver;

  fseek (m_InputFile, offset+2, SEEK_SET);
  m_ByteOrder = 0x4949;
  ver = fgetc(m_InputFile);
  if (ver == 6)
    fseek (m_InputFile, 5, SEEK_CUR);
  if (get4() != (unsigned) fsize) return;
  if (ver > 6) m_Data_Offset = get4();
  m_RawHeight = m_Height = get2();
  m_RawWidth  = m_Width  = get2();
  strcpy (m_CameraMake, "SMaL");
  sprintf (m_CameraModel, "v%d %dx%d", ver, m_Width, m_Height);
  if (ver == 6) m_LoadRawFunction = &CLASS smal_v6_load_raw;
  if (ver == 9) m_LoadRawFunction = &CLASS smal_v9_load_raw;
}

void CLASS parse_cine()
{
  unsigned off_head, off_setup, off_image, i;

  m_ByteOrder = 0x4949;
  fseek (m_InputFile, 4, SEEK_SET);
  m_IsRaw = get2() == 2;
  fseek (m_InputFile, 14, SEEK_CUR);
  m_IsRaw *= get4();
  off_head = get4();
  off_setup = get4();
  off_image = get4();
  m_TimeStamp = get4();
  if ((i = get4())) m_TimeStamp = i;
  fseek (m_InputFile, off_head+4, SEEK_SET);
  m_RawWidth = get4();
  m_RawHeight = get4();
  switch (get2(),get2()) {
    case  8:  m_LoadRawFunction = &CLASS eight_bit_load_raw;  break;
    case 16:  m_LoadRawFunction = &CLASS  unpacked_load_raw;
  }
  fseek (m_InputFile, off_setup+792, SEEK_SET);
  strcpy (m_CameraMake, "CINE");
  sprintf (m_CameraModel, "%d", get4());
  fseek (m_InputFile, 12, SEEK_CUR);
  switch ((i=get4()) & 0xffffff) {
    case  3:  m_Filters = 0x94949494;  break;
    case  4:  m_Filters = 0x49494949;  break;
    default:  m_IsRaw = 0;
  }
  fseek (m_InputFile, 72, SEEK_CUR);
  switch ((get4()+3600) % 360) {
    case 270:  m_Flip = 4;  break;
    case 180:  m_Flip = 1;  break;
    case  90:  m_Flip = 7;  break;
    case   0:  m_Flip = 2;
  }
  ASSIGN(m_CameraMultipliers[0], getreal(11));
  ASSIGN(m_CameraMultipliers[2], getreal(11));
  m_WhiteLevel = ~(-1 << get4());
  fseek (m_InputFile, 668, SEEK_CUR);
  m_Shutter = get4()/1000000000.0;
  fseek (m_InputFile, off_image, SEEK_SET);
  if (m_UserSetting_ShotSelect < m_IsRaw)
    fseek (m_InputFile, m_UserSetting_ShotSelect*8, SEEK_CUR);
  m_Data_Offset  = (int64_t) get4() + 8;
  m_Data_Offset += (int64_t) get4() << 32;
}

void CLASS parse_redcine()
{
  unsigned i, len, rdvo;

  m_ByteOrder = 0x4d4d;
  m_IsRaw = 0;
  fseek (m_InputFile, 52, SEEK_SET);
  m_Width  = get4();
  m_Height = get4();
  fseek (m_InputFile, 0, SEEK_END);
  fseek (m_InputFile, -(i = ftell(m_InputFile) & 511), SEEK_CUR);
  if (get4() != i || get4() != 0x52454f42) {
    fprintf (stderr,_("%s: Tail is missing, parsing from head...\n"), m_UserSetting_InputFileName);
    fseek (m_InputFile, 0, SEEK_SET);
    while ((int)(len = get4()) != EOF) {
      if (get4() == 0x52454456)
  if (m_IsRaw++ == m_UserSetting_ShotSelect)
    m_Data_Offset = ftell(m_InputFile) - 8;
      fseek (m_InputFile, len-8, SEEK_CUR);
    }
  } else {
    rdvo = get4();
    fseek (m_InputFile, 12, SEEK_CUR);
    m_IsRaw = get4();
    fseek (m_InputFile, rdvo+8 + m_UserSetting_ShotSelect*4, SEEK_SET);
    m_Data_Offset = get4();
  }
}

char * CLASS foveon_gets (int offset, char *str, int len)
{
  int i;
  fseek (m_InputFile, offset, SEEK_SET);
  for (i=0; i < len-1; i++)
    if ((str[i] = get2()) == 0) break;
  str[i] = 0;
  return str;
}

void CLASS parse_foveon()
{
  int entries, img=0, off, len, tag, save, i, wide, high, pent, poff[256][2];
  char name[64], value[64];

  m_ByteOrder = 0x4949;     /* Little-endian */
  fseek (m_InputFile, 36, SEEK_SET);
  m_Flip = get4();
  fseek (m_InputFile, -4, SEEK_END);
  fseek (m_InputFile, get4(), SEEK_SET);
  if (get4() != 0x64434553) return; /* SECd */
  entries = (get4(),get4());
  while (entries--) {
    off = get4();
    len = get4();
    tag = get4();
    save = ftell(m_InputFile);
    fseek (m_InputFile, off, SEEK_SET);
    if (get4() != (unsigned)(0x20434553 | (tag << 24))) return;
    switch (tag) {
      case 0x47414d49:      /* IMAG */
      case 0x32414d49:      /* IMA2 */
  fseek (m_InputFile, 8, SEEK_CUR);
  pent = get4();
  wide = get4();
  high = get4();
  if (wide > m_RawWidth && high > m_RawHeight) {
    switch (pent) {
      case  5:  m_Load_Flags = 1;
      case  6:  m_LoadRawFunction = &CLASS foveon_sd_load_raw;  break;
      case 30:  m_LoadRawFunction = &CLASS foveon_dp_load_raw;  break;
      default:  m_LoadRawFunction = 0;
    }
    m_RawWidth  = wide;
    m_RawHeight = high;
    m_Data_Offset = off+28;
  }
  fseek (m_InputFile, off+28, SEEK_SET);
  if (fgetc(m_InputFile) == 0xff && fgetc(m_InputFile) == 0xd8
              && m_ThumbLength < (unsigned)(len-28)) {
    m_ThumbOffset = off+28;
    m_ThumbLength = len-28;
    m_WriteThumb = &CLASS jpeg_thumb;
  }
  if (++img == 2 && !m_ThumbLength) {
    m_ThumbOffset = off+24;
    m_ThumbWidth = wide;
    m_ThumbHeight = high;
    m_WriteThumb = &CLASS foveon_thumb;
  }
  break;
      case 0x464d4143:      /* CAMF */
  meta_offset = off+8;
  m_MetaLength = len-28;
  break;
      case 0x504f5250:      /* PROP */
  pent = (get4(),get4());
  fseek (m_InputFile, 12, SEEK_CUR);
  off += pent*8 + 24;
  if ((unsigned) pent > 256) pent=256;
  for (i=0; i < pent*2; i++)
    poff[0][i] = off + get4()*2;
  for (i=0; i < pent; i++) {
    foveon_gets (poff[i][0], name, 64);
    foveon_gets (poff[i][1], value, 64);
    if (!strcmp (name, "ISO"))
      m_IsoSpeed = atoi(value);
    if (!strcmp (name, "CAMMANUF"))
      strcpy (m_CameraMake, value);
    if (!strcmp (name, "CAMMODEL"))
      strcpy (m_CameraModel, value);
    if (!strcmp (name, "WB_DESC"))
      strcpy (m_CameraModelBis, value);
    if (!strcmp (name, "TIME"))
      m_TimeStamp = atoi(value);
    if (!strcmp (name, "EXPTIME"))
      m_Shutter = atoi(value) / 1000000.0;
    if (!strcmp (name, "APERTURE"))
      m_Aperture = atof(value);
    if (!strcmp (name, "FLENGTH"))
      m_FocalLength = atof(value);
  }
#ifdef LOCALTIME
  m_TimeStamp = mktime (gmtime (&m_TimeStamp));
#endif
    }
    fseek (m_InputFile, save, SEEK_SET);
  }
  m_IsFoveon = 1;
}

/*
  Thanks to Adobe for providing these excellent CAM -> XYZ matrices!
  All matrices are from Adobe DNG Converter unless otherwise noted.

 */

#include "ptAdobeTable.h"

void CLASS adobe_coeff (const char *make, const char *model) {
  double MatrixXYZToCamRGB[4][3];
  char name[130];
  unsigned i;
  sprintf (name, "%s %s", make , model);
  for (i=0; i < sizeof AdobeTable / sizeof *AdobeTable; i++)
    if (!strncmp (name,
                  AdobeTable[i].Identification,
                  strlen(AdobeTable[i].Identification))) {
      strcpy(m_CameraAdobeIdentification,AdobeTable[i].Identification);
      if (AdobeTable[i].Blackpoint)   {
        m_BlackLevel = (uint16_t) AdobeTable[i].Blackpoint;
      }
      if (AdobeTable[i].Whitepoint) {
        m_WhiteLevel = (uint16_t) AdobeTable[i].Whitepoint;
      }
      if (AdobeTable[i].XYZToCam[0]) {
        for (short j=0; j < 4; j++) {
          for (short k=0; k<3; k++) {
      MatrixXYZToCamRGB[j][k] = AdobeTable[i].XYZToCam[j*3+k] / 10000.0;
          }
        }
        cam_xyz_coeff (MatrixXYZToCamRGB);
      }
      break;
    }
}

void CLASS simple_coeff (int index)
{
  static const float table[][12] = {
  /* index 0 -- all Foveon cameras */
  { 1.4032,-0.2231,-0.1016,-0.5263,1.4816,0.017,-0.0112,0.0183,0.9113 },
  /* index 1 -- Kodak DC20 and DC25 */
  { 2.25,0.75,-1.75,-0.25,-0.25,0.75,0.75,-0.25,-0.25,-1.75,0.75,2.25 },
  /* index 2 -- Logitech Fotoman Pixtura */
  { 1.893,-0.418,-0.476,-0.495,1.773,-0.278,-1.017,-0.655,2.672 },
  /* index 3 -- Nikon E880, E900, and E990 */
  { -1.936280,  1.800443, -1.448486,  2.584324,
     1.405365, -0.524955, -0.289090,  0.408680,
    -1.204965,  1.082304,  2.941367, -1.818705 }
  };
  int i, c;

  for (m_RawColor = i=0; i < 3; i++)
    for (c=0; c < m_Colors; c++) m_MatrixCamRGBToSRGB[i][c] = table[index][i*m_Colors+c];
}

short CLASS guess_byte_order (int words)
{
  uint8_t test[4][2];
  int t=2, msb;
  double diff, sum[2] = {0,0};

  ptfread (test[0], 2, 2, m_InputFile);
  for (words-=2; words--; ) {
    ptfread (test[t], 2, 1, m_InputFile);
    for (msb=0; msb < 2; msb++) {
      diff = (test[t^2][msb] << 8 | test[t^2][!msb])
     - (test[t  ][msb] << 8 | test[t  ][!msb]);
      sum[msb] += diff*diff;
    }
    t = (t+1) & 3;
  }
  return sum[0] < sum[1] ? 0x4d4d : 0x4949;
}

float CLASS find_green (int bps, int bite, int off0, int off1)
{
  uint64_t bitbuf=0;
  int vbits, col, i, c;
  uint16_t img[2][2064];
  double sum[]={0,0};

  for (c=0;c<2;c++) {
    fseek (m_InputFile, c ? off1:off0, SEEK_SET);
    for (vbits=col=0; col < m_Width; col++) {
      for (vbits -= bps; vbits < 0; vbits += bite) {
  bitbuf <<= bite;
  for (i=0; i < bite; i+=8)
    bitbuf |= (unsigned) (fgetc(m_InputFile) << i);
      }
      img[c][col] = bitbuf << (64-bps-vbits) >> (64-bps);
    }
  }
  for (c=0;c<(m_Width-1);c++) {
    sum[ c & 1] += ABS(img[0][c]-img[1][c+1]);
    sum[~c & 1] += ABS(img[1][c]-img[0][c+1]);
  }
  return 100 * log(sum[0]/sum[1]);
}

/*
   Identify which camera created this file, and set global variables
   accordingly.
 */

void CLASS identify() {
  char         l_Head[32];
  char*        l_CharPointer;
  unsigned     l_HLen;
  unsigned     l_FLen;
  unsigned     l_FileSize;
  int          zero_fsize = 1;
  unsigned     l_IsCanon;
  struct jhead l_JHead;
  unsigned     i,c;
  short pana[][6] = {
    { 3130, 1743,  4,  0, -6,  0 },
    { 3130, 2055,  4,  0, -6,  0 },
    { 3130, 2319,  4,  0, -6,  0 },
    { 3170, 2103, 18,  0,-42, 20 },
    { 3170, 2367, 18, 13,-42,-21 },
    { 3177, 2367,  0,  0, -1,  0 },
    { 3304, 2458,  0,  0, -1,  0 },
    { 3330, 2463,  9,  0, -5,  0 },
    { 3330, 2479,  9,  0,-17,  4 },
    { 3370, 1899, 15,  0,-44, 20 },
    { 3370, 2235, 15,  0,-44, 20 },
    { 3370, 2511, 15, 10,-44,-21 },
    { 3690, 2751,  3,  0, -8, -3 },
    { 3710, 2751,  0,  0, -3,  0 },
    { 3724, 2450,  0,  0,  0, -2 },
    { 3770, 2487, 17,  0,-44, 19 },
    { 3770, 2799, 17, 15,-44,-19 },
    { 3880, 2170,  6,  0, -6,  0 },
    { 4060, 3018,  0,  0,  0, -2 },
    { 4290, 2391,  3,  0, -8, -1 },
    { 4330, 2439, 17, 15,-44,-19 },
    { 4508, 2962,  0,  0, -3, -4 },
    { 4508, 3330,  0,  0, -3, -6 } };

  static const struct {
    int fsize;
    char make[12], model[19], withjpeg;
  } l_Table[] = {
    {    62464, "Kodak",    "DC20"            ,0 },
    {   124928, "Kodak",    "DC20"            ,0 },
    {  1652736, "Kodak",    "DCS200"          ,0 },
    {  4159302, "Kodak",    "C330"            ,0 },
    {  4162462, "Kodak",    "C330"            ,0 },
    {   460800, "Kodak",    "C603v"           ,0 },
    {   614400, "Kodak",    "C603v"           ,0 },
    {  6163328, "Kodak",    "C603"            ,0 },
    {  6166488, "Kodak",    "C603"            ,0 },
    {  9116448, "Kodak",    "C603y"           ,0 },
    {   311696, "ST Micro", "STV680 VGA"      ,0 },  /* SPYz */
    {   787456, "Creative", "PC-CAM 600"      ,0 },
    {  1138688, "Minolta",  "RD175"           ,0 },
    {  3840000, "Foculus",  "531C"            ,0 },
    {   307200, "Generic",  "640x480"         ,0 },
    {   786432, "AVT",      "F-080C"          ,0 },
    {  1447680, "AVT",      "F-145C"          ,0 },
    {  1920000, "AVT",      "F-201C"          ,0 },
    {  5067304, "AVT",      "F-510C"          ,0 },
    {  5067316, "AVT",      "F-510C"          ,0 },
    { 10134608, "AVT",      "F-510C"          ,0 },
    { 10134620, "AVT",      "F-510C"          ,0 },
    { 16157136, "AVT",      "F-810C"          ,0 },
    {  1409024, "Sony",     "XCD-SX910CR"     ,0 },
    {  2818048, "Sony",     "XCD-SX910CR"     ,0 },
    {  3884928, "Micron",   "2010"            ,0 },
    {  6624000, "Pixelink", "A782"            ,0 },
    { 13248000, "Pixelink", "A782"            ,0 },
    {  6291456, "RoverShot","3320AF"          ,0 },
    {  6553440, "Canon",    "PowerShot A460"  ,0 },
    {  6653280, "Canon",    "PowerShot A530"  ,0 },
    {  6573120, "Canon",    "PowerShot A610"  ,0 },
    {  9219600, "Canon",    "PowerShot A620"  ,0 },
    {  9243240, "Canon",    "PowerShot A470"  ,0 },
    { 10341600, "Canon",    "PowerShot A720 IS"  ,0 },
    { 10383120, "Canon",    "PowerShot A630"  ,0 },
    { 12945240, "Canon",    "PowerShot A640"  ,0 },
    { 15636240, "Canon",    "PowerShot A650"  ,0 },
    {  5298000, "Canon",    "PowerShot SD300" ,0 },
    {  7710960, "Canon",    "PowerShot S3 IS" ,0 },
    { 15467760, "Canon",    "PowerShot SX110 IS",0 },
    { 15534576, "Canon",    "PowerShot SX120 IS",0 },
    { 18653760, "Canon",    "PowerShot SX20 IS",0 },
    { 19131120, "Canon",    "PowerShot SX220 HS",0 },
    { 21936096, "Canon",    "PowerShot SX30 IS",0 },
    {  5939200, "OLYMPUS",  "C770UZ"          ,0 },
    {  1581060, "NIKON",    "E900"            ,1 },  /* or E900s,E910 */
    {  2465792, "NIKON",    "E950"            ,1 },  /* or E800,E700 */
    {  2940928, "NIKON",    "E2100"           ,1 },  /* or E2500 */
    {  4771840, "NIKON",    "E990"            ,1 },  /* or E995, Oly C3030Z */
    {  4775936, "NIKON",    "E3700"           ,1 },  /* or Optio 33WR */
    {  5869568, "NIKON",    "E4300"           ,1 },  /* or DiMAGE Z2 */
    {  5865472, "NIKON",    "E4500"           ,1 },
    {  7438336, "NIKON",    "E5000"           ,1 },  /* or E5700 */
    {  8998912, "NIKON",    "COOLPIX S6"      ,1 },
    {  1976352, "CASIO",    "QV-2000UX"       ,1 },
    {  3217760, "CASIO",    "QV-3*00EX"       ,1 },
    {  6218368, "CASIO",    "QV-5700"         ,1 },
    {  6054400, "CASIO",    "QV-R41"          ,1 },
    {  7530816, "CASIO",    "QV-R51"          ,1 },
    {  7684000, "CASIO",    "QV-4000"         ,1 },
    {  2937856, "CASIO",    "EX-S20"          ,1 },
    {  4948608, "CASIO",    "EX-S100"         ,1 },
    {  7542528, "CASIO",    "EX-Z50"          ,1 },
    {  7562048, "CASIO",    "EX-Z500"         ,1 },
    {  7753344, "CASIO",    "EX-Z55"          ,1 },
    {  7816704, "CASIO",    "EX-Z60"          ,1 },
    { 10843712, "CASIO",    "EX-Z75"          ,1 },
    { 10834368, "CASIO",    "EX-Z750"         ,1 },
    { 12310144, "CASIO",    "EX-Z850"         ,1 },
    { 12489984, "CASIO",    "EX-Z8"           ,1 },
    { 15499264, "CASIO",    "EX-Z1050"        ,1 },
    { 18702336, "CASIO",    "EX-ZR100"        ,1 },
    {  7426656, "CASIO",    "EX-P505"         ,1 },
    {  9313536, "CASIO",    "EX-P600"         ,1 },
    { 10979200, "CASIO",    "EX-P700"         ,1 },
    {  3178560, "PENTAX",   "Optio S"         ,1 },
    {  4841984, "PENTAX",   "Optio S"         ,1 },
    {  6114240, "PENTAX",   "Optio S4"        ,1 },  /* or S4i, CASIO EX-Z4 */
    { 10702848, "PENTAX",   "Optio 750Z"      ,1 },
    { 15980544, "AGFAPHOTO","DC-833m"         ,1 },
    { 16098048, "SAMSUNG",  "S85"             ,1 },
    { 16215552, "SAMSUNG",  "S85"             ,1 },
    { 20487168, "SAMSUNG",  "WB550"           ,1 },
    { 24000000, "SAMSUNG",  "WB550"           ,1 },
    { 12582980, "Sinar",    ""                ,0 },
    { 33292868, "Sinar",    ""                ,0 },
    { 44390468, "Sinar",    ""                ,0 } };

  static const char *l_Corporation[] =
    { "Canon", "NIKON", "EPSON", "KODAK", "Kodak", "OLYMPUS", "PENTAX",
      "MINOLTA", "Minolta", "Konica", "CASIO", "Sinar", "Phase One",
      "SAMSUNG", "Mamiya", "MOTOROLA", "LEICA" };

  // Byte order of input file.
  memset (m_Mask, 0, sizeof m_Mask);
  m_ByteOrder = get2();

  // Length of header.
  l_HLen = get4();

/*  m_BlackLevel = 0;
  for (int k=0; k<4; k++) m_CBlackLevel[k]=0; TODO Mike */

  // Length of file.
  fseek (m_InputFile, 0, SEEK_SET);
  ptfread (l_Head, 1, 32, m_InputFile);
  fseek (m_InputFile, 0, SEEK_END);
  l_FLen = l_FileSize = ftell(m_InputFile);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  if ((l_CharPointer = (char *) memmem (l_Head, 32, "MMMM", 4)) ||
      (l_CharPointer = (char *) memmem (l_Head, 32, "IIII", 4))) {
#pragma GCC diagnostic pop
    parse_phase_one (l_CharPointer-l_Head);
    if ((l_CharPointer-l_Head) && parse_tiff(0)) apply_tiff();
  } else if (m_ByteOrder == 0x4949 || m_ByteOrder == 0x4d4d) {
    if (!memcmp (l_Head+6,"HEAPCCDR",8)) {
      m_Data_Offset = l_HLen;
      parse_ciff (l_HLen, l_FLen - l_HLen);
    } else if (parse_tiff(0)) apply_tiff();
  } else if (!memcmp (l_Head,"\xff\xd8\xff\xe1",4) &&
       !memcmp (l_Head+6,"Exif",4)) {
    fseek (m_InputFile, 4, SEEK_SET);
    m_Data_Offset = 4 + get2();
    fseek (m_InputFile, m_Data_Offset, SEEK_SET);
    if (fgetc(m_InputFile) != 0xff)
      parse_tiff(12);
    m_ThumbOffset = 0;
  } else if (!memcmp (l_Head+25,"ARECOYK",7)) {
    strcpy (m_CameraMake, "Contax");
    strcpy (m_CameraModel,"N Digital");
    fseek (m_InputFile, 33, SEEK_SET);
    get_timestamp(1);
    fseek (m_InputFile, 60, SEEK_SET);
    for (c=0; c < 4; c++) ASSIGN(m_CameraMultipliers[c ^ (c >> 1)], get4());
  } else if (!strcmp (l_Head, "PXN")) {
    strcpy (m_CameraMake, "Logitech");
    strcpy (m_CameraModel,"Fotoman Pixtura");
  } else if (!strcmp (l_Head, "qktk")) {
    strcpy (m_CameraMake, "Apple");
    strcpy (m_CameraModel,"QuickTake 100");
    m_LoadRawFunction = &CLASS quicktake_100_load_raw;
  } else if (!strcmp (l_Head, "qktn")) {
    strcpy (m_CameraMake, "Apple");
    strcpy (m_CameraModel,"QuickTake 150");
    m_LoadRawFunction = &CLASS kodak_radc_load_raw;
  } else if (!memcmp (l_Head,"FUJIFILM",8)) {
    fseek (m_InputFile, 84, SEEK_SET);
    m_ThumbOffset = get4();
    m_ThumbLength = get4();
    fseek (m_InputFile, 92, SEEK_SET);
    parse_fuji (get4());
    if (m_ThumbOffset > 120) {
      fseek (m_InputFile, 120, SEEK_SET);
      m_IsRaw += (i = get4());
      if (m_IsRaw == 2 && m_UserSetting_ShotSelect)
  parse_fuji (i);
    }
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    fseek (m_InputFile, 100+28*(m_UserSetting_ShotSelect > 0), SEEK_SET);
    parse_tiff (m_Data_Offset = get4());
    parse_tiff (m_ThumbOffset+12);
    apply_tiff();
  } else if (!memcmp (l_Head,"RIFF",4)) {
    fseek (m_InputFile, 0, SEEK_SET);
    parse_riff();
  } else if (!memcmp (l_Head,"\0\001\0\001\0@",6)) {
    fseek (m_InputFile, 6, SEEK_SET);
    ptfread (m_CameraMake, 1, 8, m_InputFile);
    ptfread (m_CameraModel, 1, 8, m_InputFile);
    ptfread (m_CameraModelBis, 1, 16, m_InputFile);
    m_Data_Offset = get2();
    get2();
    m_RawWidth = get2();
    m_RawHeight = get2();
    m_LoadRawFunction = &CLASS nokia_load_raw;
    m_Filters = 0x61616161;
  } else if (!memcmp (l_Head,"NOKIARAW",8)) {
    strcpy (m_CameraMake, "NOKIA");
    strcpy (m_CameraModel, "X2");
    m_ByteOrder = 0x4949;
    fseek (m_InputFile, 300, SEEK_SET);
    m_Data_Offset = get4();
    i = get4();
    m_Width = get2();
    m_Height = get2();
    m_Data_Offset += i - m_Width * 5 / 4 * m_Height;
    m_LoadRawFunction = &CLASS nokia_load_raw;
    m_Filters = 0x61616161;
  } else if (!memcmp (l_Head,"ARRI",4)) {
    m_ByteOrder = 0x4949;
    fseek (m_InputFile, 20, SEEK_SET);
    m_Width = get4();
    m_Height = get4();
    strcpy (m_CameraMake, "ARRI");
    fseek (m_InputFile, 668, SEEK_SET);
    ptfread (m_CameraModel, 1, 64, m_InputFile);
    m_Data_Offset = 4096;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 88;
    m_Filters = 0x61616161;
  } else if (!memcmp (l_Head+4,"RED1",4)) {
    strcpy (m_CameraMake, "RED");
    strcpy (m_CameraModel,"ONE");
    parse_redcine();
    m_LoadRawFunction = &CLASS redcine_load_raw;
    gamma_curve (1/2.4, 12.92, 1, 4095);
    m_Filters = 0x49494949;
  } else if (!memcmp (l_Head,"DSC-Image",9))
    parse_rollei();
  else if (!memcmp (l_Head,"PWAD",4))
    parse_sinar_ia();
  else if (!memcmp (l_Head,"\0MRM",4))
    parse_minolta(0);
  else if (!memcmp (l_Head,"FOVb",4))
    parse_foveon();
  else if (!memcmp (l_Head,"CI",2))
    parse_cine();
  else {
    for (zero_fsize=i=0; i < sizeof l_Table / sizeof *l_Table; i++)
      if (l_FileSize == (unsigned) l_Table[i].fsize) {
  strcpy (m_CameraMake,  l_Table[i].make );
  strcpy (m_CameraModel, l_Table[i].model);
  if (l_Table[i].withjpeg)
    parse_external_jpeg();
      }
  }
  if (zero_fsize) l_FileSize = 0;
  if (m_CameraMake[0] == 0) parse_smal (0, l_FLen);
  if (m_CameraMake[0] == 0) parse_jpeg (m_IsRaw = 0);

  for (i=0; i < sizeof l_Corporation / sizeof *l_Corporation; i++)
    if (strstr (m_CameraMake, l_Corporation[i])) /* Simplify company names */
  strcpy (m_CameraMake, l_Corporation[i]);
  if (!strncmp (m_CameraMake,"KODAK",5) &&
  ((l_CharPointer = strstr(m_CameraModel," DIGITAL CAMERA")) ||
   (l_CharPointer = strstr(m_CameraModel," Digital Camera")) ||
   (l_CharPointer = strstr(m_CameraModel,"FILE VERSION"))))
     *l_CharPointer = 0;
  l_CharPointer = m_CameraMake + strlen(m_CameraMake);  /* Remove trailing spaces */
  while (*--l_CharPointer == ' ') *l_CharPointer = 0;
  l_CharPointer = m_CameraModel + strlen(m_CameraModel);
  while (*--l_CharPointer == ' ') *l_CharPointer = 0;
  i = strlen(m_CameraMake);     /* Remove make from model */
  if (!strncasecmp (m_CameraModel, m_CameraMake, i) && m_CameraModel[i++] == ' ')
    memmove (m_CameraModel, m_CameraModel+i, 64-i);
  if (!strncmp (m_CameraModel,"FinePix ",8))
    strcpy (m_CameraModel, m_CameraModel+8);
  if (!strncmp (m_CameraModel,"Digital Camera ",15))
    strcpy (m_CameraModel, m_CameraModel+15);

  m_Description[511] = m_Artist[63] = m_CameraMake[63] = m_CameraModel[63] = m_CameraModelBis[63] = 0;

  if (!m_IsRaw) goto notraw;

  if (!m_Height) m_Height = m_RawHeight;
  if (!m_Width)  m_Width  = m_RawWidth;
  if (m_Height == 2624 && m_Width == 3936) /* Pentax K10D and Samsung GX10 */
    { m_Height  = 2616;   m_Width  = 3896; }
  if (m_Height == 3136 && m_Width == 4864)  /* Pentax K20D and Samsung GX20 */
    { m_Height  = 3124;   m_Width  = 4688; m_Filters = 0x16161616; }
  if (m_Width == 4352 && (!strcmp(m_CameraModel,"K-r") || !strcmp(m_CameraModel,"K-x")))
    { m_Width  = 4309; m_Filters = 0x16161616; }
  if (m_Width >= 4960 && !strncmp(m_CameraModel,"K-5",3))
    { m_LeftMargin = 10; m_Width  = 4950; m_Filters = 0x16161616; }
  if (m_Width == 4736 && !strcmp(m_CameraModel,"K-7"))
    { m_Height  = 3122;   m_Width  = 4684; m_Filters = 0x16161616; m_TopMargin = 2; }
  if (m_Width == 7424 && !strcmp(m_CameraModel,"645D"))
    { m_Height  = 5502;   m_Width  = 7328; m_Filters = 0x61616161; m_TopMargin = 29;
      m_LeftMargin = 48; }
  if (m_Height == 3014 && m_Width == 4096)  /* Ricoh GX200 */
      m_Width  = 4014;
  if (m_DNG_Version) {
    if (m_Filters == UINT_MAX) m_Filters = 0;
    if (m_Filters) m_IsRaw = m_Tiff_Samples;
    else   m_Colors = m_Tiff_Samples;
    if (m_Tiff_Compress == 1)
      m_LoadRawFunction = &CLASS packed_dng_load_raw;
    if (m_Tiff_Compress == 7)
      m_LoadRawFunction = &CLASS lossless_dng_load_raw;
    goto dng_skip;
  }
  if ((l_IsCanon = !strcmp(m_CameraMake,"Canon"))) {
    m_LoadRawFunction = memcmp (l_Head+6,"HEAPCCDR",8) ?
  &CLASS lossless_jpeg_load_raw : &CLASS canon_load_raw;
  }
  if (!strcmp(m_CameraMake,"NIKON")) {
    if (!m_LoadRawFunction)
      m_LoadRawFunction = &CLASS packed_load_raw;
    if (m_CameraModel[0] == 'E')
      m_Load_Flags |= !m_Data_Offset << 2 | 2;
  }
  if (!strcmp(m_CameraMake,"CASIO")) {
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_WhiteLevel = 0xf7f;
  }

/* Set parameters based on camera name (for non-DNG files). */

  if (m_IsFoveon) {
    if (m_Height*2 < m_Width) m_PixelAspect = 0.5;
    if (m_Height   > m_Width) m_PixelAspect = 2;
    m_Filters = 0;
    simple_coeff(0);
  } else if (l_IsCanon && m_Tiff_bps == 15) {
    switch (m_Width) {
      case 3344: m_Width -= 66;
      case 3872: m_Width -= 6;
    }
    if (m_Height > m_Width) SWAP(m_Height,m_Width);
    m_Filters = 0;
    m_LoadRawFunction = &CLASS canon_sraw_load_raw;
  } else if (!strcmp(m_CameraModel,"PowerShot 600")) {
    m_Height = 613;
    m_Width  = 854;
    m_RawWidth = 896;
    m_PixelAspect = 607/628.0;
    m_Colors = 4;
    m_Filters = 0xe1e4e1e4;
    m_LoadRawFunction = &CLASS canon_600_load_raw;
  } else if (!strcmp(m_CameraModel,"PowerShot A5") ||
       !strcmp(m_CameraModel,"PowerShot A5 Zoom")) {
    m_Height = 773;
    m_Width  = 960;
    m_RawWidth = 992;
    m_PixelAspect = 256/235.0;
    m_Colors = 4;
    m_Filters = 0x1e4e1e4e;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A50")) {
    m_Height =  968;
    m_Width  = 1290;
    m_RawWidth = 1320;
    m_Colors = 4;
    m_Filters = 0x1b4e4b1e;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot Pro70")) {
    m_Height = 1024;
    m_Width  = 1552;
    m_Colors = 4;
    m_Filters = 0x1e4b4e1b;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot SD300")) {
    m_Height = 1752;
    m_Width  = 2344;
    m_RawHeight = 1766;
    m_RawWidth  = 2400;
    m_TopMargin  = 12;
    m_LeftMargin = 12;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A460")) {
    m_Height = 1960;
    m_Width  = 2616;
    m_RawHeight = 1968;
    m_RawWidth  = 2664;
    m_TopMargin  = 4;
    m_LeftMargin = 4;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A530")) {
    m_Height = 1984;
    m_Width  = 2620;
    m_RawHeight = 1992;
    m_RawWidth  = 2672;
    m_TopMargin  = 6;
    m_LeftMargin = 10;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A610")) {
    if (canon_s2is()) strcpy (m_CameraModel+10, "S2 IS");
    m_Height = 1960;
    m_Width  = 2616;
    m_RawHeight = 1968;
    m_RawWidth  = 2672;
    m_TopMargin  = 8;
    m_LeftMargin = 12;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A620")) {
    m_Height = 2328;
    m_Width  = 3112;
    m_RawHeight = 2340;
    m_RawWidth  = 3152;
    m_TopMargin  = 12;
    m_LeftMargin = 36;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A470")) {
    m_Height = 2328;
    m_Width  = 3096;
    m_RawHeight = 2346;
    m_RawWidth  = 3152;
    m_TopMargin  = 6;
    m_LeftMargin = 12;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A720 IS")) {
    m_Height = 2472;
    m_Width  = 3298;
    m_RawHeight = 2480;
    m_RawWidth  = 3336;
    m_TopMargin  = 5;
    m_LeftMargin = 6;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A630")) {
    m_Height = 2472;
    m_Width  = 3288;
    m_RawHeight = 2484;
    m_RawWidth  = 3344;
    m_TopMargin  = 6;
    m_LeftMargin = 12;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot A640")) {
    m_Height = 2760;
    m_Width  = 3672;
    m_RawHeight = 2772;
    m_RawWidth  = 3736;
    m_TopMargin  = 6;
    m_LeftMargin = 12;
    goto canon_a5;
 } else if (!strcmp(m_CameraModel,"PowerShot A650")) {
    m_Height = 3024;
    m_Width  = 4032;
    m_RawHeight = 3048;
    m_RawWidth  = 4104;
    m_TopMargin  = 12;
    m_LeftMargin = 48;
    goto canon_a5;
  } else if (!strcmp(m_CameraModel,"PowerShot S3 IS")) {
    m_Height = 2128;
    m_Width  = 2840;
    m_RawHeight = 2136;
    m_RawWidth  = 2888;
    m_TopMargin  = 8;
    m_LeftMargin = 44;
canon_a5:
    m_Tiff_bps = 10;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 40;
    if (m_RawWidth > 1600) m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot SX110 IS")) {
    m_Height = 2760;
    m_Width  = 3684;
    m_RawHeight = 2772;
    m_RawWidth  = 3720;
    m_TopMargin  = 12;
    m_LeftMargin = 6;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 40;
    m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot SX120 IS")) {
    m_Height = 2742;
    m_Width  = 3664;
    m_RawHeight = 2778;
    m_RawWidth  = 3728;
    m_TopMargin  = 18;
    m_LeftMargin = 16;
    m_Filters = 0x49494949;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 40;
    m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot SX20 IS")) {
    m_Height = 3024;
    m_Width  = 4032;
    m_RawHeight = 3048;
    m_RawWidth  = 4080;
    m_TopMargin  = 12;
    m_LeftMargin = 24;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 40;
    m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot SX220 HS")) {
    m_Height = 3043;
    m_Width  = 4072;
    m_RawHeight = 3060;
    m_RawWidth  = 4168;
    m_Mask[0][0] = m_TopMargin = 16;
    m_Mask[0][2] = m_TopMargin + m_Height;
    m_Mask[0][3] = m_LeftMargin = 92;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 8;
    m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot SX30 IS")) {
    m_Height = 3254;
    m_Width  = 4366;
    m_RawHeight = 3276;
    m_RawWidth  = 4464;
    m_TopMargin  = 10;
    m_LeftMargin = 25;
    m_Filters = 0x16161616;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 40;
    m_ZeroIsBad = 1;
  } else if (!strcmp(m_CameraModel,"PowerShot Pro90 IS")) {
    m_Width  = 1896;
    m_Colors = 4;
    m_Filters = 0xb4b4b4b4;
  } else if (l_IsCanon && m_RawWidth == 2144) {
    m_Height = 1550;
    m_Width  = 2088;
    m_TopMargin  = 8;
    m_LeftMargin = 4;
    if (!strcmp(m_CameraModel,"PowerShot G1")) {
      m_Colors = 4;
      m_Filters = 0xb4b4b4b4;
    }
  } else if (l_IsCanon && m_RawWidth == 2224) {
    m_Height = 1448;
    m_Width  = 2176;
    m_TopMargin  = 6;
    m_LeftMargin = 48;
  } else if (l_IsCanon && m_RawWidth == 2376) {
    m_Height = 1720;
    m_Width  = 2312;
    m_TopMargin  = 6;
    m_LeftMargin = 12;
  } else if (l_IsCanon && m_RawWidth == 2672) {
    m_Height = 1960;
    m_Width  = 2616;
    m_TopMargin  = 6;
    m_LeftMargin = 12;
  } else if (l_IsCanon && m_RawWidth == 3152) {
    m_Height = 2056;
    m_Width  = 3088;
    m_TopMargin  = 12;
    m_LeftMargin = 64;
    if (unique_id == 0x80000170)
      adobe_coeff ("Canon","EOS 300D");
  } else if (l_IsCanon && m_RawWidth == 3160) {
    m_Height = 2328;
    m_Width  = 3112;
    m_TopMargin  = 12;
    m_LeftMargin = 44;
  } else if (l_IsCanon && m_RawWidth == 3344) {
    m_Height = 2472;
    m_Width  = 3288;
    m_TopMargin  = 6;
    m_LeftMargin = 4;
  } else if (!strcmp(m_CameraModel,"EOS D2000C")) {
    m_Filters = 0x61616161;
    m_BlackLevel = m_Curve[200];
  } else if (l_IsCanon && m_RawWidth == 3516) {
    m_TopMargin  = 14;
    m_LeftMargin = 42;
    if (unique_id == 0x80000189)
      adobe_coeff ("Canon","EOS 350D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 3596) {
    m_TopMargin  = 12;
    m_LeftMargin = 74;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 3744) {
    m_Height = 2760;
    m_Width  = 3684;
    m_TopMargin  = 16;
    m_LeftMargin = 8;
    if (unique_id > 0x2720000) {
      m_TopMargin  = 12;
      m_LeftMargin = 52;
    }
  } else if (l_IsCanon && m_RawWidth == 3944) {
    m_Height = 2602;
    m_Width  = 3908;
    m_TopMargin  = 18;
    m_LeftMargin = 30;
  } else if (l_IsCanon && m_RawWidth == 3948) {
    m_TopMargin  = 18;
    m_LeftMargin = 42;
    m_Height -= 2;
    if (unique_id == 0x80000236)
      adobe_coeff ("Canon","EOS 400D");
    if (unique_id == 0x80000254)
      adobe_coeff ("Canon","EOS 1000D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 3984) {
    m_TopMargin  = 20;
    m_LeftMargin = 76;
    m_Height -= 2;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 4104) {
    m_Height = 3024;
    m_Width  = 4032;
    m_TopMargin  = 12;
    m_LeftMargin = 48;
 } else if (l_IsCanon && m_RawWidth == 4152) {
    m_TopMargin  = 12;
    m_LeftMargin = 192;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 4160) {
    m_Height = 3048;
    m_Width  = 4048;
    m_TopMargin  = 11;
    m_LeftMargin = 104;
  } else if (l_IsCanon && m_RawWidth == 4176) {
    m_Height = 3045;
    m_Width  = 4072;
    m_LeftMargin = 96;
    m_Mask[0][0] = m_TopMargin = 17;
    m_Mask[0][2] = m_RawHeight;
    m_Mask[0][3] = 80;
    m_Filters = 0x49494949;
  } else if (l_IsCanon && m_RawWidth == 4312) {
    m_TopMargin  = 18;
    m_LeftMargin = 22;
    m_Height -= 2;
    if (unique_id == 0x80000176)
      adobe_coeff ("Canon","EOS 450D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 4352) {
    m_TopMargin  = 18;
    m_LeftMargin = 62;
    if (unique_id == 0x80000288)
      adobe_coeff ("Canon","EOS 1100D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 4476) {
    m_TopMargin  = 34;
    m_LeftMargin = 90;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 4480) {
    m_Height = 3326;
    m_Width  = 4432;
    m_TopMargin  = 10;
    m_LeftMargin = 12;
    m_Filters = 0x49494949;
  } else if (l_IsCanon && m_RawWidth == 4496) {
    m_Height = 3316;
    m_Width  = 4404;
    m_TopMargin  = 50;
    m_LeftMargin = 80;
  } else if (l_IsCanon && m_RawWidth == 4832) {
    m_TopMargin = unique_id == 0x80000261 ? 51:26;
    m_LeftMargin = 62;
    if (unique_id == 0x80000252)
      adobe_coeff ("Canon","EOS 500D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5108) {
    m_TopMargin  = 13;
    m_LeftMargin = 98;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5120) {
    m_Height -= m_TopMargin = 45;
    m_LeftMargin = 142;
    m_Width = 4916;
  } else if (l_IsCanon && m_RawWidth == 5280) {
    m_TopMargin  = 52;
    m_LeftMargin = 72;
    if (unique_id == 0x80000301)
      adobe_coeff ("Canon","EOS 650D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5344) {
    m_TopMargin = 51;
    m_LeftMargin = 142;
    if (unique_id == 0x80000269) {
      m_TopMargin = 100;
      m_LeftMargin = 126;
      m_Height -= 2;
      adobe_coeff ("Canon","EOS-1D X");
    }
    if (unique_id == 0x80000270)
      adobe_coeff ("Canon","EOS 550D");
    if (unique_id == 0x80000286)
      adobe_coeff ("Canon","EOS 600D");
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5360) {
    m_TopMargin = 51;
    m_LeftMargin = 158;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5568) {
    m_TopMargin = 38;
    m_LeftMargin = 72;
    goto canon_cr2;
  } else if (l_IsCanon && m_RawWidth == 5712) {
    m_Height = 3752;
    m_Width  = 5640;
    m_TopMargin  = 20;
    m_LeftMargin = 62;
  } else if (l_IsCanon && m_RawWidth == 5792) {
    m_TopMargin  = 51;
    m_LeftMargin = 158;
canon_cr2:
    m_Height -= m_TopMargin;
    m_Width  -= m_LeftMargin;
  } else if (l_IsCanon && m_RawWidth == 5920) {
    m_Height = 3870;
    m_Width  = 5796;
    m_TopMargin  = 80;
    m_LeftMargin = 122;
  } else if (!strcmp(m_CameraModel,"D1")) {
    ASSIGN(m_CameraMultipliers[0],VALUE(m_CameraMultipliers[0]) * 256/527.0);
    ASSIGN(m_CameraMultipliers[2],VALUE(m_CameraMultipliers[2]) * 256/317.0);
  } else if (!strcmp(m_CameraModel,"D1X")) {
    m_Width -= 4;
    m_PixelAspect = 0.5;
  } else if (!strcmp(m_CameraModel,"D40X") ||
       !strcmp(m_CameraModel,"D60")  ||
       !strcmp(m_CameraModel,"D80")  ||
       !strcmp(m_CameraModel,"D3000")) {
    m_Height -= 3;
    m_Width  -= 4;
  } else if (!strcmp(m_CameraModel,"D3")   ||
       !strcmp(m_CameraModel,"D3S") ||
       !strcmp(m_CameraModel,"D700")) {
    m_Width -= 4;
    m_LeftMargin = 2;
  } else if (!strcmp(m_CameraModel,"D3100")) {
    m_Width -= 28;
    m_LeftMargin = 6;
  } else if (!strcmp(m_CameraModel,"D5000") ||
       !strcmp(m_CameraModel,"D90")) {
    m_Width -= 42;
  } else if (!strcmp(m_CameraModel,"D5100") ||
       !strcmp(m_CameraModel,"D7000")) {
    m_Width -= 44;
  } else if (!strcmp(m_CameraModel,"D3200") ||
	     !strcmp(m_CameraModel,"D600")  ||
       !strcmp(m_CameraModel,"D800")) {
    m_Width -= 46;
  } else if (!strcmp(m_CameraModel,"D4")) {
    m_Width -= 52;
    m_LeftMargin = 2;
  } else if (!strncmp(m_CameraModel,"D40",3) ||
       !strncmp(m_CameraModel,"D50",3) ||
       !strncmp(m_CameraModel,"D70",3)) {
    m_Width--;
  } else if (!strcmp(m_CameraModel,"D100")) {
    if (m_Load_Flags)
      m_RawWidth = (m_Width += 3) + 3;
  } else if (!strcmp(m_CameraModel,"D200")) {
    m_LeftMargin = 1;
    m_Width -= 4;
    m_Filters = 0x94949494;
  } else if (!strncmp(m_CameraModel,"D2H",3)) {
    m_LeftMargin = 6;
    m_Width -= 14;
  } else if (!strncmp(m_CameraModel,"D2X",3)) {
    if (m_Width == 3264) m_Width -= 32;
    else m_Width -= 8;
  } else if (!strncmp(m_CameraModel,"D300",4)) {
    m_Width -= 32;
  } else if (!strcmp(m_CameraMake,"NIKON") && m_RawWidth == 4032) {
    adobe_coeff ("NIKON","COOLPIX P7700");
  } else if (!strncmp(m_CameraModel,"COOLPIX P",9)) {
    m_Load_Flags = 24;
    m_Filters = 0x94949494;
    if (m_CameraModel[9] == '7' && m_IsoSpeed >= 400)
      m_BlackLevel = 255;
  } else if (!strncmp(m_CameraModel,"1 ",2)) {
    m_Height -= 2;
  } else if (l_FileSize == 1581060) {
    m_Height = 963;
    m_Width = 1287;
    m_RawWidth = 1632;
    m_WhiteLevel = 0x3f4;
    m_Colors = 4;
    m_Filters = 0x1e1e1e1e;
    simple_coeff(3);
    ASSIGN(m_D65Multipliers[0], 1.2085);
    ASSIGN(m_D65Multipliers[1], 1.0943);
    ASSIGN(m_D65Multipliers[3], 1.1103);
    goto e900;
  } else if (l_FileSize == 2465792) {
    m_Height = 1203;
    m_Width  = 1616;
    m_RawWidth = 2048;
    m_Colors = 4;
    m_Filters = 0x4b4b4b4b;
    adobe_coeff ("NIKON","E950");
e900:
    m_Tiff_bps = 10;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 6;
  } else if (l_FileSize == 4771840) {
    m_Height = 1540;
    m_Width  = 2064;
    m_Colors = 4;
    m_Filters = 0xe1e1e1e1;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 6;
    if (!m_TimeStamp && nikon_e995())
      strcpy (m_CameraModel, "E995");
    if (strcmp(m_CameraModel,"E995")) {
      m_Filters = 0xb4b4b4b4;
      simple_coeff(3);
      ASSIGN(m_D65Multipliers[0], 1.196);
      ASSIGN(m_D65Multipliers[1], 1.246);
      ASSIGN(m_D65Multipliers[2], 1.018);
    }
  } else if (!strcmp(m_CameraModel,"E2100")) {
    if (!m_TimeStamp && !nikon_e2100()) goto cp_e2500;
    m_Height = 1206;
    m_Width  = 1616;
    m_Load_Flags = 30;
  } else if (!strcmp(m_CameraModel,"E2500")) {
cp_e2500:
    strcpy (m_CameraModel, "E2500");
    m_Height = 1204;
    m_Width  = 1616;
    m_Colors = 4;
    m_Filters = 0x4b4b4b4b;
  } else if (l_FileSize == 4775936) {
    m_Height = 1542;
    m_Width  = 2064;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 30;
    if (!m_TimeStamp) nikon_3700();
    if (m_CameraModel[0] == 'E' && atoi(m_CameraModel+1) < 3700)
      m_Filters = 0x49494949;
    if (!strcmp(m_CameraModel,"Optio 33WR")) {
      m_Flip = 1;
      m_Filters = 0x16161616;
    }
     if (m_CameraMake[0] == 'O') {
      int li = find_green (12, 32, 1188864, 3576832);
      int lc = find_green (12, 32, 2383920, 2387016);
      if (abs(li) < abs(lc)) {
  SWAP(li,lc);
  m_Load_Flags = 24;
      }
      if (li < 0) m_Filters = 0x61616161;
    }
  } else if (l_FileSize == 5869568) {
    m_Height = 1710;
    m_Width  = 2288;
    m_Filters = 0x16161616;
    if (!m_TimeStamp && minolta_z2()) {
      strcpy (m_CameraMake, "Minolta");
      strcpy (m_CameraModel,"DiMAGE Z2");
    }
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 6 + 24 * (m_CameraMake[0] == 'M');
  } else if (!strcmp(m_CameraModel,"E4500")) {
    m_Height = 1708;
    m_Width  = 2288;
    m_Colors = 4;
    m_Filters = 0xb4b4b4b4;
  } else if (l_FileSize == 7438336) {
    m_Height = 1924;
    m_Width  = 2576;
    m_Colors = 4;
    m_Filters = 0xb4b4b4b4;
  } else if (l_FileSize == 8998912) {
    m_Height = 2118;
    m_Width  = 2832;
    m_WhiteLevel = 0xf83;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 30;
 } else if (!strcmp(m_CameraMake,"FUJIFILM")) {
    if (!strcmp(m_CameraModel+7,"S2Pro")) {
      strcpy (m_CameraModel,"S2Pro");
      m_Height = 2144;
      m_Width  = 2880;
      m_Flip = 6;
    } else if (m_LoadRawFunction != &CLASS packed_load_raw)
      m_WhiteLevel = (m_IsRaw == 2 && m_UserSetting_ShotSelect) ? 0x2f00 : 0x3e00;
    m_TopMargin = (m_RawHeight - m_Height) >> 2 << 1;
    m_LeftMargin = (m_RawWidth - m_Width ) >> 2 << 1;
    if (m_Width == 2848) m_Filters = 0x16161616;
    if (m_Width == 3328) {
      m_Width = 3262;
      m_LeftMargin = 34;
    }
    if (m_Width == 4952) {
      m_LeftMargin = 0;
      m_Filters = 2;
    }
    if (fuji_layout) m_RawWidth *= m_IsRaw;
  } else if (!strcmp(m_CameraModel,"RD175")) {
    m_Height = 986;
    m_Width = 1534;
    m_Data_Offset = 513;
    m_Filters = 0x61616161;
    m_LoadRawFunction = &CLASS minolta_rd175_load_raw;
  } else if (!strcmp(m_CameraModel,"KD-400Z")) {
    m_Height = 1712;
    m_Width  = 2312;
    m_RawWidth = 2336;
    goto konica_400z;
  } else if (!strcmp(m_CameraModel,"KD-510Z")) {
    goto konica_510z;
  } else if (!strcasecmp(m_CameraMake,"MINOLTA")) {
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0xfff;
    if (!strncmp(m_CameraModel,"DiMAGE A",8)) {
      if (!strcmp(m_CameraModel,"DiMAGE A200"))
  m_Filters = 0x49494949;
      m_Tiff_bps = 12;
      m_LoadRawFunction = &CLASS packed_load_raw;
    } else if (!strncmp(m_CameraModel,"ALPHA",5) ||
         !strncmp(m_CameraModel,"DYNAX",5) ||
         !strncmp(m_CameraModel,"MAXXUM",6)) {
      sprintf (m_CameraModel+20, "DYNAX %-10s", m_CameraModel+6+(m_CameraModel[0]=='M'));
      adobe_coeff (m_CameraMake, m_CameraModel+20);
      m_LoadRawFunction = &CLASS packed_load_raw;
    } else if (!strncmp(m_CameraModel,"DiMAGE G",8)) {
      if (m_CameraModel[8] == '4') {
  m_Height = 1716;
  m_Width  = 2304;
      } else if (m_CameraModel[8] == '5') {
konica_510z:
  m_Height = 1956;
  m_Width  = 2607;
  m_RawWidth = 2624;
      } else if (m_CameraModel[8] == '6') {
  m_Height = 2136;
  m_Width  = 2848;
      }
      m_Data_Offset += 14;
      m_Filters = 0x61616161;
konica_400z:
      m_LoadRawFunction = &CLASS unpacked_load_raw;
      m_WhiteLevel = 0x3df;
      m_ByteOrder = 0x4d4d;
    }
  } else if (!strcmp(m_CameraModel,"*ist D")) {
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    data_error = -1;
  } else if (!strcmp(m_CameraModel,"*ist DS")) {
    m_Height -= 2;
  } else if (!strcmp(m_CameraModel,"K-x")) {
    m_Width = 4309;
    m_Filters = 0x16161616;
  } else if (!strcmp(m_CameraModel,"Optio S")) {
    if (l_FileSize == 3178560) {
      m_Height = 1540;
      m_Width  = 2064;
      m_LoadRawFunction = &CLASS eight_bit_load_raw;
      ASSIGN(m_CameraMultipliers[0],VALUE(m_CameraMultipliers[0]) * 4);
      ASSIGN(m_CameraMultipliers[2],VALUE(m_CameraMultipliers[2]) * 4);
    } else {
      m_Height = 1544;
      m_Width  = 2068;
      m_RawWidth = 3136;
      m_LoadRawFunction = &CLASS packed_load_raw;
      m_WhiteLevel = 0xf7c;
    }
  } else if (l_FileSize == 6114240) {
    m_Height = 1737;
    m_Width  = 2324;
    m_RawWidth = 3520;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_WhiteLevel = 0xf7a;
  } else if (!strcmp(m_CameraModel,"Optio 750Z")) {
    m_Height = 2302;
    m_Width  = 3072;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 30;
  } else if (!strcmp(m_CameraModel,"DC-833m")) {
    m_Height = 2448;
    m_Width  = 3264;
    m_ByteOrder = 0x4949;
    m_Filters = 0x61616161;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0xfc00;
  } else if (!strncmp(m_CameraModel,"S85",3)) {
    m_Height = 2448;
    m_Width  = 3264;
    m_RawWidth = l_FileSize/m_Height/2;
    m_ByteOrder = 0x4d4d;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
  } else if (!strcmp(m_CameraMake,"SAMSUNG") && m_RawWidth == 4704) {
    m_Height -= m_TopMargin = 8;
    m_Width -= 2 * (m_LeftMargin = 8);
    m_Load_Flags = 32;
  } else if (!strcmp(m_CameraMake,"SAMSUNG") && m_RawWidth == 5632) {
    m_ByteOrder = 0x4949;
    m_Height = 3694;
    m_TopMargin = 2;
    m_Width  = 5574 - (m_LeftMargin = 32 + m_Tiff_bps);
    if (m_Tiff_bps == 12) m_Load_Flags = 80;
  } else if (!strcmp(m_CameraModel,"EX1")) {
    m_ByteOrder = 0x4949;
    m_Height -= 20;
    m_TopMargin = 2;
    if ((m_Width -= 6) > 3682) {
      m_Height -= 10;
      m_Width  -= 46;
      m_TopMargin = 8;
    }
  } else if (!strcmp(m_CameraModel,"WB2000")) {
    m_ByteOrder = 0x4949;
    m_Height -= 3;
    m_TopMargin = 2;
    if ((m_Width -= 10) > 3718) {
      m_Height -= 28;
      m_Width  -= 56;
      m_TopMargin = 8;
    }
  } else if (l_FileSize == 20487168) {
    m_Height = 2808;
    m_Width  = 3648;
    goto wb550;
  } else if (l_FileSize == 24000000) {
    m_Height = 3000;
    m_Width  = 4000;
wb550:
    strcpy (m_CameraModel, "WB550");
    m_ByteOrder = 0x4d4d;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_Load_Flags = 6;
    m_WhiteLevel = 0x3df;
  } else if (!strcmp(m_CameraModel,"EX2F")) {
    m_Height = 3045;
    m_Width  = 4070;
    m_TopMargin = 3;
    m_ByteOrder = 0x4949;
    m_Filters = 0x49494949;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
  } else if (!strcmp(m_CameraModel,"STV680 VGA")) {
    m_Height = 484;
    m_Width  = 644;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
    m_Flip = 2;
    m_Filters = 0x16161616;
    m_BlackLevel = 16;
  } else if (!strcmp(m_CameraModel,"N95")) {
    m_TopMargin = 2;
    m_Height = m_RawHeight - m_TopMargin;
  } else if (!strcmp(m_CameraModel,"531C")) {
    m_Height = 1200;
    m_Width  = 1600;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_Filters = 0x49494949;
  } else if (!strcmp(m_CameraModel,"640x480")) {
    m_Height = 480;
    m_Width  = 640;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
    gamma_curve (0.45, 4.5, 1, 255);
  } else if (!strcmp(m_CameraModel,"F-080C")) {
    m_Height = 768;
    m_Width  = 1024;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strcmp(m_CameraModel,"F-145C")) {
    m_Height = 1040;
    m_Width  = 1392;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strcmp(m_CameraModel,"F-201C")) {
    m_Height = 1200;
    m_Width  = 1600;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strcmp(m_CameraModel,"F-510C")) {
    m_Height = 1958;
    m_Width  = 2588;
    m_LoadRawFunction = l_FileSize < 7500000 ?
  &CLASS eight_bit_load_raw : &CLASS unpacked_load_raw;
    m_Data_Offset = l_FileSize - m_Width*m_Height*(l_FileSize >> 22);
    m_WhiteLevel = 0xfff0;
  } else if (!strcmp(m_CameraModel,"F-810C")) {
    m_Height = 2469;
    m_Width  = 3272;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0xfff0;
  } else if (!strcmp(m_CameraModel,"XCD-SX910CR")) {
    m_Height = 1024;
    m_Width  = 1375;
    m_RawWidth = 1376;
    m_Filters = 0x49494949;
    m_WhiteLevel = 0x3ff;
    m_LoadRawFunction = l_FileSize < 2000000 ?
  &CLASS eight_bit_load_raw : &CLASS unpacked_load_raw;
  } else if (!strcmp(m_CameraModel,"2010")) {
    m_Height = 1207;
    m_Width  = 1608;
    m_ByteOrder = 0x4949;
    m_Filters = 0x16161616;
    m_Data_Offset = 3212;
    m_WhiteLevel = 0x3ff;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
  } else if (!strcmp(m_CameraModel,"A782")) {
    m_Height = 3000;
    m_Width  = 2208;
    m_Filters = 0x61616161;
    m_LoadRawFunction = l_FileSize < 10000000 ?
  &CLASS eight_bit_load_raw : &CLASS unpacked_load_raw;
    m_WhiteLevel = 0xffc0;
  } else if (!strcmp(m_CameraModel,"3320AF")) {
    m_Height = 1536;
    m_RawWidth = m_Width = 2048;
    m_Filters = 0x61616161;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0x3ff;
    fseek (m_InputFile, 0x300000, SEEK_SET);
    if ((m_ByteOrder = guess_byte_order(0x10000)) == 0x4d4d) {
      m_Height -= (m_TopMargin = 16);
      m_Width -= (m_LeftMargin = 28);
      m_WhiteLevel = 0xf5c0;
      strcpy (m_CameraMake, "ISG");
      m_CameraModel[0] = 0;
    }
  } else if (!strcmp(m_CameraMake,"Hasselblad")) {
    if (m_LoadRawFunction == &CLASS lossless_jpeg_load_raw)
      m_LoadRawFunction = &CLASS hasselblad_load_raw;
    if (m_RawWidth == 7262) {
      m_Height = 5444;
      m_Width  = 7248;
      m_TopMargin  = 4;
      m_LeftMargin = 7;
      m_Filters = 0x61616161;
    } else if (m_RawWidth == 7410) {
      m_Height = 5502;
      m_Width  = 7328;
      m_TopMargin  = 4;
      m_LeftMargin = 41;
      m_Filters = 0x61616161;
    } else if (m_RawWidth == 9044) {
      m_Height = 6716;
      m_Width  = 8964;
      m_TopMargin  = 8;
      m_LeftMargin = 40;
      m_BlackLevel += m_Load_Flags = 256;
      m_WhiteLevel = 0x8101;
    } else if (m_RawWidth == 4090) {
      strcpy (m_CameraModel, "V96C");
      m_Height -= (m_TopMargin = 6);
      m_Width -= (m_LeftMargin = 3) + 7;
      m_Filters = 0x61616161;
    }
  } else if (!strcmp(m_CameraMake,"Sinar")) {
    if (!memcmp(l_Head,"8BPS",4)) {
      fseek (m_InputFile, 14, SEEK_SET);
      m_Height = get4();
      m_Width  = get4();
      m_Filters = 0x61616161;
      m_Data_Offset = 68;
    }
    if (!m_LoadRawFunction) m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0x3fff;
  } else if (!strcmp(m_CameraMake,"Leaf")) {
    m_WhiteLevel = 0x3fff;
    fseek (m_InputFile, m_Data_Offset, SEEK_SET);
    if (ljpeg_start (&l_JHead, 1) && l_JHead.bits == 15)
      m_WhiteLevel = 0x1fff;
    if (m_Tiff_Samples > 1) m_Filters = 0;
    if (m_Tiff_Samples > 1 || m_TileLength < m_RawHeight) {
      m_LoadRawFunction = &CLASS leaf_hdr_load_raw;
      m_RawWidth = m_TileWidth;
    }
    if ((m_Width | m_Height) == 2048) {
      if (m_Tiff_Samples == 1) {
  m_Filters = 1;
  strcpy (m_ColorDescriptor, "RBTG");
  strcpy (m_CameraModel, "CatchLight");
  m_TopMargin =  8; m_LeftMargin = 18; m_Height = 2032; m_Width = 2016;
      } else {
  strcpy (m_CameraModel, "DCB2");
  m_TopMargin = 10; m_LeftMargin = 16; m_Height = 2028; m_Width = 2022;
      }
    } else if (m_Width+m_Height == 3144+2060) {
      if (!m_CameraModel[0]) strcpy (m_CameraModel, "Cantare");
      if (m_Width > m_Height) {
   m_TopMargin = 6; m_LeftMargin = 32; m_Height = 2048;  m_Width = 3072;
  m_Filters = 0x61616161;
      } else {
  m_LeftMargin = 6;  m_TopMargin = 32;  m_Width = 2048; m_Height = 3072;
  m_Filters = 0x16161616;
      }
      if (!VALUE(m_CameraMultipliers[0]) || m_CameraModel[0] == 'V') m_Filters = 0;
      else m_IsRaw = m_Tiff_Samples;
    } else if (m_Width == 2116) {
      strcpy (m_CameraModel, "Valeo 6");
      m_Height -= 2 * (m_TopMargin = 30);
      m_Width -= 2 * (m_LeftMargin = 55);
      m_Filters = 0x49494949;
    } else if (m_Width == 3171) {
      strcpy (m_CameraModel, "Valeo 6");
      m_Height -= 2 * (m_TopMargin = 24);
      m_Width -= 2 * (m_LeftMargin = 24);
      m_Filters = 0x16161616;
    }
  } else if (!strcmp(m_CameraMake,"LEICA") || !strcmp(m_CameraMake,"Panasonic")) {
    if ((l_FLen-m_Data_Offset) / (m_RawWidth*8/7) == m_RawHeight)
      m_LoadRawFunction = &CLASS panasonic_load_raw;
    if (!m_LoadRawFunction) {
      m_LoadRawFunction = &CLASS unpacked_load_raw;
      m_Load_Flags = 4;
    }
    m_ZeroIsBad = 1;
    if ((m_Height += 12) > m_RawHeight) m_Height = m_RawHeight;
    for (i=0; i < sizeof pana / sizeof *pana; i++)
      if (m_RawWidth == pana[i][0] && m_RawHeight == pana[i][1]) {
        m_LeftMargin = pana[i][2];
        m_TopMargin = pana[i][3];
        m_Width += pana[i][4];
        m_Height += pana[i][5];
      }
    m_Filters = 0x01010101 * (unsigned char) "\x94\x61\x49\x16"
      [((m_Filters-1) ^ (m_LeftMargin & 1) ^ (m_TopMargin << 1)) & 3];
  } else if (!strcmp(m_CameraModel,"C770UZ")) {
    m_Height = 1718;
    m_Width  = 2304;
    m_Filters = 0x16161616;
    m_LoadRawFunction = &CLASS packed_load_raw;
    m_Load_Flags = 30;
  } else if (!strcmp(m_CameraMake,"OLYMPUS")) {
    m_Height += m_Height & 1;
    m_Filters = exif_cfa;
    if (m_Width == 4100) m_Width -= 4;
    if (m_Width == 4080) m_Width -= 24;
    if (m_LoadRawFunction == &CLASS unpacked_load_raw)
      m_Load_Flags = 4;
    m_Tiff_bps = 12;
    if (!strcmp(m_CameraModel,"E-300") ||
      !strcmp(m_CameraModel,"E-500")) {
      m_Width -= 20;
      if (m_LoadRawFunction == &CLASS unpacked_load_raw) {
  m_WhiteLevel = 0xfc3;
  memset (m_CBlackLevel, 0, sizeof m_CBlackLevel);
      }
    } else if (!strcmp(m_CameraModel,"E-330")) {
      m_Width -= 30;
      if (m_LoadRawFunction == &CLASS unpacked_load_raw)
  m_WhiteLevel = 0xf79;
    } else if (!strcmp(m_CameraModel,"SP550UZ")) {
      m_ThumbLength = l_FLen - (m_ThumbOffset = 0xa39800);
      m_ThumbHeight = 480;
      m_ThumbWidth  = 640;
    } else if (!strcmp(m_CameraModel,"XZ-2")) {
      m_LoadRawFunction = &CLASS packed_load_raw;
      m_Load_Flags = 24;
    }
  } else if (!strcmp(m_CameraModel,"N Digital")) {
    m_Height = 2047;
    m_Width  = 3072;
    m_Filters = 0x61616161;
    m_Data_Offset = 0x1a00;
    m_LoadRawFunction = &CLASS packed_load_raw;
  } else if (!strcmp(m_CameraModel,"DSC-F828")) {
    m_Width = 3288;
    m_LeftMargin = 5;
    m_Mask[1][3] = -17;
    m_Data_Offset = 862144;
    m_LoadRawFunction = &CLASS sony_load_raw;
    m_Filters = 0x9c9c9c9c;
    m_Colors = 4;
    strcpy (m_ColorDescriptor, "RGBE");
  } else if (!strcmp(m_CameraModel,"DSC-V3")) {
    m_Width = 3109;
    m_LeftMargin = 59;
    m_Mask[0][1] = 9;
    m_Data_Offset = 787392;
    m_LoadRawFunction = &CLASS sony_load_raw;
  } else if (!strcmp(m_CameraMake,"SONY") && m_RawWidth == 3984) {
    adobe_coeff ("SONY","DSC-R1");
    m_Width = 3925;
    m_ByteOrder = 0x4d4d;
  } else if (!strcmp(m_CameraMake,"SONY") && m_RawWidth == 5504) {
    m_Width -= 8;
  } else if (!strcmp(m_CameraMake,"SONY") && m_RawWidth == 6048) {
    m_Width -= 24;
  } else if (!strcmp(m_CameraModel,"DSLR-A100")) {
    if (m_Width == 3880) {
      m_Height--;
      m_Width = ++m_RawWidth;
    } else {
      m_ByteOrder = 0x4d4d;
      m_Load_Flags = 2;
    }
    m_Filters = 0x61616161;
  } else if (!strcmp(m_CameraModel,"DSLR-A350")) {
    m_Height -= 4;
  } else if (!strcmp(m_CameraModel,"PIXL")) {
    m_Height -= m_TopMargin = 4;
    m_Width -= m_LeftMargin = 32;
    gamma_curve (0, 7, 1, 255);
  } else if (!strcmp(m_CameraModel,"C603v")) {
    m_Height = 480;
    m_Width  = 640;
    if (l_FileSize < 614400 || find_green (16, 16, 3840, 5120) < 25) goto c603v;
    strcpy (m_CameraModel,"KAI-0340");
    m_Height -= 3;
    m_Data_Offset = 3840;
    m_ByteOrder = 0x4949;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
  } else if (!strcmp(m_CameraModel,"C603y")) {
    m_Height = 2134;
    m_Width  = 2848;
c603v:
    m_Filters = 0;
    m_LoadRawFunction = &CLASS kodak_yrgb_load_raw;
    gamma_curve (0, 3.875, 1, 255);
  } else if (!strcmp(m_CameraModel,"C603")) {
    m_RawHeight = m_Height = 2152;
    m_RawWidth  = m_Width  = 2864;
    goto c603;
  } else if (!strcmp(m_CameraModel,"C330")) {
    m_Height = 1744;
    m_Width  = 2336;
    m_RawHeight = 1779;
    m_RawWidth  = 2338;
    m_TopMargin = 33;
    m_LeftMargin = 1;
c603:
    m_ByteOrder = 0x4949;
    if ((m_Data_Offset = l_FileSize - m_RawHeight*m_RawWidth)) {
      fseek (m_InputFile, 168, SEEK_SET);
      read_shorts (m_Curve, 256);
    } else gamma_curve (0, 3.875, 1, 255);
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strncasecmp(m_CameraMake,"EasyShare",9)) {
    m_Data_Offset = m_Data_Offset < 0x15000 ? 0x15000 : 0x17000;
    m_LoadRawFunction = &CLASS packed_load_raw;
  } else if (!strcasecmp(m_CameraMake,"KODAK")) {
    if (m_Filters == UINT_MAX) m_Filters = 0x61616161;
    if (!strncmp(m_CameraModel,"NC2000",6)) {
      m_Width -= 4;
      m_LeftMargin = 2;
    } else if (!strcmp(m_CameraModel,"EOSDCS3B")) {
      m_Width -= 4;
      m_LeftMargin = 2;
    } else if (!strcmp(m_CameraModel,"EOSDCS1")) {
      m_Width -= 4;
      m_LeftMargin = 2;
    } else if (!strcmp(m_CameraModel,"DCS420")) {
      m_Width -= 4;
      m_LeftMargin = 2;
    } else if (!strncmp(m_CameraModel,"DCS460",7)) {
      m_CameraModel[6]=0;
      m_Width -= 4;
      m_LeftMargin = 2;
    } else if (!strcmp(m_CameraModel,"DCS460A")) {
      m_Width -= 4;
      m_LeftMargin = 2;
      m_Colors = 1;
      m_Filters = 0;
    } else if (!strcmp(m_CameraModel,"DCS660M")) {
      m_BlackLevel = 214;
      m_Colors = 1;
      m_Filters = 0;
    } else if (!strcmp(m_CameraModel,"DCS760M")) {
      m_Colors = 1;
      m_Filters = 0;
    }
    if (!strcmp(m_CameraModel+4,"20X"))
      strcpy (m_ColorDescriptor, "MYCY");
    if (strstr(m_CameraModel,"DC25")) {
      strcpy (m_CameraModel, "DC25");
      m_Data_Offset = 15424;
    }
    if (!strncmp(m_CameraModel,"DC2",3)) {
      m_RawHeight = m_Height = 242;
      if (l_FLen < 100000) {
  m_RawWidth = 256; m_Width = 249;
  m_PixelAspect = (4.0*m_Height) / (3.0*m_Width);
      } else {
  m_RawWidth = 512; m_Width = 501;
  m_PixelAspect = (493.0*m_Height) / (373.0*m_Width);
      }
      m_Data_Offset += m_RawWidth + 1;
      m_Colors = 4;
      m_Filters = 0x8d8d8d8d;
      simple_coeff(1);
      ASSIGN(m_D65Multipliers[1], 1.179);
      ASSIGN(m_D65Multipliers[2], 1.209);
      ASSIGN(m_D65Multipliers[3], 1.036);
      m_LoadRawFunction = &CLASS eight_bit_load_raw;
    } else if (!strcmp(m_CameraModel,"40")) {
      strcpy (m_CameraModel, "DC40");
      m_Height = 512;
      m_Width  = 768;
      m_Data_Offset = 1152;
      m_LoadRawFunction = &CLASS kodak_radc_load_raw;
    } else if (strstr(m_CameraModel,"DC50")) {
      strcpy (m_CameraModel, "DC50");
      m_Height = 512;
      m_Width  = 768;
      m_Data_Offset = 19712;
      m_LoadRawFunction = &CLASS kodak_radc_load_raw;
    } else if (strstr(m_CameraModel,"DC120")) {
      strcpy (m_CameraModel, "DC120");
      m_Height = 976;
      m_Width  = 848;
      m_PixelAspect = m_Height/0.75/m_Width;
      m_LoadRawFunction = m_Tiff_Compress == 7 ?
  &CLASS kodak_jpeg_load_raw : &CLASS kodak_dc120_load_raw;
    } else if (!strcmp(m_CameraModel,"DCS200")) {
      m_ThumbHeight = 128;
      m_ThumbWidth  = 192;
      m_ThumbOffset = 6144;
      m_ThumbMisc   = 360;
      m_WriteThumb = &CLASS layer_thumb;
      m_Height = 1024;
      m_Width  = 1536;
      m_Data_Offset = 79872;
      m_LoadRawFunction = &CLASS eight_bit_load_raw;
      m_BlackLevel = 17;
    }
  } else if (!strcmp(m_CameraModel,"Fotoman Pixtura")) {
    m_Height = 512;
    m_Width  = 768;
    m_Data_Offset = 3632;
    m_LoadRawFunction = &CLASS kodak_radc_load_raw;
    m_Filters = 0x61616161;
    simple_coeff(2);
  } else if (!strncmp(m_CameraModel,"QuickTake",9)) {
    if (l_Head[5]) strcpy (m_CameraModel+10, "200");
    fseek (m_InputFile, 544, SEEK_SET);
    m_Height = get2();
    m_Width  = get2();
    m_Data_Offset = (get4(),get2()) == 30 ? 738:736;
    if (m_Height > m_Width) {
      SWAP(m_Height,m_Width);
      fseek (m_InputFile, m_Data_Offset-6, SEEK_SET);
      m_Flip = ~get2() & 3 ? 5:6;
    }
    m_Filters = 0x61616161;
  } else if (!strcmp(m_CameraMake,"Rollei") && !m_LoadRawFunction) {
    switch (m_RawWidth) {
      case 1316:
  m_Height = 1030;
  m_Width  = 1300;
  m_TopMargin  = 1;
  m_LeftMargin = 6;
  break;
      case 2568:
  m_Height = 1960;
  m_Width  = 2560;
  m_TopMargin  = 2;
  m_LeftMargin = 8;
    }
    m_Filters = 0x16161616;
    m_LoadRawFunction = &CLASS rollei_load_raw;
  } else if (!strcmp(m_CameraModel,"PC-CAM 600")) {
    m_Height = 768;
    m_Data_Offset = m_Width = 1024;
    m_Filters = 0x49494949;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strcmp(m_CameraModel,"QV-2000UX")) {
    m_Height = 1208;
    m_Width  = 1632;
    m_Data_Offset = m_Width * 2;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (l_FileSize == 3217760) {
    m_Height = 1546;
    m_Width  = 2070;
    m_RawWidth = 2080;
    m_LoadRawFunction = &CLASS eight_bit_load_raw;
  } else if (!strcmp(m_CameraModel,"QV-4000")) {
    m_Height = 1700;
    m_Width  = 2260;
    m_LoadRawFunction = &CLASS unpacked_load_raw;
    m_WhiteLevel = 0xffff;
  } else if (!strcmp(m_CameraModel,"QV-5700")) {
    m_Height = 1924;
    m_Width  = 2576;
    m_RawWidth = 3232;
    m_Tiff_bps = 10;
  } else if (!strcmp(m_CameraModel,"QV-R41")) {
    m_Height = 1720;
    m_Width  = 2312;
    m_RawWidth = 3520;
    m_LeftMargin = 2;
  } else if (!strcmp(m_CameraModel,"QV-R51")) {
    m_Height = 1926;
    m_Width  = 2580;
    m_RawWidth = 3904;
  } else if (!strcmp(m_CameraModel,"EX-S20")) {
    m_Height = 1208;
    m_Width  = 1620;
    m_RawWidth = 2432;
    m_Flip = 3;
  } else if (!strcmp(m_CameraModel,"EX-S100")) {
    m_Height = 1544;
    m_Width  = 2058;
    m_RawWidth = 3136;
  } else if (!strcmp(m_CameraModel,"EX-Z50")) {
    m_Height = 1931;
    m_Width  = 2570;
    m_RawWidth = 3904;
  } else if (!strcmp(m_CameraModel,"EX-Z500")) {
    m_Height = 1937;
    m_Width  = 2577;
    m_RawWidth = 3904;
    m_Filters = 0x16161616;
  } else if (!strcmp(m_CameraModel,"EX-Z55")) {
    m_Height = 1960;
    m_Width  = 2570;
    m_RawWidth = 3904;
 } else if (!strcmp(m_CameraModel,"EX-Z60")) {
    m_Height = 2145;
    m_Width  = 2833;
    m_RawWidth = 3584;
    m_Filters = 0x16161616;
    m_Tiff_bps = 10;
  } else if (!strcmp(m_CameraModel,"EX-Z75")) {
    m_Height = 2321;
    m_Width  = 3089;
    m_RawWidth = 4672;
    m_WhiteLevel = 0xfff;
  } else if (!strcmp(m_CameraModel,"EX-Z750")) {
    m_Height = 2319;
    m_Width  = 3087;
    m_RawWidth = 4672;
    m_WhiteLevel = 0xfff;
  } else if (!strcmp(m_CameraModel,"EX-Z850")) {
    m_Height = 2468;
    m_Width  = 3279;
    m_RawWidth = 4928;
    m_WhiteLevel = 0xfff;
  } else if (!strcmp(m_CameraModel,"EX-Z8")) {
    m_Height = 2467;
    m_Width  = 3281;
    m_RawHeight = 2502;
    m_RawWidth  = 4992;
    m_WhiteLevel = 0xfff;
  } else if (l_FileSize == 15499264) {	/* EX-Z1050 or EX-Z1080 */
    m_Height = 2752;
    m_Width  = 3672;
    m_RawWidth = 5632;
  } else if (!strcmp(m_CameraModel,"EX-ZR100")) {
    m_Height = 3044;
    m_Width  = 4072;
    m_RawWidth = 4096;
    m_Load_Flags = 80;
  } else if (!strcmp(m_CameraModel,"EX-P505")) {
    m_Height = 1928;
    m_Width  = 2568;
    m_RawWidth = 3852;
    m_WhiteLevel = 0xfff;
  } else if (l_FileSize == 9313536) { /* EX-P600 or QV-R61 */
    m_Height = 2142;
    m_Width  = 2844;
    m_RawWidth = 4288;
  } else if (!strcmp(m_CameraModel,"EX-P700")) {
    m_Height = 2318;
    m_Width  = 3082;
    m_RawWidth = 4672;
  }
  if (!m_CameraModel[0])
    sprintf (m_CameraModel, "%dx%d", m_Width, m_Height);
  if (m_Filters == UINT_MAX) m_Filters = 0x94949494;
  if (m_RawColor) adobe_coeff (m_CameraMake, m_CameraModel);
  if (m_LoadRawFunction == &CLASS kodak_radc_load_raw)
    if (m_RawColor) adobe_coeff ("Apple","Quicktake");
  if (m_ThumbOffset && !m_ThumbHeight) {
    fseek (m_InputFile, m_ThumbOffset, SEEK_SET);
    if (ljpeg_start (&l_JHead, 1)) {
      m_ThumbWidth  = l_JHead.wide;
      m_ThumbHeight = l_JHead.high;
    }
  }
dng_skip:
  if (m_Fuji_Width) {
    m_Fuji_Width = m_Width >> !fuji_layout;
    if (~m_Fuji_Width & 1) m_Filters = 0x49494949;
    m_Width = (m_Height >> fuji_layout) + m_Fuji_Width;
    m_Height = m_Width - 1;
    m_PixelAspect = 1;
  } else {
    if (m_RawHeight < m_Height) m_RawHeight = m_Height;
    if (m_RawWidth  < m_Width ) m_RawWidth  = m_Width;
  }
  if (!m_Tiff_bps) m_Tiff_bps = 12;
  if (!m_WhiteLevel) m_WhiteLevel = (1 << m_Tiff_bps) - 1;
  if (!m_LoadRawFunction || m_Height < 22) m_IsRaw = 0;
#ifdef NO_JASPER
  if (m_LoadRawFunction == &CLASS redcine_load_raw) {
    fprintf (stderr,_("%s: You must link dcraw with %s!\n"),
        m_UserSetting_InputFileName, "libjasper");
    m_IsRaw = 0;
  }
#endif
#ifdef NO_JPEG
  if (m_LoadRawFunction == &CLASS kodak_jpeg_load_raw ||
      m_LoadRawFunction == &CLASS lossy_dng_load_raw) {
        fprintf (stderr,_("%s: You must link dcraw with %s!\n"),
        m_UserSetting_InputFileName, "libjpeg");
    m_IsRaw = 0;
  }
#endif
  if (!m_ColorDescriptor[0])
    strcpy (m_ColorDescriptor, m_Colors == 3 ? "RGBG":"GMCY");
  if (!m_RawHeight) m_RawHeight = m_Height;
  if (!m_RawWidth ) m_RawWidth  = m_Width;
  if (m_Filters && m_Colors == 3)
    m_Filters |= ((m_Filters >> 2 & 0x22222222) |
      (m_Filters << 2 & 0x88888888)) & m_Filters << 1;
notraw:
  // Somehow Nikon D90 files give a corrupt value here... although they shouldn't.
  if (m_Flip == -1) m_Flip = m_Tiff_Flip;
  if (m_Flip == -1) m_Flip = 0;
}

void CLASS fuji_rotate() {
  int i, row, col;
  double step;
  float r, c, fr, fc;
  unsigned ur, uc;
  uint16_t wide, high, (*img)[4], (*pix)[4];

  if (!m_Fuji_Width) return;
  TRACEKEYVALS("Rotating image 45 degrees","%s","");
  TRACEKEYVALS("m_Fuji_Width","%d",m_Fuji_Width);
  TRACEKEYVALS("m_Shrink","%d",m_Shrink);
  TRACEKEYVALS("m_UserSetting_HalfSize","%d",m_UserSetting_HalfSize);
  // XXX JDLA : Was : m_Fuji_Width = (m_Fuji_Width - 1 + m_Shrink) >> m_Shrink;
  m_Fuji_Width =
    (m_Fuji_Width - 1 + m_UserSetting_HalfSize) >> m_UserSetting_HalfSize;
  TRACEKEYVALS("m_Fuji_Width","%d",m_Fuji_Width);
  step = sqrt(0.5);
  wide = (uint16_t) (m_Fuji_Width / step);
  high = (uint16_t) ((m_Height - m_Fuji_Width) / step);
  img = (uint16_t (*)[4]) CALLOC (wide*high, sizeof *img);
  TRACEKEYVALS("m_Width","%d",m_Width);
  TRACEKEYVALS("m_Height","%d",m_Height);
  TRACEKEYVALS("wide","%d",wide);
  TRACEKEYVALS("high","%d",high);
  merror (img, "fuji_rotate()");

  for (row=0; row < high; row++)
    for (col=0; col < wide; col++) {
      ur = (unsigned) (r = m_Fuji_Width + (row-col)*step);
      uc = (unsigned) (c = (row+col)*step);
      if (ur > (unsigned)(m_Height-2) || uc > (unsigned)(m_Width-2)) continue;
      fr = r - ur;
      fc = c - uc;
      pix = m_Image + ur*m_Width + uc;
      for (i=0; i < m_Colors; i++)
  img[row*wide+col][i] = (uint16_t) (
    (pix[    0][i]*(1-fc) + pix[      1][i]*fc) * (1-fr) +
    (pix[m_Width][i]*(1-fc) + pix[m_Width+1][i]*fc) * fr);
    }
  FREE (m_Image);
  m_OutWidth  = m_Width  = wide;
  m_OutHeight = m_Height = high;
  m_Image  = img;
  m_Fuji_Width = 0; // this prevents image rotation when only phase 2 is called

  TRACEKEYVALS("m_Width","%d",m_Width);
  TRACEKEYVALS("m_Height","%d",m_Height);
  TRACEKEYVALS("m_OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("m_OutHeight","%d",m_OutHeight);
}

void CLASS stretch()
{
  uint16_t newdim, (*img)[4], *pix0, *pix1;
  int row, col, c;
  double rc, frac;

  if (m_PixelAspect == 1) return;
  TRACEKEYVALS("Stretching the image","%s","");
  TRACEKEYVALS("m_PixelAspect","%f",m_PixelAspect);
  if (m_PixelAspect < 1) {
    newdim = (uint16_t) (m_Height / m_PixelAspect + 0.5);
    img = (uint16_t (*)[4]) CALLOC (m_Width*newdim, sizeof *img);
    merror (img, "stretch()");
    for (rc=row=0; row < newdim; row++, rc+=m_PixelAspect) {
      frac = rc - (c = (int) rc);
      pix0 = pix1 = m_Image[c*m_Width];
      if (c+1 < m_Height) pix1 += m_Width*4;
      for (col=0; col < m_Width; col++, pix0+=4, pix1+=4)
  for (c=0; c < m_Colors; c++) img[row*m_Width+col][c] =
          (uint16_t)(pix0[c]*(1-frac) + pix1[c]*frac + 0.5);
    }
    m_OutHeight = m_Height = newdim;
  } else {
    newdim = (uint16_t) (m_Width * m_PixelAspect + 0.5);
    img = (uint16_t (*)[4]) CALLOC (m_Height*newdim, sizeof *img);
    merror (img, "stretch()");
    for (rc=col=0; col < newdim; col++, rc+=1/m_PixelAspect) {
      frac = rc - (c = (int) rc);
      pix0 = pix1 = m_Image[c];
      if (c+1 < m_Width) pix1 += 4;
      for (row=0; row < m_Height; row++, pix0+=m_Width*4, pix1+=m_Width*4)
  for (c=0; c < m_Colors; c++) img[row*newdim+col][c] =
         (uint16_t) (pix0[c]*(1-frac) + pix1[c]*frac + 0.5);
    }
    m_OutWidth = m_Width = newdim;
  }
  FREE (m_Image);
  m_Image = img;

  TRACEKEYVALS("m_Width","%d",m_Width);
  TRACEKEYVALS("m_Height","%d",m_Height);
  TRACEKEYVALS("m_OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("m_OutHeight","%d",m_OutHeight);
}

int CLASS flip_index (int row, int col)
{
  if (m_Flip & 4) SWAP(row,col);
  if (m_Flip & 2) row = m_OutHeight - 1 - row;
  if (m_Flip & 1) col = m_OutWidth  - 1 - col;
  return row * m_OutWidth + col;
}

struct tiff_tag {
  uint16_t tag, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

struct tiff_hdr {
  uint16_t order, magic;
  int ifd;
  uint16_t pad, ntag;
  struct tiff_tag tag[23];
  int nextifd;
  uint16_t pad2, nexif;
  struct tiff_tag exif[4];
  uint16_t pad3, ngps;
  struct tiff_tag gpst[10];
  short bps[4];
  int rat[10];
  unsigned gps[26];
  char desc[512], make[64], model[64], soft[32], date[20], artist[64];
};

void CLASS tiff_set (uint16_t *ntag,
  uint16_t tag, uint16_t type, int count, int val)
{
  struct tiff_tag *tt;
  int c;

  tt = (struct tiff_tag *)(ntag+1) + (*ntag)++;
  tt->tag = tag;
  tt->type = type;
  tt->count = count;
  if (type < 3 && count <= 4)
    for(c=0;c<4;c++) tt->val.c[c] = val >> (c << 3);
  else if (type == 3 && count <= 2)
    for(c=0;c<2;c++) tt->val.s[c] = val >> (c << 4);
  else tt->val.i = val;
}

#define TOFF(ptr) ((char *)(&(ptr)) - (char *)th)

void CLASS tiff_head (struct tiff_hdr *th)
{
  int psize=0;
  struct tm *t;

  memset (th, 0, sizeof *th);
  th->order = htonl(0x4d4d4949) >> 16;
  th->magic = 42;
  th->ifd = 10;
  tiff_set (&th->ntag, 270, 2, 512, TOFF(th->desc));
  tiff_set (&th->ntag, 271, 2, 64, TOFF(th->make));
  tiff_set (&th->ntag, 272, 2, 64, TOFF(th->model));
  tiff_set (&th->ntag, 274, 3, 1, "12435867"[m_Flip]-'0');
  tiff_set (&th->ntag, 282, 5, 1, TOFF(th->rat[0]));
  tiff_set (&th->ntag, 283, 5, 1, TOFF(th->rat[2]));
  tiff_set (&th->ntag, 284, 3, 1, 1);
  tiff_set (&th->ntag, 296, 3, 1, 2);
  tiff_set (&th->ntag, 305, 2, 32, TOFF(th->soft));
  tiff_set (&th->ntag, 306, 2, 20, TOFF(th->date));
  tiff_set (&th->ntag, 315, 2, 64, TOFF(th->artist));
  tiff_set (&th->ntag, 34665, 4, 1, TOFF(th->nexif));
  if (psize) tiff_set (&th->ntag, 34675, 7, psize, sizeof *th);
  tiff_set (&th->nexif, 33434, 5, 1, TOFF(th->rat[4]));
  tiff_set (&th->nexif, 33437, 5, 1, TOFF(th->rat[6]));
  tiff_set (&th->nexif, 34855, 3, 1, (int) m_IsoSpeed);
  tiff_set (&th->nexif, 37386, 5, 1, TOFF(th->rat[8]));
  strncpy (th->desc, m_Description, 512);
  strncpy (th->make, m_CameraMake, 64);
  strncpy (th->model, m_CameraModel, 64);
  strcpy (th->soft, DCRAW_VERSION);
  t = localtime (&m_TimeStamp);
  sprintf (th->date, "%04d:%02d:%02d %02d:%02d:%02d",
      t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
  strncpy (th->artist, m_Artist, 64);
}

void CLASS jpeg_thumb ()
/* Remember: This function is modified to write the raw’s thumbnail to the
   m_Thumb instead of a file on disk. Always access thumbnails via DcRaw::thumbnail()!
*/
{
  char *thumb;
  uint16_t exif[5];
  struct tiff_hdr th;

  thumb = (char *) MALLOC (m_ThumbLength);
  merror (thumb, "jpeg_thumb()");
  ptfread (thumb, 1, m_ThumbLength, m_InputFile);
//  fputc (0xff, m_OutputFile);
//  fputc (0xd8, m_OutputFile);
  m_Thumb.push_back('\xff');
  m_Thumb.push_back('\xd8');

  if (strcmp (thumb+6, "Exif")) {
    memcpy (exif, "\xff\xe1  Exif\0\0", 10);
    exif[1] = htons (8 + sizeof th);
    //ptfwrite (exif, 1, sizeof exif, m_OutputFile);
    VAppend(m_Thumb, (char*)&exif[0], sizeof(exif));
    tiff_head (&th);
    //ptfwrite (&th, 1, sizeof th, m_OutputFile);
    VAppend(m_Thumb, (char*)&th, sizeof(th));
  }
  //ptfwrite (thumb+2, 1, m_ThumbLength-2, m_OutputFile);
  VAppend(m_Thumb, thumb+2, m_ThumbLength-2);

  FREE (thumb);
}

////////////////////////////////////////////////////////////////////////////////
//
// Identify
//
// First method to be called when in an photivo application.
// Hereafter camera, model, and a number of other picture and camera
// parameters are identified.
// Returns 0 on success.
//
////////////////////////////////////////////////////////////////////////////////

short CLASS Identify(const QString NewInputFile) {

  // This is here to support multiple calls.
  ResetNonUserSettings();
  FCLOSE(m_InputFile);

  if (NewInputFile != "") {
    FREE(m_UserSetting_InputFileName);
    m_UserSetting_InputFileName = (char*) MALLOC(1 + strlen(NewInputFile.toLocal8Bit().data()));
    ptMemoryError(m_UserSetting_InputFileName,__FILE__,__LINE__);
    strcpy(m_UserSetting_InputFileName, NewInputFile.toLocal8Bit().data());
  }

  if (!(m_InputFile = fopen (m_UserSetting_InputFileName, "rb"))) {
    perror (m_UserSetting_InputFileName);
    return -1;
  }

  identify();

  if (!m_IsRaw) {
    FCLOSE(m_InputFile);
  }
  return !m_IsRaw;
}

////////////////////////////////////////////////////////////////////////////////
//
// RunDcRaw_Phase1
//
// Settings are given via the m_UserSetting* members.
// It will end there where the raw image is loaded.
// Badpixels and darkframes subtracted.
// But before colorscaling and the like so that we can
// intervene in the whitebalances etc.
// Returns 0 on success.
//
////////////////////////////////////////////////////////////////////////////////

short CLASS RunDcRaw_Phase1() {

  // TODO foveon for the moment not in. Need study material
  // to have this +/- right.
  assert (!m_IsFoveon);

  TRACEKEYVALS("PreMult[0]","%f",VALUE(m_PreMultipliers[0]));
  TRACEKEYVALS("PreMult[1]","%f",VALUE(m_PreMultipliers[1]));
  TRACEKEYVALS("PreMult[2]","%f",VALUE(m_PreMultipliers[2]));
  TRACEKEYVALS("PreMult[3]","%f",VALUE(m_PreMultipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("PreMult File","%s",m_PreMultipliers[0].File);
  TRACEKEYVALS("PreMult Line","%d",m_PreMultipliers[0].Line);
#endif
  TRACEKEYVALS("D65Mult[0]","%f",VALUE(m_D65Multipliers[0]));
  TRACEKEYVALS("D65Mult[1]","%f",VALUE(m_D65Multipliers[1]));
  TRACEKEYVALS("D65Mult[2]","%f",VALUE(m_D65Multipliers[2]));
  TRACEKEYVALS("D65Mult[3]","%f",VALUE(m_D65Multipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("D65Mult File","%s",m_D65Multipliers[0].File);
  TRACEKEYVALS("D65Mult Line","%d",m_D65Multipliers[0].Line);
#endif

  TRACEKEYVALS("RawWidth","%d",m_RawWidth);
  TRACEKEYVALS("RawHeight","%d",m_RawHeight);
  TRACEKEYVALS("TopMargin","%d",m_TopMargin);
  TRACEKEYVALS("LeftMargin","%d",m_LeftMargin);
  TRACEKEYVALS("BlackLevel","%d",m_BlackLevel);

  // OK for second entry.
  if (m_LoadRawFunction == &CLASS kodak_ycbcr_load_raw) {
    m_Height += m_Height & 1;
    m_Width  += m_Width  & 1;
  }

  m_OutHeight = m_Height;
  m_OutWidth  = m_Width;

  TRACEKEYVALS("OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("Width","%d",m_Width);

  // Also some reshuffling for second entry problem.
  // not sure how c_matrix comes into play here ...
  int l_CameraMatrix = (m_UserSetting_CameraMatrix < 0) ?
     m_UserSetting_CameraWb : m_UserSetting_CameraMatrix;
  TRACEKEYVALS("US_CameraWB","%d",m_UserSetting_CameraWb);
  TRACEKEYVALS("US_CameraMatr","%d",m_UserSetting_CameraMatrix);
  TRACEKEYVALS("CameraMatrix","%d",l_CameraMatrix);
  TRACEKEYVALS("m_cmatrix[0][0","%f",m_cmatrix[0][0]);
  if (l_CameraMatrix && m_cmatrix[0][0] > 0.25) {
    TRACEKEYVALS("Using CamMatr","%s","Yes");
    memcpy (m_MatrixCamRGBToSRGB, m_cmatrix, sizeof m_cmatrix);
    m_RawColor = 0;
  }

  // Allocation is depending on m_Raw_Image below.
  FREE(m_Image);

  if (m_MetaLength) {
    FREE(m_MetaData);
    m_MetaData = (char *) MALLOC (m_MetaLength);
    merror (m_MetaData, "main()");
  }
  if (m_Filters || m_Colors == 1) {
    m_Raw_Image = (uint16_t *) CALLOC ((m_RawHeight+7)*m_RawWidth, 2);
    merror (m_Raw_Image, "main().m_Raw_Image");
  } else {
    // Basic image memory allocation @ 4 int per pixel happens here.
    m_Image = (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
    merror (m_Image, "main()");
  }

  TRACEKEYVALS("CameraMake","%s",m_CameraMake);
  TRACEKEYVALS("CameraModel","%s",m_CameraModel);
  TRACEKEYVALS("InputFile","%s",m_UserSetting_InputFileName);
  TRACEKEYVALS("Filters","%x",m_Filters);
  TRACEKEYVALS("Flip","%x",m_Flip);

  fseek (m_InputFile, m_Data_Offset, SEEK_SET);

  // Basic loading operation.
  // That's the hook into the real workhorse.

  (*this.*m_LoadRawFunction)();

  if (m_Raw_Image) {
    // Basic image memory allocation @ 4 int per pixel happens here.
    m_Image = (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
    merror (m_Image, "main()");
    crop_masked_pixels();
    FREE (m_Raw_Image);
  }

  if (m_ZeroIsBad) remove_zeroes();
  bad_pixels (m_UserSetting_BadPixelsFileName);
  if (m_UserSetting_DarkFrameFileName)
    subtract (m_UserSetting_DarkFrameFileName);

  // photivo extra.
  // Calculation of the inverse m_MatrixCamRGBToSRGB
  double rgb_cam_transpose[4][3];
  for (short i=0; i<4; i++) for (short j=0; j<3; j++)
    rgb_cam_transpose[i][j] = m_MatrixCamRGBToSRGB[j][i];
  pseudoinverse(rgb_cam_transpose,m_MatrixSRGBToCamRGB,m_Colors);

  // ReportedWidth & Height correct for fuji_rotate stuff and pixelaspects.
  m_ReportedWidth  = m_Width;
  m_ReportedHeight = m_Height;
  if (m_Fuji_Width) {
    m_IsFuji = m_Fuji_Width;
    m_ReportedWidth  = (m_Fuji_Width-1) / sqrt(0.5);
    m_ReportedHeight = (m_Height-m_Fuji_Width+1)/sqrt(0.5);
  }
  if (m_PixelAspect<1)
    m_ReportedHeight = (uint16_t) (m_Height / m_PixelAspect + 0.5);
  if (m_PixelAspect>1)
    m_ReportedWidth = (uint16_t) (m_Width * m_PixelAspect + 0.5);

  // TODO Mike: m_ReportedH/W is never set back to m_H/W, CHECK!

  TRACEKEYVALS("m_Width","%d",m_Width);
  TRACEKEYVALS("m_Height","%d",m_Height);
  TRACEKEYVALS("m_ReportedWidth","%d",m_ReportedWidth);
  TRACEKEYVALS("m_ReportedHeight","%d",m_ReportedHeight);

  TRACEKEYVALS("PreMult[0]","%f",VALUE(m_PreMultipliers[0]));
  TRACEKEYVALS("PreMult[1]","%f",VALUE(m_PreMultipliers[1]));
  TRACEKEYVALS("PreMult[2]","%f",VALUE(m_PreMultipliers[2]));
  TRACEKEYVALS("PreMult[3]","%f",VALUE(m_PreMultipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("PreMult File","%s",m_PreMultipliers[0].File);
  TRACEKEYVALS("PreMult Line","%d",m_PreMultipliers[0].Line);
#endif
  TRACEKEYVALS("D65Mult[0]","%f",VALUE(m_D65Multipliers[0]));
  TRACEKEYVALS("D65Mult[1]","%f",VALUE(m_D65Multipliers[1]));
  TRACEKEYVALS("D65Mult[2]","%f",VALUE(m_D65Multipliers[2]));
  TRACEKEYVALS("D65Mult[3]","%f",VALUE(m_D65Multipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("D65Mult File","%s",m_D65Multipliers[0].File);
  TRACEKEYVALS("D65Mult Line","%d",m_D65Multipliers[0].Line);
#endif
  TRACEKEYVALS("Colors","%d",m_Colors);
  TRACEKEYVALS("Filters","%x",m_Filters);
  TRACEKEYVALS("Flip","%x",m_Flip);

  // Cache the image after Phase1.
  FREE(m_Image_AfterPhase1);
  m_Image_AfterPhase1 =
    (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
  merror (m_Image_AfterPhase1, "main()");
  memcpy(m_Image_AfterPhase1,m_Image,m_OutHeight*m_OutWidth*sizeof(*m_Image));
  // Some other stuff to cache.
  m_Filters_AfterPhase1 = m_Filters;
  m_BlackLevel_AfterPhase1 = m_BlackLevel;
  for (int k=0; k<8; k++) m_CBlackLevel_AfterPhase1[k] = m_CBlackLevel[k];
  m_WhiteLevel_AfterPhase1 = m_WhiteLevel;
  m_Width_AfterPhase1 = m_Width;
  m_Height_AfterPhase1 = m_Height;
  m_OutWidth_AfterPhase1 = m_OutWidth;
  m_OutHeight_AfterPhase1 = m_OutHeight;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// RunDcRaw_Phase2
//
// Color scaling.
// Interpolation.
// Green mixing.
//
////////////////////////////////////////////////////////////////////////////////

short CLASS RunDcRaw_Phase2(const short NoCache) {

  // Make sure we are starting from the right image.
  if (NoCache) {
    FREE(m_Image_AfterPhase1);
  } else {
    FREE(m_Image);
    m_Width = m_Width_AfterPhase1;
    m_Height = m_Height_AfterPhase1;
    m_OutWidth = m_OutWidth_AfterPhase1;
    m_OutHeight = m_OutHeight_AfterPhase1;
    m_Image =
      (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
    merror (m_Image, "main()");
    memcpy(m_Image,m_Image_AfterPhase1,m_OutHeight*m_OutWidth*sizeof(*m_Image));
    // Restore some other cached values.
    m_Filters    = m_Filters_AfterPhase1;
    m_BlackLevel = m_BlackLevel_AfterPhase1;
    for (int k=0; k<8; k++) m_CBlackLevel[k] = m_CBlackLevel_AfterPhase1[k];
    m_WhiteLevel = m_WhiteLevel_AfterPhase1;
  }
  m_Fuji_Width = m_IsFuji;
  unsigned i = m_CBlackLevel[3];
  for (int c=0; c<3; c++) if (i > m_CBlackLevel[c]) i = m_CBlackLevel[c];
  for (int c=0; c<4; c++) m_CBlackLevel[c] -= i;
  m_BlackLevel += i;
  if (m_UserSetting_BlackPoint >= 0) m_BlackLevel = m_UserSetting_BlackPoint;
  for (int c=0; c<4; c++) m_CBlackLevel[c] += m_BlackLevel;
  if (m_UserSetting_Saturation > 0)  m_WhiteLevel = m_UserSetting_Saturation;

  // If m_UserSetting_HalfSize then the BAYER(row,col) macro
  // will map 2X2 pixels of the Bayer array, directly onto one
  // pixel of the image (and the correct color channel) foregoing
  // the need for interpolation and much faster !
  m_Shrink = m_Filters && m_UserSetting_HalfSize;

  TRACEKEYVALS("Shrink","%d",m_Shrink);

  TRACEKEYVALS("BlackLevel","%d",m_BlackLevel);
  TRACEKEYVALS("WhiteLevel","%d",m_WhiteLevel);
  TRACEKEYVALS("Colors","%d",m_Colors);

  TRACEKEYVALS("Phase2 begin Width","%d",m_Width);
  TRACEKEYVALS("Phase2 begin Height","%d",m_Height);
  TRACEKEYVALS("Phase2 begin OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("Phase2 begin OutHeight","%d",m_OutHeight);

  // Crop for detail view
  if (m_UserSetting_DetailView == 1 &&
      m_IsFuji == 0 &&
      m_PixelAspect == 1.0f) {
    ptCrop();
  }

  // Copied from earlier to here also.
  // Enables Phase3 to reenter with a different FourColorRGB setting.
  if ((m_UserSetting_Quality == ptInterpolation_VNG4 || m_UserSetting_HalfSize)
      && m_Filters != 0) { // m_Filters==0 <-> 3 channel RAWs
    m_MixGreen = 1;
  } else {
    m_MixGreen = 0;
  }

  if (!m_IsFoveon && m_UserSetting_AdjustMaximum > 0) {
    ptAdjustMaximum(1-m_UserSetting_AdjustMaximum);
  }

  assert (!m_IsFoveon);
  // if (m_IsFoveon ) foveon_interpolate();

  if (m_UserSetting_GreenEquil) {
    TRACEKEYVALS("Green equilibration","%s","");
    green_equilibrate((float)m_UserSetting_GreenEquil/100.0f);
  }

  if (m_UserSetting_HotpixelReduction) {
    TRACEKEYVALS("Hotpixel reduction on bayer","%s","");
    ptHotpixelReductionBayer();
  }

  if (m_UserSetting_CfaLineDn !=0) {
    TRACEKEYVALS("Cfa line denoise","%s","");
    cfa_linedn(0.00002*(float)m_UserSetting_CfaLineDn);
  }

  if (!m_IsFoveon) {
    ptScaleColors();
  }

  if (m_UserSetting_CaCorrect !=0) {
    TRACEKEYVALS("CA correction","%s","");
    CA_correct(m_UserSetting_CaRed,m_UserSetting_CaBlue);
  }

  TRACEKEYVALS("Colors","%d",m_Colors);

  // not 1:1 pipe, use FC marco instead of interpolation
  uint16_t    (*TempImage)[4];
  if (m_Shrink && m_Filters != 2) { // -> preinterpolate will set m_Filters = 0 if != 2
    m_OutHeight = (m_Height + 1) / 2;
    m_OutWidth  = (m_Width + 1) / 2;
    TempImage = (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *TempImage);
    merror (TempImage, "main()");
#pragma omp parallel for schedule(static)
    for (uint16_t row=0; row < m_Height; row++) {
      for (uint16_t col=0; col < m_Width; col++) {
        TempImage[((row) >> m_Shrink)*m_OutWidth + ((col) >> m_Shrink)][FC(row,col)] =
          m_Image[(row)*m_Width + (col)][FC(row,col)];
      }
    }
    FREE(m_Image);
    m_Image = TempImage;
  }

  // RG1BG2 -> RGB (if not 4 colors needed later on)
  pre_interpolate();

  TRACEKEYVALS("Colors","%d",m_Colors);


  // Interpolation/demosaicing according to one of the algorithms.
  if (m_Filters) {
    if (m_UserSetting_BayerDenoise && !m_UserSetting_HalfSize) {
      if (m_UserSetting_BayerDenoise==1) fbdd(0);
      else if (m_UserSetting_BayerDenoise==2) fbdd(1);
    }
    if (m_Filters == 2) // for 3x3 pattern we fix to VNG
      vng_interpolate();
    else if (m_UserSetting_Quality == ptInterpolation_Linear)
      lin_interpolate();
    else if (m_UserSetting_Quality == ptInterpolation_VNG ||
             m_UserSetting_Quality == ptInterpolation_VNG4)
      vng_interpolate();
    else if (m_UserSetting_Quality == ptInterpolation_PPG)
      ppg_interpolate();
    else if (m_UserSetting_Quality == ptInterpolation_DCB)
      dcb(m_UserSetting_InterpolationPasses, 1);
    else if (m_UserSetting_Quality == ptInterpolation_DCBSoft)
      dcb_interpolate_soft_old(m_UserSetting_InterpolationPasses, 1);
    else if (m_UserSetting_Quality == ptInterpolation_DCBSharp)
      dcb_interpolate_sharp_old(m_UserSetting_InterpolationPasses, 1);
    else if (m_UserSetting_Quality == ptInterpolation_AHD)
      ahd_interpolate();
    else if (m_UserSetting_Quality == ptInterpolation_AHD_mod)
      ahd_interpolate_mod();
    else if (m_UserSetting_Quality == ptInterpolation_VCD)
      vcd_interpolate(12);
    else if (m_UserSetting_Quality == ptInterpolation_LMMSE)
      lmmse_interpolate(1);
    else if (m_UserSetting_Quality == ptInterpolation_AMaZE)
      amaze_demosaic();
  } else {
    if (m_UserSetting_HotpixelReduction && m_UserSetting_HalfSize) {
      TRACEKEYVALS("Hotpixel reduction on RGB","%s","");
      ptHotpixelReduction();
    }
    if (m_UserSetting_BayerDenoise && !m_UserSetting_HalfSize) {
      if (m_UserSetting_BayerDenoise==1) fbdd(0);
      else if (m_UserSetting_BayerDenoise==2) fbdd(1);
    }
  }
  // If 1:1 and no interpolation is chosen show the Bayer pattern.

  TRACEKEYVALS("Interpolation type","%d",m_UserSetting_Quality);

  // Additional photivo stuff. Other halvings on request.
  if (m_UserSetting_HalfSize > 1                      ||
      (m_UserSetting_HalfSize == 1 && m_Shrink  == 0) || // 3 channel RAWs
      (m_UserSetting_HalfSize == 1 && m_Filters == 2)) { // 3x3 pattern
    short Factor = m_UserSetting_HalfSize - 1;
    if (m_Shrink == 0 || m_Filters == 2) Factor += 1;

    uint16_t NewHeight = m_Height >> Factor;
    uint16_t NewWidth = m_Width >> Factor;

    short Step = 1 << Factor;
    int Average = 2 * Factor;

    uint16_t (*NewImage)[4] =
      (uint16_t (*)[4]) CALLOC(NewWidth*NewHeight,sizeof(*m_Image));
    ptMemoryError(NewImage,__FILE__,__LINE__);

#pragma omp parallel for schedule(static)
    for (uint16_t Row=0; Row < NewHeight*Step; Row+=Step) {
      for (uint16_t Col=0; Col < NewWidth*Step; Col+=Step) {
        uint32_t  PixelValue[4] = {0,0,0,0};
        for (uint8_t sRow=0; sRow < Step; sRow++) {
          for (uint8_t sCol=0; sCol < Step; sCol++) {
            int32_t index = (Row+sRow)*m_Width+Col+sCol;
            for (short c=0; c < 4; c++) {
              PixelValue[c] += m_Image[index][c];
            }
          }
        }
        for (short c=0; c < 4; c++) {
          NewImage[Row/Step*NewWidth+Col/Step][c]
            = PixelValue[c] >> Average;
        }
      }
    }

    FREE(m_Image);
    m_Height = m_OutHeight = NewHeight;
    m_Width = m_OutWidth = NewWidth;
    m_Image = NewImage;
  }

  // Green mixing
  if (m_MixGreen && m_Colors != 3) {
#pragma omp parallel for schedule(static) default(shared)
    for (uint32_t i=0; i < (uint32_t) m_Height*m_Width; i++)
      m_Image[i][1] = (m_Image[i][1] + m_Image[i][3]) >> 1;
    m_Colors = 3;
  }

  // Median filter.
  if (!m_IsFoveon && m_Colors == 3) {
    if (m_UserSetting_MedianPasses > 0)  median_filter_new();
    if (m_UserSetting_ESMedianPasses > 0 && !m_UserSetting_HalfSize) es_median_filter();
    if (m_UserSetting_EeciRefine == 1)  refinement();
  }

  // Additional cleaning with hotpixel reduction
  if (m_UserSetting_HotpixelReduction && m_UserSetting_Quality != ptInterpolation_Bayer) {
    ptHotpixelReduction();
  }

  // XXX JDLA Additional steps for Fuji
  // And they don't hurt for others as they are early stopped.
  fuji_rotate();
  stretch();

  // Crop for detail view
  if (m_UserSetting_DetailView == 1 &&
      (m_IsFuji != 0 ||
       m_PixelAspect != 1.0f)) {
    ptCrop();
  }

  // Cache the image after Phase2.
  FREE(m_Image_AfterPhase2);
  m_Image_AfterPhase2 =
    (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
  merror (m_Image_AfterPhase2, "main()");
  memcpy(m_Image_AfterPhase2,m_Image,m_OutHeight*m_OutWidth*sizeof(*m_Image));
  // Some other stuff to cache.
  m_Filters_AfterPhase2 = m_Filters;

  TRACEKEYVALS("BlackLevel","%d",m_BlackLevel);
  TRACEKEYVALS("WhiteLevel","%d",m_WhiteLevel);
  TRACEKEYVALS("Colors","%d",m_Colors);
  TRACEKEYVALS("Filters","%x",m_Filters);

  TRACEKEYVALS("Phase2 end Width","%d",m_Width);
  TRACEKEYVALS("Phase2 end Height","%d",m_Height);
  TRACEKEYVALS("Phase2 end OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("Phase2 end OutHeight","%d",m_OutHeight);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// RunDcRaw_Phase3
//
// Highlights
//
////////////////////////////////////////////////////////////////////////////////

short CLASS RunDcRaw_Phase3(const short NoCache) {

  // Make sure we are starting from the right image.
  if (NoCache) {
    FREE(m_Image_AfterPhase2);
  } else {
    FREE(m_Image);
    m_Image =
      (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
    merror (m_Image, "main()");
    memcpy(m_Image,m_Image_AfterPhase2,m_OutHeight*m_OutWidth*sizeof(*m_Image));
    // Restore some other cached values.
    m_Filters    = m_Filters_AfterPhase2;
  }

  TRACEKEYVALS("CamMult[0]","%f",VALUE(m_CameraMultipliers[0]));
  TRACEKEYVALS("CamMult[1]","%f",VALUE(m_CameraMultipliers[1]));
  TRACEKEYVALS("CamMult[2]","%f",VALUE(m_CameraMultipliers[2]));
  TRACEKEYVALS("CamMult[3]","%f",VALUE(m_CameraMultipliers[3]));
#ifdef TRACE_ORIGIN
  TRACEKEYVALS("CamMult File","%s",m_CameraMultipliers[0].File);
  TRACEKEYVALS("CamMult Line","%d",m_CameraMultipliers[0].Line);
#endif

  ptHighlight(m_UserSetting_photivo_ClipMode,
              m_UserSetting_photivo_ClipParameter);

  // Cache the image after Phase3.
  FREE(m_Image_AfterPhase3);
  m_Image_AfterPhase3 =
    (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *m_Image);
  merror (m_Image_AfterPhase3, "main()");
  memcpy(m_Image_AfterPhase3,m_Image,m_OutHeight*m_OutWidth*sizeof(*m_Image));
  // Some other stuff to cache.
  m_Filters_AfterPhase3 = m_Filters;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// BlendHighlights
// (original from dcraw TODO Refine and analyse)
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptBlendHighlights() {

  int ClipLevel=INT_MAX;
  int i;
  int j;

  static const float trans[2][4][4] =
  { { { 1,1,1 }, { 1.7320508,-1.7320508,0 }, { -1,-1,2 } },
    { { 1,1,1,1 }, { 1,-1,1,-1 }, { 1,1,-1,-1 }, { 1,-1,-1,1 } } };
  static const float itrans[2][4][4] =
  { { { 1,0.8660254,-0.5 }, { 1,-0.8660254,-0.5 }, { 1,0,1 } },
    { { 1,1,1,1 }, { 1,-1,1,-1 }, { 1,1,-1,-1 }, { 1,-1,-1,1 } } };
  float Cam[2][4], lab[2][4], Sum[2], chratio;

  const int transIdx = m_Colors - 3;
  assert(transIdx > -1);
  if (transIdx > 1) return;

  // to shut up gcc warnings...
  const int localColors = ptMin(static_cast<int>(m_Colors), 4);

  for (short c=0; c<m_Colors; c++) {
    if (ClipLevel > (i = (int)(0xFFFF*VALUE(m_PreMultipliers[c])))) {
      ClipLevel = i;
    }
  }

  for (uint16_t Row=0; Row < m_Height; Row++) {
    for (uint16_t Column=0; Column < m_Width; Column++) {
      short c;
      for (c=0; c<m_Colors; c++) {
        if (m_Image[Row*m_Width+Column][c] > ClipLevel) break;
      }
      if (c == m_Colors) continue; // No clip
      for (c=0; c<m_Colors; c++) {
  Cam[0][c] = m_Image[Row*m_Width+Column][c];
  Cam[1][c] = MIN(Cam[0][c],(float)ClipLevel);
      }
      for (i=0; i < 2; i++) {
  for (c=0; c<m_Colors; c++) {
          for (lab[i][c]=j=0; j < m_Colors; j++) {
      lab[i][c] += trans[transIdx][c][j] * Cam[i][j];
          }
        }
  for (Sum[i]=0,c=1; c < m_Colors; c++) {
    Sum[i] += SQR(lab[i][c]);
        }
      }
      chratio = sqrt(Sum[1]/Sum[0]);
      for (c = 1; c < m_Colors; c++) {
        lab[0][c] *= chratio;
      }
      for (c = 0; (c < localColors); c++) {
        for (Cam[0][c]=j=0; (j < localColors); j++) {
          Cam[0][c] += itrans[transIdx][c][j] * lab[0][j];
        }
      }
      for (c = 0; c < m_Colors; c++) {
        m_Image[Row*m_Width+Column][c] = (uint16_t)(Cam[0][c] / m_Colors);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Hotpixel reduction
//
////////////////////////////////////////////////////////////////////////////////


void CLASS ptHotpixelReduction() {
  uint16_t HotpixelThreshold = 0x00ff;
  uint16_t Threshold = (int32_t) ((1.0-m_UserSetting_HotpixelReduction)*0x2fff);
  uint16_t Width = m_OutWidth;
  uint16_t Height = m_OutHeight;
  // bei 1:2 oder kleiner m_Image hat in jedem Punkt RGBG
  // bei 1:1 m_Image hat Bayer Pattern
  // Länge und Breite OutWidth und OutHeight
  // unterschieptiche pixel in unterschieptichen ebenen ansprechen und werte zur luminanz skalieren
#pragma omp parallel for schedule(static) default(shared)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      uint16_t TempValue = 0;
      for (int c=0; c<4; c++) {
        if (m_Image[Row*Width+Col][c] > HotpixelThreshold) {
          if (Row > 1) TempValue = MAX(m_Image[(Row-1)*Width+Col][c],TempValue);
          if (Row < Height-1) TempValue = MAX(m_Image[(Row+1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MAX(m_Image[Row*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MAX(m_Image[Row*Width+Col+1][c],TempValue);
          if (TempValue+Threshold<m_Image[Row*Width+Col][c])
            m_Image[Row*Width+Col][c] = TempValue;
        }
        if (m_Image[Row*Width+Col][c] < 10*HotpixelThreshold) {
          TempValue = 0xffff;
          if (Row > 1) TempValue = MIN(m_Image[(Row-1)*Width+Col][c],TempValue);
          if (Row < Height-1) TempValue = MIN(m_Image[(Row+1)*Width+Col][c],TempValue);
          if (Col > 1) TempValue = MIN(m_Image[Row*Width+Col-1][c],TempValue);
          if (Col < Width-1) TempValue = MIN(m_Image[Row*Width+Col+1][c],TempValue);
          if (TempValue-Threshold>m_Image[Row*Width+Col][c])
            m_Image[Row*Width+Col][c] = TempValue;
        }
      }
    }
  }
}

void CLASS ptHotpixelReductionBayer() {
  uint16_t HotpixelThreshold = 0x00ff;
  uint16_t Threshold = (int32_t) ((1.0-m_UserSetting_HotpixelReduction)*0x2fff);
  uint16_t Width = m_OutWidth;
  uint16_t Height = m_OutHeight;

#pragma omp parallel for schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      uint16_t TempValue = 0;
      for (int Channel=0; Channel<4; Channel++) {
        if (m_Image[Row*Width+Col][Channel] > HotpixelThreshold) {
          if (Row > 2) TempValue = MAX(m_Image[(Row-2)*Width+Col][Channel],TempValue);
          if (Row < Height-2) TempValue = MAX(m_Image[(Row+2)*Width+Col][Channel],TempValue);
          if (Col > 2) TempValue = MAX(m_Image[Row*Width+Col-2][Channel],TempValue);
          if (Col < Width-2) TempValue = MAX(m_Image[Row*Width+Col+2][Channel],TempValue);
          if (TempValue+Threshold<m_Image[Row*Width+Col][Channel]) {
            for (int c=0; c<4; c++) {
              if (Row > 1) {
                TempValue = MAX(m_Image[(Row-1)*Width+Col][c],TempValue);
                if (Col > 1) TempValue = MAX(m_Image[(Row-1)*Width+Col-1][c],TempValue);
                if (Col < Width-1) TempValue = MAX(m_Image[(Row-1)*Width+Col+1][c],TempValue);
              }
              if (Row < Height-1) {
                TempValue = MAX(m_Image[(Row+1)*Width+Col][c],TempValue);
                if (Col > 1) TempValue = MAX(m_Image[(Row+1)*Width+Col-1][c],TempValue);
                if (Col < Width-1) TempValue = MAX(m_Image[(Row+1)*Width+Col+1][c],TempValue);
              }
              if (Col > 1) TempValue = MAX(m_Image[Row*Width+Col-1][c],TempValue);
              if (Col < Width-1) TempValue = MAX(m_Image[Row*Width+Col+1][c],TempValue);
            }
            if (TempValue+Threshold<m_Image[Row*Width+Col][Channel])
              m_Image[Row*Width+Col][Channel] = TempValue;
          }
        }
      }
      for (int Channel=0; Channel<4; Channel++) {
        if (m_Image[Row*Width+Col][Channel] < 10*HotpixelThreshold) {
          TempValue = 0xffff;
          if (Row > 2) TempValue = MIN(m_Image[(Row-2)*Width+Col][Channel],TempValue);
          if (Row < Height-2) TempValue = MIN(m_Image[(Row+2)*Width+Col][Channel],TempValue);
          if (Col > 2) TempValue = MIN(m_Image[Row*Width+Col-2][Channel],TempValue);
          if (Col < Width-2) TempValue = MIN(m_Image[Row*Width+Col+2][Channel],TempValue);
          if (TempValue-Threshold>m_Image[Row*Width+Col][Channel]) {
            for (int c=0; c<4; c++) {
              if (Row > 1) {
                TempValue = MIN(m_Image[(Row-1)*Width+Col][c],TempValue);
                if (Col > 1) TempValue = MIN(m_Image[(Row-1)*Width+Col-1][c],TempValue);
                if (Col < Width-1) TempValue = MIN(m_Image[(Row-1)*Width+Col+1][c],TempValue);
              }
              if (Row < Height-1) {
                TempValue = MIN(m_Image[(Row+1)*Width+Col][c],TempValue);
                if (Col > 1) TempValue = MIN(m_Image[(Row+1)*Width+Col-1][c],TempValue);
                if (Col < Width-1) TempValue = MIN(m_Image[(Row+1)*Width+Col+1][c],TempValue);
              }
              if (Col > 1) TempValue = MIN(m_Image[Row*Width+Col-1][c],TempValue);
              if (Col < Width-1) TempValue = MIN(m_Image[Row*Width+Col+1][c],TempValue);
            }
            if (TempValue-Threshold>m_Image[Row*Width+Col][Channel])
              m_Image[Row*Width+Col][Channel] = TempValue;
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Crop for detail view
//
////////////////////////////////////////////////////////////////////////////////

inline uint16_t MultOf6(uint16_t AValue) {
  return ptMax(AValue - (AValue % 6), 0);
}

void CLASS ptCrop() {
  assert (m_UserSetting_HalfSize == 0);

  uint16_t (*TempImage)[4];
  uint16_t CropW = m_UserSetting_DetailViewCropW;
  uint16_t CropH = m_UserSetting_DetailViewCropH;
  uint16_t CropX = m_UserSetting_DetailViewCropX;
  uint16_t CropY = m_UserSetting_DetailViewCropY;

  if (m_Filters == 2) { // for 3x3 pattern
    CropW = MultOf6(CropW);
    CropH = MultOf6(CropH);
    CropX = MultOf6(CropX);
    CropY = MultOf6(CropY);
  }

  if (m_Flip & 2) {
    CropX = m_Height - CropX - CropW;
  }
  if (m_Flip & 1) {
    CropY = m_Width - CropY - CropH;
  }
  if (m_Flip & 4) {
    SWAP(CropW, CropH);
    SWAP(CropX, CropY);
  }
  m_OutHeight = CropH;
  m_OutWidth  = CropW;
  TempImage = (uint16_t (*)[4]) CALLOC (m_OutHeight*m_OutWidth, sizeof *TempImage);
  merror (TempImage, "Temp for detail view");

#pragma omp parallel for schedule(static)
  for (uint16_t row=0; row < m_OutHeight; row++) {
    for (uint16_t col=0; col < m_OutWidth; col++) {
      for (short c=0; c<4; c++) {
        TempImage[row *m_OutWidth + col][c] =
          m_Image[(row+CropY)*m_Width + (col+CropX)][c];
      }
    }
  }

  m_Width = m_OutWidth;
  m_Height = m_OutHeight;
  FREE(m_Image);
  m_Image = TempImage;
  TRACEKEYVALS("Phase2 detail view Width","%d",m_Width);
  TRACEKEYVALS("Phase2 detail view Height","%d",m_Height);
  TRACEKEYVALS("Phase2 detail view OutWidth","%d",m_OutWidth);
  TRACEKEYVALS("Phase2 detail view OutHeight","%d",m_OutHeight);
}

////////////////////////////////////////////////////////////////////////////////
//
// MedianFilter
// Repeatepty 3x3 median filter on R-G and B-G channels
// Repeat : m_UserSetting_MedianPasses
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptMedianFilter() {

  int      Median[9];
  static const uint8_t opt[] =  /* Optimal 9-element median search */
  { 1,2, 4,5, 7,8, 0,1, 3,4, 6,7, 1,2, 4,5, 7,8,
    0,3, 5,8, 4,7, 3,6, 1,4, 2,5, 4,7, 4,2, 6,4, 4,2 };

  for (short Pass=1; Pass <= m_UserSetting_MedianPasses; Pass++) {
    for (short c=0; c < 3; c+=2) {
#pragma omp parallel for schedule(static) default(shared)
      for (int32_t i = 0; i< (int32_t)m_Width*m_Height; i++) {
        m_Image[i][3] = m_Image[i][c];
      }
#pragma omp parallel for schedule(static) default(shared) private(Median)
      for (int32_t n=m_Width; n<m_Width*(m_Height-1); n++) {
        if ((n+1) % m_Width < 2) continue;
        short k=0;
        for (int32_t i = -m_Width; i <= m_Width; i += m_Width) {
          for (int32_t j = i-1; j <= i+1; j++) {
            Median[k++] = m_Image[n+j][3] - m_Image[n+j][1];
          }
        }
        for (unsigned short i=0; i < sizeof opt; i+=2) {
          if (Median[opt[i]] > Median[opt[i+1]]) {
            SWAP (Median[opt[i]] , Median[opt[i+1]]);
          }
        }
        m_Image[n][c] = CLIP(Median[4] + m_Image[n][1]);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// WaveletDenoise
// (original from dcraw TODO Refine and analyse)
// Be aware :
//   Changes m_WhiteLevel
//   Changes m_Blacklevel
//
////////////////////////////////////////////////////////////////////////////////

void CLASS ptWaveletDenoise() {
  static const float Noise[] =
  { 0.8002,0.2735,0.1202,0.0585,0.0291,0.0152,0.0080,0.0044 };

  int Scale=1;
  int lev, hpass, lpass, wlast, blk[2];

  TRACEKEYVALS("BlackLevel","%d",m_BlackLevel);
  TRACEKEYVALS("WhiteLevel","%d",m_WhiteLevel);

  while (m_WhiteLevel << Scale < 0x10000) Scale++;
  m_WhiteLevel <<= --Scale;
  m_BlackLevel <<= Scale;
  for (int c=0; c<4; c++) m_CBlackLevel[c] <<= Scale;

  TRACEKEYVALS("Scale(denoise)","%d",Scale);
  TRACEKEYVALS("BlackLevel","%d",m_BlackLevel);
  TRACEKEYVALS("WhiteLevel","%d",m_WhiteLevel);

  uint32_t Size = m_OutHeight*m_OutWidth;
  float *fImage = (float*)
    MALLOC((Size*3 +m_OutHeight+ m_OutWidth)* sizeof(*fImage));
  merror(fImage,"WaveletDenoise()");

  float* fTemp = fImage + Size*3;
  short NrColors = m_Colors;
  if (NrColors == 3 && m_Filters) NrColors++;
  for (short c=0; c<NrColors; c++) {  /* deNoise R,G1,B,G3 individually */
    for (uint32_t i=0; i < Size; i++) {
      fImage[i] = 256 * sqrt((unsigned) (m_Image[i][c] << Scale));
    }
    for (hpass=lev=0; lev < 5; lev++) {
      lpass = Size*((lev & 1)+1);
      for (uint16_t Row=0; Row < m_OutHeight; Row++) {
        hat_transform(fTemp,fImage+hpass+Row*m_OutWidth,1,m_OutWidth,1<<lev);
  for (uint16_t Col=0; Col < m_OutWidth; Col++) {
          fImage[lpass + Row*m_OutWidth + Col] = fTemp[Col] * 0.25;
        }
      }
      for (uint16_t Col=0; Col < m_OutWidth; Col++) {
        hat_transform(fTemp,fImage+lpass+Col,m_OutWidth,m_OutHeight,1<<lev);
        for (uint16_t Row=0; Row < m_OutHeight; Row++) {
          fImage[lpass + Row*m_OutWidth + Col] = fTemp[Row] * 0.25;
        }
      }
      float Threshold = m_UserSetting_DenoiseThreshold * Noise[lev];
      for (uint32_t i=0; i < Size; i++) {
        fImage[hpass+i] -= fImage[lpass+i];
        if (fImage[hpass+i] < -Threshold) {
          fImage[hpass+i] += Threshold;
        } else if (fImage[hpass+i] >  Threshold) {
          fImage[hpass+i] -= Threshold;
        } else {
          fImage[hpass+i] = 0;
        }
        if (hpass) {
          fImage[i] += fImage[hpass+i];
        }
      }
      hpass = lpass;
    }
    for (uint32_t i=0; i < Size; i++) {
      m_Image[i][c] = CLIP((int32_t)(SQR(fImage[i]+fImage[lpass+i])/0x10000));
    }
  }
  if (m_Filters && m_Colors == 3) {  /* pull G1 and G3 closer together */
    float Multiplier[2];
    for (uint16_t Row=0; Row < 2; Row++) {
      Multiplier[Row] = 0.125 *
        VALUE(m_PreMultipliers[FC(Row+1,0) | 1]) /
        VALUE(m_PreMultipliers[FC(Row,0) | 1]);
      blk[Row] = m_CBlackLevel[FC(Row,0) | 1];
    }
    uint16_t* Window[4];
    for (short i=0; i < 4; i++) {
      Window[i] = (uint16_t *) fImage + m_Width*i;
    }
    wlast = -1;
    for (uint16_t Row=1; Row < m_Height-1; Row++) {
      while (wlast < Row+1) {
        wlast++;
  for (short i=0; i < 4; i++) {
    Window[(i+3) & 3] = Window[i];
        }
  for (uint16_t Col = FC(wlast,1) & 1; Col < m_Width; Col+=2) {
    Window[2][Col] = BAYER(wlast,Col);
        }
      }
      float Threshold = m_UserSetting_DenoiseThreshold/512;
      float Average;
      float Difference;
      for (uint16_t Col = (FC(Row,0) & 1)+1; Col < m_Width-1; Col+=2) {
  Average = ( Window[0][Col-1] + Window[0][Col+1] +
        Window[2][Col-1] + Window[2][Col+1] - blk[~Row & 1]*4 )
          * Multiplier[Row & 1]
          + (Window[1][Col] + blk[Row & 1]) * 0.5;
  Average = Average < 0 ? 0 : sqrt(Average);
  Difference = sqrt(BAYER(Row,Col)) - Average;
  if (Difference < -Threshold) {
          Difference += Threshold;
  } else if (Difference >  Threshold) {
          Difference -= Threshold;
  } else {
          Difference = 0;
        }
  BAYER(Row,Col) = CLIP((int32_t)(SQR(Average+Difference) + 0.5));
      }
    }
  }
  FREE(fImage);
}

////////////////////////////////////////////////////////////////////////////////
//
// RebuildHighlights
// (original from dcraw TODO Refine and analyse)
//
////////////////////////////////////////////////////////////////////////////////

#define SCALE (4 >> m_Shrink)
void CLASS ptRebuildHighlights(const short highlight) {
  float *map, sum, wgt, grow;
  int hsat[4], count, spread, change, val, i;
  unsigned high, wide, mrow, mcol, row, col, d, y, x;
  uint16_t *pixel;
  static const signed char dir[8][2] =
    { {-1,-1}, {-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1} };

  grow = pow (2, 1-highlight);
  for (short c=0;c<m_Colors;c++)  hsat[c] = (int)(32000 * VALUE(m_PreMultipliers[c]));
  short kc = 0;
  for (short c=1; c < m_Colors; c++)
    if (VALUE(m_PreMultipliers[kc]) < VALUE(m_PreMultipliers[c])) kc = c;
  high = m_Height / SCALE;
  wide =  m_Width / SCALE;
  map = (float *) CALLOC (high*wide, sizeof *map);
  merror (map, "recover_highlights()");
  for (short c=0;c<m_Colors;c++) {
    if (c != kc) {
      memset (map, 0, high*wide*sizeof *map);
#pragma omp parallel for schedule(static) private(mrow, mcol, sum, wgt, count, pixel, row, col)
      for (mrow=0; mrow < high; mrow++) {
        for (mcol=0; mcol < wide; mcol++) {
          sum = wgt = count = 0;
          for (row = mrow*SCALE; row < (mrow+1)*SCALE; row++) {
            for (col = mcol*SCALE; col < (mcol+1)*SCALE; col++) {
              pixel = m_Image[row*m_Width+col];
              if (pixel[c] / hsat[c] == 1 && pixel[kc] > 24000) {
                sum += pixel[c];
                wgt += pixel[kc];
                count++;
              }
            }
            if (count == SCALE*SCALE) map[mrow*wide+mcol] = sum / wgt;
          }
        }
      }
      for (spread = (int)(32/grow); spread--; ) {
#pragma omp parallel for schedule(static) private(mrow, mcol, sum, count, x, y, d)
        for (mrow=0; mrow < high; mrow++) {
          for (mcol=0; mcol < wide; mcol++) {
            if (map[mrow*wide+mcol]) continue;
            sum = count = 0;
            for (d=0; d < 8; d++) {
              y = mrow + dir[d][0];
              x = mcol + dir[d][1];
              if (y < high && x < wide && map[y*wide+x] > 0) {
                sum  += (1 + (d & 1)) * map[y*wide+x];
                count += 1 + (d & 1);
              }
            }
            if (count > 3)
              map[mrow*wide+mcol] = - (sum+grow) / (count+grow);
          }
        }
        change = 0;
#pragma omp parallel for schedule(static) private(i) reduction(||:change)
        for (i=0; i < (int)(high*wide); i++) {
          if (map[i] < 0) {
            map[i] = -map[i];
            change = 1;
          }
        }
        if (!change) break;
      }
#pragma omp parallel for schedule(static) private(i)
      for (i=0; i < (int)(high*wide); i++) {
        if (map[i] == 0) map[i] = 1;
      }
#pragma omp parallel for schedule(static) private(mrow, mcol, row, col, pixel, val)
      for (mrow=0; mrow < high; mrow++) {
        for (mcol=0; mcol < wide; mcol++) {
          for (row = mrow*SCALE; row < (mrow+1)*SCALE; row++) {
            for (col = mcol*SCALE; col < (mcol+1)*SCALE; col++) {
              pixel = m_Image[row*m_Width+col];
              if (pixel[c] / hsat[c] > 1) {
                val = (int)(pixel[kc] * map[mrow*wide+mcol]);
                if (pixel[c] < val) pixel[c] = CLIP(val);
              }
            }
          }
        }
      }
    }
  }
  FREE (map);
}
#undef SCALE

////////////////////////////////////////////////////////////////////////////////
//
// LabToCam helper function.
//
// FIXME
// At this moment this is verbatim copies of ptImage.cpp. Room for improvement.
//
////////////////////////////////////////////////////////////////////////////////


void CLASS LabToCam(double Lab[3],uint16_t Cam[4]) {

  if(!ToCamFunctionInited) {
    for(short i=0;i<m_Colors;i++) {
      for (short j=0;j<3;j++) {
        MatrixXYZToCam[i][j] = 0.0;
        for (short k=0;k<3;k++) {
          MatrixXYZToCam[i][j] +=
            m_MatrixSRGBToCamRGB[i][k] * MatrixXYZToRGB[ptSpace_sRGB_D65][k][j];
        }
      }
    }
  ToCamFunctionInited = 1;
  }

  double xyz[3];
  double fx,fy,fz;
  double xr,yr,zr;
  const double epsilon = 216.0/24389.0;
  const double kappa   = 24389.0/27.0;

  const double L = Lab[0];
  const double a = Lab[1];
  const double b = Lab[2];

  const double Tmp1 = (L+16.0)/116.0;

  yr = (L<=kappa*epsilon)?
       (L/kappa):(Tmp1*Tmp1*Tmp1);
  fy = (yr<=epsilon) ? ((kappa*yr+16.0)/116.0) : Tmp1;
  fz = fy - b/200.0;
  fx = a/500.0 + fy;
  const double fz3 = fz*fz*fz;
  const double fx3 = fx*fx*fx;
  zr = (fz3<=epsilon) ? ((116.0*fz-16.0)/kappa) : fz3;
  xr = (fx3<=epsilon) ? ((116.0*fx-16.0)/kappa) : fx3;

  xyz[0] = xr*D65Reference[0]*65535.0 - 0.5;
  xyz[1] = yr*D65Reference[1]*65535.0 - 0.5;
  xyz[2] = zr*D65Reference[2]*65535.0 - 0.5;

  // And finally to RGB via the matrix.
  for (short c=0; c<m_Colors; c++) {
    double Value = 0;
    Value += MatrixXYZToCam[c][0] * xyz[0];
    Value += MatrixXYZToCam[c][1] * xyz[1];
    Value += MatrixXYZToCam[c][2] * xyz[2];
    Cam[c] = (uint16_t) CLIP((int32_t)(Value));
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// CamToLab helper function.
//
////////////////////////////////////////////////////////////////////////////////


void CLASS CamToLab(uint16_t Cam[4], double Lab[3]) {

  // Initialize the lookup table for the RGB->LAB function
  // if this would be the first time.
  if (!ToLABFunctionInited) {
    // Remark : we extend the table well beyond r>1.0 for numerical
    // stability purposes. XYZ>1.0 occurs sometimes and this way
    // it stays stable (srgb->lab->srgb correct within 0.02%)
    for (uint32_t i=0; i<0x20000; i++) {
      double r = (double)(i) / 0xffff;
      ToLABFunctionTable[i] = r > 216.0/24389.0 ?
                              pow(r,1/3.0) :
                              (24389.0/27.0*r + 16.0)/116.0;
    }
    for(short i=0;i<3;i++) {
      for (short j=0;j<m_Colors;j++) {
        MatrixCamToXYZ[i][j] = 0.0;
        for (short k=0;k<3;k++) {
          MatrixCamToXYZ[i][j] +=
            MatrixRGBToXYZ[ptSpace_sRGB_D65][i][k] * m_MatrixCamRGBToSRGB[k][j];
        }
      }
    }
  ToLABFunctionInited = 1;
  }

  // First go to XYZ
  double xyz[3] = {0.5,0.5,0.5};
  for (short Color = 0; Color < m_Colors; Color++) {
    xyz[0] += MatrixCamToXYZ[0][Color] * Cam[Color];
    xyz[1] += MatrixCamToXYZ[1][Color] * Cam[Color];
    xyz[2] += MatrixCamToXYZ[2][Color] * Cam[Color];
  }

  // Reference White
  xyz[0] /= D65Reference[0];
  xyz[1] /= D65Reference[1];
  xyz[2] /= D65Reference[2];

  // Then to Lab
  xyz[0] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[0]) ];
  xyz[1] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[1]) ];
  xyz[2] = ToLABFunctionTable[ (int32_t) MAX(0.0,xyz[2]) ];

  // L in 0 , a in 1, b in 2
  Lab[0] = 116.0 * xyz[1] - 16.0;
  Lab[1] = 500.0*(xyz[0]-xyz[1]);
  Lab[2] = 200.0*(xyz[1]-xyz[2]);
}

//==============================================================================

bool CLASS thumbnail(std::vector<char> &thumbnail) {
  if(!m_InputFile || !m_LoadRawFunction) {
    return false;
  }

  m_Thumb.clear();
  m_Thumb.reserve(1000000);

  fseek (m_InputFile, m_ThumbOffset, SEEK_SET);
  (*this.*m_WriteThumb)();

  bool result = (!m_Thumb.empty());

  if (result) thumbnail.swap(m_Thumb);

  return result;
}

