/*******************************************************************************
**
** Photivo
**
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

// This file is largely based on work
// Copyright (C) 2010 Emil Martinec for the RawTherapee project.

#include "ptImage.h"
#include "ptConstants.h"
#include "ptDefines.h"
#include "ptError.h"
#include "ptSettings.h"

#include <cmath>
#include <cassert>

#ifdef _OPENMP
  #include <omp.h>
#endif

#ifdef WIN32
  #define cimg_display_type 0
#else
  #define cimg_display 0
#endif

#ifdef SYSTEM_CIMG
  #include <CImg.h>
#else
  #include "greyc/CImg.h"
#endif


using namespace cimg_library;

#define RTCLIPTO(a,b,c) ((a)>(b)?((a)<(c)?(a):(c)):(b))
#define RTCLIPC(a) ((a)>-32000?((a)<32000?(a):32000):-32000)
#define RTCLIP(a) (RTCLIPTO(a,0,65535))

#define DIRWT_L(t,i,j) (rangefn_L[RTCLIPTO((int32_t)(data_fine->m_Image[t][0]-data_fine->m_Image[i*width+j][0]+0x10000),0,0x1ffff)] )

#define DIRWT_AB(t,i,j) (rangefn_ab[RTCLIPTO((int32_t)(data_fine->m_Image[t][1]-data_fine->m_Image[i*width+j][1]+0x10000),0,0x1ffff)] * \
rangefn_ab[RTCLIPTO((int32_t)(data_fine->m_Image[t][0]-data_fine->m_Image[i*width+j][0]+0x10000),0,0x1ffff)] * \
rangefn_ab[RTCLIPTO((int32_t)(data_fine->m_Image[t][2]-data_fine->m_Image[i*width+j][2]+0x10000),0,0x1ffff)] )

#define NRWT_L(a) (nrwt_l[a] )

#define NRWT_AB (nrwt_ab[RTCLIPTO((int32_t)((hipass[1]+0x10000)),0,0x1ffff)] * nrwt_ab[RTCLIPTO((int32_t)((hipass[2]+0x10000)),0,0x1ffff)])


#define PIX_SORT(a,b) { if ((a)>(b)) {temp=(a);(a)=(b);(b)=temp;} }

#define med3x3(a0,a1,a2,a3,a4,a5,a6,a7,a8,median) { \
p[0]=a0; p[1]=a1; p[2]=a2; p[3]=a3; p[4]=a4; p[5]=a5; p[6]=a6; p[7]=a7; p[8]=a8; \
PIX_SORT(p[1],p[2]); PIX_SORT(p[4],p[5]); PIX_SORT(p[7],p[8]); \
PIX_SORT(p[0],p[1]); PIX_SORT(p[3],p[4]); PIX_SORT(p[6],p[7]); \
PIX_SORT(p[1],p[2]); PIX_SORT(p[4],p[5]); PIX_SORT(p[7],p[8]); \
PIX_SORT(p[0],p[3]); PIX_SORT(p[5],p[8]); PIX_SORT(p[4],p[7]); \
PIX_SORT(p[3],p[6]); PIX_SORT(p[1],p[4]); PIX_SORT(p[2],p[5]); \
PIX_SORT(p[4],p[7]); PIX_SORT(p[4],p[2]); PIX_SORT(p[6],p[4]); \
PIX_SORT(p[4],p[2]); median=p[4];} //a4 is the median

inline float RTgamma(float x, float gamma, float start, float slope, float mul, float add){
  return (x <= start ? x*slope : exp(log(x)/gamma)*mul-add);
}

void dirpyr(ptImage* data_fine, ptImage* data_coarse, uint16_t * rangefn_L, uint16_t * rangefn_ab, int pitch, int scale)
{

  //pitch is spacing of subsampling
  //scale is spacing of directional averaging weights
  //example 1: no subsampling at any level -- pitch=1, scale=2^n
  //example 2: subsampling by 2 every level -- pitch=2, scale=1 at each level
  //example 3: no subsampling at first level, subsampling by 2 thereafter --
  //  pitch =1, scale=1 at first level; pitch=2, scale=2 thereafter


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // calculate weights, compute directionally weighted average

  uint16_t width = data_fine->m_Width;
  uint16_t height = data_fine->m_Height;

  uint16_t outwidth = data_coarse->m_Width;

  //generate domain kernel
  int halfwin = 3;//MIN(ceil(2*sig),3);
  int scalewin = halfwin*scale;

  int32_t i=0,j=0,i1=0,j1=0,Temp=0;

#pragma omp parallel for private(i,j,i1, j1,Temp)
  for(i = 0; i < height; i+=pitch) {
    i1=i/pitch;
    for(j = 0; j < width; j+=pitch) {
      j1=j/pitch;
      float dirwt_l, dirwt_ab, norm_l, norm_ab;
      float Lout, aout, bout;
      norm_l = norm_ab = 0;//if we do want to include the input pixel in the sum
      Lout = 0;
      aout = 0;
      bout = 0;

      for(int32_t inbr=MAX(0,i-scalewin); inbr<=MIN(height-1,i+scalewin); inbr+=scale) {
        for (int32_t jnbr=MAX(0,j-scalewin); jnbr<=MIN(width-1,j+scalewin); jnbr+=scale) {
          Temp = inbr*width+jnbr;
          dirwt_l = DIRWT_L(Temp, i, j);
          dirwt_ab = DIRWT_AB(Temp, i, j);
          Lout += dirwt_l*data_fine->m_Image[Temp][0];
          aout += dirwt_ab*data_fine->m_Image[Temp][1];
          bout += dirwt_ab*data_fine->m_Image[Temp][2];
          norm_l += dirwt_l;
          norm_ab += dirwt_ab;
        }
      }

      Temp = i1*outwidth+j1;
      data_coarse->m_Image[Temp][0]=Lout/norm_l;//low pass filter
      data_coarse->m_Image[Temp][1]=aout/norm_ab;
      data_coarse->m_Image[Temp][2]=bout/norm_ab;
    }
  }
};

void idirpyr(ptImage* data_coarse, ptImage* data_fine, int level, float * nrwt_l, float * nrwt_ab, int pitch, const int luma, const int chroma )
{

  uint16_t width = data_fine->m_Width;
  uint16_t height = data_fine->m_Height;

  uint16_t cwidth = data_coarse->m_Width;

  //float eps = 0.0;

  // c[0] noise_L
  // c[1] noise_ab (relative to noise_L)
  // c[2] decrease of noise var with scale
  // c[3] radius of domain blur at each level
  // c[4] shadow smoothing

  float radius = 1.5;
  float temp, median;

  float noisevar_L = 4*SQR(25.0 * luma);
  float noisevar_ab = 2*SQR(100.0 * chroma);
  float scalefactor = 1.0/pow(2.0,(level+1)*2);//change the last 2 to 1 for longer tail of higher scale NR
  noisevar_L *= scalefactor;

  //temporary array to store NR factors
  /*float** nrfactorL = new float*[height];
  float** nrfactorab = new float*[height];
  for (int i=0; i<height; i++) {
    nrfactorL[i] = new float[width];
    nrfactorab[i] = new float[width];
  }*/

  CImg <float> nrfactorL(height,width,1,1,0);
  CImg <float> nrfactorab(height,width,1,1,0);

  // for coarsest level, take non-subsampled lopass image and subtract from lopass_fine to generate hipass image

  // denoise hipass image, add back into lopass_fine to generate denoised image at fine scale

  // now iterate:
  // (1) take denoised image at level n, expand and smooth using gradient weights from lopass image at level n-1
  //     the result is the smoothed image at level n-1
  // (2) subtract smoothed image at level n-1 from lopass image at level n-1 to make hipass image at level n-1
  // (3) denoise the hipass image at level n-1
  // (4) add the denoised image at level n-1 to the smoothed image at level n-1 to make the denoised image at level n-1

  // note that the coarsest level amounts to skipping step (1) and doing (2,3,4).
  // in other words, skip step one if pitch=1

  // step (1)

  if (pitch==1) {// step (1) not needed

    // step (2-3-4)
#pragma omp parallel for schedule(static)
    for(uint16_t  i = 0; i < height; i++) {
      uint32_t Temp = i*width;
      for(uint16_t  j = 0; j < width; j++) {

        float hipass[3], hpffluct[3], tonefactor;//, nrfactor;

        tonefactor = ((NRWT_L(data_coarse->m_Image[Temp][0])));

        //Wiener filter
        //luma
        if (level<2) {
          hipass[0] = data_fine->m_Image[Temp][0]-data_coarse->m_Image[Temp][0];
          hpffluct[0]=SQR(hipass[0])+0.001;
          nrfactorL(i,j) = hpffluct[0]/(hpffluct[0]+noisevar_L);
          //hipass[0] *= hpffluct[0]/(hpffluct[0]+noisevar_L);
          //data_fine->L[i][j] = RTCLIP(hipass[0]+data_coarse->L[i][j]);
        }

        //chroma
        hipass[1] = data_fine->m_Image[Temp][1]-data_coarse->m_Image[Temp][1];
        hipass[2] = data_fine->m_Image[Temp][2]-data_coarse->m_Image[Temp][2];
        hpffluct[1]=SQR(hipass[1]*tonefactor)+0.001;
        hpffluct[2]=SQR(hipass[2]*tonefactor)+0.001;
        nrfactorab(i,j) = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * NRWT_AB);
        /*nrfactor = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * NRWT_AB);

        hipass[1] *= nrfactor;
        hipass[2] *= nrfactor;

        data_fine->a[i][j] = hipass[1]+data_coarse->a[i][j];
        data_fine->b[i][j] = hipass[2]+data_coarse->b[i][j];*/
        ++Temp;
      }
    }

    //nrfactorL.blur(radius);
    nrfactorab.blur(radius);

#pragma omp parallel for schedule(static)
    for(uint16_t  i = 0; i < height; i++) {
      uint32_t Temp = i*width;
      for(uint16_t  j = 0; j < width; j++) {
        float hipass[3],p[9];

        //luma
        if (level<2) {
          if (i>0 && i<height-1 && j>0 && j<width-1) {
            med3x3(nrfactorL(i-1,j-1), nrfactorL(i-1,j), nrfactorL(i-1,j+1), \
                 nrfactorL(i,j-1), nrfactorL(i,j), nrfactorL(i,j+1), \
                 nrfactorL(i+1,j-1), nrfactorL(i+1,j), nrfactorL(i+1,j+1), median);
          } else {
            median = nrfactorL(i,j);
          }

          hipass[0] = median*(data_fine->m_Image[Temp][0]-data_coarse->m_Image[Temp][0]);
          //hipass[0] = nrfactorL[i][j]*(data_fine->L[i][j]-data_coarse->L[i][j]);
          data_fine->m_Image[Temp][0] = CLIP((int32_t)(hipass[0]+data_coarse->m_Image[Temp][0]));
        }

        //chroma
        hipass[1] = nrfactorab(i,j)*(data_fine->m_Image[Temp][1]-data_coarse->m_Image[Temp][1]);
        hipass[2] = nrfactorab(i,j)*(data_fine->m_Image[Temp][2]-data_coarse->m_Image[Temp][2]);

        data_fine->m_Image[Temp][1] = hipass[1]+data_coarse->m_Image[Temp][1];
        data_fine->m_Image[Temp][2] = hipass[2]+data_coarse->m_Image[Temp][2];
        ++Temp;
      }
    }

  } else {//pitch >1; need to fill in data by upsampling

    ptImage* smooth = new ptImage();
    smooth->Set(width, height);

#pragma omp parallel
{ // begin parallel
#pragma omp for schedule(static)
    for(uint16_t i = 0; i < height; i+=pitch) {
      uint16_t ix = i/pitch;
      uint32_t Temp1 = i * width;
      uint32_t Temp2 = ix * cwidth;
      for(uint16_t j = 0, jx = 0; j < width; j+=pitch, jx++) {
        //copy common pixels
        smooth->m_Image[Temp1][0] = data_coarse->m_Image[Temp2][0];
        smooth->m_Image[Temp1][1] = data_coarse->m_Image[Temp2][1];
        smooth->m_Image[Temp1][2] = data_coarse->m_Image[Temp2][2];
        Temp1 += pitch;
        Temp2++;
      }
    }
    //if (pitch>1) {//pitch=2; step (1) expand coarse image, fill in missing data

#pragma omp for schedule(static)
    for(uint16_t  i = 0; i < height-1; i+=2) {
      uint32_t Temp1 = (i+1)*width+1;
      for(uint16_t j = 0; j < width-1; j+=2) {
        //do midpoint first
        double norm=0.0,wtdsum[3]={0.0,0.0,0.0};
        //wtdsum[0]=wtdsum[1]=wtdsum[2]=0.0;
        for(uint16_t ix=i; ix<MIN((int)height,i+3); ix+=2) {
          for (uint16_t jx=j; jx<MIN((int)width,j+3); jx+=2) {
            uint32_t Temp2 = ix*width+jx;
            wtdsum[0] += smooth->m_Image[Temp2][0];
            wtdsum[1] += smooth->m_Image[Temp2][1];
            wtdsum[2] += smooth->m_Image[Temp2][2];
            norm++;
          }
        }
        norm = 1/norm;
        smooth->m_Image[Temp1][0]=wtdsum[0]*norm;
        smooth->m_Image[Temp1][1]=wtdsum[1]*norm;
        smooth->m_Image[Temp1][2]=wtdsum[2]*norm;
        Temp1 += 2;
      }
    }

#pragma omp for schedule(static)
    for(uint16_t i = 0; i < height; i+=2) {
      for(uint16_t j = 0; j < width; j+=2) {
        uint32_t Temp = 0;
        double norm=0.0,wtdsum[3]={0.0,0.0,0.0};
        //now right neighbor
        if (j+1<width) {
          for (int jx=j; jx<MIN((int)width,j+3); jx+=2) {
            Temp = i*width+jx;
            wtdsum[0] += smooth->m_Image[Temp][0];
            wtdsum[1] += smooth->m_Image[Temp][1];
            wtdsum[2] += smooth->m_Image[Temp][2];
            norm++;
          }
          for (int ix=i>0?i-1:1; ix<MIN((int)height,i+2); ix+=2) {
            Temp = ix*width+j+1;
            wtdsum[0] += smooth->m_Image[Temp][0];
            wtdsum[1] += smooth->m_Image[Temp][1];
            wtdsum[2] += smooth->m_Image[Temp][2];
            norm++;
          }
          norm = 1/norm;
          Temp = i*width+j+1;
          smooth->m_Image[Temp][0]=wtdsum[0]*norm;
          smooth->m_Image[Temp][1]=wtdsum[1]*norm;
          smooth->m_Image[Temp][2]=wtdsum[2]*norm;
        }

        //now down neighbor
        if (i+1<height) {
          norm=0.0;wtdsum[0]=wtdsum[1]=wtdsum[2]=0.0;
          for (int ix=i; ix<MIN((int)height,i+3); ix+=2) {
            Temp = ix*width+j;
            wtdsum[0] += smooth->m_Image[Temp][0];
            wtdsum[1] += smooth->m_Image[Temp][1];
            wtdsum[2] += smooth->m_Image[Temp][2];
            norm++;
          }
          for (int jx=j>0?j-1:1; jx<MIN((int)width,j+2); jx+=2) {
            Temp = (i+1)*width+jx;
            wtdsum[0] += smooth->m_Image[Temp][0];
            wtdsum[1] += smooth->m_Image[Temp][1];
            wtdsum[2] += smooth->m_Image[Temp][2];
            norm++;
          }
          norm=1/norm;
          Temp = (i+1)*width+j;
          smooth->m_Image[Temp][0]=wtdsum[0]*norm;
          smooth->m_Image[Temp][1]=wtdsum[1]*norm;
          smooth->m_Image[Temp][2]=wtdsum[2]*norm;
        }
      }
    }


// step (2-3-4)
#pragma omp for schedule(static)
    for(uint16_t  i = 0; i < height; i++) {
      uint32_t Temp = i*width;
      for(uint16_t  j = 0; j < width; j++) {

        float hipass[3], hpffluct[3], tonefactor;//, nrfactor;

        tonefactor = (NRWT_L(smooth->m_Image[Temp][0]));

        //Wiener filter
        //luma
        if (level<2) {
          hipass[0] = data_fine->m_Image[Temp][0]-smooth->m_Image[Temp][0];
          hpffluct[0]=SQR(hipass[0])+0.001;
          nrfactorL(i,j) = hpffluct[0]/(hpffluct[0]+noisevar_L);
          //hipass[0] *= hpffluct[0]/(hpffluct[0]+noisevar_L);
          //data_fine->L[i][j] = RTCLIP(hipass[0]+data_coarse->L[i][j]);
        }

        //chroma
        hipass[1] = data_fine->m_Image[Temp][1]-smooth->m_Image[Temp][1];
        hipass[2] = data_fine->m_Image[Temp][2]-smooth->m_Image[Temp][2];
        hpffluct[1]=SQR(hipass[1]*tonefactor)+0.001;
        hpffluct[2]=SQR(hipass[2]*tonefactor)+0.001;
        nrfactorab(i,j) = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * NRWT_AB);
        /*nrfactor = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * NRWT_AB);

        hipass[1] *= nrfactor;
        hipass[2] *= nrfactor;

        data_fine->a[i][j] = hipass[1]+data_coarse->a[i][j];
        data_fine->b[i][j] = hipass[2]+data_coarse->b[i][j];*/
        ++Temp;
      }
    }

    //nrfactorL.blur(radius);
#pragma omp master
    nrfactorab.blur(radius);

#pragma omp for schedule(static)
    for(uint16_t  i = 0; i < height; i++) {
      uint32_t Temp = i*width;
      for(uint16_t  j = 0; j < width; j++) {
        float hipass[3],p[9];

        //luma
        if (level<2) {
          if (i>0 && i<height-1 && j>0 && j<width-1) {
            med3x3(nrfactorL(i-1,j-1), nrfactorL(i-1,j), nrfactorL(i-1,j+1), \
                 nrfactorL(i,j-1), nrfactorL(i,j), nrfactorL(i,j+1), \
                 nrfactorL(i+1,j-1), nrfactorL(i+1,j), nrfactorL(i+1,j+1), median);
          } else {
            median = nrfactorL(i,j);
          }

          hipass[0] = median*(data_fine->m_Image[Temp][0]-smooth->m_Image[Temp][0]);
          //hipass[0] = nrfactorL[i][j]*(data_fine->L[i][j]-data_coarse->L[i][j]);
          data_fine->m_Image[Temp][0] = CLIP((int32_t)(hipass[0]+smooth->m_Image[Temp][0]));
        }

        //chroma
        hipass[1] = nrfactorab(i,j)*(data_fine->m_Image[Temp][1]-smooth->m_Image[Temp][1]);
        hipass[2] = nrfactorab(i,j)*(data_fine->m_Image[Temp][2]-smooth->m_Image[Temp][2]);

        data_fine->m_Image[Temp][1] = hipass[1]+smooth->m_Image[Temp][1];
        data_fine->m_Image[Temp][2] = hipass[2]+smooth->m_Image[Temp][2];
        ++Temp;
      }
    }
} // end parallel
    delete smooth;
  }//end of pitch>1
};

ptImage* ptImage::dirpyrLab_denoise(const int luma, const int chroma, const double gamma, const int levels)
{
  assert ((m_ColorSpace == ptSpace_Lab));

  const int maxlevel = levels;
  //sequence of scales
  //static const int scales[8] = {1,2,4,8,16,32,64,128};
  //sequence of pitches
  //static const int pitches[8] = {1,1,1,1,1,1,1,1};

  //sequence of scales
  //static const int scales[8] = {1,1,1,1,1,1,1,1};
  //sequence of pitches
  //static const int pitches[8] = {2,2,2,2,2,2,2,2};

  //sequence of scales
  //static const int scales[8] = {1,1,2,2,4,4,8,8};
  //sequence of pitches
  //static const int pitches[8] = {2,1,2,1,2,1,2,1};

  //sequence of scales
  static const int scales[8] = {1,1,2,4,8,16,32,64};
  //sequence of pitches
  static const int pitches[8] = {2,1,1,1,1,1,1,1};

  //pitch is spacing of subsampling
  //scale is spacing of directional averaging weights
  //example 1: no subsampling at any level -- pitch=1, scale=2^n
  //example 2: subsampling by 2 every level -- pitch=2, scale=1 at each level
  //example 3: no subsampling at first level, subsampling by 2 thereafter --
  //  pitch =1, scale=1 at first level; pitch=2, scale=2 thereafter

  //float gam = 2.0;//MIN(3.0, 0.1*fabs(c[4])/3.0+0.001);
  float gamthresh = 0.03;
  float gamslope = exp(log((double)gamthresh)/gamma)/gamthresh;
  uint16_t gamcurve[0x10000];
#pragma omp parallel for schedule(static)
  for (int32_t i=0; i<0x10000; i++) {
    gamcurve[i] = CLIP((int32_t)(RTgamma((double)i/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0) * 65535.0));
  }

#pragma omp parallel for schedule(static)
  for (uint16_t i=0; i<m_Height; i++) {
    for (uint16_t j=0; j<m_Width; j++) {
      m_Image[i*m_Width+j][0] = gamcurve[m_Image[i*m_Width+j][0]];
    }
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



  uint16_t * rangefn_L = new uint16_t [0x20000];
  float * nrwt_l = new float [0x10000];


  uint16_t * rangefn_ab = new uint16_t [0x20000];
  float * nrwt_ab = new float [0x20000];

  int intfactor = 1024;//16384;


  //set up NR weight functions

  //gamma correction for chroma in shadows
  float nrwtl_norm = ((RTgamma((double)65535.0/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0)) -
                     (RTgamma((double)75535.0/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0)));
#pragma omp parallel for
  for (int32_t i=0; i<0x10000; i++) {
    nrwt_l[i] = ((RTgamma((float)i/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0) -
                  RTgamma((float)(i+10000)/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0)) )/nrwtl_norm;
  }

  float tonefactor = nrwt_l[0x8000];

  float noise_L = 25.0*luma;
  float noisevar_L = 4*SQR(noise_L);

  float noise_ab = 25*chroma;
  float noisevar_ab = SQR(noise_ab);

  //set up range functions
#pragma omp parallel for
  for (int32_t i=0; i<0x20000; i++) {
    rangefn_L[i] = (uint16_t)(( exp(-(float)fabs(i-0x10000) * tonefactor / (1+3*noise_L)) * noisevar_L/((float)(i-0x10000)*(float)(i-0x10000) + noisevar_L))*intfactor);
    rangefn_ab[i] = (uint16_t)(( exp(-(float)fabs(i-0x10000) * tonefactor / (1+3*noise_ab)) * noisevar_ab/((float)(i-0x10000)*(float)(i-0x10000) + noisevar_ab))*intfactor);
    nrwt_ab[i] = ((1+abs(i-0x10000)/(1+8*noise_ab)) * exp(-(float)fabs(i-0x10000)/ (1+8*noise_ab) ) );
  }


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  int level;

  ptImage* dirpyrLablo[maxlevel];
  uint16_t w = (uint16_t)((m_Width-1)/pitches[0])+1;
  uint16_t h = (uint16_t)((m_Height-1)/pitches[0])+1;
  dirpyrLablo[0] = new ptImage();
  dirpyrLablo[0]->Set(w,h);
  for (level=1; level<maxlevel; level++) {
    w = (uint16_t)((w-1)/pitches[level])+1;
    h = (uint16_t)((h-1)/pitches[level])+1;
    dirpyrLablo[level] = new ptImage();
    dirpyrLablo[level]->Set(w,h);
  };

  //////////////////////////////////////////////////////////////////////////////


  // c[0] = luma = noise_L
  // c[1] = chroma = noise_ab
  // c[2] decrease of noise var with scale
  // c[3] radius of domain blur at each level
  // c[4] shadow smoothing
  // c[5] edge preservation

  level = 0;

  int scale = scales[level];
  int pitch = pitches[level];
  //int thresh = 10 * c[8];
  //impulse_nr (src, src, m_w1, m_h1, thresh, noisevar);

  dirpyr(this, dirpyrLablo[0], rangefn_L, rangefn_ab, pitch, scale);

  level = 1;

  while(level < maxlevel)
  {
    scale = scales[level];
    pitch = pitches[level];

    dirpyr(dirpyrLablo[level-1], dirpyrLablo[level], rangefn_L, rangefn_ab, pitch, scale );

    level ++;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  level = maxlevel - 1;
  while(level > 0) {
    // int scale = scales[level];
    int pitch = pitches[level];
    idirpyr(dirpyrLablo[level], dirpyrLablo[level-1], level, nrwt_l, nrwt_ab, pitch, luma, chroma );

    level--;
  }

  scale = scales[0];
  pitch = pitches[0];
  idirpyr(dirpyrLablo[0], this, 0, nrwt_l, nrwt_ab, pitch, luma, chroma );


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  float igam = 1/gamma;
  float igamthresh = gamthresh*gamslope;
  float igamslope = 1/gamslope;
#pragma omp parallel for schedule(static)
  for (uint32_t i=0; i<0x10000; i++) {
    gamcurve[i] = CLIP((int32_t)(RTgamma((float)i/65535.0, igam, igamthresh, igamslope, 1.0, 0.0) * 65535.0));
  }

#pragma omp parallel for schedule(static)
  for (int32_t i=0; i<(int32_t) m_Height*m_Width; i++) {
    m_Image[i][0] = gamcurve[m_Image[i][0]];
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  for(int i = 0; i < maxlevel; i++)
  {
    delete dirpyrLablo[i];
  }

  delete [] rangefn_L;
  delete [] rangefn_ab;
  delete [] nrwt_l;
  delete [] nrwt_ab;

  return this;

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
};

#undef med3x3
#undef PIX_SORT

#undef DIRWT_L
#undef DIRWT_AB

#undef NRWT_L
#undef NRWT_AB
