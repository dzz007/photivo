/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

extern "C"
{
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
}

#include <cassert>
#include <cstring>

#include <QtCore>
#include <cstdlib>
#include <cstdio>

#include "ptError.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

void ptQuery();
void ptRun(const gchar *name,
     gint nparams,
     const GimpParam *param,
     gint *nreturn_vals,
     GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_procedure */
    NULL,  /* quit_procedure */
    ptQuery, /* query_procedure */
    ptRun,   /* run_procedure */
};

MAIN()

void ptQuery() {

  printf("(%s,%d) '%s'\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);

  static const GimpParamDef SendToGimpArguments[] = {
    { GIMP_PDB_INT32,
      (gchar*) "run_mode",
      (gchar*) "Interactive, non-interactive" },
    { GIMP_PDB_STRING,
      (gchar*) "filename",
      (gchar*) "The name of the send to gimp descriptor" },
    { GIMP_PDB_STRING,
      (gchar*) "raw_filename",
      (gchar*) "The name of the send to gimp descriptor" },
  };

  static const GimpParamDef SendToGimpReturnValues[] = {
    { GIMP_PDB_IMAGE,
      (gchar*) "image",
      (gchar*) "Output image" },
  };

  gimp_install_procedure("photivoSendToGimp",
                   "photivo send to gimp loader",
                   "photivo send to gimp loader",
                   "Jos De Laender, Michael Munzert",
                   "Copyright 2009 by Jos De Laender\n Copyright 2010 by Michael Munzert",
                   "April 5, 2009",
                   // No menu path required in this case.
                   "photivo",
                   // image types
                   NULL,
                   // GimpPDBProcType
                   GIMP_PLUGIN,
                   G_N_ELEMENTS(SendToGimpArguments),
                   G_N_ELEMENTS(SendToGimpReturnValues),
                   SendToGimpArguments,
                   SendToGimpReturnValues);

  gimp_register_load_handler("photivoSendToGimp",(char *)"ptg","");
}

////////////////////////////////////////////////////////////////////////////////
//
void ptRun(const gchar*     Name,
           gint       NrParameters,
           const GimpParam* Parameter,
           gint *nreturn_vals,
           GimpParam **return_vals) {
  printf("(%s,%d) '%s'\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
  printf("Name         : '%s'\n",Name);
  printf("NrParameters : %d\n",NrParameters);
  if (!strcmp(Name,"photivoSendToGimp")) {
    printf("RunMode      : %d\n",Parameter[0].data.d_int32);
    printf("FileName1    : '%s'\n",Parameter[1].data.d_string);
    printf("FileName2    : '%s'\n",Parameter[2].data.d_string);

    QFile GimpFile(Parameter[1].data.d_string);
    assert(GimpFile.open(QIODevice::ReadOnly | QIODevice::Text));

    QTextStream In(&GimpFile);

    QString ImageFileName = In.readLine();
    QString ExifFileName  = In.readLine();
    QString ICCFileName   = In.readLine();

    // Read image
    FILE *InputFile = fopen(ImageFileName.toAscii().data(),"rb");
    if (!InputFile) {
      ptLogError(1,ImageFileName.toAscii().data());
      return; // ptError_FileOpen;
    }

    short    Colors;
    unsigned short Width;
    unsigned short Height;
    unsigned short BitsPerColor;
    char     Buffer[128];

    // Extremely naive. Probably just enough for testcases.
    assert ( fgets(Buffer,127,InputFile) );
    assert ( 1 == sscanf(Buffer,"P%hd",&Colors) );
    assert(Colors == 6 );
    do {
      assert ( fgets(Buffer,127,InputFile) );
    } while (Buffer[0] == '#');
    sscanf(Buffer,"%hd %hd",&Width,&Height);
    assert ( fgets(Buffer,127,InputFile) );
    sscanf(Buffer,"%hd",&BitsPerColor);
    assert(BitsPerColor == 0xffff);

    Colors = 3;

    unsigned short (* ImageForGimp)[3] =
      (unsigned short (*)[3]) CALLOC(Width*Height,sizeof(*ImageForGimp));
    ptMemoryError(ImageForGimp,__FILE__,__LINE__);

    unsigned short*  PpmRow = (unsigned short *) CALLOC(Width*Height,sizeof(*PpmRow));
    ptMemoryError(PpmRow,__FILE__,__LINE__);

    for (unsigned short Row=0; Row<Height; Row++) {
      size_t RV = fread(PpmRow,Colors*2,Width,InputFile);
      if (RV != (size_t) Width) {
        printf("ReadPpm error. Expected %d bytes. Got %d\n",Width,(int)RV);
        exit(EXIT_FAILURE);
      }
      if (htons(0x55aa) != 0x55aa) {
        swab((char *)PpmRow,(char *)PpmRow,Width*Colors*2);
      }
      for (unsigned short Col=0; Col<Width; Col++) {
        for (short c=0;c<3;c++) {
          ImageForGimp[Row*Width+Col][c] = PpmRow[Col*Colors+c];
        }
      }
    }

    FREE(PpmRow);
    FCLOSE(InputFile);

    QFile ExifFile(ExifFileName);
    assert(ExifFile.open(QIODevice::ReadOnly));
    qint64 FileSize = ExifFile.size();
    QDataStream ExifIn(&ExifFile);
    char* ExifBuffer = (char *) MALLOC(FileSize);
    ptMemoryError(ExifBuffer,__FILE__,__LINE__);
    unsigned ExifBufferLength = ExifIn.readRawData(ExifBuffer,FileSize);
    ExifFile.close();

    QFile ICCFile(ICCFileName);
    assert(ICCFile.open(QIODevice::ReadOnly));
    qint64 FileSize2 = ICCFile.size();
    QDataStream ICCIn(&ICCFile);
    char* ICCBuffer = (char *) MALLOC(FileSize2);
    ptMemoryError(ICCBuffer,__FILE__,__LINE__);
    unsigned ICCBufferLength = ICCIn.readRawData(ICCBuffer,FileSize2);
    ICCFile.close();

    // And now copy to gimp.
    gint32 GimpImage = gimp_image_new(Width,
                                      Height,
                                      GIMP_RGB);
    assert (GimpImage != -1);
    gint32 GimpLayer = gimp_layer_new(GimpImage,
                                      "BG",
                                      Width,
                                      Height,
                                      GIMP_RGB_IMAGE,
                                      100.0,
                                      GIMP_NORMAL_MODE);
#if GIMP_MINOR_VERSION<=6
    gimp_image_add_layer(GimpImage,GimpLayer,0);
#else
    gimp_image_insert_layer(GimpImage,GimpLayer,NULL,0);
#endif
    GimpDrawable* Drawable = gimp_drawable_get(GimpLayer);
    GimpPixelRgn PixelRegion;
    gimp_pixel_rgn_init(&PixelRegion,
                        Drawable,
                        0,
                        0,
                        Drawable->width,
                        Drawable->height,
                        true,
                        false);
    unsigned short TileHeight = gimp_tile_height();
    for (unsigned short Row=0; Row<Height; Row+=TileHeight) {
      unsigned short NrRows = MIN(Height-Row,TileHeight);
      guint8* Buffer = g_new(guint8,TileHeight*Width*3);
      for (unsigned short i=0; i<NrRows; i++) {
        for (unsigned short j=0; j<Width; j++) {
          for (short c=0;c<3;c++) {
            Buffer[3*(i*Width+j)+c] =
              ImageForGimp[(Row+i)*Width+j][c]>>8;
          }
        }
      }
      gimp_pixel_rgn_set_rect(&PixelRegion,
                              Buffer,
                              0,
                              Row,
                              Width,
                              NrRows);
      g_free(Buffer);
    }
    gimp_drawable_flush(Drawable);
    gimp_drawable_detach(Drawable);
    FREE(ImageForGimp);
    GimpParasite* GimpExifData = gimp_parasite_new("exif-data",
                                                   GIMP_PARASITE_PERSISTENT,
                                                   ExifBufferLength,
                                                   ExifBuffer);
    gimp_image_parasite_attach(GimpImage,GimpExifData);
    gimp_parasite_free(GimpExifData);
    FREE(ExifBuffer);

    GimpParasite* GimpICCData = gimp_parasite_new("icc-profile",
                                                   GIMP_PARASITE_PERSISTENT,
                                                   ICCBufferLength,
                                                   ICCBuffer);
    gimp_image_parasite_attach(GimpImage,GimpICCData);
    gimp_parasite_free(GimpICCData);
    FREE(ICCBuffer);

    static GimpParam Values[2];
    *nreturn_vals = 2;
    *return_vals = Values;
    Values[0].type = GIMP_PDB_STATUS;
    Values[0].data.d_status = GIMP_PDB_SUCCESS;
    Values[1].type = GIMP_PDB_IMAGE;
    Values[1].data.d_image = GimpImage;

    QFile::remove(ImageFileName);
    QFile::remove(ExifFileName);
    QFile::remove(ICCFileName);
    QFile::remove(Parameter[1].data.d_string);

  }
}
