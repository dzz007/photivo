/*******************************************************************************
**
** Photivo
**
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include <lensfun.h>
#include "ptImage.h"
#include "ptError.h"

#ifdef _OPENMP
  #include <omp.h>
#endif

typedef float Tfloat;   // needed by the interpolation code

ptImage* ptImage::Lensfun(const int LfunActions, const lfModifier* LfunData) {
  // Stage 2: Vignetting.
  if ((LfunActions & LF_MODIFY_VIGNETTING) || (LfunActions == LF_MODIFY_ALL)) {
    if (!LfunData->ApplyColorModification(m_Image, 0.0, 0.0, m_Width, m_Height,
                                          LF_CR_3(RED, GREEN, BLUE),
                                          m_Width * 3 * sizeof(uint16_t)) )
    {
      ptLogError(ptError_Lensfun, "Could not apply vignetting correction.");
    }
  }

  /**
   * Stage 1 and/or 3: CA and lens geometry/distortion correction
   * Processing row by row to avoid *huge* memory requirements.
   * Note that lensfun’s ApplySubpixelGeometryDistortion() always returns separated
   * RGB channels in contrast to what the docu says.
   *
   * NewCoords
   *     A pointer to an array of output pixels containing for each pixel its x and y
   *     position in the input image. Pixels are split into R, G, and B channels. Memory
   *     layout for each pixel is as follows: Rx, Ry, Gx, Gy, Bx, By
   */
  if (((LfunActions & LF_MODIFY_TCA) || (LfunActions & LF_MODIFY_DISTORTION) || (LfunActions & LF_MODIFY_GEOMETRY)) ||
      (LfunActions == LF_MODIFY_ALL) )
  {
    uint16_t (*TempImage)[3] = (uint16_t (*)[3]) CALLOC((int32_t)m_Width * m_Height, sizeof(*TempImage));
    bool LfunSuccess = true;

#pragma omp parallel
{
    float* NewCoords = (float*) CALLOC((uint32_t)m_Width * 3 * 2, sizeof(float));

#pragma omp for schedule(static) reduction(||:LfunSuccess)
    for (uint16_t row = 0; row < m_Height; row++) {
      int LfunLineSuccess = LfunData->ApplySubpixelGeometryDistortion(0.0, row, m_Width, 1, NewCoords);

      if (!LfunLineSuccess) {
        LfunSuccess = false;
      } else {
        // Bicubic interpolation
        int32_t Temp = row * m_Width;
        for (uint16_t col = 0; col < m_Width; col++) {
          for (short channel = 0; channel < 3; channel++) {
            uint32_t MemPosX = col * 6 + channel * 2;
            if (NewCoords[MemPosX] < 0 || NewCoords[MemPosX] > m_Width-1 || NewCoords[MemPosX+1] < 0 || NewCoords[MemPosX+1] > m_Height-1) {
              TempImage[Temp+col][channel] = 0;
              continue;
            }
            const int32_t
              x = (int32_t)NewCoords[MemPosX],
              y = (int32_t)NewCoords[MemPosX+1];
            const float
              dx = NewCoords[MemPosX] - x,
              dy = NewCoords[MemPosX+1] - y;
            const int32_t
              px = x-1<0?0:x-1, nx = dx>0?x+1:x, ax = x+2>=m_Width?m_Width-1:x+2,
              py = y-1<0?0:y-1, ny = dy>0?y+1:y, ay = y+2>=m_Height?m_Height-1:y+2;
            const int32_t
              pyw = py * m_Width, yw = y * m_Width,
              nyw = ny * m_Width, ayw = ay * m_Width;
            const Tfloat
              Ipp = m_Image[px+pyw][channel], Icp = m_Image[x+pyw][channel], Inp = m_Image[nx+pyw][channel], Iap = m_Image[ax+pyw][channel],
              Ip = Icp + 0.5f*(dx*(-Ipp+Inp) + dx*dx*(2*Ipp-5*Icp+4*Inp-Iap) + dx*dx*dx*(-Ipp+3*Icp-3*Inp+Iap)),
              Ipc = m_Image[px+yw][channel], Icc = m_Image[x+yw][channel], Inc = m_Image[nx+yw][channel], Iac = m_Image[ax+yw][channel],
              Ic = Icc + 0.5f*(dx*(-Ipc+Inc) + dx*dx*(2*Ipc-5*Icc+4*Inc-Iac) + dx*dx*dx*(-Ipc+3*Icc-3*Inc+Iac)),
              Ipn = m_Image[px+nyw][channel], Icn = m_Image[x+nyw][channel], Inn = m_Image[nx+nyw][channel], Ian = m_Image[ax+nyw][channel],
              In = Icn + 0.5f*(dx*(-Ipn+Inn) + dx*dx*(2*Ipn-5*Icn+4*Inn-Ian) + dx*dx*dx*(-Ipn+3*Icn-3*Inn+Ian)),
              Ipa = m_Image[px+ayw][channel], Ica = m_Image[x+ayw][channel], Ina = m_Image[nx+ayw][channel], Iaa = m_Image[ax+ayw][channel],
              Ia = Ica + 0.5f*(dx*(-Ipa+Ina) + dx*dx*(2*Ipa-5*Ica+4*Ina-Iaa) + dx*dx*dx*(-Ipa+3*Ica-3*Ina+Iaa));
            const Tfloat val = Ic + 0.5f*(dy*(-Ip+In) + dy*dy*(2*Ip-5*Ic+4*In-Ia) + dy*dy*dy*(-Ip+3*Ic-3*In+Ia));
            TempImage[Temp+col][channel] = CLIP((int32_t) val);
          }
        }
      }
    } //end of pragma omp for schedule(static)

    FREE(NewCoords);
} //end of pragma omp parallel



    if (LfunSuccess) {
      FREE(m_Image);
      m_Image = TempImage;
    } else {
      ptLogError(ptError_Lensfun, "Could not apply geometry/distortion correction.");
      FREE(TempImage);
    }
  }

  return this;
}
