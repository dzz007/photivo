#include <QMessageBox>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>

#include "ptImage.h"
#include "ptError.h"
#include "ptCalloc.h"

#ifdef _OPENMP
  #include <omp.h>
#endif

/*
 * Copyright of the original implementation of this algorithm by
 * Johannes Hanika of the darktable project.
 * See https://sourceforge.net/projects/darktable/
 */

// edge-avoiding wavelet:
#define gweight(i, j, ii, jj) 1.0/(fabsf(weight_a[l][wd*((j)>>(l-1)) + ((i)>>(l-1))] - weight_a[l][wd*((jj)>>(l-1)) + ((ii)>>(l-1))])+1.e-5)
// #define gweight(i, j, ii, jj) 1.0/(powf(fabsf(weight_a[l][wd*((j)>>(l-1)) + ((i)>>(l-1))] - weight_a[l][wd*((jj)>>(l-1)) + ((ii)>>(l-1))]),0.8)+1.e-5)
// std cdf(2,2) wavelet:
// #define gweight(i, j, ii, jj) (wd ? 1.0 : 1.0) //1.0
/*
 * For 3 channels:
 * #define gbuf(BUF, A, B) ((BUF)[3*width*((B)) + 3*((A)) + ch])
 */
#define gbuf(BUF, A, B) ((BUF)[width*((B)) + ((A))])

void wtf_channel(float *buf, float **weight_a, const int l, const int width, const int height)
{
  const int wd = (int)(1 + (width>>(l-1))), ht = (int)(1 + (height>>(l-1)));
  // store weights for luma channel only, chroma uses same basis.
  memset(weight_a[l], 0, sizeof(float)*wd*ht);
  for(int j=0;j<ht-1;j++) for(int i=0;i<wd-1;i++) weight_a[l][j*wd+i] = gbuf(buf, i<<(l-1), j<<(l-1));

  const int step = 1<<l;
  const int st = step/2;

#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static) //private(ch)
#endif
  for(int j=0;j<height;j++)
  { // rows
    // precompute weights:
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(width,sizeof(float));
    for(int i=0;i<width-st;i+=st) tmp[i] = gweight(i, j, i+st, j);
    // predict, get detail
    int i = st;
    for(;i<width-st;i+=step)
      gbuf(buf, i, j) -= (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(tmp[i-st] + tmp[i]);
    if(i < width) gbuf(buf, i, j) -= gbuf(buf, i-st, j);
    // update coarse
    gbuf(buf, 0, j) += gbuf(buf, st, j)*0.5f;
    for(i=step;i<width-st;i+=step)
      gbuf(buf, i, j) += (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(2.0*(tmp[i-st] + tmp[i]));
    if(i < width) gbuf(buf, i, j) += gbuf(buf, i-st, j)*.5f;
    FREE2(tmp);
  }
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static) //private(ch)
#endif
  for(int i=0;i<width;i++)
  { // cols
    // precompute weights:
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(height,sizeof(float));
    for(int j=0;j<height-st;j+=st) tmp[j] = gweight(i, j, i, j+st);
    int j = st;
    // predict, get detail
    for(;j<height-st;j+=step)
      gbuf(buf, i, j) -= (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(tmp[j-st] + tmp[j]);
    if(j < height) gbuf(buf, i, j) -= gbuf(buf, i, j-st);
    // update
    gbuf(buf, i, 0) += gbuf(buf, i, st)*0.5;
    for(j=step;j<height-st;j+=step)
      gbuf(buf, i, j) += (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(2.0*(tmp[j-st] + tmp[j]));
    if(j < height) gbuf(buf, i, j) += gbuf(buf, i, j-st)*.5f;
    FREE2(tmp);
  }
}

void iwtf_channel(float *buf, float **weight_a, const int l, const int width, const int height)
{
  const int step = 1<<l;
  const int st = step/2;
  const int wd = (int)(1 + (width>>(l-1)));

#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static) //private(ch)
#endif
  for(int i=0;i<width;i++)
  { //cols
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(height,sizeof(float));
    int j;
    for(j=0;j<height-st;j+=st) tmp[j] = gweight(i, j, i, j+st);
    // update coarse
    gbuf(buf, i, 0) -= gbuf(buf, i, st)*0.5f;
    for(j=step;j<height-st;j+=step)
      gbuf(buf, i, j) -= (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(2.0*(tmp[j-st] + tmp[j]));
    if(j < height) gbuf(buf, i, j) -= gbuf(buf, i, j-st)*.5f;
    // predict
    for(j=st;j<height-st;j+=step)
      gbuf(buf, i, j) += (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(tmp[j-st] + tmp[j]);
    if(j < height) gbuf(buf, i, j) += gbuf(buf, i, j-st);
    FREE2(tmp);
  }
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static) //private(ch)
#endif
  for(int j=0;j<height;j++)
  { // rows
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(width,sizeof(float));
    int i;
    for(int i=0;i<width-st;i+=st) tmp[i] = gweight(i, j, i+st, j);
    // update
    gbuf(buf, 0, j) -= gbuf(buf, st, j)*0.5f;
    for(i=step;i<width-st;i+=step)
      gbuf(buf, i, j) -= (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(2.0*(tmp[i-st] + tmp[i]));
    if(i < width) gbuf(buf, i, j) -= gbuf(buf, i-st, j)*0.5f;
    // predict
    for(i=st;i<width-st;i+=step)
      gbuf(buf, i, j) += (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(tmp[i-st] + tmp[i]);
    if(i < width) gbuf(buf, i, j) += gbuf(buf, i-st, j);
    FREE2(tmp);
  }
}

void dt_iop_equalizer_wtf(float *buf, float **weight_a, const int l, const int width, const int height)
{
  const int wd = (int)(1 + (width>>(l-1))), ht = (int)(1 + (height>>(l-1)));
  int ch = 0;
  // store weights for luma channel only, chroma uses same basis.
  memset(weight_a[l], 0, sizeof(float)*wd*ht);
  for(int j=0;j<ht-1;j++) for(int i=0;i<wd-1;i++) weight_a[l][j*wd+i] = gbuf(buf, i<<(l-1), j<<(l-1));

  const int step = 1<<l;
  const int st = step/2;

#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) private(ch) schedule(static)
#endif
  for(int j=0;j<height;j++)
  { // rows
    // precompute weights:
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(width,sizeof(float));
    for(int i=0;i<width-st;i+=st) tmp[i] = gweight(i, j, i+st, j);
    // predict, get detail
    int i = st;
    for(;i<width-st;i+=step) for(ch=0;ch<3;ch++)
      gbuf(buf, i, j) -= (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(tmp[i-st] + tmp[i]);
    if(i < width) for(ch=0;ch<3;ch++) gbuf(buf, i, j) -= gbuf(buf, i-st, j);
    // update coarse
    for(ch=0;ch<3;ch++) gbuf(buf, 0, j) += gbuf(buf, st, j)*0.5f;
    for(i=step;i<width-st;i+=step) for(ch=0;ch<3;ch++)
      gbuf(buf, i, j) += (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(2.0*(tmp[i-st] + tmp[i]));
    if(i < width) for(ch=0;ch<3;ch++) gbuf(buf, i, j) += gbuf(buf, i-st, j)*.5f;
    FREE2(tmp);
  }
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) private(ch) schedule(static)
#endif
  for(int i=0;i<width;i++)
  { // cols
    // precompute weights:
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(height,sizeof(float));
    for(int j=0;j<height-st;j+=st) tmp[j] = gweight(i, j, i, j+st);
    int j = st;
    // predict, get detail
    for(;j<height-st;j+=step) for(ch=0;ch<3;ch++)
      gbuf(buf, i, j) -= (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(tmp[j-st] + tmp[j]);
    if(j < height) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) -= gbuf(buf, i, j-st);
    // update
    for(ch=0;ch<3;ch++) gbuf(buf, i, 0) += gbuf(buf, i, st)*0.5;
    for(j=step;j<height-st;j+=step) for(ch=0;ch<3;ch++)
      gbuf(buf, i, j) += (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(2.0*(tmp[j-st] + tmp[j]));
    if(j < height) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) += gbuf(buf, i, j-st)*.5f;
    FREE2(tmp);
  }
}

void dt_iop_equalizer_iwtf(float *buf, float **weight_a, const int l, const int width, const int height)
{
  const int step = 1<<l;
  const int st = step/2;
  const int wd = (int)(1 + (width>>(l-1)));

#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static)
#endif
  for(int i=0;i<width;i++)
  { //cols
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(height,sizeof(float));
    int j;
    for(j=0;j<height-st;j+=st) tmp[j] = gweight(i, j, i, j+st);
    // update coarse
    for(int ch=0;ch<3;ch++) gbuf(buf, i, 0) -= gbuf(buf, i, st)*0.5f;
    for(j=step;j<height-st;j+=step) for(int ch=0;ch<3;ch++)
      gbuf(buf, i, j) -= (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(2.0*(tmp[j-st] + tmp[j]));
    if(j < height) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) -= gbuf(buf, i, j-st)*.5f;
    // predict
    for(j=st;j<height-st;j+=step) for(int ch=0;ch<3;ch++)
      gbuf(buf, i, j) += (tmp[j-st]*gbuf(buf, i, j-st) + tmp[j]*gbuf(buf, i, j+st))
        /(tmp[j-st] + tmp[j]);
    if(j < height) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) += gbuf(buf, i, j-st);
    FREE2(tmp);
  }
#ifdef _OPENMP
#pragma omp parallel for default(none) shared(weight_a,buf) schedule(static)
#endif
  for(int j=0;j<height;j++)
  { // rows
    // Allocation is done with calloc to avoid a compiler error with OpenMP/MacOSX Lion
    float *tmp = (float*)CALLOC2(width,sizeof(float));
    int i;
    for(int i=0;i<width-st;i+=st) tmp[i] = gweight(i, j, i+st, j);
    // update
    for(int ch=0;ch<3;ch++) gbuf(buf, 0, j) -= gbuf(buf, st, j)*0.5f;
    for(i=step;i<width-st;i+=step) for(int ch=0;ch<3;ch++)
      gbuf(buf, i, j) -= (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(2.0*(tmp[i-st] + tmp[i]));
    if(i < width) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) -= gbuf(buf, i-st, j)*0.5f;
    // predict
    for(i=st;i<width-st;i+=step) for(int ch=0;ch<3;ch++)
      gbuf(buf, i, j) += (tmp[i-st]*gbuf(buf, i-st, j) + tmp[i]*gbuf(buf, i+st, j))
        /(tmp[i-st] + tmp[i]);
    if(i < width) for(int ch=0;ch<3;ch++) gbuf(buf, i, j) += gbuf(buf, i-st, j);
    FREE2(tmp);
  }
}

#undef gbuf
#undef gweight

extern float ToFloatTable[0x10000];

ptImage* ptImage::EAWChannel(const double scaling,
                             const double level1,
                             const double level2,
                             const double level3,
                             const double level4,
                             const double level5,
                             const double level6) {

  assert (m_ColorSpace == ptSpace_Lab);

  uint16_t w = m_Width;
  uint16_t h = m_Height;

  float (*out) = (float (*)) CALLOC2(w*h,sizeof(*out));
  ptMemoryError(out,__FILE__,__LINE__);

#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < m_Width*m_Height; i++) {
    out[i] = ToFloatTable[m_Image[i][0]];
  }

  const int width = w;
  const int height = h;

  float alpha = 1; // leftover ;-)
  int maxband = 7;
  int mode = 0;

  if (level6 == 0) {
    maxband--;
    if (level5 == 0) {
      maxband--;
      if (level4 == 0) {
        maxband--;
        if (level3 == 0) {
          maxband--;
          if (level2 == 0) {
            maxband--;
          }
        }
      }
    }
  }
  maxband = maxband - scaling;
  if (maxband <= 0) return this;

/* PROCESSING*/

#if 1
  const int max_level = maxband;
  const int sf = 1; // piece->iscale/scale

  // 1 pixel in this buffer represents 1.0/scale pixels in original image:
  const float l1 = 1.0f + log2f(sf);                          // finest level
  float lm = 0; for(int k=MIN(width,height)*sf;k;k>>=1) lm++; // coarsest level
  lm = MIN((float)max_level, l1 + lm);
  // level 1 => full resolution
  int numl = 0; for(int k=MIN(width,height);k;k>>=1) numl++;
  const int numl_cap = MIN(max_level-l1+1.5f, (float)numl);

  // TODO: fixed alloc for data piece at capped resolution?
  float **tmp = (float **)malloc(sizeof(float *)*numl_cap);
  for(int k=1;k<numl_cap;k++)
  {
    const int wd = (int)(1 + (width>>(k-1))), ht = (int)(1 + (height>>(k-1)));
    tmp[k] = (float *)malloc(sizeof(float)*wd*ht);
  }

  for(int level=1;level<numl_cap;level++) wtf_channel(out, tmp, level, width, height);

  for(int l=1;l<numl_cap;l++)
  {
    const float lv = (lm-l1)*(l-1)/(float)(numl_cap-1) + l1; // appr level in real image.
    {
      // coefficients in range [0, 2], 1 being neutral.
      float coeff = 1;
      if (mode == 0) { // linear amplification
        if (lv == 1 - scaling) coeff = level1 + 1.0;
        else if (lv == 2 - scaling) coeff = level2 + 1.0;
        else if (lv == 3 - scaling) coeff = level3 + 1.0;
        else if (lv == 4 - scaling) coeff = level4 + 1.0;
        else if (lv == 5 - scaling) coeff = level5 + 1.0;
        else if (lv == 6 - scaling) coeff = level6 + 1.0;
      } else if (mode == 1) { // suppress lowest level
        if ((int)lv == 1) coeff = 1;
        else if ((int) lv == 2) coeff = (alpha>1?(fabs(1-alpha)/lv/2+1):(1-fabs(1-alpha)/lv/2));
        else coeff = (alpha>1?(fabs(1-alpha)/lv+1):(1-fabs(1-alpha)/lv));
      } else if (mode == 2) { // show only one level; here level 1
        if ((int)lv == 1 ) coeff = alpha;
        else coeff = 0;
      }

      const int step = 1<<l;
#if 1 // scale coefficients
      for(int j=0;j<height;j+=step)      for(int i=step/2;i<width;i+=step) out[width*j + i] *= coeff;
      for(int j=step/2;j<height;j+=step) for(int i=0;i<width;i+=step)      out[width*j + i] *= coeff;
      for(int j=step/2;j<height;j+=step) for(int i=step/2;i<width;i+=step) out[width*j + i] *= coeff*coeff;
#else // soft-thresholding (shrinkage)
#define wshrink (copysignf(fmaxf(0.0f, fabsf(out[width*j + i]) - (1.0-coeff)), out[width*j + i]))
      for(int j=0;j<height;j+=step)      for(int i=step/2;i<width;i+=step) out[width*j + i] = wshrink;
      for(int j=step/2;j<height;j+=step) for(int i=0;i<width;i+=step)      out[width*j + i] = wshrink;
      for(int j=step/2;j<height;j+=step) for(int i=step/2;i<width;i+=step) out[width*j + i] = wshrink;
#undef wshrink
#endif
    }
  }
  // printf("applied\n");

  for(int level=numl_cap-1;level>0;level--) iwtf_channel(out, tmp, level, width, height);

  for(int k=1;k<numl_cap;k++) free(tmp[k]);
  free(tmp);
#endif


#pragma omp parallel for schedule(static)
  for (int32_t i = 0; i < m_Width*m_Height; i++) {
    m_Image[i][0] = CLIP((int32_t)(out[i]*0xffff));
  }

  FREE (out);
  return this;
}
