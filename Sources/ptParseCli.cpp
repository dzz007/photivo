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

#include "ptParseCli.h"
#include <QStringList>
#include <QFileInfo>

struct Entities {
  QString   Filename;
  QString   PtsFilename;
  int       LoadFile;
  int       JobMode;
  int       DelInputFile;
  int       NewInstance;
  bool      ShowHelp;
};


ptCliCommands ParseCli(int argc, char *argv[]) {
  Entities cli = { "", "", 0, 0, 0, 0, false };
  ptCliCommands result = { cliNoAction, "", "", false };

  if (argc == 1) {
    return result;
  }

  QStringList params;
  params << "-i" << "-j" << "--load-and-delete" << "--pts" << "--new-instance" << "-h";

  int i = 1;
  bool MustBeFilename = false;
  bool MustBePtsName = false;
  while (i < argc) {
    QString current = argv[i];
    int whichParam = params.indexOf(current.toLower());

    if (MustBeFilename) {
      if (whichParam > -1 || cli.Filename != "") {
        cli.ShowHelp++;
        break;
      } else {
        cli.Filename = current;
        MustBeFilename = false;
        i++;
        continue;
      }
    }

    if (MustBePtsName) {
      if (whichParam > -1 || cli.PtsFilename != "") {
        cli.ShowHelp++;
        break;
      } else {
        cli.PtsFilename = current;
        MustBePtsName = false;
        i++;
        continue;
      }
    }

    if (whichParam == 0) {
      cli.LoadFile++;
      MustBeFilename = true;
    } else if (whichParam == 1) {
      cli.JobMode++;
      MustBeFilename = true;
    } else if (whichParam == 2) {
      cli.DelInputFile++;
      MustBeFilename = true;
    } else if (whichParam == 3) {
      MustBePtsName = true;
    } else if (whichParam == 4) {
      cli.NewInstance++;
    } else if (whichParam == 5) {
      cli.ShowHelp++;
      break;
    } else if (whichParam == -1) {  // can only be image file without -i param
      if (QFileInfo(current).suffix().toLower() == "pts") {
        MustBePtsName = true;
      } else {
        cli.LoadFile++;
        MustBeFilename = true;
      }
      continue;
    }

    if ((MustBeFilename || MustBePtsName) && (i >= argc - 1)) {
      cli.ShowHelp++;
      break;
    }

    i++;
  }


  if (cli.ShowHelp > 0 ||
      (cli.LoadFile + cli.JobMode + cli.DelInputFile > 1) ||
      ((cli.JobMode > 0 || cli.DelInputFile > 0) && (cli.PtsFilename != "")) ||
      (cli.NewInstance > 1))
  {
    result.Mode = cliShowHelp;

  } else {
    result.Filename = cli.Filename;
    result.PtsFilename = cli.PtsFilename;
    result.NewInstance = cli.NewInstance > 0;

    if (cli.LoadFile > 0) {
      result.Mode = cliLoadImage;
    } else if (cli.JobMode > 0) {
      result.Mode = cliProcessJob;
    } else if (cli.DelInputFile > 0) {
      result.Mode = cliLoadAndDelImage;
    }
  }

  return result;
}
