////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "ptDefines.h"
#include "ptCurve.h"
#include "ptImage.h"
#include "ptError.h"

////////////////////////////////////////////////////////////////////////////////
//
// Elementary constructor.
// Sets the curve to 0 with no anchors.
//
////////////////////////////////////////////////////////////////////////////////

ptCurve::ptCurve(const short Channel) {
  if (Channel == ptCurveChannel_Saturation ||
      Channel == ptCurveChannel_LByHue ||
      Channel == ptCurveChannel_Texture ||
      Channel == ptCurveChannel_Denoise)
    m_IntType = ptCurveIT_Cosine;
  else
    m_IntType = ptCurveIT_Spline;

  SetNullCurve(Channel);
};

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptCurve::~ptCurve() {
}

////////////////////////////////////////////////////////////////////////////////
//
// SetNullCurve
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::SetNullCurve(const short Channel) {
  m_IntendedChannel = Channel;
  if (Channel == ptCurveChannel_Saturation ||
      Channel == ptCurveChannel_LByHue) {
    m_Type            = ptCurveType_Anchor;
    m_NrAnchors       = 3;
    m_XAnchor[0]      = 0.0;
    m_YAnchor[0]      = 0.5;
    m_XAnchor[1]      = 0.5;
    m_YAnchor[1]      = 0.5;
    m_XAnchor[2]      = 1.0;
    m_YAnchor[2]      = 0.5;
  } else if (Channel == ptCurveChannel_Texture) {
    m_Type            = ptCurveType_Anchor;
    m_NrAnchors       = 3;
    m_XAnchor[0]      = 0.0;
    m_YAnchor[0]      = 0.25;
    m_XAnchor[1]      = 0.5;
    m_YAnchor[1]      = 0.25;
    m_XAnchor[2]      = 1.0;
    m_YAnchor[2]      = 0.25;
  } else if (Channel == ptCurveChannel_Denoise) {
    m_Type            = ptCurveType_Anchor;
    m_NrAnchors       = 3;
    m_XAnchor[0]      = 0.0;
    m_YAnchor[0]      = 0.2;
    m_XAnchor[1]      = 0.5;
    m_YAnchor[1]      = 0.2;
    m_XAnchor[2]      = 1.0;
    m_YAnchor[2]      = 0.2;
  } else if (Channel == ptCurveChannel_a || Channel == ptCurveChannel_b) {
    m_Type            = ptCurveType_Anchor;
    m_NrAnchors       = 3;
    m_XAnchor[0]      = 0.0;
    m_YAnchor[0]      = 0.0;
    m_XAnchor[1]      = 0.501960784; // 0x8080/0xffff
    m_YAnchor[1]      = 0.501960784; // 0x8080/0xffff
    m_XAnchor[2]      = 1.0;
    m_YAnchor[2]      = 1.0;
  } else {
    m_Type            = ptCurveType_Anchor;
    m_NrAnchors       = 3;
    m_XAnchor[0]      = 0.0;
    m_YAnchor[0]      = 0.0;
    m_XAnchor[1]      = 0.5;
    m_YAnchor[1]      = 0.5;
    m_XAnchor[2]      = 1.0;
    m_YAnchor[2]      = 1.0;
  }
  return SetCurveFromAnchors();
}

////////////////////////////////////////////////////////////////////////////////
//
// SetCurveFromAnchors
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::SetCurveFromAnchors() {

  assert (m_NrAnchors != 0);

  m_Type = ptCurveType_Anchor;

  if (m_IntType == ptCurveIT_Spline) {
    double *ypp = spline_cubic_set(m_NrAnchors,
                                   m_XAnchor,
                                   m_YAnchor,
                                   2, 0.0,
                                   2, 0.0);
    if (NULL==ypp) {
      ptLogError(ptError_Spline,"Unexpected NULL return at %s line %d\n",
                 __FILE__,__LINE__);
      return -1;
    }

    //first derivative at a point
    double ypval = 0;
    //second derivate at a point
    double yppval = 0;

    //Now build a table
    uint16_t firstPointX = (uint16_t) (m_XAnchor[0] * 0xffff);
    uint16_t firstPointY = (uint16_t) (m_YAnchor[0] * 0xffff);
    uint16_t lastPointX  = (uint16_t) (m_XAnchor[m_NrAnchors-1] * 0xffff);
    uint16_t lastPointY  = (uint16_t) (m_YAnchor[m_NrAnchors-1] * 0xffff);

    double Resolution = 1.0/(double)(0xffff);

    for(uint32_t i = 0; i < 0x10000; i++) {
      if (i < firstPointX) {
        m_Curve[i] = firstPointY;
      } else if (i > lastPointX) {
        m_Curve[i] = lastPointY;
      } else  {
        int32_t Value = (int32_t) (spline_cubic_val(m_NrAnchors,
                                             m_XAnchor, i*Resolution,
                                             m_YAnchor, ypp, &ypval, &yppval)
                                   *0xffff + 0.5);
        m_Curve[i] = CLIP(Value);
      }
    }
    FREE(ypp);
  } else  if (m_IntType == ptCurveIT_Cosine) {
    for(uint32_t i = 0; i < m_XAnchor[0] * 0xffff; i++)
      m_Curve[i] = m_YAnchor[0] * 0xffff;
    for(short l = 0; l < m_NrAnchors-1; l++) {
      float Factor = m_YAnchor[l+1]-m_YAnchor[l];
      float Scale = ptPI/(float)0xffff/(m_XAnchor[l+1]-m_XAnchor[l]);
      for(uint32_t i = 0; i < m_XAnchor[l+1] * 0xffff - m_XAnchor[l] * 0xffff; i++) {
        m_Curve[i+(int32_t)(m_XAnchor[l] * 0xffff)] =
          (int32_t)((( (1-cosf(i*Scale))/2 ) * Factor + m_YAnchor[l])*0xffff);
      }
    }
    for(uint32_t i = m_XAnchor[m_NrAnchors-1] * 0xffff; i < 0x10000; i++)
      m_Curve[i] = m_YAnchor[m_NrAnchors-1] * 0xffff;
  } else { // Linear
    for(uint32_t i = 0; i < m_XAnchor[0] * 0xffff; i++)
      m_Curve[i] = m_YAnchor[0] * 0xffff;
    for(short l = 0; l < m_NrAnchors-1; l++) {
      float Factor = (float)(m_YAnchor[l+1] - m_YAnchor[l])/
                     (float)(m_XAnchor[l+1] - m_XAnchor[l]);
      for(uint32_t i = 0; i < m_XAnchor[l+1] * 0xffff - m_XAnchor[l] * 0xffff; i++) {
        m_Curve[i+(int32_t)(m_XAnchor[l] * 0xffff)] =
          m_YAnchor[l] * 0xffff + (int32_t)(Factor*i);
      }
    }
    for(uint32_t i = m_XAnchor[m_NrAnchors-1] * 0xffff; i < 0x10000; i++)
      m_Curve[i] = m_YAnchor[m_NrAnchors-1] * 0xffff;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// DumpData
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::DumpData(const char* FileName,
                        const short Scale) {

  FILE *OutputFile = fopen(FileName,"wb");
  if (!OutputFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  for (uint32_t i=0;i<0x10000;i+=Scale) {
    fprintf(OutputFile,"%5d\t%5d\n",i/Scale,m_Curve[i]/Scale);
  }

  FCLOSE(OutputFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// A WriteCurve function. Naive, but should do !
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::WriteCurve(const char *FileName,
                          const char *Header) {

  FILE *OutFile = fopen(FileName,"wb");
  if (!OutFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }
  if (Header) {
    fprintf(OutFile,"%s",Header);
  } else {
    fprintf(OutFile,";\n; photivo Curve File\n; Generated by photivo\n;\n");
  }
  fprintf(OutFile,"Magic FLOSSCurveFile\n");
  fprintf(OutFile,"IntendedChannel %d\n",m_IntendedChannel);
  fprintf(OutFile,"CurveType %d\n",m_Type);
  switch(m_Type) {
    case ptCurveType_Anchor :
      for (short i=0; i<m_NrAnchors; i++) {
        fprintf(OutFile,"%d %d\n",
                         // We represent a float in 1/1000ths to avoid
                         // any ',','.' related problem.
                         (int)(1000*m_XAnchor[i]),
                         (int)(1000*m_YAnchor[i]));
      }
      break;
    case ptCurveType_Full :
      for (uint32_t i=0;i<0x10000;i++) {
        fprintf(OutFile,"0x%04X 0x%04X\n",i,m_Curve[i]);
      }
      break;
  }
  FCLOSE(OutFile);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// A ReadCurve function. Naive, but should do !
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::ReadCurve(const char *FileName) {

  // Some safe presets in case things go wrong.
  m_NrAnchors = 0;
  m_IntendedChannel = ptCurveChannel_RGB;
  m_Type = ptCurveType_Full;
  memset(m_Curve,0,sizeof(m_Curve));

  FILE *InFile = fopen(FileName,"r");
  if (!InFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  char Buffer[100];
  char Key[100];
  char Value[100];
  int  LineNr = 0;
  int  NrKeys = 0;

  do {
    if (NULL == fgets(Buffer,100,InFile)) break;
    LineNr++;
    if (';' == Buffer[0]) continue;
    sscanf(Buffer,"%s %s",Key,Value);
    NrKeys++;
    if (1 == NrKeys) {
      if ((strcmp(Key,"Magic") || strcmp(Value,"FLOSSCurveFile")) &&
          (strcmp(Key,"Magic") || strcmp(Value,"photivoCurveFile")) &&
          (strcmp(Key,"Magic") || strcmp(Value,"dlRawCurveFile"))) {
        ptLogError(ptError_FileFormat,
                   "'%s' has wrong format at line %d\n",
                   FileName,
                   LineNr);
        return ptError_FileFormat;
      }
    } else if (!strcmp(Key,"IntendedChannel")) {
      short IntendedChannel;
      sscanf(Value,"%hd",&IntendedChannel);
      switch (IntendedChannel) {
        case ptCurveChannel_RGB :
        case ptCurveChannel_R :
        case ptCurveChannel_G :
        case ptCurveChannel_B :
        case ptCurveChannel_L :
        case ptCurveChannel_LByHue :
        case ptCurveChannel_Texture :
        case ptCurveChannel_Saturation :
        case ptCurveChannel_ShadowsHighlights :
        case ptCurveChannel_Denoise :
        case ptCurveChannel_Base :
        case ptCurveChannel_Base2 :
          m_IntendedChannel = IntendedChannel;
          break;
        default :
          ptLogError(ptError_Argument,
                     "Error reading %s at line %d : out of range\n",
                     FileName,LineNr);
          return ptError_Argument;
        }
    } else if (!strcmp(Key,"CurveType")) {
      short CurveType;
      sscanf(Value,"%hd",&CurveType);
      switch (CurveType) {
        case ptCurveType_Anchor :
        case ptCurveType_Full :
          m_Type = CurveType;
          break;
        default :
          ptLogError(ptError_Argument,
                     "Error reading %s at line %d : out of range\n",
                     FileName,LineNr);
          return ptError_Argument;
       }
    } else if (m_Type == ptCurveType_Anchor) {
      // Read further an Anchor type curve.
      // Remember we expressed in thousands.
      double Value1 = atof(Key)/1000.0;
      double Value2 = atof(Value)/1000.0;
      const double Epsilon = 0.00001;
      if ( Value1<0.0-Epsilon || Value2 <0.0-Epsilon ||
           Value1>1.0+Epsilon || Value2>1.0+Epsilon) {
        ptLogError(ptError_Argument,
                   "Error reading %s at line %d (out of box : %f %f)\n",
                   FileName,LineNr,Value1,Value2);
        return ptError_Argument;
      }
      if (m_NrAnchors >= ptMaxAnchors) {
        ptLogError(ptError_FileFormat,
                   "Error reading %s at line %d (too many anchors)\n",
                   FileName,LineNr);
        return ptError_FileFormat;
      }
      m_XAnchor[m_NrAnchors] = Value1;
      m_YAnchor[m_NrAnchors] = Value2;
      m_NrAnchors++;
    } else if (m_Type == ptCurveType_Full) {
      int Value1 = strtol(Key,NULL,16);
      int Value2 = strtol(Value,NULL,16);
      if ( Value1<0 || Value2 <0 || Value1>0xffff || Value2>0xffff) {
        ptLogError(ptError_Argument,
                   "Error reading %s at line %d (out of box : %x %x)\n",
                   FileName,LineNr,Value1,Value2);
        return ptError_Argument;
      }
      m_Curve[Value1] = Value2;
    }
  } while (!feof(InFile));

  if (m_Type == ptCurveType_Anchor) {
    // Construct the curve now also complete.
    SetCurveFromAnchors();
  }

  // Looks OK.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// ReadAnchors
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::ReadAnchors(const char *FileName) {

  m_NrAnchors = 0;

  FILE *InFile = fopen(FileName,"r");
  if (!InFile) {
    ptLogError(ptError_FileOpen,FileName);
    return ptError_FileOpen;
  }

  char Buffer[100];
  int  Line = 0;
  while (!feof(InFile)) {
    Line++;
    if (NULL == fgets(Buffer,100,InFile)) {
      if (feof(InFile)) break;
      ptLogError(ptError_FileFormat,
                 "Error reading %s at line %d (line too long ?)\n",
                 FileName,Line);
      return ptError_FileFormat;
    }
    double Value1;
    double Value2;
    if (2 != sscanf(Buffer,"%lf %lf",&Value1,&Value2)) {
      ptLogError(ptError_FileFormat,
                 "Error reading %s at line %d\n",FileName,Line);
      return ptError_FileFormat;
    }
    if ( Value1<0.0 || Value2 <0.0 || Value1>1.0 || Value2>1.0) {
      ptLogError(ptError_FileFormat,
                 "Error reading %s at line %d (out of box : %f %f)\n",
                 FileName,Line,Value1,Value2);
      return ptError_FileFormat;
    }
    if (m_NrAnchors >= ptMaxAnchors) {
      ptLogError(ptError_FileFormat,
                 "Error reading %s at line %d (too many anchors)\n",
                 FileName,Line);
      return ptError_FileFormat;
    }
    m_XAnchor[m_NrAnchors] = Value1;
    m_YAnchor[m_NrAnchors] = Value2;
    m_NrAnchors++;

  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetCurveFromFunction
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::SetCurveFromFunction(double(*Function)
                                          (double r, double Arg1, double Arg2),
                                    double Arg1,
                                    double Arg2) {

  m_Type = ptCurveType_Full;

  for (uint32_t i=0; i<0x10000; i++) {
    double r = (double(i) / 0xffff);
    int32_t Value = (int32_t) (0xffff * Function(r,Arg1,Arg2));
    m_Curve[i] = CLIP(Value);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Set Curve From Curve
//
////////////////////////////////////////////////////////////////////////////////

short ptCurve::Set(ptCurve *Curve) {
  m_Type = Curve->m_Type;
  m_IntType = Curve->m_IntType;
  m_IntendedChannel = Curve->m_IntendedChannel;
  if (m_Type == ptCurveType_Anchor) {
    m_NrAnchors = Curve->m_NrAnchors;
    for (int i=0; i<m_NrAnchors; i++) {
      m_XAnchor[i] = Curve->m_XAnchor[i];
      m_YAnchor[i] = Curve->m_YAnchor[i];
    }
  }

  for (uint32_t i=0; i<0x10000; i++) {
    m_Curve[i] = Curve->m_Curve[i];
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// ApplyCurve
//
////////////////////////////////////////////////////////////////////////////////

ptCurve* ptCurve::ApplyCurve(const ptCurve* Curve,
                             const short    AfterThis) {

  m_Type = ptCurveType_Full;

  if (AfterThis) {
    for (uint32_t i=0; i<0x10000; i++) {
      int32_t Value = (int32_t) (Curve->m_Curve[m_Curve[i]]);
      m_Curve[i] = CLIP(Value);
    }
  } else {
    for (uint32_t i=0; i<0x10000; i++) {
      int32_t Value = (int32_t) (m_Curve[Curve->m_Curve[i]]);
      m_Curve[i] = CLIP(Value);
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Non member utility functions for Gamma curves.
//   BT709 curve
//   sRGB curve
//   Pure22 curve
//   GammaTool        : Gamma function on basis of Gamma and Linearity
//                      (as in ufraw)
//   DeltaGammaTool   : (Inverse(sRGB))*GammaTool
//   InverseGammaSRGB : Inverse(sRGB)
//
////////////////////////////////////////////////////////////////////////////////

double GammaBT709(double r, double, double) {
  return (r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099 );
}

double GammaSRGB(double r, double, double) {
  return (r <= 0.00304 ? r*12.92 : pow(r,2.5/6)*1.055-0.055 );
}

double GammaPure22(double r, double, double) {
  return (pow(r,2.2));
}

double GammaTool(double r, double Gamma, double Linearity) {
  const double g = Gamma * (1 - Linearity) / (1-Gamma*Linearity);
  const double a = 1/(1 + Linearity*(g-1));
  const double b = Linearity * (g-1)*a;
  const double c = pow((a*Linearity+b),g) / Linearity;
  return r<Linearity ? c*r : pow (a*r+b,g);
}

double DeltaGammaTool(double r, double Gamma, double Linearity) {
  return InverseGammaSRGB(GammaTool(r,Gamma,Linearity),0,0);
};

double InverseGammaSRGB(double r, double, double) {
  return (r <= 0.04045 ? r/12.92 : pow((r+0.055)/1.055,2.4) );
}

////////////////////////////////////////////////////////////////////////////////
//
// Non member utility function for Sigmoidal contrast function.
//
////////////////////////////////////////////////////////////////////////////////

double Sigmoidal(double r, double Threshold, double Contrast) {
  float Scaling = 1.0/(1.0+exp(-0.5*Contrast))-1.0/(1.0+exp(0.5*Contrast));
  float Offset = -1.0/(1.0+exp(0.5*Contrast));
  float logtf = -logf(Threshold)/logf(2.0);
  float logft = -logf(2.0)/logf(Threshold);
  float Value = 0;
  if (Contrast > 0) {
    if (r != 0.0)
      Value = powf((((1.0/(1.0+exp(Contrast*(0.5-powf(r,logft)))))+Offset)/Scaling),logtf);
  } else {
    if (r != 0.0)
      Value = powf(0.5-1.0/Contrast*logf(1.0/(Scaling*powf(r,logft)-Offset)-1.0),logtf);
  }
  return Value;
}

////////////////////////////////////////////////////////////////////////////////
//
// From here go verbatim copies of the spline functions.
//
////////////////////////////////////////////////////////////////////////////////

//**********************************************************************
//
//  Purpose:
//
//    D3_NP_FS factors and solves a D3 system.
//
//  Discussion:
//
//    The D3 storage format is used for a tridiagonal matrix.
//    The superdiagonal is stored in entries (1,2:N), the diagonal in
//    entries (2,1:N), and the subdiagonal in (3,1:N-1).  Thus, the
//    original matrix is "collapsed" vertically into the array.
//
//    This algorithm requires that each diagonal entry be nonzero.
//    It does not use pivoting, and so can fail on systems that
//    are actually nonsingular.
//
//  Example:
//
//    Here is how a D3 matrix of order 5 would be stored:
//
//       *  A12 A23 A34 A45
//      A11 A22 A33 A44 A55
//      A21 A32 A43 A54  *
//
//  Modified:
//
//      07 January 2005    Shawn Freeman (pure C modifications)
//    15 November 2003    John Burkardt
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the order of the linear system.
//
//    Input/output, double A[3*N].
//    On input, the nonzero diagonals of the linear system.
//    On output, the data in these vectors has been overwritten
//    by factorization information.
//
//    Input, double B[N], the right hand side.
//
//    Output, double D3_NP_FS[N], the solution of the linear system.
//    This is NULL if there was an error because one of the diagonal
//    entries was zero.
//
double *ptCurve::d3_np_fs ( int n, double a[], double b[] ) {
  int i;
  double *x;
  double xmult;
//
//  Check.
//
  for ( i = 0; i < n; i++ ) {
    if ( a[1+i*3] == 0.0E+00 ) {
      return NULL;
    }
  }
  x = (double *)CALLOC(n,sizeof(double));
  ptMemoryError(x,__FILE__,__LINE__);

  for ( i = 0; i < n; i++ ) {
    x[i] = b[i];
  }

  for ( i = 1; i < n; i++ ) {
    xmult = a[2+(i-1)*3] / a[1+(i-1)*3];
    a[1+i*3] = a[1+i*3] - xmult * a[0+i*3];
    x[i] = x[i] - xmult * x[i-1];
  }

  x[n-1] = x[n-1] / a[1+(n-1)*3];
  for ( i = n-2; 0 <= i; i-- ) {
    x[i] = ( x[i] - a[0+(i+1)*3] * x[i+1] ) / a[1+i*3];
  }

  return x;
}

//**********************************************************************
//
//  Purpose:
//
//    SPLINE_CUBIC_SET computes the second derivatives of a piecewise cubic spline.
//
//  Discussion:
//
//    For data interpolation, the user must call SPLINE_SET to determine
//    the second derivative data, passing in the data to be interpolated,
//    and the desired boundary conditions.
//
//    The data to be interpolated, plus the SPLINE_SET output, defines
//    the spline.  The user may then call SPLINE_VAL to evaluate the
//    spline at any point.
//
//    The cubic spline is a piecewise cubic polynomial.  The intervals
//    are determined by the "knots" or abscissas of the data to be
//    interpolated.  The cubic spline has continous first and second
//    derivatives over the entire interval of interpolation.
//
//    For any point T in the interval T(IVAL), T(IVAL+1), the form of
//    the spline is
//
//      SPL(T) = A(IVAL)
//             + B(IVAL) * ( T - T(IVAL) )
//             + C(IVAL) * ( T - T(IVAL) )**2
//             + D(IVAL) * ( T - T(IVAL) )**3
//
//    If we assume that we know the values Y(*) and YPP(*), which represent
//    the values and second derivatives of the spline at each knot, then
//    the coefficients can be computed as:
//
//      A(IVAL) = Y(IVAL)
//      B(IVAL) = ( Y(IVAL+1) - Y(IVAL) ) / ( T(IVAL+1) - T(IVAL) )
//        - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * ( T(IVAL+1) - T(IVAL) ) / 6
//      C(IVAL) = YPP(IVAL) / 2
//      D(IVAL) = ( YPP(IVAL+1) - YPP(IVAL) ) / ( 6 * ( T(IVAL+1) - T(IVAL) ) )
//
//    Since the first derivative of the spline is
//
//      SPL'(T) =     B(IVAL)
//              + 2 * C(IVAL) * ( T - T(IVAL) )
//              + 3 * D(IVAL) * ( T - T(IVAL) )**2,
//
//    the requirement that the first derivative be continuous at interior
//    knot I results in a total of N-2 equations, of the form:
//
//      B(IVAL-1) + 2 C(IVAL-1) * (T(IVAL)-T(IVAL-1))
//      + 3 * D(IVAL-1) * (T(IVAL) - T(IVAL-1))**2 = B(IVAL)
//
//    or, setting H(IVAL) = T(IVAL+1) - T(IVAL)
//
//      ( Y(IVAL) - Y(IVAL-1) ) / H(IVAL-1)
//      - ( YPP(IVAL) + 2 * YPP(IVAL-1) ) * H(IVAL-1) / 6
//      + YPP(IVAL-1) * H(IVAL-1)
//      + ( YPP(IVAL) - YPP(IVAL-1) ) * H(IVAL-1) / 2
//      =
//      ( Y(IVAL+1) - Y(IVAL) ) / H(IVAL)
//      - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * H(IVAL) / 6
//
//    or
//
//      YPP(IVAL-1) * H(IVAL-1) + 2 * YPP(IVAL) * ( H(IVAL-1) + H(IVAL) )
//      + YPP(IVAL) * H(IVAL)
//      =
//      6 * ( Y(IVAL+1) - Y(IVAL) ) / H(IVAL)
//      - 6 * ( Y(IVAL) - Y(IVAL-1) ) / H(IVAL-1)
//
//    Boundary conditions must be applied at the first and last knots.
//    The resulting tridiagonal system can be solved for the YPP values.
//
//  Modified:
//
//      07 January 2005    Shawn Freeman (pure C modifications)
//    06 February 2004    John Burkardt
//
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the number of data points.  N must be at least 2.
//    In the special case where N = 2 and IBCBEG = IBCEND = 0, the
//    spline will actually be linear.
//
//    Input, double T[N], the knot values, that is, the points were data is
//    specified.  The knot values should be distinct, and increasing.
//
//    Input, double Y[N], the data values to be interpolated.
//
//    Input, int IBCBEG, left boundary condition flag:
//      0: the cubic spline should be a quadratic over the first interval;
//      1: the first derivative at the left endpoint should be YBCBEG;
//      2: the second derivative at the left endpoint should be YBCBEG.
//
//    Input, double YBCBEG, the values to be used in the boundary
//    conditions if IBCBEG is equal to 1 or 2.
//
//    Input, int IBCEND, right boundary condition flag:
//      0: the cubic spline should be a quadratic over the last interval;
//      1: the first derivative at the right endpoint should be YBCEND;
//      2: the second derivative at the right endpoint should be YBCEND.
//
//    Input, double YBCEND, the values to be used in the boundary
//    conditions if IBCEND is equal to 1 or 2.
//
//    Output, double SPLINE_CUBIC_SET[N], the second derivatives of the cubic spline.
//
//
double *ptCurve::spline_cubic_set ( int n, double t[], double y[], int ibcbeg,
    double ybcbeg, int ibcend, double ybcend ) {
  double *a;
  double *b;
  int i;
  double *ypp;
//
//  Check.
//
  if ( n <= 1 ) {
    ptLogError(ptError_Spline,
               "spline_cubic_set() error: "
         "The number of data points must be at least 2.\n");
    return NULL;
  }

  for ( i = 0; i < n - 1; i++ ) {
    if ( t[i+1] <= t[i] ) {
      ptLogError(ptError_Spline,
                 "spline_cubic_set() error: "
           "The knots must be strictly increasing, but "
           "T(%u) = %e, T(%u) = %e\n",i,t[i],i+1,t[i+1]);
      return NULL;
    }
  }
  a = (double *)CALLOC(3*n,sizeof(double));
  ptMemoryError(a,__FILE__,__LINE__);
  b = (double *)CALLOC(n,sizeof(double));
  ptMemoryError(b,__FILE__,__LINE__);
//
//  Set up the first equation.
//
  if ( ibcbeg == 0 ) {
    b[0] = 0.0E+00;
    a[1+0*3] = 1.0E+00;
    a[0+1*3] = -1.0E+00;
  } else if ( ibcbeg == 1 ) {
    b[0] = ( y[1] - y[0] ) / ( t[1] - t[0] ) - ybcbeg;
    a[1+0*3] = ( t[1] - t[0] ) / 3.0E+00;
    a[0+1*3] = ( t[1] - t[0] ) / 6.0E+00;
  } else if ( ibcbeg == 2 ) {
    b[0] = ybcbeg;
    a[1+0*3] = 1.0E+00;
    a[0+1*3] = 0.0E+00;
  } else {
    ptLogError(ptError_Spline,
               "spline_cubic_set() error: "
         "IBCBEG must be 0, 1 or 2. The input value is %u.\n", ibcbeg);
    FREE(a);
    FREE(b);
    return NULL;
  }
//
//  Set up the intermediate equations.
//
  for ( i = 1; i < n-1; i++ ) {
    b[i] = ( y[i+1] - y[i] ) / ( t[i+1] - t[i] )
      - ( y[i] - y[i-1] ) / ( t[i] - t[i-1] );
    a[2+(i-1)*3] = ( t[i] - t[i-1] ) / 6.0E+00;
    a[1+ i   *3] = ( t[i+1] - t[i-1] ) / 3.0E+00;
    a[0+(i+1)*3] = ( t[i+1] - t[i] ) / 6.0E+00;
  }
//
//  Set up the last equation.
//
  if ( ibcend == 0 ) {
    b[n-1] = 0.0E+00;
    a[2+(n-2)*3] = -1.0E+00;
    a[1+(n-1)*3] = 1.0E+00;
  } else if ( ibcend == 1 ) {
    b[n-1] = ybcend - ( y[n-1] - y[n-2] ) / ( t[n-1] - t[n-2] );
    a[2+(n-2)*3] = ( t[n-1] - t[n-2] ) / 6.0E+00;
    a[1+(n-1)*3] = ( t[n-1] - t[n-2] ) / 3.0E+00;
  } else if ( ibcend == 2 ) {
    b[n-1] = ybcend;
    a[2+(n-2)*3] = 0.0E+00;
    a[1+(n-1)*3] = 1.0E+00;
  } else {
    ptLogError(ptError_Spline,
               "spline_cubic_set() error: "
         "IBCEND must be 0, 1 or 2. The input value is %u", ibcend);
    FREE(a);
    FREE(b);
    return NULL;
  }
//
//  Solve the linear system.
//
  if ( n == 2 && ibcbeg == 0 && ibcend == 0 ) {
    ypp = (double *)CALLOC(2,sizeof(double));
    ptMemoryError(ypp,__FILE__,__LINE__);

    ypp[0] = 0.0E+00;
    ypp[1] = 0.0E+00;
  } else {
    ypp = d3_np_fs ( n, a, b );

    if ( !ypp ) {
      ptLogError(ptError_Spline,
                 "spline_cubic_set() error: "
           "The linear system could not be solved.\n");
      FREE(a);
      FREE(b);
      return NULL;
    }

  }

  FREE(a);
  FREE(b);
  return ypp;
}

//**********************************************************************
//
//  Purpose:
//
//    SPLINE_CUBIC_VAL evaluates a piecewise cubic spline at a point.
//
//  Discussion:
//
//    SPLINE_CUBIC_SET must have already been called to define the values of YPP.
//
//    For any point T in the interval T(IVAL), T(IVAL+1), the form of
//    the spline is
//
//      SPL(T) = A
//             + B * ( T - T(IVAL) )
//             + C * ( T - T(IVAL) )**2
//             + D * ( T - T(IVAL) )**3
//
//    Here:
//      A = Y(IVAL)
//      B = ( Y(IVAL+1) - Y(IVAL) ) / ( T(IVAL+1) - T(IVAL) )
//        - ( YPP(IVAL+1) + 2 * YPP(IVAL) ) * ( T(IVAL+1) - T(IVAL) ) / 6
//      C = YPP(IVAL) / 2
//      D = ( YPP(IVAL+1) - YPP(IVAL) ) / ( 6 * ( T(IVAL+1) - T(IVAL) ) )
//
//  Modified:
//
//      07 January 2005    Shawn Freeman (pure C modifications)
//    04 February 1999    John Burkardt
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int n, the number of knots.
//
//    Input, double Y[N], the data values at the knots.
//
//    Input, double T[N], the knot values.
//
//    Input, double TVAL, a point, typically between T[0] and T[N-1], at
//    which the spline is to be evalulated.  If TVAL lies outside
//    this range, extrapolation is used.
//
//    Input, double Y[N], the data values at the knots.
//
//    Input, double YPP[N], the second derivatives of the spline at
//    the knots.
//
//    Output, double *YPVAL, the derivative of the spline at TVAL.
//
//    Output, double *YPPVAL, the second derivative of the spline at TVAL.
//
//    Output, double SPLINE_VAL, the value of the spline at TVAL.
//

double ptCurve::spline_cubic_val ( int n, double t[], double tval, double y[],
  double ypp[], double *ypval, double *yppval ) {
  double dt;
  double h;
  int i;
  int ival;
  double yval;
//
//  Determine the interval [ T(I), T(I+1) ] that contains TVAL.
//  Values below T[0] or above T[N-1] use extrapolation.
//
  ival = n - 2;

  for ( i = 0; i < n-1; i++ ) {
    if ( tval < t[i+1] ) {
      ival = i;
      break;
    }
  }
//
//  In the interval I, the polynomial is in terms of a normalized
//  coordinate between 0 and 1.
//
  dt = tval - t[ival];
  h = t[ival+1] - t[ival];

  yval = y[ival]
    + dt * ( ( y[ival+1] - y[ival] ) / h
     - ( ypp[ival+1] / 6.0E+00 + ypp[ival] / 3.0E+00 ) * h
    + dt * ( 0.5E+00 * ypp[ival]
    + dt * ( ( ypp[ival+1] - ypp[ival] ) / ( 6.0E+00 * h ) ) ) );

  *ypval = ( y[ival+1] - y[ival] ) / h
    - ( ypp[ival+1] / 6.0E+00 + ypp[ival] / 3.0E+00 ) * h
    + dt * ( ypp[ival]
    + dt * ( 0.5E+00 * ( ypp[ival+1] - ypp[ival] ) / h ) );

  *yppval = ypp[ival] + dt * ( ypp[ival+1] - ypp[ival] ) / h;

  return yval;
}
////////////////////////////////////////////////////////////////////////////////
