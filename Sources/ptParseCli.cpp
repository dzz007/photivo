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
#include <iostream>
using namespace std;

struct Entities {
  QString   Filename;
  QString   PtsFilename;
  QString   Sidecar;
  int       LoadFile;
  int       JobMode;
  int       DelInputFile;
  int       NewInstance;
  int       NoOpenFileMgr;
  bool      ShowHelp;
};


ptCliCommands ParseCli(int argc, char *argv[]) {
  Entities cli = Entities();
  ptCliCommands result = ptCliCommands();

  if (argc == 1) {
    return result;
  }

  QStringList params;
  params << "-i" << "-j" << "--load-and-delete" << "--pts" << "--new-instance" << "--no-fmgr" << "-p"
         << "-h" << "--help" << "-help" << "--sidecar";

  int i = 1;
  bool MustBeFilename = false;
  bool MustBePtsName  = false;
  bool MustBeSidecar  = false;
  while (i < argc) {
    QString current = QString::fromLocal8Bit(argv[i]);
    int whichParam = params.indexOf(current.toLower());

    if (MustBeFilename) {
      if (whichParam > -1 || cli.Filename != "") {
        cli.ShowHelp = true;
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
        cli.ShowHelp = true;
        break;
      } else {
        cli.PtsFilename = current;
        MustBePtsName = false;
        i++;
        continue;
      }
    }

    if (MustBeSidecar) {
      if (whichParam > -1 || cli.Sidecar != "") {
        cli.ShowHelp = true;
        break;
      } else {
        cli.Sidecar = current;
        MustBeSidecar = false;
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
    } else if (whichParam == 5 || whichParam == 6) {
      cli.NoOpenFileMgr++;
    } else if (whichParam == 7 || whichParam == 8 || whichParam == 9) {
      cli.ShowHelp = true;
      break;
    } else if (whichParam == 10) { // --sidecar
      MustBeSidecar = true;
    } else if (whichParam == -1) {  // can only be image file without -i param
      if (QFileInfo(current).suffix().toLower() == "pts") {
        MustBePtsName = true;
      } else {
        cli.LoadFile++;
        MustBeFilename = true;
      }
      continue;
    }

    if ((MustBeFilename || MustBePtsName || MustBeSidecar) && (i >= argc - 1)) {
      cli.ShowHelp = true;
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
    result.Filename      = cli.Filename;
    result.PtsFilename   = cli.PtsFilename;
    result.NewInstance   = cli.NewInstance > 0;
    result.NoOpenFileMgr = cli.NoOpenFileMgr;
    result.Sidecar       = cli.Sidecar;

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
