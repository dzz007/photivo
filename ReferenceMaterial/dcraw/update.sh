#!/bin/bash

# Download dcraw.c
# and process it to simplify Photivo update.
# To check against former versions of dcraw, get the dcraw.c,v file
# and use 'co -r1.439 dcraw.c,v'.
# The Adobe coefficients go into ptAdobeTable.h.
# Static variables go into class DcRaw, this is currently not fully done.

wget http://www.cybercom.net/~dcoffin/dcraw/dcraw.c -O "dcraw.c"

FILE="dcraw.c"

# replace with sed -i 's/old/new/g' dcraw.c
sed -i 's/uchar/uint8_t/g' $FILE
sed -i 's/ushort/uint16_t/g' $FILE
sed -i 's/fread/ptfread/g' $FILE
sed -i 's/image/m_Image/g' $FILE
sed -i 's/raw_width/m_RawWidth/g' $FILE
sed -i 's/raw_height/m_RawHeight/g' $FILE
sed -i 's/height/m_Height/g' $FILE
sed -i 's/width/m_Width/g' $FILE
sed -i 's/top_margin/m_TopMargin/g' $FILE
sed -i 's/left_margin/m_LeftMargin/g' $FILE
sed -i 's/order/m_ByteOrder/g' $FILE
sed -i 's/ifp/m_InputFile/g' $FILE
sed -i 's/load_raw/m_LoadRawFunction/g' $FILE
sed -i 's/make/m_CameraMake/g' $FILE
sed -i 's/model/m_CameraModel/g' $FILE
sed -i 's/filters/m_Filters/g' $FILE
sed -i 's/is_raw/m_IsRaw/g' $FILE
sed -i 's/flip/m_Flip/g' $FILE
sed -i 's/data_offset/m_Data_Offset/g' $FILE
sed -i 's/shot_select/m_UserSetting_ShotSelect/g' $FILE
sed -i 's/shutter/m_Shutter/g' $FILE
sed -i 's/maximum/m_WhiteLevel/g' $FILE
sed -i 's/cblack/m_CBlackLevel/g' $FILE
sed -i 's/black/m_BlackLevel/g' $FILE
sed -i 's/colors/m_Colors/g' $FILE
sed -i 's/pixel_aspect/m_PixelAspect/g' $FILE
sed -i 's/load_flags/m_Load_Flags/g' $FILE
sed -i 's/zero_is_bad/m_ZeroIsBad/g' $FILE
sed -i 's/pre_mul/m_D65Multipliers/g' $FILE
sed -i 's/FORC3/for (c=0; c<3; c++)/g' $FILE
sed -i 's/FORC4/for (c=0; c<4; c++)/g' $FILE
