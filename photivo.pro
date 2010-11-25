################################################################################
##
## photivo
##
## Copyright (C) 2008 Jos De Laender
## Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
##
## This file is part of photivo.
##
## photivo is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 3
## as published by the Free Software Foundation.
##
## photivo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with photivo.  If not, see <http:##www.gnu.org#licenses#>.
##
################################################################################

######################################################################
#
# This is the Qt project file for photivo.
# Don't let it overwrite by qmake -project !
# A number of settings is tuned.
#
# qmake will make a platform dependent makefile of it.
#
######################################################################

CONFIG += release silent
#CONFIG += debug
TEMPLATE = subdirs

isEmpty(PREFIX) {
  PREFIX = $$[QT_INSTALL_PREFIX]
}

# Hack to clean old makefiles
unix {
  RETURN = $$system(touch ./photivoProject/Makefile && rm ./photivoProject/Makefile)
  RETURN = $$system(touch ./ptClearProject/Makefile && rm ./ptClearProject/Makefile)
  RETURN = $$system(touch ./ptGimpProject/Makefile && rm ./ptGimpProject/Makefile)
}

SUBDIRS += photivoProject
SUBDIRS += ptCreateAdobeProfilesProject
SUBDIRS += ptCreateCurvesProject
SUBDIRS += ptGimpProject
SUBDIRS += ptClearProject

RETURN = $$system(touch ./photivoProject/config && rm ./photivoProject/config && echo $${PREFIX} >> ./photivoProject/config)

# Install
unix {
  binaries.path = $${PREFIX}/bin
  binaries.files = photivo
  binaries.files += ptClear
  curves.path = $${PREFIX}/share/photivo/Curves
  curves.files = ./Curves/*
  mixer.path = $${PREFIX}/share/photivo/ChannelMixers
  mixer.files = ./ChannelMixers/*
  presets.path = $${PREFIX}/share/photivo/Presets
  presets.files = ./Presets/*
  profiles.path = $${PREFIX}/share/photivo/Profiles
  profiles.files = ./Profiles/*
  translations.path = $${PREFIX}/share/photivo/Translations
  translations.files = ./Translations/*
  lensfun.path = $${PREFIX}/share/photivo/LensfunDatabase
  lensfun.files = ./LensfunDatabase/*
  images.path = $${PREFIX}/share/photivo/
  images.files = ./photivo.png
  images.files += ./photivoLogo.png
  images.files += ./photivoPreview.jpg
  INSTALLS += binaries
  INSTALLS += curves
  INSTALLS += mixer
  INSTALLS += presets
  INSTALLS += profiles
  INSTALLS += translations
  INSTALLS += lensfun
  INSTALLS += images
}

###############################################################################
