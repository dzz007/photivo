////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#include <QtCore>
#include <QtGui>

QApplication* TheApplication;

int main(int Argc, char *Argv[]) {

  //QApplication TheApplication(Argc,Argv);
  TheApplication = new QApplication(Argc,Argv);

  // User home folder
  QString UserDirectory = QDir::homePath() + QDir::separator() + ".photivo" + QDir::separator();
  QString SettingsFileName = UserDirectory + "photivo.ini";

  if (QMessageBox::warning(0,
        QObject::tr("Are you sure?"),
        QObject::tr("I'm going to clear all your settings for photivo.\nProceed?"),
        QMessageBox::Ok,QMessageBox::Cancel)==QMessageBox::Ok){
    if(QFile::remove(SettingsFileName))
      QMessageBox::information(0,"Done","Settings are removed.");
    else
      QMessageBox::information(0,"Done","Nothing to remove.");
  }

  return 0;
}


