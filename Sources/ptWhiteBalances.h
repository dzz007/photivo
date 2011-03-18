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

#ifndef DLWHITEBALANCES_H
#define DLWHITEBALANCES_H

#include <QString>

class ptWhiteBalance {
public:
const QString m_Make;
const QString m_Model;
const QString m_Name;
const double  m_Multipliers[4];
};

extern const ptWhiteBalance ptWhiteBalances[];

#endif
////////////////////////////////////////////////////////////////////////////////
