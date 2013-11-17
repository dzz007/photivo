/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Michael Munzert <mail@mm-log.com>
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

#include "ptImage.h"
#include "ptError.h"
#include "ptCalloc.h"

#include <lqr-1/lqr.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include <cassert>

//==============================================================================

ptImage* ptImage::LiquidRescaleRelative(const double HorScale,
                                        const double VertScale,
                                        const short Energy,
                                        const short VertFirst)
{
  uint16_t Width = m_Width * HorScale;
  uint16_t Height = m_Height * VertScale;

  LiquidRescale(Width, Height, Energy, VertFirst);

  return this;
}

//==============================================================================

ptImage* ptImage::LiquidRescale(const uint16_t Width,
                                const uint16_t Height,
                                const short Energy,
                                const short VertFirst)
{
  // Destroying the carver further down will also free the copied image.
  LqrCarver* carver = lqr_carver_new_ext(memcpy(CALLOC(m_Width*m_Height,sizeof(*m_Image)),
                                                m_Image,
                                                m_Width*m_Height*sizeof(*m_Image)),
                                         m_Width,
                                         m_Height,
                                         3, // channels
                                         LQR_COLDEPTH_16I);

  switch(Energy) {
    case ptLqr_GradXabs:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_GRAD_XABS);
      break;
    case ptLqr_GradSumabs:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_GRAD_SUMABS);
      break;
    case ptLqr_GradNorm:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_GRAD_NORM);
      break;
    case ptLqr_LumaGradXabs:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_LUMA_GRAD_XABS);
      break;
    case ptLqr_LumaGradSumabs:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_LUMA_GRAD_SUMABS);
      break;
    case ptLqr_LumaGradNorm:
      lqr_carver_set_energy_function_builtin(carver, LQR_EF_LUMA_GRAD_NORM);
      break;
    default:
      assert(0);
  }

  lqr_carver_set_side_switch_frequency(carver, 10);

  if (VertFirst == 1) {
    lqr_carver_set_resize_order(carver,LQR_RES_ORDER_VERT);
    // otherwise horizontal is default
  }

  lqr_carver_init(carver,1,0.0);
  lqr_carver_resize(carver, Width, Height);
  this->setSize((size_t)Width*Height);

  int Row, Col;
  void *rgb;
  uint16_t *rgb_out;
  int32_t Index = 0;

  while (lqr_carver_scan_ext (carver, &Col, &Row, &rgb)) {
    rgb_out = (uint16_t*) rgb;
    Index = Row*Width+Col;
    m_Image[Index][0] = rgb_out[0];
    m_Image[Index][1] = rgb_out[1];
    m_Image[Index][2] = rgb_out[2];
  }

  m_Width = Width;
  m_Height = Height;

  lqr_carver_destroy(carver);

  return this;
}

//==============================================================================
