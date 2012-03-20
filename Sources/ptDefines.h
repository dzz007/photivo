/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLDEFINES_H
#define DLDEFINES_H

// pulls in INT32_MAX etc.
#define __STDC_LIMIT_MACROS
#include <stdint.h>

// disable the file manager
// #define PT_WITHOUT_FILEMGR

////////////////////////////////////////////////////////////////////////////////
//
// Delete an object and null the pointer
//
////////////////////////////////////////////////////////////////////////////////

template<class T>
inline void DelAndNull(T*& p) {
  delete p;
  p = nullptr;
}

//==============================================================================
// Calculate the square
template <typename T>
inline T ptSqr(const T a) { return a*a; }

////////////////////////////////////////////////////////////////////////////////
//
// Some macro's (most cannot go efficiently in functions).
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SQR
  #define SQR(x) ((x)*(x))
#endif
#undef  ABS
#undef  MIN
#undef  MAX
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM(x,0,0xffff)
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }
#define SIGN(x) ((x) == 0 ? 0 : ((x) < 0 ? -1 : 1 ))
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

////////////////////////////////////////////////////////////////////////////////
//
// Following 'functions' are macro implemented for :
//   - configurability to get them out and in.
//   - to get always a correct reference to the line number where inserted.
//
////////////////////////////////////////////////////////////////////////////////

// Uncomment me for malloc/calloc/free related leaks.
// #define DEBUG_MEMORY

#ifdef DEBUG_MEMORY

  #include "ptCalloc.h"

  #define CALLOC(Num,Size)  ptCalloc(Num,Size,__FILE__,__LINE__,this)
  #define CALLOC2(Num,Size) ptCalloc(Num,Size,__FILE__,__LINE__,NULL)
  #define MALLOC(Size)      ptCalloc(1,Size,__FILE__,__LINE__,this)
  #define MALLOC2(Size)     ptCalloc(1,Size,__FILE__,__LINE__,NULL)
  #define FREE(x)           {ptFree(x,__FILE__,__LINE__,this); x=NULL;}
  #define FREE2(x)          {ptFree(x,__FILE__,__LINE__,NULL); x=NULL;}
  #define ALLOCATED(x)      ptAllocated(x,__FILE__,__LINE__)

#else

  #define CALLOC(Num,Size)  ptCalloc_Ex(Num,Size)
  #define CALLOC2(Num,Size) ptCalloc_Ex(Num,Size)
  #define MALLOC(Size) malloc(Size)
  #define MALLOC2(Size) malloc(Size)
  // Remark free(NULL) is valid nop !
  #define FREE(x) {free(x); x=NULL; }
  #define FREE2(x) {free(x); x=NULL; }
  #define ALLOCATED(x) ;

#endif

// Close a file descriptor and put it on NULL

#define FCLOSE(x) {                                                \
  if (x) fclose(x);                                                \
  x=NULL;                                                          \
};

// A function to trace key values in the program (debug of course)

#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#define TRACEKEYVALS(x,y,z) {                                      \
  printf("(%-25s,%5d): ",__FILE__,__LINE__);                       \
  printf("%-20s: ",x);                                              \
  printf(y,z);                                                     \
  printf("\n");                                                    \
  fflush(stdout);                                                  \
}
#else
#define TRACEKEYVALS(x,y,z) ;
#endif

// TRACEMAIN does something comparably , but basically gives
// only progress info on the graphical pipe.

#define ENABLE_TRACEMAIN
#ifdef ENABLE_TRACEMAIN
#define TRACEMAIN(x,y) {                                           \
  printf("(%-25s,%5d): ",__FILE__,__LINE__);                       \
  printf(x,y);                                                     \
  printf("\n");                                                    \
  fflush(stdout);                                                  \
}
#else
#define TRACEMAIN(x,y) ;
#endif

// Debug function can be used to drop quick and dirty an image.
// w,h are the dimensions. b is the buffer of the image.
// depth is 3 or 4 (according to buffer layout).

#define DROP_IMAGE(w,h,b,depth) {                                  \
  char FileName[100];                                              \
  sprintf(FileName,"%s_%d.imagedrop",__FILE__,__LINE__);           \
  FILE *DropFile = fopen(FileName,"wb");                           \
  if (!DropFile) exit(EXIT_FAILURE);                               \
  if (fwrite(b,2*depth,w*h,DropFile) != w*h) exit(EXIT_FAILURE);   \
  fclose(DropFile);                                                \
}

#endif

////////////////////////////////////////////////////////////////////////////////
