#!/bin/bash

# Download dcraw.c
# and process it to simplify Photivo update.
# To check against former versions of dcraw, get the dcraw.c,v file
# and use 'co -r1.439 dcraw.c,v'.
# The Adobe coefficients go into ptAdobeTable.h.
# Static variables go into class DcRaw, this is currently not fully done.

wget http://www.cybercom.net/~dcoffin/dcraw/dcraw.c -O "dcraw.c"

# replace with sed -i 's/old/new/g' dcraw.c
sed -i 's/uchar/uint8_t/g' dcraw.c
sed -i 's/ushort/uint16_t/g' dcraw.c
sed -i 's/fread/ptfread/g' dcraw.c
sed -i 's/image/m_Image/g' dcraw.c
sed -i 's/raw_width/m_RawWidth/g' dcraw.c
sed -i 's/raw_height/m_RawHeight/g' dcraw.c
sed -i 's/height/m_Height/g' dcraw.c
sed -i 's/width/m_Width/g' dcraw.c
sed -i 's/top_margin/m_TopMargin/g' dcraw.c
sed -i 's/left_margin/m_LeftMargin/g' dcraw.c
sed -i 's/order/m_ByteOrder/g' dcraw.c
sed -i 's/ifp/m_InputFile/g' dcraw.c
sed -i 's/load_raw/m_LoadRawFunction/g' dcraw.c
sed -i 's/make/m_CameraMake/g' dcraw.c
sed -i 's/model/m_CameraModel/g' dcraw.c
sed -i 's/filters/m_Filters/g' dcraw.c
sed -i 's/is_raw/m_IsRaw/g' dcraw.c
sed -i 's/flip/m_Flip/g' dcraw.c
sed -i 's/data_offset/m_Data_Offset/g' dcraw.c
sed -i 's/shot_select/m_UserSetting_ShotSelect/g' dcraw.c
sed -i 's/shutter/m_Shutter/g' dcraw.c
sed -i 's/maximum/m_WhiteLevel/g' dcraw.c
sed -i 's/cblack/m_CBlackLevel/g' dcraw.c
sed -i 's/black/m_BlackLevel/g' dcraw.c
sed -i 's/colors/m_Colors/g' dcraw.c
sed -i 's/pixel_aspect/m_PixelAspect/g' dcraw.c
sed -i 's/load_flags/m_Load_Flags/g' dcraw.c
sed -i 's/zero_is_bad/m_ZeroIsBad/g' dcraw.c
sed -i 's/pre_mul/m_D65Multipliers/g' dcraw.c
sed -i 's/FORC3/for (c=0; c<3; c++)/g' dcraw.c
sed -i 's/FORC4/for (c=0; c<4; c++)/g' dcraw.c
