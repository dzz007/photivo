////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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

#ifndef DLCHANNELMIXER_H
#define DLCHANNELMIXER_H

#include "ptDefines.h"
#include "ptConstants.h"

////////////////////////////////////////////////////////////////////////////////
//
// Class containing a mixer and its operations.
//
////////////////////////////////////////////////////////////////////////////////

class ptChannelMixer {
public :

double m_Mixer[3][3]; // [To][From]

// Reading and writing function (compatible).
// Header is a free text that is inserted as comment to describe
// the mixer.
short WriteChannelMixer(const char* FileName,const char *Header = NULL);
short ReadChannelMixer(const char* FileName);

// Constructor
ptChannelMixer();

// Destructor
~ptChannelMixer();
};

// Our instantiation at toplevel
extern ptChannelMixer* ChannelMixer;

#endif

////////////////////////////////////////////////////////////////////////////////
