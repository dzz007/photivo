/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/
/*!
  \class ptCurve

  \brief The class ptCurve is the abstraction of a curve to be used.
*/

#ifndef PTCURVE_H
#define PTCURVE_H

//==============================================================================

#include <QSettings>

#include "ptDefines.h"
#include "ptConstants.h"

class ptImage;

//==============================================================================

/*!
   'gamma' functions operating in the (0,0)(1,1) box.

   GammaSRGB is the correct one for sRGB encoding.
   GammaBT709 is the BT709 one , used by dcraw.
   GammaPure22 is a pure 2.2 gamma curve.
   GammaTool is the gamma function as used in ufraw.
   DeltaGammaTool is the gamma function as used in ufraw, but
      sRGB 'subtracted' as it is added afterwards as standard part of the flow.
   InverseGammaSRGB is the inverse for sRGB encoding.

   Args is sometimes dummy but required for matching general signature of
   SetCurveFromFunction.
*/
double GammaBT709(double r, double Dummy1, double Dummy2);
double GammaSRGB(double r, double Dummy1, double Dummy2);
double GammaPure22(double r, double Dummy1, double Dummy2);
double GammaTool(double r, double Gamma, double Linearity);
double DeltaGammaTool(double r, double Gamma, double Linearity);
double InverseGammaSRGB(double r, double Dummy1, double Dummy2);

/*! 'Sigmoidal' function operating in the (0,0)(1,1) box. */
double Sigmoidal(double r, double Threshold, double Contrast);

//==============================================================================

class ptCurve {
public:
  /*! Constructor sets the curve to 0 with no anchors. */
  ptCurve(const short Channel = 0);
  ~ptCurve() {}


  QString id() { return FId; }
  void setId(const QString &AId) { FId = AId; }

  /*! Reads the curve from the settings file object given by APtsFile.
      Atm this function is only used by the "spot adjust" tool. */
  void ReadFromFile(QSettings *APtsFile);

  /*! Writes the curve to the settings file object given by APtsFile.
      Atm this function is only used by the "spot adjust" tool. */
  void WriteToFile(QSettings *APtsFile);




  /*! Set Curve from Curve */
  short Set(ptCurve *Curve);

  /*! Read the anchors from a simple file in the format X0 Y0 \n X1 Y1 ...
   Returns 0 on success.*/
  short ReadAnchors(const char *FileName);

  /*! More complete reading and writing function (compatible).
      Header is a free text that is inserted as comment to describe
      the curve.*/
  short WriteCurve(const char* FileName,const char *Header = NULL);
  short ReadCurve(const char* FileName);

  /*! Sets a curve, via splines, from anchors. Returns 0 on success. */
  short SetCurveFromAnchors();

  /*! Sets a straight line with Anchors. */
  short SetNullCurve(const short Channel);

  /*! Sets a curve from a mathematical function over the (0,0)..(1,1) box.
       The function is passed as parameter Function who's first argument
       will be the x (0..1) of the function.
       Optional arguments in ... are passed to the Args part of Function.
       Returns 0 on success.*/
  short SetCurveFromFunction(double(*Function)(double r, double Arg1, double Arg2),
                             double Arg1,
                             double Arg2);

  /*! Apply a curve on a curve (=multiplication).
     Multiplication of curves is not commutative.
     AfterThis (read 'after this') = 1 applies the Curve argument
       on the result of 'this' curve.
     AfterThis = 0 applies the Curve argument before applying 'this' curve.*/
  ptCurve* ApplyCurve(const ptCurve  *Curve,
                      const short    AfterThis);

  /*! Dumps the curve data to a file.
     Scale defines by what Y is divided and over what amount X is sampled
     Returns 0 on success.*/
  short DumpData(const char* FileName,
                 const short Scale=0x100);

  /*! Spline functions.
   If needed see details in the code, but you normally would not use those.*/
  double *d3_np_fs( int n, double a[], double b[] );
  double *spline_cubic_set (int n, double t[], double y[], int ibcbeg,
                            double ybcbeg, int ibcend, double ybcend );
  double spline_cubic_val (int n, double t[], double tval, double y[],
                           double ypp[], double *ypval, double *yppval );


  short     m_IntendedChannel;  // RGB|R|G|B|L
  short     m_Type;             // Anchor or Full.
  uint16_t  m_Curve[0x10000];   // 0x10000 samples of 0..0xffff values.
  short     m_IntType;          // Interpolation Type

  // Nr of anchors when described in terms of anchors.
  // The anchors in the (0,0)-(1,1) box.
  short     m_NrAnchors;
  double    m_XAnchor[ptMaxAnchors];
  double    m_YAnchor[ptMaxAnchors];

//------------------------------------------------------------------------------

private:
  // Unique ID string used to construct the keys in pts files.
  // Atm only used by the "spot adjust" tool.
  QString FId;
  short   FChannel;


};

//==============================================================================

// Some program wide defined curves
extern ptCurve*  RGBGammaCurve;
extern ptCurve*  RGBContrastCurve;
extern ptCurve*  ExposureCurve;
extern ptCurve*  ContrastCurve;
// RGB,R,G,B,L,a,b,Base
extern ptCurve*  Curve[17];


#endif // PTCURVE_H
