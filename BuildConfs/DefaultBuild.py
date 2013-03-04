##################################################################################
##
## Photivo
##
## Copyright (C) 2013 Jos De Laender <jos@de-laender.be>
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

# This is a file that should be in python syntax and drives the build
# of photivo.

# Make a copy (f.i. MyBuild.py) if you want to tweak settings and then
# use that copy for driving the build by issuing on the toplevel :
# ./scons.py -Q -ptBuildConfFile=BuildConfs/MyBuild.py

# Put here whatever name you want to give your build.
# It's only used to create a different VariantDir in the Build directory.
# And for the base name of the log file.
#
PT_BUILD_CONF_NAME = 'Default'

# If you would want an alternative log file name.
#
# PT_LOGFILE_NAME = 'SomeName.log'

# Where the result will be installed. 
# (used to be 'PREFIX' in qmake based approach)
#
PT_INSTALL_PATH = '/usr/local'

# Whether omp is enabled. Default is True anyway.
#
PT_OMP = True

# Whether this is a release type build. 
# Set False for a debug type build.
#
PT_RELEASE = True

# Whether in windows a console should be kept when running photivo.
#
PT_WITH_CONSOLE = False

# Whether the file manager should be built-in.
#
PT_WITH_FILEMGR = True

# Whether to use the System CIMG (TODO , just found in the .pro files)
#
PT_WITH_SYSTEMCIMG = False

# XXX JDLA TODO Now just coming from the linux build page.
# When True, the presence of gimp-2.0 dev package is checked, but nothing else.
# I guess this has to see with WithGimp but still TODO
#
PT_WITH_GIMPPLUGIN = False

# Where to find the QT4 installation if it is not default.
# Note that when uncommenting f.i. next line, qmake is expected to be at :
# /usr/local/Trolltech/Qt-4.7.0/bin/qmake
# This is consistent with the way Qt is built if you build it yourself.
#
# QT4DIR = '/usr/local/Trolltech/Qt-4.7.0'

# Comment out if you want to define an 'alternative' gcc/g++ to be used.
# System default otherwise.
#
# CC  = '/opt/gcc-4.7.0/bin/gcc-4.7.0'
# CXX = '/opt/gcc-4.7.0/bin/g++-4.7.0'

# Future. If we want to play with install modes.
#
PT_INSTALL_MODE = 'Original'

# Add or extend the PKG_CONFIG_PATH. Likely not needed.
#
# PGK_CONFIG_PATH = '/some/path'

###############################################################################
#
# Below this line is probably only of interest for cross compilations.
#
###############################################################################

# Host platform. If not automatically detected. 
# 'win32' or 'posix'
#
# PT_HOST_PLATFORM = 'posix'

# Target platform. If not automatically detected. 
# 'win32' or 'posix'
#
# PT_TARGET_PLATFORM = 'posix'

# Prefix used for cross-compiling tools.
#
# PT_CROSS = 'i686-pc-mingw32-'

# Where to find the gcc/g++ and bin tools.
#
# PT_TOOLS_DIR = '/home/jos/SoftwareUnderDevelopment/mxe/usr/bin'
