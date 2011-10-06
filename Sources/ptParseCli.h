/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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
/*!
  \brief Command line parser for Photivo
*/

#ifndef PTPARSECLI_H
#define PTPARSECLI_H

#include <QString>

/*! Enum listing the actions that can be invoked from command line. */
enum ptCliMode {
  cliNoAction        = -1,
  cliLoadImage       = 0,
  cliLoadAndDelImage = 1,
  cliProcessJob      = 2,
  cliShowHelp        = 3
};

/*! Struct for the results of cli parse. */
struct ptCliCommands {
  ptCliMode Mode;
  QString Filename;
  QString PtsFilename;
  bool NewInstance;
};

/*! Parses the command line and returns invoked action and files to open. */
ptCliCommands ParseCli(int argc, char *argv[]);

#endif // PTPARSECLI_H
