/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
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

#include <cmath>
#include <QMessageBox>
#include "ptImage.h"
#include "ptConstants.h"
#include "ptError.h"
#include "ptSettings.h"

#ifdef _OPENMP
  #include <omp.h>
#endif

// This file contains algorithms from CImg.h ported to Photivo
// for direct and faster use.
// Look in greyc/CImg.h for original copy right.

// Lut
extern float ToFloatTable[0x10000];

typedef float Tfloat;

//! Compute the result of the Deriche filter.
/**
   The Canny-Deriche filter is a recursive algorithm allowing to compute blurred derivatives of
   order 0,1 or 2 of an image.
**/
ptImage* ptImage::ptCIDeriche(const float sigma,
                              const int order /*=0*/,
                              const char axis /*='x'*/,
                              const bool cond /*=true*/,
                              const short ChannelMask /*=7*/) {
  int Channels = 0;
  int Channel[3] = {0,1,2};
  if (ChannelMask & 1) {Channel[Channels] = 0; Channels++;}
  if (ChannelMask & 2) {Channel[Channels] = 1; Channels++;}
  if (ChannelMask & 4) {Channel[Channels] = 2; Channels++;}

  const float nsigma = sigma>=0?sigma:-sigma*(axis=='x'?m_Width:axis=='y'?m_Height:0)/100;
  if (nsigma<0.1) return this;
  const float
    nnsigma = nsigma<0.1f?0.1f:nsigma,
    alpha = 1.695f/nnsigma,
    ema = (float)std::exp(-alpha),
    ema2 = (float)std::exp(-2*alpha),
    b1 = -2*ema,
    b2 = ema2;
  float a0 = 0, a1 = 0, a2 = 0, a3 = 0, coefp = 0, coefn = 0;
  switch (order) {
    case 0 : {
      const float k = (1-ema)*(1-ema)/(1+2*alpha*ema-ema2);
      a0 = k;
      a1 = k*(alpha-1)*ema;
      a2 = k*(alpha+1)*ema;
      a3 = -k*ema2;
    } break;
    case 1 : {
      const float k = (1-ema)*(1-ema)/ema;
      a0 = k*ema;
      a1 = a3 = 0;
      a2 = -a0;
    } break;
    case 2 : {
      const float
        ea = (float)std::exp(-alpha),
        k = -(ema2-1)/(2*alpha*ema),
        kn = (-2*(-1+3*ea-3*ea*ea+ea*ea*ea)/(3*ea+1+3*ea*ea+ea*ea*ea));
      a0 = kn;
      a1 = -kn*(1+k*alpha)*ema;
      a2 = kn*(1-k*alpha)*ema;
      a3 = -kn*ema2;
    } break;
    default :
      return this;
  }
  coefp = (a0+a1)/(1+b1+b2);
  coefn = (a2+a3)/(1+b1+b2);

#define _cimg_deriche2_apply \
  Tfloat *ptrY = (Tfloat*)Y, yb = 0, yp = 0; \
  uint16_t xp = (uint16_t)0; \
  if (cond) { xp = *ptrX; yb = yp = (Tfloat)(coefp*xp); } \
  for (uint32_t m = 0; m<N; ++m) { \
    const uint16_t xc = *ptrX; ptrX+=off; \
    const Tfloat yc = *(ptrY++) = (Tfloat)(a0*xc + a1*xp - b1*yp - b2*yb); \
    xp = xc; yb = yp; yp = yc; \
  } \
  uint16_t xn = (uint16_t)0, xa = (uint16_t)0; \
  Tfloat yn = 0, ya = 0; \
  if (cond) { xn = xa = *(ptrX-off); yn = ya = (Tfloat)coefn*xn; } \
  for (int n = N-1; n>=0; --n) { \
    const uint16_t xc = *(ptrX-=off); \
    const Tfloat yc = (Tfloat)(a2*xn + a3*xa - b1*yn - b2*ya); \
    xa = xn; xn = xc; ya = yn; yn = yc; \
    *ptrX = (uint16_t)(*(--ptrY)+yc); \
  }

  switch (axis) {
    case 'x' : {
      const uint32_t N = m_Width, off = 1*3;
#pragma omp parallel
{ // begin OpenMP
      float (*Y)[3] = (float (*)[3]) CALLOC(N,sizeof(*Y));
      ptMemoryError(Y,__FILE__,__LINE__);
#pragma omp for schedule(static)
      for (int y = 0; y<m_Height; ++y)
        for (int c = 0; c<Channels; c++)
          { uint16_t *ptrX = &m_Image[y*m_Width+0][Channel[c]]; _cimg_deriche2_apply; }
      FREE(Y);
} // end OpenMP
    } break;
    case 'y' : {
      const uint32_t N = m_Height, off = m_Width*3;
#pragma omp parallel
{ // begin OpenMP
      float (*Y)[3] = (float (*)[3]) CALLOC(N,sizeof(*Y));
      ptMemoryError(Y,__FILE__,__LINE__);
#pragma omp for schedule(static)
      for (int x = 0; x<m_Width; ++x)
        for (int c = 0; c<Channels; c++)
          { uint16_t *ptrX = &m_Image[x][Channel[c]]; _cimg_deriche2_apply; }
      FREE(Y);
} // end OpenMP
    } break;
    default : {
      return this;
    }
  }
  return this;
}

// Blur (Deriche of order 0)
ptImage* ptImage::ptCIBlur(const double Sigma, const short ChannelMask /*=7*/) {
  ptCIDeriche(Sigma,0,'x',1,ChannelMask);
  ptCIDeriche(Sigma,0,'y',1,ChannelMask);
  return this;
}


static inline float CImod(const float& x, const float& m) {
  const float dx = x, dm = m;
  if (x<0) { return (float)(dm+dx+dm*floorf(-dx/dm)); }
  return (float)(dx-dm*floorf(dx/dm));
}


ptImage* ptImage::ptCIPerspective(const float RotateAngle,
                                  const float FocalLength,
                                  const float TiltAngle,
                                  const float TurnAngle,
                                  const float ScaleX,
                                  const float ScaleY) {
  const float nangle = CImod(RotateAngle,360.0f);
  uint16_t NewWidth = 0, NewHeight = 0;
  uint16_t (*TempImage)[3];
  if (CImod(nangle,90.0f)==0 &&
      TiltAngle == 0.0f &&
      TurnAngle == 0.0f &&
      ScaleX == 1.0f &&
      ScaleY == 1.0f) { // optimized version for orthogonal angles
    TempImage = (uint16_t (*)[3]) CALLOC(m_Width*m_Height,sizeof(*TempImage));
    ptMemoryError(TempImage,__FILE__,__LINE__);
    const uint16_t wm1 = m_Width - 1, hm1 = m_Height - 1;
    const int iangle = (int32_t)nangle/90;
    switch (iangle) {
      case 1 : {
        NewWidth = m_Height;
        NewHeight = m_Width;
        uint16_t *ptrd = (uint16_t*) TempImage;
        for (int32_t y=0; y<NewHeight; y++) {
          for (int32_t x=0; x<NewWidth; x++) {
            for (short c=0; c<3; c++) {
              *(ptrd++) = m_Image[y+(hm1-x)*m_Width][c];
            }
          }
        }
      } break;
      case 2 : {
        NewWidth = m_Width;
        NewHeight = m_Height;
        uint16_t *ptrd = (uint16_t*) TempImage;
        for (int32_t y=0; y<NewHeight; y++) {
          for (int32_t x=0; x<NewWidth; x++) {
            for (short c=0; c<3; c++) {
              *(ptrd++) = m_Image[(wm1-x)+(hm1-y)*m_Width][c];
            }
          }
        }
      } break;
      case 3 : {
        NewWidth = m_Height;
        NewHeight = m_Width;
        uint16_t *ptrd = (uint16_t*) TempImage;
        for (int32_t y=0; y<NewHeight; y++) {
          for (int32_t x=0; x<NewWidth; x++) {
            for (short c=0; c<3; c++) {
              *(ptrd++) = m_Image[(wm1-y)+x*m_Width][c];
            }
          }
        }
      } break;
      default : {
        FREE(TempImage);
        return this;
      }
    }
  } else { // generic version
    // horizontal perspective (tilt)
    const float
      r_hor = FocalLength/powf(36.0f*36.0f+24.0f*24.0f,0.5f)*powf((float)m_Width*m_Width+(float)m_Height*m_Height,0.5f),
      angle_hor = (float)(TiltAngle*ptPI/180.0);
    const float MinDenom = 0.1;
    short OutOfBounds = 0;
    if (r_hor*cosf(angle_hor)+m_Height/2.0f*sinf(angle_hor) < MinDenom ||
        r_hor*cosf(-angle_hor)+m_Height/2.0f*sinf(-angle_hor) < MinDenom)
      OutOfBounds = 1;
    const float
      uy_hor = m_Height/2.0f*r_hor/(r_hor*cosf(angle_hor)+m_Height/2.0f*sinf(angle_hor)),
      vy_hor = m_Height/2.0f*r_hor/(r_hor*cosf(-angle_hor)+m_Height/2.0f*sinf(-angle_hor)),
      ux_hor = m_Width*powf((powf(r_hor-uy_hor*sinf(angle_hor),2.0f)+powf(uy_hor*cosf(angle_hor),2.0f))/(r_hor*r_hor+(float)m_Height*m_Height/4.0f),0.5f),
      vx_hor = m_Width*powf((powf(r_hor-vy_hor*sinf(-angle_hor),2.0f)+powf(vy_hor*cosf(-angle_hor),2.0f))/(r_hor*r_hor+(float)m_Height*m_Height/4.0f),0.5f),
      x_hor = MAX(fabs(ux_hor),fabs(vx_hor)),
      x_hor1 = 0.5f*x_hor,
      y_mid_hor = m_Height*0.5f,
      x_mid_hor = m_Width*0.5f;
    uint16_t Height_hor = (int32_t)(uy_hor+vy_hor);
    uint16_t Width_hor = (int32_t) x_hor;
    // vertical perspective (turn)
    const float
      r_ver = cosf(angle_hor)*r_hor,
      angle_ver = (float)(-TurnAngle*ptPI/180.0);
    if (r_ver*cosf(angle_ver)+Width_hor/2.0f*sinf(angle_ver) < MinDenom ||
        r_ver*cosf(-angle_ver)+Width_hor/2.0f*sinf(-angle_ver) < MinDenom)
      OutOfBounds = 1;
    const float
      ux_ver = Width_hor/2.0f*r_ver/(r_ver*cosf(angle_ver)+Width_hor/2.0f*sinf(angle_ver)),
      vx_ver = Width_hor/2.0f*r_ver/(r_ver*cosf(-angle_ver)+Width_hor/2.0f*sinf(-angle_ver)),
      uy_ver = Height_hor*powf((powf(r_ver-ux_ver*sinf(angle_ver),2.0f)+powf(ux_ver*cosf(angle_ver),2.0f))/(r_ver*r_ver+(float)Width_hor*Width_hor/4.0f),0.5f),
      vy_ver = Height_hor*powf((powf(r_ver-vx_ver*sinf(-angle_ver),2.0f)+powf(vx_ver*cosf(-angle_ver),2.0f))/(r_ver*r_ver+(float)Width_hor*Width_hor/4.0f),0.5f),
      y_ver = MAX(fabs(uy_ver),fabs(vy_ver)),
      y_ver1 = 0.5f*y_ver,
      y_mid_ver = Height_hor*0.5f,
      x_mid_ver = Width_hor*0.5f;
    uint16_t Width_ver = (int32_t)(ux_ver+vx_ver);
    uint16_t Height_ver = (int32_t) y_ver;
    if (OutOfBounds == 1) return this;
    // rotation and scaling
    const float
      rad = (float)(nangle*ptPI/180.0),
      ca = (float)cosf(rad),
      sa = (float)sinf(rad),
      sc1 = 1.0f/ScaleX,
      sc2 = 1.0f/ScaleY,
      ux = abs(Width_ver/sc1*ca), uy = abs(Width_ver/sc1*sa),
      vx = abs(Height_ver/sc2*sa), vy = abs(Height_ver/sc2*ca),
      w2 = 0.5f*Width_ver, h2 = 0.5f*Height_ver,
      dw2 = 0.5f*(ux+vx), dh2 = 0.5f*(uy+vy);
    NewWidth = (int32_t)(ux+vx);
    NewHeight = (int32_t)(uy+vy);
    TempImage = (uint16_t (*)[3]) CALLOC((int32_t)NewWidth*NewHeight,sizeof(*TempImage));
    ptMemoryError(TempImage,__FILE__,__LINE__);
    // temp values
    const float
      sin_ver = sinf(angle_ver),
      cos_ver = cosf(angle_ver),
      sin_hor = sinf(angle_hor),
      cos_hor = cosf(angle_hor);
    float fx,fy,gx,gy,hx,hy;
    {
#pragma omp parallel for schedule(static) private(fx,fy,gx,gy,hx,hy)
      for (int32_t Row = 0; Row < NewHeight; Row++) {
        int32_t Temp = Row*NewWidth;
        for (int32_t Col = 0; Col < NewWidth; Col++) {
          // rotation and scaling
          hx = w2 + (Col-dw2)*ca*sc1 + (Row-dh2)*sa*sc1;
          hy = h2 - (Col-dw2)*sa*sc2 + (Row-dh2)*ca*sc2;

          // vertical perspective (turn)
          if (TurnAngle != 0.0f) {
            const float ver_x_temp_sin = (hx-vx_ver)*sin_ver;
            const float ver_x_temp_cos = (hx-vx_ver)*cos_ver;
            const float gx_temp = r_ver*ver_x_temp_cos/(r_ver-ver_x_temp_sin);
            gx = x_mid_ver+gx_temp;
            gy = y_mid_ver+(hy-y_ver1)*powf((r_ver*r_ver+gx_temp*gx_temp)/(powf(r_ver-ver_x_temp_sin,2.0f)+powf(ver_x_temp_cos,2.0f)),0.5f);
          } else {
            gx = hx;
            gy = hy;
          }

          // horizontal perspective (tilt)
          if (TiltAngle != 0.0f) {
            const float hor_y_temp_sin = (gy-vy_hor)*sin_hor;
            const float hor_y_temp_cos = (gy-vy_hor)*cos_hor;
            const float fy_temp = r_hor*hor_y_temp_cos/(r_hor-hor_y_temp_sin);
            fy = y_mid_hor+fy_temp;
            fx = x_mid_hor+(gx-x_hor1)*powf((r_hor*r_hor+fy_temp*fy_temp)/(powf(r_hor-hor_y_temp_sin,2.0f)+powf(hor_y_temp_cos,2.0f)),0.5f);
          } else {
            fx = gx;
            fy = gy;
          }

          // outside source?
          if (fx < 0 || fx > m_Width-1 || fy < 0 || fy > m_Height-1) {
            for (short c = 0; c < 3; c++) {
              TempImage[Temp+Col][c] = 0;
            }
            continue;
          }
          const int32_t x = (int32_t)fx, y = (int32_t)fy;
          const float dx = fx - x, dy = fy - y;
          const int32_t
            px = x-1<0?0:x-1, nx = dx>0?x+1:x, ax = x+2>=m_Width?m_Width-1:x+2,
            py = y-1<0?0:y-1, ny = dy>0?y+1:y, ay = y+2>=m_Height?m_Height-1:y+2;
          const int32_t
            pyw = py * m_Width, yw = y * m_Width,
            nyw = ny * m_Width, ayw = ay * m_Width;
          for (short c = 0; c < 3; c++) {
            const Tfloat
              Ipp = m_Image[px+pyw][c], Icp = m_Image[x+pyw][c], Inp = m_Image[nx+pyw][c], Iap = m_Image[ax+pyw][c],
              Ip = Icp + 0.5f*(dx*(-Ipp+Inp) + dx*dx*(2*Ipp-5*Icp+4*Inp-Iap) + dx*dx*dx*(-Ipp+3*Icp-3*Inp+Iap)),
              Ipc = m_Image[px+yw][c], Icc = m_Image[x+yw][c], Inc = m_Image[nx+yw][c], Iac = m_Image[ax+yw][c],
              Ic = Icc + 0.5f*(dx*(-Ipc+Inc) + dx*dx*(2*Ipc-5*Icc+4*Inc-Iac) + dx*dx*dx*(-Ipc+3*Icc-3*Inc+Iac)),
              Ipn = m_Image[px+nyw][c], Icn = m_Image[x+nyw][c], Inn = m_Image[nx+nyw][c], Ian = m_Image[ax+nyw][c],
              In = Icn + 0.5f*(dx*(-Ipn+Inn) + dx*dx*(2*Ipn-5*Icn+4*Inn-Ian) + dx*dx*dx*(-Ipn+3*Icn-3*Inn+Ian)),
              Ipa = m_Image[px+ayw][c], Ica = m_Image[x+ayw][c], Ina = m_Image[nx+ayw][c], Iaa = m_Image[ax+ayw][c],
              Ia = Ica + 0.5f*(dx*(-Ipa+Ina) + dx*dx*(2*Ipa-5*Ica+4*Ina-Iaa) + dx*dx*dx*(-Ipa+3*Ica-3*Ina+Iaa));
            const Tfloat val = Ic + 0.5f*(dy*(-Ip+In) + dy*dy*(2*Ip-5*Ic+4*In-Ia) + dy*dy*dy*(-Ip+3*Ic-3*In+Ia));
            TempImage[Temp+Col][c] = CLIP((int32_t) val);
          }
        }
      }
    }
  }
  FREE(m_Image);
  m_Image = TempImage;
  m_Height = NewHeight;
  m_Width = NewWidth;
  return this;
}
