/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2009 Michael Munzert <mail@mm-log.com>
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

#include <QString>
#include <QObject>
#include <QMessageBox>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include "ptFastBilateral.h"
#include "ptImage.h"
#include "ptError.h"

#include "fastbilateral/fast_lbf.h"
//~ #include "fastbilateral/linear_bf.h"

typedef Array_2D<float> image_type;

extern float ToFloatTable[0x10000];

// From the theoretical part, the bilateral filtershould blur more when values
// are closer together. Since we use it with linear data, an additional gamma
// correction could give better results.

void ptFastBilateralChannel(ptImage* Image,
                            const float Sigma_s,
                            const float Sigma_r,
                            const short Iterations,
                            const short ChannelMask) {

  uint16_t Width  = Image->m_Width;
  uint16_t Height = Image->m_Height;

  image_type InImage(Width,Height);
  image_type FilteredImage(Width,Height);

  for (short Channel = 0; Channel<3; Channel++) {
    // Is it a channel we are supposed to handle ?
    if  (! (ChannelMask & (1<<Channel))) continue;
#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        InImage(Col,Row) = ToFloatTable[Image->m_Image[Row*Width+Col][Channel]];
      }
    }

    for (int i=0;i<Iterations;i++)
      Image_filter::fast_LBF(InImage,InImage,
           Sigma_s,Sigma_r,
           1,
           &FilteredImage,&FilteredImage);

#pragma omp parallel for default(shared) schedule(static)
    for (uint16_t Row=0; Row<Image->m_Height; Row++) {
      for (uint16_t Col=0; Col<Image->m_Width; Col++) {
        Image->m_Image[Row*Width+Col][Channel] = CLIP((int32_t)(FilteredImage(Col,Row)*0xffff));
      }
    }
  }
}

void ptFastBilateralLayer(uint16_t *Layer,
                          const uint16_t Width,
                          const uint16_t Height,
                          const float Sigma_s,
                          const float Sigma_r,
                          const short Iterations) {

  image_type InImage(Width,Height);
  image_type FilteredImage(Width,Height);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      InImage(Col,Row) = ToFloatTable[Layer[Row*Width+Col]];
    }
  }

  for (int i=0;i<Iterations;i++)
    Image_filter::fast_LBF(InImage,InImage,
         Sigma_s,Sigma_r,
         1,
         &FilteredImage,&FilteredImage);

#pragma omp parallel for default(shared) schedule(static)
  for (uint16_t Row=0; Row<Height; Row++) {
    for (uint16_t Col=0; Col<Width; Col++) {
      Layer[Row*Width+Col] = CLIP((int32_t)(FilteredImage(Col,Row)*0xffff));
    }
  }
}
