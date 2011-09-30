/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLCALLOC_H
#define DLCALLOC_H

#include <cstddef>

//==============================================================================

void* ptCalloc(size_t num,
               size_t size,
               const char*  FileName,
               const int    LineNumber,
               const void*  ObjectPointer);

//==============================================================================

void* ptCalloc_Ex(size_t num,
                  size_t size);

//==============================================================================

void  ptFree(void* Ptr,
             const char* FileName,
             const int   LineNumber,
             const void* ObjectPointer);

//==============================================================================

void  ptAllocated(const int   MinimumToShow,
                  const char* FileName,
                  const int   LineNumber);

//==============================================================================

#endif
