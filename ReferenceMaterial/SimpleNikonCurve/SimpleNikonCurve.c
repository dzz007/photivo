/***************************************************
 nikon_curve.c - read Nikon NTC/NCV files

 Copyright 2004-2007 by Shawn Freeman, Udi Fuchs

 This program reads in a Nikon NTC/NCV file,
 interperates it's tone curve, and writes out a
 simple ASCII file containing a table of interpolation
 values. See the header file for more information.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2
 as published by the Free Software Foundation. You should have received
 a copy of the license along with this program.

****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h> /* For variable argument lists */
#include "SimpleNikonCurve.h"

#ifdef __WITH_UFRAW__
    #include "uf_glib.h"
    #include "ufraw.h"
#else
    #define MAX(a,b) ((a) > (b) ? (a) : (b))
    #define MIN(a,b) ((a) < (b) ? (a) : (b))
    #define g_fopen fopen
#endif

/*************************************************
 * Internal static data
 *************************************************/

//file offsets for the different data in different file types
static const int FileOffsets[2][4] = {
    {NTC_PATCH_OFFSET,NTC_BOX_DATA, NTC_NUM_ANCHOR_POINTS, NTC_ANCHOR_DATA_START},
    {NCV_PATCH_OFFSET,NCV_BOX_DATA, NCV_NUM_ANCHOR_POINTS, NCV_ANCHOR_DATA_START},
};

//file header indicating ntc file
static const unsigned char NTCFileHeader[] = {0x9d,0xdc,0x7d,0x00,0x65,0xd4,
			0x11,0xd1,0x91,0x94,0x44,0x45,0x53,0x54,0x00,0x00};

//file header indicating an ncv file
static const unsigned char NCVFileHeader[] = {0x40,0xa9,0x86,0x7a,0x1b,0xe9,
			0xd2,0x11,0xa9,0x0a,0x00,0xaa,0x00,0xb1,0xc1,0xb7};

//This is an additional header chunk at the beginning of the file
//There are some similarities between the headers, but not enough to fully crack.
//This does not appear to change.
static const unsigned char NCVSecondFileHeader[] = {0x01,0x32,0xa4,0x76,0xa2,
			0x17,0xd4,0x11,0xa9,0x0a,0x00,0xaa,0x00,0xb1,0xc1,
			0xb7,0x01,0x00,0x05,0x00,0x00,0x00,0x01};

//This is the terminator of an NCV file. Again there are some similarites
//to other sections, but not enough for to crack what it means. However,
//it does not appear to change.
static const unsigned char NCVFileTerminator[] = {0x45,0xd3,0x0d,0x77,0xa3,0x6e,
			0x1e,0x4e,0xa4,0xbe,0xcf,0xc1,0x8e,0xb5,0xb7,0x47,
			0x01,0x00,0x05,0x00,0x00,0x00,0x01 };

//File section header. Only a one byte difference between this and an NTC file header
static const unsigned char FileSectionHeader[] = {0x9d,0xdc,0x7d,0x03,0x65,0xd4,
			0x11,0xd1,0x91,0x94,0x44,0x45,0x53,0x54,0x00,0x00};
//file type header array
static const unsigned char *FileTypeHeaders[NUM_FILE_TYPES] = {
    NTCFileHeader,
    NCVFileHeader,
};

/**STANDALONE**/
#ifdef _STAND_ALONE_

//filenames
char exportFilename[1024];
char nikonFilename[1024];

unsigned int standalone_samplingRes = 65536;
unsigned int standalone_outputRes = 256;
unsigned int program_mode = CURVE_MODE;

/*******************************************
ProcessArgs:
    Convenient function for processing the args
    for the test runner.
********************************************/
int ProcessArgs(int num_args, char *args[])
{
    exportFilename[0] = '\0';
    nikonFilename[0] = '\0';

    int i;
    for(i = 0; i < num_args; i++)
    {
	if (strcmp(args[i],"-h") == 0 || strcmp(args[i],"-H") == 0 || num_args <= 1)
	{
	    printf("NikonCurveGenerator %s %s\n",NC_VERSION, NC_DATE);
	    printf("Written by Shawn Freeman\n");
	    printf("Thanks go out to Udi Fuchs, UFRaw, and GIMP :)\n\n");
	    printf("Usage:\n");
	    printf("-nef   Specify an NEF file to get tone curve data from.\n\n");

	    //signal that processing cannot occur
	    return NC_ERROR;
	}
	else if (strcmp(args[i],"-nef") == 0)
	{
	    i++;
	    program_mode = NEF_MODE;
	    strncpy(nikonFilename, args[i], 1023);
	    nikonFilename[1023] = '\0';
	}
	//don't load argument 0
	else if (i != 0)
	{
	    //consider this the file name to load
	    strncpy(nikonFilename, args[i], 1023);
	    nikonFilename[1023] = '\0';
	}
    }

    if (strlen(exportFilename) == 0)
    {
	//set it to have a default output file name
	strncpy(exportFilename, nikonFilename, 1023);
	exportFilename[1023] = '\0';
        int StrLen = strlen(exportFilename);
        for (short i=strlen(exportFilename);exportFilename[i] != '.'; i--) {
          exportFilename[i]=0;
        }
	strncat(exportFilename, "anchors", 1023);
	exportFilename[1023] = '\0';
    }

    return NC_SUCCESS;
}
#endif //End STAND_ALONE

/************************************************************
nc_message_handler:
    The Nikon Curve message handler. Udi Fuchs created this
to make the error hanpting consistent acros the code.

  code - Message code
  message - The message
**************************************************************/
void nc_message(int code, char *format, ...)
{
    char message[256];
    va_list ap;
    va_start(ap, format);

    vsnprintf(message, 255, format, ap);
    message[255] = '\0';
    va_end(ap);

#ifdef _STAND_ALONE_    //if we're running standalone mode

    code = code;
    fprintf(stderr, message);
    fflush(stderr);

#else

#ifdef __WITH_UFRAW__    //and if compiling with UFRAW

    if (code==NC_SET_ERROR)
    {
	ufraw_message(UFRAW_SET_ERROR, message);
    }
    else
    {
	ufraw_message(code, message);
    }

#else    //else, just print out the errors normally

    code = code;
    g_printerr("%s", message);

#endif //End WITH_UFRAW

#endif //End STAND_ALONE
}

// #define _DEBUG
void DEBUG_PRINT(char *format, ...)
{
#ifdef _DEBUG
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fflush(stderr);
    va_end(ap);
#else
    format = format;
#endif
}

/* nc_merror(): Handle memory allocaltion errors */
void nc_merror(void *ptr, char *where)
{
    if (ptr) return;
#ifdef __WITH_UFRAW__
    g_error("Out of memory in %s\n", where);
#else
    fprintf(stderr, "Out of memory in %s\n", where);
    exit(1);
#endif
}

// Assert something at compile time (must use this inside a function);
// works because compilers won't let us declare negative-length arrays.
#define STATIC_ASSERT(cond) \
    { (void)((int (*)(char failed_static_assertion[(cond)?1:-1]))0); }

/***********************************************************************
isBigEndian:
	Determines if the machine we are running on is big endian or not.
************************************************************************/
int isBigEndian()
{
    STATIC_ASSERT(sizeof(short)==2);
    short x;
    unsigned char EndianTest[2] = { 1, 0 };

    x = *(short *) EndianTest;

    return (x!=1);
}

/***********************************************************************
ShortVal:
	Convert short int (16 bit) from little endian to machine endianess.
************************************************************************/
short ShortVal(short s)
{
    STATIC_ASSERT(sizeof(short)==2);
    if (isBigEndian()) {
	unsigned char b1, b2;

	b1 = s & 255;
	b2 = (s >> 8) & 255;

	return (b1 << 8) + b2;
    } else
	return s;
}

/***********************************************************************
LongVal:
	Convert long int (32 bit) from little endian to machine endianess.
************************************************************************/
int LongVal(int i)
{
    STATIC_ASSERT(sizeof(int)==4);
    if (isBigEndian()) {
	unsigned char b1, b2, b3, b4;

	b1 = i & 255;
	b2 = ( i >> 8 ) & 255;
	b3 = ( i>>16 ) & 255;
	b4 = ( i>>24 ) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
    } else
	return i;
}

/***********************************************************************
FloatVal:
	Convert float from little endian to machine endianess.
************************************************************************/
float FloatVal(float f)
{
    STATIC_ASSERT(sizeof(float)==4);
    if (isBigEndian()) {
	union {
	    float f;
	    unsigned char b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
    } else
	return f;
}

/***********************************************************************
DoubleVal:
	Convert double from little endian to machine endianess.
************************************************************************/
double DoubleVal(double d)
{
    STATIC_ASSERT(sizeof(double)==8);
    if (isBigEndian()) {
	union {
	    double d;
	    unsigned char b[8];
	} dat1, dat2;

	dat1.d = d;
	dat2.b[0] = dat1.b[7];
	dat2.b[1] = dat1.b[6];
	dat2.b[2] = dat1.b[5];
	dat2.b[3] = dat1.b[4];
	dat2.b[4] = dat1.b[3];
	dat2.b[5] = dat1.b[2];
	dat2.b[6] = dat1.b[1];
	dat2.b[7] = dat1.b[0];
	return dat2.d;
    } else
	return d;
}

/*********************************************
GetNikonFileType:
    Gets the nikon file type by comparing file
    headers.

  file - The file to check.
**********************************************/
int GetNikonFileType(FILE *file)
{
    unsigned char buff[HEADER_SIZE];
    int i = 0, j = 0;
    int found = 1;

    fread(buff,HEADER_SIZE,1,file);

    for(i = 0; i < NUM_FILE_TYPES; i++)
    {
	found = 1;
	for(j = 0; j < HEADER_SIZE; j++)
	{
	    if (buff[j] != FileTypeHeaders[i][j])
	    {
	        found = 0;
	        break;
	    }
	}

	if (found)
	{
	    //return the file type
	    return i;
	}
    }
	nc_message(NC_SET_ERROR, "Error, no compatible file types found!\n");
    return -1;
}

/*********************************************
LoadNikonCurve:
    Loads all curves from a Nikon ntc or ncv file.

    fileName    - The filename.
    curve        - Pointer to curve struct to hold the data.
    resolution    - How many data points to sample from the curve.
**********************************************/
int LoadNikonData(char *fileName, NikonData *data)
{
    FILE *input = NULL;
    int offset = 0;
    CurveData *curve = NULL;

    if (fileName == NULL || strlen(fileName) == 0)
    {
	nc_message(NC_SET_ERROR,
	            "Error, input filename cannot be NULL or empty!\n");
	return NC_ERROR;
    }

    DEBUG_PRINT("DEBUG: OPENING FILE: %s\n",fileName);

    //open file for reading only!
    input = g_fopen(fileName,"rb");

    //make sure we have a valid file
    if (input == NULL)
    {
	nc_message(NC_SET_ERROR, "Error opening '%s': %s\n",
	        fileName, strerror(errno));
	return NC_ERROR;
    }

    //init the curve;
    memset(data,0,sizeof(NikonData));

    //get the file type
    data->m_fileType = GetNikonFileType(input);
    // set file seek positions for curve tones depending of file type
    // todo: is it possible to find one common rule?
    long curveFilePos[4][4] = {
	{FileOffsets[data->m_fileType][BOX_DATA], SEEK_SET, FileOffsets[data->m_fileType][ANCHOR_DATA], SEEK_SET},
	{NEXT_SECTION_BOX_DATA_OFFSET, SEEK_CUR, NUM_POINTS_TO_ANCHOR_OFFSET, SEEK_CUR},
	{NEXT_SECTION_BOX_DATA_OFFSET, SEEK_CUR, NUM_POINTS_TO_ANCHOR_OFFSET, SEEK_CUR},
	{NEXT_SECTION_BOX_DATA_OFFSET, SEEK_CUR, NUM_POINTS_TO_ANCHOR_OFFSET, SEEK_CUR}
    };

    //make sure we have a good file type
    if (data->m_fileType == -1)
	return NC_ERROR;

    //advance file pointer to necessary data section
    fseek(input,offset,SEEK_SET);

    //Conevenience and opt if compiler doesn't already do it
    curve = &data->curves[0];

    //set curve type
    curve->m_curveType = TONE_CURVE;

    //read patch version
    fseek(input,FileOffsets[data->m_fileType][PATCH_DATA],SEEK_SET);
    fread(&data->m_patch_version,sizeof(unsigned short),1,input);
    data->m_patch_version = ShortVal(data->m_patch_version);

    // read all tone curves data follow from here
    int i;
    for(i = 0; i < NUM_CURVE_TYPES; i++)
    {
	curve = &data->curves[i];

	//set curve type
	curve->m_curveType = i;

	//get box data
	fseek(input, curveFilePos[i][0], curveFilePos[i][1]);

	fread(&curve->m_min_x,sizeof(double),1,input);
	curve->m_min_x = DoubleVal(curve->m_min_x);

	fread(&curve->m_max_x,sizeof(double),1,input);
	curve->m_max_x = DoubleVal(curve->m_max_x);

	fread(&curve->m_gamma,sizeof(double),1,input);
	curve->m_gamma = DoubleVal(curve->m_gamma);

	fread(&curve->m_min_y,sizeof(double),1,input);
	curve->m_min_y = DoubleVal(curve->m_min_y);

	fread(&curve->m_max_y,sizeof(double),1,input);
	curve->m_max_y = DoubleVal(curve->m_max_y);

	//get number of anchors (always located after box data)
	fread(&curve->m_numAnchors,1,1,input);

	// It seems that if there is no curve then the 62 bytes in the buffer
	// are either all 0x00 (D70) or 0xFF (D2H).
	// We therefore switch these values with the default values.
	if (curve->m_min_x==1.0) {
	    curve->m_min_x = 0.0;
	    DEBUG_PRINT("DEBUG: NEF X MIN -> %e (changed)\n",curve->m_min_x);
	}
	if (curve->m_max_x==0.0) {
	    curve->m_max_x = 1.0;
	    DEBUG_PRINT("DEBUG: NEF X MAX -> %e (changed)\n",curve->m_max_x);
	}
	if (curve->m_min_y==1.0) {
	    curve->m_min_y = 0.0;
	    DEBUG_PRINT("DEBUG: NEF Y MIN -> %e (changed)\n",curve->m_min_y);
	}
	if (curve->m_max_y==0.0) {
	    curve->m_max_y = 1.0;
	    DEBUG_PRINT("DEBUG: NEF Y MAX -> %e (changed)\n",curve->m_max_y);
	}
	if (curve->m_gamma==0.0 || curve->m_gamma==255.0+255.0/256.0) {
	    curve->m_gamma = 1.0;
	    DEBUG_PRINT("DEBUG: NEF GAMMA -> %e (changed)\n",curve->m_gamma);
	}
	if (curve->m_numAnchors==255) {
	    curve->m_numAnchors = 0;
	    DEBUG_PRINT("DEBUG: NEF NUMBER OF ANCHORS -> %u (changed)\n",
		    curve->m_numAnchors);
	}
	if (curve->m_numAnchors > NIKON_MAX_ANCHORS) {
	    curve->m_numAnchors = NIKON_MAX_ANCHORS;
	    DEBUG_PRINT("DEBUG: NEF NUMBER OF ANCHORS -> %u (changed)\n",
		    curve->m_numAnchors);
	}
	//Move to start of the anchor data
	fseek(input, curveFilePos[i][2], curveFilePos[i][3]);

	//read in the anchor points
	int rs = fread(curve->m_anchors, sizeof(CurveAnchorPoint),
		curve->m_numAnchors, input);
	if (curve->m_numAnchors != rs) {
	    nc_message(NC_SET_ERROR, "Error reading all anchor points\n");
	    return NC_ERROR;
	}

	int j;
	for (j = 0; j < curve->m_numAnchors; j++)
	{
	    curve->m_anchors[j].x = DoubleVal(curve->m_anchors[j].x);
	    curve->m_anchors[j].y = DoubleVal(curve->m_anchors[j].y);
	}

	DEBUG_PRINT("DEBUG: Loading Data:\n");
	DEBUG_PRINT("DEBUG: CURVE_TYPE: %u \n",curve->m_curveType);
	DEBUG_PRINT("DEBUG: BOX->MIN_X: %f\n",curve->m_min_x);
	DEBUG_PRINT("DEBUG: BOX->MAX_X: %f\n",curve->m_max_x);
	DEBUG_PRINT("DEBUG: BOX->GAMMA: %f\n",curve->m_gamma);
	DEBUG_PRINT("DEBUG: BOX->MIN_Y: %f\n",curve->m_min_y);
	DEBUG_PRINT("DEBUG: BOX->MAX_Y: %f\n",curve->m_max_x);

#ifdef _DEBUG
	int i_dbg;
	for(i_dbg = 0; i_dbg < curve->m_numAnchors; i_dbg++)
	{
	    DEBUG_PRINT("DEBUG: ANCHOR X,Y: %f,%f\n",
		    curve->m_anchors[i_dbg].x,curve->m_anchors[i_dbg].y);
	}
	DEBUG_PRINT("\n");
#endif
  
        // We expect only tone curves, no R,G,B
        if (i) {
          if (curve->m_numAnchors != 2) {
            printf("XXX JDLA : Not foreseen at %d processing %s\n",
                   __LINE__,nikonFilename);
            exit(EXIT_FAILURE);
          }
	  if (curve->m_anchors[0].x != 0.0 || curve->m_anchors[0].y != 0.0) {
            printf("XXX JDLA : Not foreseen at %d processing %s\n",
                   __LINE__,nikonFilename);
            exit(EXIT_FAILURE);
          }
	  if (curve->m_anchors[1].x != 1.0 || curve->m_anchors[1].y != 1.0) {
            printf("XXX JDLA : Not foreseen at %d processing %s\n",
                   __LINE__,nikonFilename);
            exit(EXIT_FAILURE);
          }
        } else {
	  if (curve->m_min_x < 0.0 || curve->m_max_x > 1.0) {
            printf("XXX JDLA : Not foreseen at %d processing %s\n"
                   "=> m_min_x : %f m_max_x : %f\n",
                   __LINE__,nikonFilename,curve->m_min_x,curve->m_max_x);
            exit(EXIT_FAILURE);
          }
	  if (curve->m_min_y < 0.0 || curve->m_max_y > 1.0) {
            printf("XXX JDLA : Not foreseen at %d processing %s\n",
                   __LINE__,nikonFilename);
            exit(EXIT_FAILURE);
          }
          // Write the output, gammaing the anchor points.
          FILE *OutFile = fopen(exportFilename,"w");
	  for(short k = 0; k < curve->m_numAnchors; k++) {
	    fprintf(OutFile,
                    "%f %f\n",
	 	    curve->m_anchors[k].x,
                    pow(curve->m_anchors[k].y,1/curve->m_gamma));
	  }
          fclose(OutFile);
        }
         
    }
    fclose(input);
    return NC_SUCCESS;
}

/****************************************
ConvertNikonCurveData:
    The main driver. Takes a filename and
    processes the curve, if possible.

    fileName    -    The file to process.
*****************************************/
int ConvertNikonCurveData(char *inFileName, char *outFileName,
    unsigned int samplingRes, unsigned int outputRes)
{
    //Load the curve data from the ncv/ntc file
    NikonData data;
    char tmpstr[1024];

    if ( samplingRes <= 1 || outputRes <= 1 || samplingRes > MAX_RESOLUTION
	    || outputRes > MAX_RESOLUTION )
    {
	nc_message(NC_SET_ERROR, "Error, sampling and output resolution"
	            "must be 1 <= res <= %u\n", MAX_RESOLUTION);
	return NC_ERROR;
    }

    //loads all the curve data. Does not allocate sample arrays.
    if (LoadNikonData(inFileName, &data) != NC_SUCCESS)
    {
	return NC_ERROR;
    }

    return NC_SUCCESS;
}

/*****************************************************
FindTIFFOffset:
    Moves the file pointer to the location
    indicated by the TAG-TYPE pairing. This is meant just
    as a helper function for this code. Uses elsewhere
    may be limited.

    file    - Nikon File ptr
    num_entries - Number of entries to search through
    tiff_tag - The tiff tag to match.
    tiff_type - The tiff type to match.
*******************************************************/
int FindTIFFOffset(FILE *file, unsigned short num_entries, unsigned short tiff_tag, unsigned short tiff_type)
{
    unsigned short tag = 0;
    unsigned short type = 0;
    unsigned int offset = 0;

    int i;
    for(i = 0; i < num_entries; i++)
    {
	//get tag 2 bytes
	tag = (fgetc(file)<<8)|fgetc(file);
	if (tag == tiff_tag)
	{
	    //get type 2 bytes
	    type = (fgetc(file)<<8)|fgetc(file);
	    if (type == tiff_type)    //Type for length of field
	    {
	        //get length (4 bytes)
	        offset = (fgetc(file)<<24)|(fgetc(file)<<16)|(fgetc(file)<<8)|fgetc(file);
	        //get value\offset 4 bytes
	        offset = (fgetc(file)<<24)|(fgetc(file)<<16)|(fgetc(file)<<8)|fgetc(file);
	        fseek(file,offset,SEEK_SET);
	        return 1; //true;
	    }
	}
	else
	{
	    //advance to next entry
	    fseek(file,10,SEEK_CUR);
	}
    }
    return 0; //false;
}

/*******************************************************
RipNikonNEFData:
    Gets Nikon NEF data. For now, this is just the tone
    curve data.

    infile -	The input file
    curve  -	data structure to hold data in.
    sample_p -	pointer to the curve sample reference.
		can be NULL if curve sample is not needed.
********************************************************/
int RipNikonNEFData(char *infile, CurveData *data, CurveSample **sample_p)
{
    unsigned short byte_order = 0;
    unsigned short num_entries = 0;
    unsigned short version = 0;
    unsigned int offset = 0;

    //open the file
    FILE *file = g_fopen(infile,"rb");

    //make sure we have a valid file
    if (file == NULL)
    {
	nc_message(NC_SET_ERROR, "Error opening '%s': %s\n",
	        infile, strerror(errno));
	return NC_ERROR;
    }

    //gets the byte order
    fread(&byte_order,2,1,file);
    byte_order = ShortVal(byte_order);
    if (byte_order != 0x4d4d)
    {
	//Must be in motorola format if it came from a NIKON
	nc_message(NC_SET_ERROR,
	    "NEF file data format is Intel. Data format should be Motorola.\n");
	return NC_ERROR;
    }

    //get the version
    //fread(&version,2,1,file);
    version = (fgetc(file)<<8)|fgetc(file);
    if (version != 0x002a)
    {
	//must be 42 or not a valid TIFF
	nc_message(NC_SET_ERROR,
	    "NEF file version is %u. Version should be 42.\n",version);
	return NC_ERROR;
    }

    //get offset to first IFD
    offset = (fgetc(file)<<24)|(fgetc(file)<<16)|(fgetc(file)<<8)|fgetc(file);
    //go to the IFD
    fseek(file,offset,SEEK_SET);
    //get number of entries
    num_entries = (fgetc(file)<<8)|fgetc(file);

    //move file pointer to exif offset
    if (!FindTIFFOffset(file,num_entries,TIFF_TAG_EXIF_OFFSET,TIFF_TYPE_LONG))
    {
	nc_message(NC_SET_ERROR,
	    "NEF data entry could not be found with tag %u and type %u.\n",
	    TIFF_TAG_EXIF_OFFSET, TIFF_TYPE_LONG);
	return NC_ERROR;
    }

    //get number of entries
    num_entries = (fgetc(file)<<8)|fgetc(file);

    //move file pointer to maker note offset
    if (!FindTIFFOffset(file,num_entries,TIFF_TAG_MAKER_NOTE_OFFSET,TIFF_TYPE_UNDEFINED))
    {
	nc_message(NC_SET_ERROR,
	    "NEF data entry could not be found with tag %u and type %u.\n",
	    TIFF_TAG_MAKER_NOTE_OFFSET, TIFF_TYPE_UNDEFINED);
	return NC_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////
    //NOTE: At this point, this section of the file acts almost like another
    //      file header. Skip the first bytes, (which just say nikon with a
    //      few bytes at the end. Offsets from here on in are from the start
    //      of this section, not the start of the file.
    //////////////////////////////////////////////////////////////////////////

    //Check the name. If it isn't Nikon then we can't do anything with this file.
    char name[6];
    fread(name,6,1,file);
    if (strcmp(name,"Nikon") != 0)
    {
	nc_message(NC_SET_ERROR,
	    "NEF string identifier is %s. Should be: Nikon.\n",name);
	return NC_ERROR;
    }
    fseek(file,4,SEEK_CUR);

    //save the current file location, as all other offsets for this section run off this.
    unsigned long pos = ftell(file);

    //get byte order (use a regular fread)
    fread(&byte_order,2,1,file);
    byte_order = ShortVal(byte_order);
    if (byte_order != 0x4d4d)
    {
	//Must be in motorola format or not from a Nikon
	nc_message(NC_SET_ERROR,
	    "NEF secondary file data format is Intel. "
	    "Data format should be Motorola.\n");
	return NC_ERROR;
    }

    //get version
    version = (fgetc(file)<<8)|fgetc(file);
    if (version != 0x002a)
    {
	nc_message(NC_SET_ERROR,
	    "NEF secondary file version is %u. Version should be 42.\n",
	    version);
	return NC_ERROR;
    }

    //get offset to first IFD
    offset = (fgetc(file)<<24)| (fgetc(file)<<16)|(fgetc(file)<<8)|fgetc(file);
    //go to the IFD (these offsets are NOT from the start of the file,
    //just the start of the section).
    fseek(file,pos+offset,SEEK_SET);
    //get number of entries
    num_entries = (fgetc(file)<<8)|fgetc(file);

    //move file position to tone curve data
    if (!FindTIFFOffset(file,num_entries,TIFF_TAG_CURVE_OFFSET,TIFF_TYPE_UNDEFINED))
    {
	nc_message(NC_SET_ERROR,
	    "NEF data entry could not be found with tag %u and type %u.\n",
	    TIFF_TAG_CURVE_OFFSET, TIFF_TYPE_UNDEFINED);
	return NC_ERROR;
    }

    offset = ftell(file);
    return RipNikonNEFCurve(file, offset+pos, data, sample_p);
}


/*******************************************************
RipNikonNEFCurve:
    The actual retriever for the curve data from the NEF
    file.

    file   - The input file.
    infile - Offset to retrieve the data
    curve  - data structure to hold curve in.
    sample_p -	pointer to the curve sample reference.
		can be NULL if curve sample is not needed.
********************************************************/
int RipNikonNEFCurve(FILE *file, int offset, CurveData *data,
	CurveSample **sample_p)
{
    int i;

    //seek to the offset of the data. Skip first two bytes (section isn't needed).
    fseek(file, offset+2, SEEK_SET);

    memset(data,0,sizeof(CurveData));
    /////////////////////////////////////////////////
    //GET CURVE DATA
    /////////////////////////////////////////////////
    //get box data and gamma
    data->m_min_x = (double)fgetc(file)/255.0;
    data->m_max_x = (double)fgetc(file)/255.0;
    data->m_min_y = (double)fgetc(file)/255.0;
    data->m_max_y = (double)fgetc(file)/255.0;
    //16-bit fixed point.
    data->m_gamma = (double)fgetc(file) + ((double)fgetc(file)/256.0);

    //DEBUG_PRINT("DEBUG: NEF SECTION SIZE -> %u\n",data->section_size);
    DEBUG_PRINT("DEBUG: NEF X MIN -> %e\n",data->m_min_x);
    DEBUG_PRINT("DEBUG: NEF X MAX -> %e\n",data->m_max_x);
    DEBUG_PRINT("DEBUG: NEF Y MIN -> %e\n",data->m_min_y);
    DEBUG_PRINT("DEBUG: NEF Y MAX -> %e\n",data->m_max_y);
    //DEBUG_PRINT("DEBUG: NEF GAMMA (16-bit fixed point) -> %e\n",(data->m_gamma>>8)+(data->m_gamma&0x00ff)/256.0);
    DEBUG_PRINT("DEBUG: NEF GAMMA -> %e\n",data->m_gamma);
    // It seems that if there is no curve then the 62 bytes in the buffer
    // are either all 0x00 (D70) or 0xFF (D2H).
    // We therefore switch these values with the default values.
    if (data->m_min_x==1.0) {
	data->m_min_x = 0.0;
	DEBUG_PRINT("DEBUG: NEF X MIN -> %e (changed)\n",data->m_min_x);
    }
    if (data->m_max_x==0.0) {
	data->m_max_x = 1.0;
	DEBUG_PRINT("DEBUG: NEF X MAX -> %e (changed)\n",data->m_max_x);
    }
    if (data->m_min_y==1.0) {
	data->m_min_y = 0.0;
	DEBUG_PRINT("DEBUG: NEF Y MIN -> %e (changed)\n",data->m_min_y);
    }
    if (data->m_max_y==0.0) {
	data->m_max_y = 1.0;
	DEBUG_PRINT("DEBUG: NEF Y MAX -> %e (changed)\n",data->m_max_y);
    }
    if (data->m_gamma==0.0 || data->m_gamma==255.0+255.0/256.0) {
	data->m_gamma = 1.0;
	DEBUG_PRINT("DEBUG: NEF GAMMA -> %e (changed)\n",data->m_gamma);
    }

    //get number of anchor points (there should be at least 2
    fread(&data->m_numAnchors,1,1,file);
    DEBUG_PRINT("DEBUG: NEF NUMBER OF ANCHORS -> %u\n",data->m_numAnchors);
    if (data->m_numAnchors==255) {
	data->m_numAnchors = 0;
	DEBUG_PRINT("DEBUG: NEF NUMBER OF ANCHORS -> %u (changed)\n",
		data->m_numAnchors);
    }
    if (data->m_numAnchors > NIKON_MAX_ANCHORS) {
	data->m_numAnchors = NIKON_MAX_ANCHORS;
	DEBUG_PRINT("DEBUG: NEF NUMBER OF ANCHORS -> %u (changed)\n",
		data->m_numAnchors);
    }

    //convert data to doubles
    for(i = 0; i < data->m_numAnchors; i++)
    {
	//get anchor points
	data->m_anchors[i].x = (double)fgetc(file)/255.0;
	data->m_anchors[i].y = (double)fgetc(file)/255.0;
    }

    //The total number of points possible is 25 (50 bytes).
    //At this point we subtract the number of bytes read for points from the max (50+1)
    fseek(file,(51 - data->m_numAnchors*2),SEEK_CUR);

    //get data (always 4096 entries, 1 byte apiece)
    DEBUG_PRINT("DEBUG: NEF data OFFSET -> %ld\n",ftell(file));

    return NC_SUCCESS;
}

/*******************************
main:
    Uh....no comment. :)
********************************/
#ifdef _STAND_ALONE_

int main(int argc, char* argv[])
{
    //make sure we can continue processing
    if (ProcessArgs(argc,argv) == NC_SUCCESS)
    {

	//if we are in NEF mode, rip the curve out of the RAW file
	if (program_mode == NEF_MODE)
	{
	    NikonData data;

	    //intiialze the structure to zero
	    memset(&data,0,sizeof(NikonData));

	    if (RipNikonNEFData(nikonFilename, &data.curves[TONE_CURVE], NULL)
	            != NC_SUCCESS)
	    {
	        return NC_ERROR;
	    }

	}
	//else, process a nikon curve file
	else
	{
	    //do the deed
	    ConvertNikonCurveData(nikonFilename,exportFilename,
	        standalone_samplingRes, standalone_outputRes);
	}
    }
    return NC_SUCCESS;
}
#endif
