////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ptError.h"

////////////////////////////////////////////////////////////////////////////////
//
// ptMemoryError
//
// Checks if the memory allocation for Ptr succeeded and abort otherwise.
//
////////////////////////////////////////////////////////////////////////////////

void ptMemoryError(void        *Ptr,
                   const char  *FileName,
                   const int   Line) {
  if (Ptr) return;
  fprintf(stderr,"Memory allocation error at %s : %d\n",FileName,Line);
  exit(EXIT_FAILURE);
}

// Global ErrNo available for application.
int ptErrNo;
int ptWarNo;
// Global ErrorMessage.
char ptErrorMessage[1024];
char ptWarningMessage[1024];

////////////////////////////////////////////////////////////////////////////////
//
// ptLogError
//
// Log an error, set the ptErrno accordingly but dont bail out.
// Format,.. is printf like.
//
////////////////////////////////////////////////////////////////////////////////

void ptLogError(const short ErrorCode,
                const char* Format,
                ... ) {
  va_list ArgPtr;
  va_start(ArgPtr,Format);
  vfprintf(stderr,Format,ArgPtr);
  vsnprintf(ptErrorMessage,1024,Format,ArgPtr);
  va_end(ArgPtr);
  fprintf(stderr,"\n");
  ptErrNo = ErrorCode;
}

////////////////////////////////////////////////////////////////////////////////
//
// ptLogWarning
//
// Log an warning, set the ptWarNo accordingly but dont bail out.
// Format,.. is printf like.
//
////////////////////////////////////////////////////////////////////////////////

void ptLogWarning(const short WarningCode,
                  const char* Format,
                  ... ) {
  va_list ArgPtr;
  va_start(ArgPtr,Format);
  vfprintf(stderr,Format,ArgPtr);
  vsnprintf(ptWarningMessage,1024,Format,ArgPtr);
  va_end(ArgPtr);
  ptWarNo = WarningCode;
}

////////////////////////////////////////////////////////////////////////////////
