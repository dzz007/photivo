################################################################################
##
## Photivo
##
## Copyright (C) 2008 Jos De Laender
## Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
## Copyright (C) 2011 Bernd Schoeler <brother.john@photivo.org>
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
CONFIG += silent

# Check for qmake version
contains($$[QMAKE_VERSION],^2*) {
  message("Cannot build Photivo with qmake version $$[QMAKE_VERSION].")
  error("Use qmake from Qt4")
}

# Check for Qt version
contains(QT_VERSION, ^4\\.[0-5]\\..*) {
  message("Cannot build Photivo with Qt version $${QT_VERSION}.")
  error("Use at least Qt 4.6.")
}

# Remove subproject makefiles to make sure they are created again with current settings
system(rm -f $$OUT_PWD/Makefile)
system(rm -f $$OUT_PWD/photivoProject/Makefile)
system(rm -f $$OUT_PWD/ptClearProject/Makefile)
system(rm -f $$OUT_PWD/ptCreateAdobeProfilesProject/Makefile)
system(rm -f $$OUT_PWD/ptCreateCurvesProject/Makefile)
system(rm -f $$OUT_PWD/ptGimpProject/Makefile)

###############################################################################

# Configure subprojects to build. Photivo itself is always included.
BUILD_ADOBE=no
BUILD_CURVES=no
BUILD_GIMP=no
BUILD_CLEAR=no

CONFIG(WithAdobeProfiles) {
  SUBDIRS += ptCreateAdobeProfilesProject
  BUILD_ADOBE=yes
}
CONFIG(WithCurves) {
  SUBDIRS += ptCreateCurvesProject
  BUILD_CURVES=yes
}
!CONFIG(WithoutGimp) {
  SUBDIRS += ptGimpProject
  BUILD_GIMP=yes
}
!CONFIG(WithoutClear) {
  SUBDIRS += ptClearProject
  BUILD_CLEAR=yes
}
SUBDIRS += photivoProject

system(echo "Build Photivo                : yes")
system(echo "Build ptClear                : $${BUILD_CLEAR}")
system(echo "Build Gimp plugin            : $${BUILD_GIMP}")
system(echo "Build curves creator         : $${BUILD_CURVES}")
system(echo "Build Adobe profiles creator : $${BUILD_ADOBE}")

unix {
  SYSTEM_CIMG=no
  CONFIG(WithSystemCImg) {
    SYSTEM_CIMG=yes
  }
  system(echo "Use system CImg              : $${SYSTEM_CIMG}")
}

###############################################################################

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

# setup for "make install"
unix {
  QMAKE_STRIP = echo

  # Did I mention that sometimes i *HATE* qmake!? Especially building out of source
  # on Linux can be a PITA!
  # Qmake checks for the existence of files before creating the rules for make install.
  # Obviously in a fresh build folder there are no binaries present. So the rules
  # are not created and the binaries not installed. Great! Let's hack around it
  # and create dummy "binaries" when the files are not present. Now qmake is happy
  # and we get our binaries installed properly in any case.
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
  INSTALLS += themes
}
