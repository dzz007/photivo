################################################################################
##
## Photivo
##
## Copyright (C) 2008 Jos De Laender
## Copyright (C) 2010-2012 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011-2013 Bernd Schoeler <brother.john@photivo.org>
##
## This file is part of Photivo.
##
## Photivo is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 3
## as published by the Free Software Foundation.
##
## Photivo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################################
#
# This is a Qt project file for Photivo.
# All Photivo project files are heavily tuned.
# Do not overwrite any with "qmake -project"!
#
################################################################################

TEMPLATE = subdirs
CONFIG  += silent

# When compiler is GCC check for at least version 4.6
*g++* {
  GCCVer = $$system($$QMAKE_CXX --version)
  contains(GCCVer,[0-3]\\.[0-9]+.*) {
    error("At least GCC 4.6 is required to build Photivo.")
  } else {
    contains(GCCVer,4\\.[0-5].*) {
      error("At least GCC 4.6 is required to build Photivo.")
    }
  }
}

# Check for qmake version
contains($$[QMAKE_VERSION],^2*) {
  message("Cannot build Photivo with qmake version $$[QMAKE_VERSION].")
  error("Use qmake from Qt4 or newer.")
}

# Check for Qt version
contains(QT_VERSION, ^4\\.[0-5]\\..*) {
  message("Cannot build Photivo with Qt version $${QT_VERSION}.")
  error("Use at least Qt 4.6.")
}

# Remove subproject makefiles to make sure they are created again with current settings
win32 {
  DEVNULL="1>nul 2>nul"
} else {
  DEVNULL="1>/dev/null 2>/dev/null"
}

system("rm --help $$DEVNULL") {
  # "rm" is available (Linux, Mac or MSys)
  system(rm -f $$OUT_PWD/Makefile*)
  system(rm -f $$OUT_PWD/photivoProject/Makefile*)
  system(rm -f $$OUT_PWD/ptClearProject/Makefile*)
  system(rm -f $$OUT_PWD/ptCreateAdobeProfilesProject/Makefile*)
  system(rm -f $$OUT_PWD/ptGimpProject/Makefile*)
} else {
  # "rm" does not exist: Windows cmd shell
  system(del /f /q \"$$shell_path($$OUT_PWD/Makefile)*\" $$DEVNULL)
  system(del /f /q \"$$shell_path($$OUT_PWD/photivoProject/Makefile)*\" $$DEVNULL)
  system(del /f /q \"$$shell_path($$OUT_PWD/ptClearProject/Makefile)*\" $$DEVNULL)
  system(del /f /q \"$$shell_path($$OUT_PWD/ptCreateAdobeProfilesProject/Makefile)*\" $$DEVNULL)
  system(del /f /q \"$$shell_path($$OUT_PWD/ptGimpProject/Makefile)*\" $$DEVNULL)
}

#------------------------------------------------------------------------------
# --- Configure subprojects to build. Photivo itself is always included. ---

BUILD_ADOBE=no
BUILD_GIMP=no
BUILD_CLEAR=no

CONFIG(WithAdobeProfiles) {
  SUBDIRS += ptCreateAdobeProfilesProject
  BUILD_ADOBE=yes
}
CONFIG(WithGimp) {
  SUBDIRS += ptGimpProject
  BUILD_GIMP=yes
}
!CONFIG(WithoutClear) {
  SUBDIRS += ptClearProject
  BUILD_CLEAR=yes
}
SUBDIRS += photivoProject

system(echo "------------------------------------------------------")
system(echo "Use 'CONFIG-=debug' to build release version")
system(echo "------------------------------------------------------")
system(echo "Following options are currently supported:")
system(echo "'CONFIG+=WithAdobeProfiles'")
system(echo "'CONFIG+=WithGimp'")
system(echo "'CONFIG+=WithoutClear'")
system(echo "------------------------------------------------------")

CONFIG(debug) {
  system(echo "Build debug: yes")
} else {
  system(echo "Build debug: no")
}
CONFIG(release) {
  system(echo "Build release: yes")
} else {
  system(echo "Build release: no")
}
system(echo "------------------------------------------------------")


system(echo "Build Photivo                : yes")
system(echo "Build ptClear                : $${BUILD_CLEAR}")
system(echo "Build Gimp plugin            : $${BUILD_GIMP}")
#system(echo "Build curves creator         : no - obsolete")
system(echo "Build Adobe profiles creator : $${BUILD_ADOBE}")

unix {
  SYSTEM_CIMG=no
  CONFIG(WithSystemCImg) {
    SYSTEM_CIMG=yes
  }
  system(echo "Use system CImg              : $${SYSTEM_CIMG}")
}

#------------------------------------------------------------------------------

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

#------------------------------------------------------------------------------
# --- setup for "make install" ---

unix {
  # Did I mention that sometimes i *HATE* qmake!?
  # Qmake checks for the existence of files before creating the rules for make install.
  # Obviously in a fresh build folder there are no binaries present. So the rules
  # are not created and the binaries not installed. Great! Let's hack around it
  # and create dummy "binaries" when the files are not present. Now qmake is happy
  # and we get our binaries installed properly in any case.
  # Yes, I know the cause of the problem is our custom output path for the executables.
  # That doesn't make it less annoying.
  !exists($$OUT_PWD/photivo) {
    system(touch $$OUT_PWD/photivo)
  }
  !exists($$OUT_PWD/ptClear) {
    system(touch $$OUT_PWD/ptClear)
  }
  binaries.path       = $${PREFIX}/bin
  binaries.files      = $$OUT_PWD/photivo $$OUT_PWD/ptClear
  INSTALLS           += binaries

  shortcut.path       = $${PREFIX}/share/applications
  shortcut.files      = ReferenceMaterial/photivo.desktop
  INSTALLS           += shortcut

  shortcut2.path      = ~/.local/share/applications
  shortcut2.files     = ReferenceMaterial/photivo.desktop
  INSTALLS           += shortcut2

  icon.path           = $${PREFIX}/share/pixmaps
  icon.files          = qrc/photivo-appicon.png
  INSTALLS           += icon

  curves.path         = $${PREFIX}/share/photivo/Curves
  curves.files        = Curves/*
  INSTALLS           += curves

  mixer.path          = $${PREFIX}/share/photivo/ChannelMixers
  mixer.files         = ChannelMixers/*
  INSTALLS           += mixer

  presets.path        = $${PREFIX}/share/photivo/Presets
  presets.files       = Presets/*
  INSTALLS           += presets

  profiles.path       = $${PREFIX}/share/photivo/Profiles
  profiles.files      = Profiles/*
  INSTALLS           += profiles

  translations.path   = $${PREFIX}/share/photivo/Translations
  translations.files  = Translations/*
  INSTALLS           += translations

  lensfun.path        = $${PREFIX}/share/photivo/LensfunDatabase
  lensfun.files       = LensfunDatabase/*
  INSTALLS           += lensfun

  uisettings.path     = $${PREFIX}/share/photivo/UISettings
  uisettings.files    = UISettings/*
  themes.path         = $${PREFIX}/share/photivo/Themes
  themes.files        = Themes/*
  INSTALLS           += uisettings
  INSTALLS           += themes
}
