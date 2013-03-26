#!/usr/bin/env python

################################################################################
##
## Photivo
##
## Copyright (C) 2013 Sergey Salnikov <salsergey@gmail.com>
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
# This script generates CMakeLists.txt file. The script extracts all
# sources, headers and UIs from the photivoProject/photivoProject.pro
# file and adds them to CMakeLists.txt.in.
#
################################################################################

import sys
import os.path
import re

# Function to find if the source should be added.
def test_source(filename):
  if filename.endswith('cpp') and not re.match('.*qtlockedfile.*', filename):
    return True
  else:
    return False

# Function to find if the header file should be MOCed.
def test_header(filename):
  file = open(filename)
  for line in file:
    if re.match('.*Q_OBJECT.*', line):
      return True
  return False

# Function that extracts the path to a file.
# The returned value means if the file list continues.
def match_to_path(files, line, test_function=None):
  if line.endswith('\\'):
    result = True
  else:
    result = False
  if not re.match('^#', line):
    line = re.split('\\\$', line)[0].strip()
    if re.match('.*\.\./', line):
      line = re.split('\.\./', line)[1]
      if test_function == None or test_function(line):
        files.append(line)
  return result


# set the working directory to that containing this script
os.chdir(os.path.dirname(sys.argv[0]))

if not os.path.exists('CMakeLists.txt.in'):
  print 'File CMakeLists.txt.in doesn\'t exist.'
  exit(1)

if not os.path.exists('photivoProject/photivoProject.pro'):
  print 'File photivoProject/photivoProject.pro doesn\'t exist.'
  exit(1)

cmake_in = open('CMakeLists.txt.in', 'r')
qmake_pro = open('photivoProject/photivoProject.pro', 'r')
cmake_out = open('CMakeLists.txt', 'w')

sources = []
headers = []
uis = []
skip = False
copy_src = False
copy_hdr = False
copy_ui = False


for line in qmake_pro:
  line = line.strip()
# these lines correspond to win32 only and we skip them
  if re.match('win32', line):
    skip = True
# the end of the win32 section 
  if re.match('}', line):
    skip = False
  if skip:
    continue

# sources section found
  if re.match('SOURCES', line):
    copy_src = True
  if copy_src:
    copy_src = match_to_path(sources, line, test_source)
    continue

# headers section found
  if re.match('HEADERS', line):
    copy_hdr = True
  if copy_hdr:
    copy_hdr = match_to_path(headers, line, test_header)
    continue

# forms section found
  if re.match('FORMS', line):
    copy_ui = True
  if copy_ui:
    copy_ui = match_to_path(uis, line)
    continue


for line in cmake_in:
  cmake_out.write(line)

# sources section found
  if re.match('^set\( photivo_SRCS', line):
    cmake_out.write('     ' + '\n     '.join(sources))

# headers section found
  if re.match('^set\( photivo_MOC_HDRS', line):
    cmake_out.write('     ' + '\n     '.join(headers))

# forms section found
  if re.match('^set\( photivo_UI_HDRS', line):
    cmake_out.write('     ' + '\n     '.join(uis))


cmake_in.close()
qmake_pro.close()
cmake_out.close()
