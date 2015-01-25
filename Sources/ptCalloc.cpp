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

#include "ptCalloc.h"

#include <QtCore>

#include <cstdio>
#include <cstdlib>
#include <cassert>

static FILE* CallocDebugFile = NULL;

class Allocation {
  public:
  void*   Pointer;
  int     Size;
  QString FileName;
  int     LineNumber;
  const void*   ObjectPointer;
};

QList <Allocation*> Allocations;
static int Allocated = 0;

//==============================================================================

void* ptCalloc(size_t num,
               size_t size,
               const char*  FileName,
               const int    LineNumber,
               const void*  ObjectPointer) {

  if (!CallocDebugFile) {
    CallocDebugFile = fopen("CallocDebug.txt","w");
    assert(CallocDebugFile);
  }

  void* RV = calloc(num,size);

  Allocation* TheAllocation = new Allocation;
  TheAllocation->Pointer       = RV;
  TheAllocation->Size          = num*size;
  TheAllocation->FileName      = FileName;
  TheAllocation->LineNumber    = LineNumber;
  TheAllocation->ObjectPointer = ObjectPointer;
  Allocations.append(TheAllocation);
  Allocated += TheAllocation->Size;

  fprintf(CallocDebugFile,
          "Calloc : %p\n  %10s : %d\n  %10s : %s\n  %10s : %d\n  %10s : %p\n\n",
          RV,
          "Nr Bytes",(int) (num*size),
          "Filename",FileName,
          "LineNumber",LineNumber,
          "Object",ObjectPointer);
  fprintf(CallocDebugFile,"Total allocated : %d\n",Allocated);
  return RV;
}

//==============================================================================

void* ptCalloc_Ex(size_t num,
                  size_t size) {
  void* RV = calloc(num,size);
  if (RV == 0) {
    throw std::bad_alloc();
  }
  return RV;
}

//==============================================================================

void  ptFree(void* Ptr,
             const char* FileName,
             const int   LineNumber,
             const void* ObjectPointer) {

  if (!CallocDebugFile) {
    CallocDebugFile = fopen("CallocDebug.txt","w");
    assert(CallocDebugFile);
  }

  free(Ptr);
  if (!Ptr) return;

  fprintf(CallocDebugFile,
          "Free : %p\n  %10s : %s\n  %10s : %d\n  %10s : %p\n\n",
          Ptr,
          "Filename",FileName,
          "LineNumber",LineNumber,
          "Object",ObjectPointer);

  short PtrFound = 0;
  int   IdxFound;
  Allocation* TheAllocation = NULL;
  for (IdxFound=0; IdxFound<Allocations.size(); IdxFound++) {
    TheAllocation = Allocations.at(IdxFound);
    if (TheAllocation->Pointer == Ptr) {
      PtrFound = 1;
      break;
    }
  }
  assert(PtrFound);
  Allocated -= TheAllocation->Size;
  Allocations.removeAt(IdxFound);
  delete TheAllocation;

  fprintf(CallocDebugFile,"Total allocated : %d\n\n",Allocated);
}

//==============================================================================

void ptAllocated(const int   MinimumToShow,
                 const char* FileName,
                 const int   LineNumber) {

  if (!CallocDebugFile) {
    CallocDebugFile = fopen("CallocDebug.txt","w");
    assert(CallocDebugFile);
  }

  fprintf(CallocDebugFile,
          "(%s,%d) Total allocated : %d\n\n",
          FileName,LineNumber,Allocated);
  printf( "(%s,%d) Total allocated : %d\n\n",
          FileName,LineNumber,Allocated);

  for (int i=0; i<Allocations.size(); i++) {
    Allocation* TheAllocation = Allocations.at(i);
    if (TheAllocation->Size > MinimumToShow) {
      fprintf(CallocDebugFile,
              "Block of %d bytes with pointer %p\n"
              "  Allocated at %s,%d (%p object)\n\n",
              TheAllocation->Size,
              TheAllocation->Pointer,
              TheAllocation->FileName.toLocal8Bit().data(),
              TheAllocation->LineNumber,
              TheAllocation->ObjectPointer);
    }
  }
}

//==============================================================================
