////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2009 Michael Munzert <mail@mm-log.com>
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

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <cmath>

#include "ptDefines.h"
#include "ptChannelMixer.h"
#include "ptError.h"

////////////////////////////////////////////////////////////////////////////////
//
// Elementary constructor.
// Sets the mixer to unity
//
////////////////////////////////////////////////////////////////////////////////

ptChannelMixer::ptChannelMixer() {
  for (short i=0; i<3; i++) {
    for (short j=0; j<3; j++) {
      m_Mixer[i][j] = (i==j)?1.0:0.0;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptChannelMixer::~ptChannelMixer() {
}

////////////////////////////////////////////////////////////////////////////////
//
// A WriteChannelMixer function. Naive, but should do !
//
////////////////////////////////////////////////////////////////////////////////

short ptChannelMixer::WriteChannelMixer(const char *FileName,
                                        const char *Header) {

  FILE *OutFile = fopen(FileName,"wb");
  if (!OutFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }
  if (Header) {
    fprintf(OutFile,"%s",Header);
  } else {
    fprintf(OutFile,";\n; photivo ChannelMixer File\n; Generated by photivo\n;\n");
  }
  fprintf(OutFile,"Magic photivoChannelMixerFile\n");
  for (short To=0; To<3; To++) {
    for (short From=0; From<3; From++) {
       fprintf(OutFile,"%d %d\n",
                       From+To*3,
                       // Expressed in 1/1000ths to avoid ',','.' related issues.
                       (int)(1000*m_Mixer[To][From]));
    }
  }
  FCLOSE(OutFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// A ReadChannelMixer function. Naive, but should do !
//
////////////////////////////////////////////////////////////////////////////////

short ptChannelMixer::ReadChannelMixer(const char *FileName) {

  FILE *InFile = fopen(FileName,"r");
  if (!InFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  char Buffer[100];
  char Key[100];
  char Value[100];
  int  LineNr = 0;
  int  NrKeys = 0;

  do {
    if (NULL == fgets(Buffer,100,InFile)) break;
    LineNr++;
    if (';' == Buffer[0]) continue;
    sscanf(Buffer,"%s %s",Key,Value);
    NrKeys++;
    if (1 == NrKeys) {
      if ((strcmp(Key,"Magic") || strcmp(Value,"photivoChannelMixerFile")) &&
          (strcmp(Key,"Magic") || strcmp(Value,"dlRawChannelMixerFile"))) {
        ptLogError(ptError_FileFormat,
                   "'%s' has wrong format at line %d\n",
                   FileName,
                   LineNr);
        return ptError_FileFormat;
      }
    } else {
      int    Value1 = strtol(Key,NULL,10);
      double Value2 = atof(Value);
      if ( Value1<0 || Value1 >8) {
        ptLogError(ptError_Argument,
                   "Error reading %s at line %d (out of range : %d)\n",
                   FileName,LineNr,Value1);
        return ptError_Argument;
      }
      // Expressed in 1/1000ths to avoid ',','.' related issues.
      m_Mixer[Value1/3][Value1%3] = Value2/1000;
    }
  } while (!feof(InFile));

  // Looks OK.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
