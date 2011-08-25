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
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptImage.h"
#include "ptError.h"
#include "ptConstants.h"

void ptFastBilateralChannel(ptImage* Image,
          const float Sigma_s,
          const float Sigma_r,
          const short Iterations = 1,
          const short ChannelMask = 1);

void ptFastBilateralLayer(uint16_t *Layer,
                          const uint16_t Width,
                          const uint16_t Height,
                          const float Sigma_s,
                          const float Sigma_r,
                          const short Iterations);
