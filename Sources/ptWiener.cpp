/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
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

#include <QString>
#include <QObject>
#include <QMessageBox>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include "ptWiener.h"
#include "ptImage.h"
#include "ptError.h"
#include "ptCimg.h"

#include <fftw3.h>
#include <cmath>
#include <cassert>

// First idea how to realize this filter I got from Filip Rooms
// http://filiprooms.be/research/software/

void ptWienerFilterChannel(ptImage* Image,
                           const double Sigma,
                           const double Box,
                           const double LensBlur,
                           const double Amount,
                           const short UseEdgeMask)
{
  assert (Image->m_ColorSpace == ptSpace_Lab);

  const int tilesize = 512;
  const int overlapfactor = 10;

  const float K = pow(10, -1.-5.*Amount);  //Amount between 0..1

  const uint16_t w = Image->m_Width;
  const uint16_t h = Image->m_Height;

  const float eps = 0.001;

  uint16_t i, j;
  float *pimage, *outimage;
  fftw_complex *kernel, *fkernel;
  fftw_plan plan_forward, plan_backward;
  float *Mask;

// calculation for tiling
  const int overlap = MAX(MAX(ceil(Sigma), ceil(Box)),ceil(LensBlur))*overlapfactor;
  const int nrtilesw = ceil((float)w/(float)(tilesize-2*overlap));
  const int nrtilesh = ceil((float)h/(float)(tilesize-2*overlap));
  const int nrtiles = nrtilesw*nrtilesh;
// end of tiling

#pragma omp parallel default(shared)
    {

// prepare image
  {
    // it is a bit to big, but this won't hurt
#pragma omp single
    pimage = (float *) calloc (nrtiles*tilesize*tilesize,sizeof (float));
#pragma omp barrier
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < h; j++)
      for (i = 0; i < w; i++)
        pimage[i+overlap + (j+overlap) * tilesize * nrtilesw] = Image->m_Image[i + j * w][0];
    //top
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < overlap; j++)
      for (i = 0; i < w; i++)
        pimage[i+overlap + j * tilesize * nrtilesw] = Image->m_Image[i + (overlap-j-1) * w][0];
    //bottom
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < overlap; j++)
      for (i = 0; i < w; i++)
        pimage[i+overlap + (j+h+overlap) * tilesize * nrtilesw] = Image->m_Image[i + (h-j-1) * w][0];
    //left
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < h+2*overlap; j++)
      for (i = 0; i < overlap; i++)
        pimage[i + j * tilesize * nrtilesw] = pimage[-i - 1 +2*overlap + j * tilesize * nrtilesw];
    //right
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < h+2*overlap; j++)
      for (i = 0; i < overlap; i++)
        pimage[i+overlap+w + j * tilesize * nrtilesw] = pimage[-i-1+w+overlap + j * tilesize * nrtilesw];
  }

// pimage contains now the whole image with reflected borders and is filled
// with black in the remaining part. The tiles should be filled from this!

  fftw_complex *im, *fim;
  fftw_complex *conv, *out;
  float *finishedtile;

  im    = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
  fim   = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
  conv = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
  out = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
  finishedtile = (float*) malloc (sizeof (float) * tilesize * tilesize);

  const int kw = tilesize;
  const int kh = tilesize;

// this will contain the result
#pragma omp single nowait
  outimage = (float *) calloc (nrtiles*tilesize*tilesize,sizeof (float));
#pragma omp single
  plan_forward  = fftw_plan_dft_2d( tilesize, tilesize, im, fim, FFTW_FORWARD, FFTW_ESTIMATE );
#pragma omp single nowait
  plan_backward = fftw_plan_dft_2d( tilesize, tilesize, conv, out, FFTW_BACKWARD, FFTW_ESTIMATE );

// generate kernel
#pragma omp single
  {
    float sum = 0.0;
    float r = 0.0;
    float sigmasquared = Sigma*Sigma;
    float boxsquared = Box*Box;
    float lenssquared = LensBlur*LensBlur;
    // float norm = (1.0 / (sqrt (2.0 * M_PI) * Sigma));

    kernel   = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
    fkernel  = (fftw_complex *) fftw_malloc (tilesize*tilesize * sizeof (fftw_complex));
    for (j = 0; j < kh; j++) {
      for (i = 0; i < kw; i++) {
        r = (pow (i - kw / 2, 2) + pow (j - kh / 2, 2));
        if (Sigma)
          kernel[i + j * kw][0] = exp (-0.5 * r / sigmasquared);
        else
          kernel[i + j * kw][0] = 0;
        if (Box && r <= boxsquared)
          kernel[i + j * kw][0] += 1;
        if (LensBlur && r <= lenssquared)
          kernel[i + j * kw][0] += r*r/lenssquared/lenssquared*0.5+0.5;

        sum += kernel[i + j * kw][0];
        kernel[i + j * kw][1] = 0;
      }
    }
    // Normalise to 1
    for (j = 0; j < kh; j++) {
      for (i = 0; i < kw; i++) {
        kernel[i + j * kw][0] = kernel[i + j * kw][0] / sum;
      }
    }

    fftw_execute_dft(plan_forward,kernel,fkernel);
    fftw_free (kernel);
  }
#pragma omp barrier
// loop should start here!
  int tile = 0;
#pragma omp for private(i,j,tile) schedule(static)
  for (tile = 0; tile < nrtiles; tile++) {
    int tileh = floor(((float)tile+eps)/(float)nrtilesw);   //height
    int tilew = tile - tileh*nrtilesw;                  //width
    int shifth = tileh*(tilesize-2*overlap);
    int shiftw = tilew*(tilesize-2*overlap);

  // read image from pimage and transform forward
    {
      for (j = 0; j < kh; j++) {
        for (i = 0; i < kw; i++) {
          im[i + j * kw][0] = pimage[i+shiftw + (j+shifth) * nrtilesw*tilesize];
          im[i + j * kw][1] = 0;
        }
      }

      fftw_execute_dft(plan_forward,im,fim);
    }

  // convolution and transform back
    {
      for (j = 0; j < kh; j++) {
        for (i = 0; i < kw; i++) {
          conv[i + j * kw][0] =
            (fim[i + j * kw][0] * fkernel[i + j * kw][0] +
             fim[i + j * kw][1] * fkernel[i + j * kw][1])
             / (kh * kw  * (K + pow (fkernel[i + j * kw][0], 2) +
                (pow (fkernel[i + j * kw][1], 2))));
          conv[i + j * kw][1] =
            (fim[i + j * kw][0] * fkernel[i + j * kw][1] -
             fim[i + j * kw][1] * fkernel[i + j * kw][0])
             / (kh * kw * (K + pow(fkernel[i + j * kw][0], 2) +
                (pow(fkernel[i + j * kw][1], 2))));
        }
      }

      fftw_execute_dft(plan_backward,conv,out);
    }

  // regroup
    {
      for (j = 0; j < kh; j++) {
        for (i = 0; i < kw; i ++) {
          int k, l;
          long index1, index2;
          index1 = i + kw * j;
          if (i < ((kw)/2)+1) k = i + ((kw+1)/2)-1;
          else k = i - ((kw)/2)-1;
          if (j < ((kh)/2)+1) l = j + ((kh+1)/2)-1;
          else l = j - ((kh)/2)-1;

          index2 = k + kw * l;
          finishedtile[index2] = sqrt (((out[index1][0] * out[index1][0]
                           +  out[index1][1] * out[index1][1])));
        }
      }
    }

  // paste the tiles back into outimage
    {
      for (j = overlap; j < kh-overlap; j++)
        for (i = overlap; i < kw-overlap; i ++)
          outimage[i+shiftw + (j+shifth) * nrtilesw*tilesize] = finishedtile[kw-1-i + (kh-1-j) * kw];
    }
  }
// loop is over now! image contains the new image with borders
// shift the image from the center to the upper left corner
#pragma omp single
  if (UseEdgeMask)
    Mask = ptGradientMask(Image, 3,10);

  if (UseEdgeMask) {
#pragma omp for private(i,j) schedule(static)
    for (j = 0; j < h; j++)
      for (i = 0; i < w; i++) {
        uint16_t Temp = CLIP((int32_t) outimage[i+overlap + (j+overlap) * tilesize * nrtilesw]);
        if (Temp <= 0x7fff)
          Image->m_Image[i + j * w][0] =
            CLIP((int32_t) ((1.-Mask[i+j*w])*Temp + Mask[i+j*w]*Image->m_Image[i + j * w][0]));
        else
          Image->m_Image[i + j * w][0] =
            CLIP((int32_t) ((1.-Mask[i+j*w])*((Temp - (float)Image->m_Image[i+j*w][0])/3. + Image->m_Image[i+j*w][0]) +
                            Mask[i+j*w]*Image->m_Image[i + j * w][0]));
      }
#pragma omp single
    FREE2(Mask);
  } else {
#pragma omp for private(i,j) schedule(static)
  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++) {
      uint16_t Temp = CLIP((int32_t) outimage[i+overlap + (j+overlap) * tilesize * nrtilesw]);
      if (Temp <= 0x7fff)
        Image->m_Image[i + j * w][0] = Temp;
      else
        Image->m_Image[i + j * w][0] =
          CLIP((int32_t)((Temp - (float)Image->m_Image[i+j*w][0])/3. + Image->m_Image[i+j*w][0]));
    }
  }

// clean up
  fftw_free (im);
  fftw_free (fim);
  free (finishedtile);
  fftw_free (conv);
  fftw_free (out);
#pragma omp single nowait
  fftw_free (fkernel);
#pragma omp single nowait
  free (pimage);
#pragma omp single nowait
  free (outimage);
#pragma omp single nowait
  fftw_destroy_plan( plan_forward );
#pragma omp single nowait
  fftw_destroy_plan( plan_backward );

    } //end of omp parallel

  return;
}
