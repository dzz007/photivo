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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptDefines.h"

#ifndef DLERROR_H
#define DLERROR_H

// Fatal memory error.
void ptMemoryError(void *Ptr,const char *FileName,const int Line);

// Other errors logged.
void ptLogError(const short ErrorCode,const char* Format, ... );

void ptLogWarning(const short WarningCode,const char* Format, ... );

// Access to error.
extern int  ptErrNo;
extern char ptErrorMessage[1024];

#endif

////////////////////////////////////////////////////////////////////////////////
