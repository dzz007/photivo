#!/bin/bash

# Download ufRaw whitebalance settings from 
# http://ufraw.cvs.sourceforge.net/viewvc/ufraw/ufraw/wb_presets.c
# and process them to the corresponding file for Photivo

MYDATE=$(date +%Y%m%d)

rm wb_presets_reference_ufraw*

wget http://ufraw.cvs.sourceforge.net/viewvc/ufraw/ufraw/wb_presets.c -O "wb_presets_reference_ufraw_"$MYDATE".c"

cp "wb_presets_reference_ufraw_"$MYDATE".c" "tmp_presets"

./ConvertWbPresetsFromUfRaw.pl >> processed_data.c

rm "tmp_presets"

cat Header processed_data.c Footer >> ptWhiteBalances.cpp

rm processed_data.c
