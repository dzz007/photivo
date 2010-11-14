////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
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

// This file is largely based on work
// Copyright (C) 2010 Emil Martinec for the RawTherapee project.

#include <QMessageBox>
#include "ptImage.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"

#ifdef _OPENMP
  #include <omp.h>
#endif

#define RTSQR(x) ((x)*(x))


#define RTDIRWT_L(i1,j1,i,j) (/*domker[(i1-i)/scale+halfwin][(j1-j)/scale+halfwin] */  rangefn_L[(int32_t)(data_fine->m_Image[i1*width+j1][0]-data_fine->m_Image[i*width+j][0]+0x10000)] )

#define RTDIRWT_AB(i1,j1,i,j) ( /*domker[(i1-i)/scale+halfwin][(j1-j)/scale+halfwin]*/ rangefn_ab[(int32_t)(data_fine->m_Image[i1*width+j1][1]-data_fine->m_Image[i*width+j][1]+0x10000)] * rangefn_ab[(int32_t)(data_fine->m_Image[i1*width+j1][0]-data_fine->m_Image[i*width+j][0]+0x10000)] * rangefn_ab[(int32_t)(data_fine->m_Image[i1*width+j1][2]-data_fine->m_Image[i*width+j][2]+0x10000)] )

#define RTNRWT_L(a) (nrwt_l[a] )

#define RTNRWT_AB (nrwt_ab[(int32_t)((hipass[1]+0x10000))] * nrwt_ab[(int32_t)((hipass[2]+0x10000))])

inline double RTgamma(double x, double gamma, double start, double slope, double mul, double add){
  return (x <= start ? x*slope : exp(log(x)/gamma)*mul-add);
}

void dirpyr(ptImage* data_fine, ptImage* data_coarse, int level, uint16_t * rangefn_L, uint16_t * rangefn_ab, int pitch, int scale, const int luma, const int chroma )
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
  //int intfactor = 16384;

  /*float domker[7][7];
   for (int i=-halfwin; i<=halfwin; i++)
   for (int j=-halfwin; j<=halfwin; j++) {
   domker[i+halfwin][j+halfwin] = (int)(exp(-(i*i+j*j)/(2*sig*sig))*intfactor); //or should we use a value that depends on sigma???
   }*/
  //float domker[5][5] = {{1,1,1,1,1},{1,2,2,2,1},{1,2,4,2,1},{1,2,2,2,1},{1,1,1,1,1}};
  int32_t i=0,j=0,i1=0,j1=0;

#pragma omp parallel for private(i,j,i1, j1)
  for(i = 0; i < height; i+=pitch) {
    i1=i/pitch;
    for(j = 0; j < width; j+=pitch) {
      j1=j/pitch;
      float dirwt_l, dirwt_ab, norm_l, norm_ab;
      //float lops,aops,bops;
      float Lout, aout, bout;
      //norm = DIRWT(i, j, i, j);
      //Lout = -norm*data_fine->L[i][j];//if we don't want to include the input pixel in the sum
      //aout = -norm*data_fine->a[i][j];
      //bout = -norm*data_fine->b[i][j];
      //or
      norm_l = norm_ab = 0;//if we do want to include the input pixel in the sum
      Lout = 0;
      aout = 0;
      bout = 0;
      //normab = 0;

      for(int32_t inbr=MAX(0,i-scalewin); inbr<=MIN(height-1,i+scalewin); inbr+=scale) {
        for (int32_t jnbr=MAX(0,j-scalewin); jnbr<=MIN(width-1,j+scalewin); jnbr+=scale) {
          dirwt_l = RTDIRWT_L(inbr, jnbr, i, j);
          dirwt_ab = RTDIRWT_AB(inbr, jnbr, i, j);
          Lout += dirwt_l*data_fine->m_Image[inbr*width+jnbr][0];
          aout += dirwt_ab*data_fine->m_Image[inbr*width+jnbr][1];
          bout += dirwt_ab*data_fine->m_Image[inbr*width+jnbr][2];
          norm_l += dirwt_l;
          norm_ab += dirwt_ab;
        }
      }
      //lops = Lout/norm;//diagnostic
      //aops = aout/normab;//diagnostic
      //bops = bout/normab;//diagnostic

      //data_coarse->L[i1][j1]=0.5*(data_fine->L[i][j]+Lout/norm_l);//low pass filter
      //data_coarse->a[i1][j1]=0.5*(data_fine->a[i][j]+aout/norm_ab);
      //data_coarse->b[i1][j1]=0.5*(data_fine->b[i][j]+bout/norm_ab);
      //or
      data_coarse->m_Image[i1*outwidth+j1][0]=Lout/norm_l;//low pass filter
      data_coarse->m_Image[i1*outwidth+j1][1]=aout/norm_ab;
      data_coarse->m_Image[i1*outwidth+j1][2]=bout/norm_ab;
    }
  }
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void idirpyr(ptImage* data_coarse, ptImage* data_fine, int level, float * nrwt_l, float * nrwt_ab, int pitch, int scale, const int luma, const int chroma )
{

  uint16_t width = data_fine->m_Width;
  uint16_t height = data_fine->m_Height;

  uint16_t cwidth = data_coarse->m_Width;

  //float eps = 0.0;
  double wtdsum[3], norm;
  float hipass[3], hpffluct[3], tonefactor, nrfactor;

  // c[0] noise_L
  // c[1] noise_ab (relative to noise_L)
  // c[2] decrease of noise var with scale
  // c[3] radius of domain blur at each level
  // c[4] shadow smoothing

  float noisevar_L = 4*RTSQR(25.0 * luma);
  float noisevar_ab = 2*RTSQR(100.0 * chroma);
  float scalefactor = 1.0/pow(2.0,(level+1)*2);//change the last 2 to 1 for longer tail of higher scale NR
  //float recontrast = (1+((float)(c[6])/100.0));
  //float resaturate = 10*(1+((float)(c[7])/100.0));
  noisevar_L *= scalefactor;

  //int halfwin = 3;//MIN(ceil(2*sig),3);
  //int intfactor= 16384;
  //int winwidth=1+2*halfwin;//this belongs in calling function
  /*float domker[7][7];
   for (int i=-halfwin; i<=halfwin; i++)
   for (int j=-halfwin; j<=halfwin; j++) {
   domker[i][j] = (int)(exp(-(i*i+j*j)/(2*sig*sig))*intfactor); //or should we use a value that depends on sigma???
   }*/
  //float domker[5][5] = {{1,1,1,1,1},{1,2,2,2,1},{1,2,4,2,1},{1,2,2,2,1},{1,1,1,1,1}};

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

  int32_t i=0,j=0,i1=0,j1=0;

  if (pitch==1) {
    // step (1-2-3-4)
#pragma omp parallel for schedule(static) private(tonefactor, hipass, hpffluct, nrfactor, i, j)
    for(i = 0; i < height; i++) {
      for(j = 0; j < width; j++) {

        tonefactor = ((RTNRWT_L(data_coarse->m_Image[(i)*width+j][0])));

        //Wiener filter
        //luma
        if (level<2) {
          hipass[0] = data_fine->m_Image[(i)*width+j][0]-data_coarse->m_Image[(i)*width+j][0];
          hpffluct[0]=RTSQR(hipass[0])+0.001;
          hipass[0] *= hpffluct[0]/(hpffluct[0]+noisevar_L);
          data_fine->m_Image[(i)*width+j][0] = CLIP((int32_t)(hipass[0]+data_coarse->m_Image[(i)*width+j][0]));
        }

        //chroma
        hipass[1] = data_fine->m_Image[(i)*width+j][1]-data_coarse->m_Image[(i)*width+j][1];
        hipass[2] = data_fine->m_Image[(i)*width+j][2]-data_coarse->m_Image[(i)*width+j][2];
        hpffluct[1]=RTSQR(hipass[1]*tonefactor)+0.001;
        hpffluct[2]=RTSQR(hipass[2]*tonefactor)+0.001;
        nrfactor = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * RTNRWT_AB);

        hipass[1] *= nrfactor;
        hipass[2] *= nrfactor;

        data_fine->m_Image[(i)*width+j][1] = CLIP((int32_t)(hipass[1]+data_coarse->m_Image[(i)*width+j][1]));
        data_fine->m_Image[(i)*width+j][2] = CLIP((int32_t)(hipass[2]+data_coarse->m_Image[(i)*width+j][2]));
      }
    }
  } else {
    ptImage* smooth = new ptImage();
    smooth->Set(width, height);

#pragma omp parallel
{ // begin parallel
#pragma omp for private(i,j,i1, j1)
    for(i = 0; i < height; i+=pitch) {
      i1=i/pitch;
      for(j = 0; j < width; j+=pitch) {
        //copy common pixels
        j1=j/pitch;
        smooth->m_Image[i*width+j][0] = data_coarse->m_Image[i1*cwidth+j1][0];
        smooth->m_Image[i*width+j][1] = data_coarse->m_Image[i1*cwidth+j1][1];
        smooth->m_Image[i*width+j][2] = data_coarse->m_Image[i1*cwidth+j1][2];
      }
    }
#pragma omp for private(norm, wtdsum, i,j,i1, j1)
    for(i = 0; i < height-1; i+=2) {
      for(j = 0; j < width-1; j+=2) {
        //do midpoint first
        norm=0;
        wtdsum[0]=wtdsum[1]=wtdsum[2]=0.0;
        for(i1=i; i1<MIN(height,i+3); i1+=2)
          for (j1=j; j1<MIN(width,j+3); j1+=2) {
            wtdsum[0] += smooth->m_Image[i1*width+j1][0];
            wtdsum[1] += smooth->m_Image[i1*width+j1][1];
            wtdsum[2] += smooth->m_Image[i1*width+j1][2];
            norm++;
          }
        norm = 1/norm;
        smooth->m_Image[(i+1)*width+j+1][0]=wtdsum[0]*norm;
        smooth->m_Image[(i+1)*width+j+1][1]=wtdsum[1]*norm;
        smooth->m_Image[(i+1)*width+j+1][2]=wtdsum[2]*norm;
      }
    }
#pragma omp for private(norm, wtdsum, i,j,i1, j1)
    for(i = 0; i < height; i+=2) {
      for(j = 0; j < width; j+=2) {
        //now right neighbor
        if (j+1<width) {
          norm=0;
          wtdsum[0]=wtdsum[1]=wtdsum[2]=0.0;
          for (j1=j; j1<MIN(width,j+3); j1+=2) {
            wtdsum[0] += smooth->m_Image[(i)*width+j1][0];
            wtdsum[1] += smooth->m_Image[(i)*width+j1][1];
            wtdsum[2] += smooth->m_Image[(i)*width+j1][2];
            norm++;
          }
          for (i1=i>0?i-1:1; i1<MIN(height,i+2); i1+=2) {
            wtdsum[0] += smooth->m_Image[(i1)*width+j+1][0];
            wtdsum[1] += smooth->m_Image[(i1)*width+j+1][1];
            wtdsum[2] += smooth->m_Image[(i1)*width+j+1][2];
            norm++;
          }
          norm = 1/norm;
          smooth->m_Image[(i)*width+j+1][0]=wtdsum[0]*norm;
          smooth->m_Image[(i)*width+j+1][1]=wtdsum[1]*norm;
          smooth->m_Image[(i)*width+j+1][2]=wtdsum[2]*norm;
        }
        //now down neighbor
        if (i+1<height) {
          norm=0;
          wtdsum[0]=wtdsum[1]=wtdsum[2]=0.0;
          for (i1=i; i1<MIN(height,i+3); i1+=2) {
            wtdsum[0] += smooth->m_Image[(i1)*width+j][0];
            wtdsum[1] += smooth->m_Image[(i1)*width+j][1];
            wtdsum[2] += smooth->m_Image[(i1)*width+j][2];
            norm++;
          }
          for (j1=j>0?j-1:1; j1<MIN(width,j+2); j1+=2) {
            wtdsum[0] += smooth->m_Image[(i+1)*width+j1][0];
            wtdsum[1] += smooth->m_Image[(i+1)*width+j1][1];
            wtdsum[2] += smooth->m_Image[(i+1)*width+j1][2];
            norm++;
          }
          norm=1/norm;
          smooth->m_Image[(i+1)*width+j][0]=wtdsum[0]*norm;
          smooth->m_Image[(i+1)*width+j][1]=wtdsum[1]*norm;
          smooth->m_Image[(i+1)*width+j][2]=wtdsum[2]*norm;
        }
      }
    }
    // step (2-3-4)
#pragma omp parallel for private(tonefactor, hipass, hpffluct, nrfactor, i, j)
    for(i = 0; i < height; i++) {
      for(j = 0; j < width; j++) {

        tonefactor = ((RTNRWT_L(smooth->m_Image[(i)*width+j][0])));

        //Wiener filter
        //luma
        if (level<2) {
          hipass[0] = data_fine->m_Image[(i)*width+j][0]-smooth->m_Image[(i)*width+j][0];
          hpffluct[0]=RTSQR(hipass[0])+0.001;
          hipass[0] *= hpffluct[0]/(hpffluct[0]+noisevar_L);
          data_fine->m_Image[(i)*width+j][0] = CLIP((int32_t)(hipass[0]+smooth->m_Image[(i)*width+j][0]));
        }

        //chroma
        hipass[1] = data_fine->m_Image[(i)*width+j][1]-smooth->m_Image[(i)*width+j][1];
        hipass[2] = data_fine->m_Image[(i)*width+j][2]-smooth->m_Image[(i)*width+j][2];
        hpffluct[1]=RTSQR(hipass[1]*tonefactor)+0.001;
        hpffluct[2]=RTSQR(hipass[2]*tonefactor)+0.001;
        nrfactor = (hpffluct[1]+hpffluct[2]) /((hpffluct[1]+hpffluct[2]) + noisevar_ab * RTNRWT_AB);

        hipass[1] *= nrfactor;
        hipass[2] *= nrfactor;

        data_fine->m_Image[(i)*width+j][1] = CLIP((int32_t)(hipass[1]+smooth->m_Image[(i)*width+j][1]));
        data_fine->m_Image[(i)*width+j][2] = CLIP((int32_t)(hipass[2]+smooth->m_Image[(i)*width+j][2]));
      }
    }
} // end of parallel
    delete smooth;
  }
};



ptImage* ptImage::dirpyrLab_denoise(const int luma, const int chroma, const double gamma, const int levels, const int pipesize)
{
  assert ((m_ColorSpace == ptSpace_Lab));

  //~ const int maxlevel = levels-pipesize;
  //~ if (maxlevel <= 0) return this;

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
  const int scales[8] = {1,1,2,4,8,16,32,64};
  //sequence of pitches
  const int pitches[8] = {2,1,1,1,1,1,1,1};

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
  for (int32_t i=0; i<0x10000; i++) {
    gamcurve[i] = CLIP((int32_t)(RTgamma((double)i/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0) * 65535.0));
    //if (i<500)  printf("%d %d \n",i,g);
  }

if(1||luma>0) {
#pragma omp parallel for
  for (uint16_t i=0; i<m_Height; i++) {
    for (uint16_t j=0; j<m_Width; j++) {
      m_Image[i*m_Width+j][0] = gamcurve[m_Image[i*m_Width+j][0]];
    }
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
    nrwt_l[i] = ((RTgamma((double)i/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0) -
                  RTgamma((double)(i+10000)/65535.0, gamma, gamthresh, gamslope, 1.0, 0.0)) )/nrwtl_norm;
    //if (i % 100 ==0) printf("%d %f \n",i,nrwt_l[i]);
  }

  float tonefactor = nrwt_l[0x7fff];

  float noise_L = 25.0*luma;
  float noisevar_L = 4*SQR(noise_L);

  float noise_ab = 25*chroma;
  float noisevar_ab = SQR(noise_ab);


  //set up range functions
#pragma omp parallel for
  for (int32_t i=0; i<0x20000; i++) {
    rangefn_L[i] = (uint16_t)(( exp(-(double)fabs(i-0x10000) * tonefactor / (1+3*noise_L)) * noisevar_L/((double)(i-0x10000)*(double)(i-0x10000) + noisevar_L))*intfactor);
    rangefn_ab[i] = (uint16_t)(( exp(-(double)fabs(i-0x10000) * tonefactor / (1+3*noise_ab)) * noisevar_ab/((double)(i-0x10000)*(double)(i-0x10000) + noisevar_ab))*intfactor);
    nrwt_ab[i] = ((1+abs(i-0x10000)/(1+8*noise_ab)) * exp(-(double)fabs(i-0x10000)/ (1+8*noise_ab) ) );
  }

  //for (int i=0; i<65536; i+=100)  printf("%d %d \n",i,gamcurve[i]);


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

  dirpyr(this, dirpyrLablo[0], 0, rangefn_L, rangefn_ab, pitch, scale, luma, chroma );

  level = 1;

  while(level < maxlevel)
  {
    scale = scales[level];
    pitch = pitches[level];

    dirpyr(dirpyrLablo[level-1], dirpyrLablo[level], level, rangefn_L, rangefn_ab, pitch, scale, luma, chroma );

    level ++;
  }


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//~ int temp = m_ColorSpace;
//~ Set(dirpyrLablo[2]);
//~ m_ColorSpace=temp;

  level = maxlevel - 1;
  while(level > 0)
  {

    int scale = scales[level];
    int pitch = pitches[level];
    idirpyr(dirpyrLablo[level], dirpyrLablo[level-1], level, nrwt_l, nrwt_ab, pitch, scale, luma, chroma );

    level--;
  }


  scale = scales[0];
  pitch = pitches[0];
  idirpyr(dirpyrLablo[0], this, 0, nrwt_l, nrwt_ab, pitch, scale, luma, chroma );


  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  float igam = 1/gamma;
  float igamthresh = gamthresh*gamslope;
  float igamslope = 1/gamslope;
  for (uint32_t i=0; i<0x10000; i++) {
    gamcurve[i] = CLIP((int32_t)(RTgamma((float)i/65535.0, igam, igamthresh, igamslope, 1.0, 0.0) * 65535.0));
  }

if(1||luma>0) {
  for (uint16_t i=0; i<m_Height; i++)
    for (uint16_t j=0; j<m_Width; j++) {
      m_Image[i*m_Width+j][0] = gamcurve[CLIP(m_Image[i*m_Width+j][0])];
    }

  return this;
}
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


  for(int i = 0; i < maxlevel; i++)
  {
    delete dirpyrLablo[i];
  }

  delete rangefn_L;
  delete rangefn_ab;
  delete nrwt_l;
  delete nrwt_ab;

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
};

#undef RTSQR

#undef RTDIRWT_L
#undef RTDIRWT_AB

#undef RTNRWT_L
#undef RTNRWT_AB



