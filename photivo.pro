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

# Hack to clean old makefiles
unix {
  RETURN = $$system(rm ./photivoProject/Makefile 2> /dev/null)
  RETURN = $$system(rm ./ptClearProject/Makefile 2> /dev/null)
  RETURN = $$system(rm ./ptGimpProject/Makefile 2> /dev/null)
}

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

###############################################################################

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

# setup for "make install"
unix {
  QMAKE_STRIP = echo

  binaries.path       = $${PREFIX}/bin
  binaries.files      = photivo ptClear
  shortcut.path       = $${PREFIX}/share/applications
  shortcut.files      = ReferenceMaterial/photivo.desktop
  shortcut2.path      = ~/.local/share/applications
  shortcut2.files     = ReferenceMaterial/photivo.desktop
  icon.path           = $${PREFIX}/share/pixmaps
  icon.files          = qrc/photivo-appicon.png
  curves.path         = $${PREFIX}/share/photivo/Curves
  curves.files        = Curves/*
  mixer.path          = $${PREFIX}/share/photivo/ChannelMixers
  mixer.files         = ChannelMixers/*
  presets.path        = $${PREFIX}/share/photivo/Presets
  presets.files       = Presets/*
  profiles.path       = $${PREFIX}/share/photivo/Profiles
  profiles.files      = Profiles/*
  translations.path   = $${PREFIX}/share/photivo/Translations
  translations.files  = Translations/*
  lensfun.path        = $${PREFIX}/share/photivo/LensfunDatabase
  lensfun.files       = LensfunDatabase/*
  uisettings.path     = $${PREFIX}/share/photivo/UISettings
  uisettings.files    = UISettings/*
  themes.path         = $${PREFIX}/share/photivo/Themes
  themes.files        = ./Themes/*
  
  INSTALLS += binaries
  INSTALLS += shortcut
  INSTALLS += shortcut2
  INSTALLS += icon
  INSTALLS += curves
  INSTALLS += mixer
  INSTALLS += presets
  INSTALLS += profiles
  INSTALLS += translations
  INSTALLS += lensfun
  INSTALLS += uisettings
}
