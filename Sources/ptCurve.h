/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010-2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012-2013 Bernd Schoeler <brjohn@brother-john.net>
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
#ifndef PTCURVE_H
#define PTCURVE_H

#include "ptStorable.h"
#include <array>
#include <vector>
#include <utility>

class ptImage;

typedef std::pair<double,double> TAnchor;
typedef std::vector<TAnchor>     TAnchorList;

class ptCurve: public ptStorable {
public:
  // Don’t mess with the enum values!
  enum TInterpolation { LinearInterpol = 0, SplineInterpol = 1, CosineInterpol = 2 };
  enum TType          { FullPrecalcType = 0, AnchorType = 1 };
  enum TMask {
    NoMask       = 0,
    GammaMask    = 1,
    LumaMask     = 2,
    ChromaMask   = 4,
    AChannelMask = 8,
    BChannelMask = 16
  };
  typedef QFlags<TMask> TMasks;

public:
  /*! \name Static helper functions that return common null curves. *//*! @{*/
  static TAnchorList diagonalNull();
  static TAnchorList horizontalMidNull();
  static TAnchorList horizontalQuarterNull();
  /*! @}*/

public:
  ptCurve();
  ptCurve(const TAnchorList   &AAnchors,
          const bool           AImmediateCalc = true);
  ptCurve(const TAnchorList    &ANullAnchors,
          const TMasks         &ASupportedMasks,
          const TMask          &ACurrentMask,
          const TInterpolation &AInterpolType   = SplineInterpol);
  ~ptCurve();

  void            calcCurve();
  void            set(const ptCurve &AOther);
  void            reset();
  void            setNullCurve(const TAnchorList &AAnchors);
  void            setFromAnchors(const TAnchorList &AAnchors);
  void            setAnchor(const int AIdx, const TAnchor &APoint);
  void            setAnchorY(const int AIdx, const float y);
  void            setFromFunc(double(*Function)(double r, double Arg1, double Arg2),
                              double Arg1,
                              double Arg2);
  int             readCurveFile(const QString &AFileName,
                                const bool     AOnlyAnchors);

  /*! \name Standard setters and getters. *//*! @{*/
  TAnchorList        *anchors        ()       { return &FAnchors; }
  int                 anchorCount    () const { return FAnchors.size(); }
  TMask               mask           () const { return FCurrentMask; }
  TMasks              supportedMasks () const { return FSupportedMasks; }
  void                setMask        (const TMask AMask);
  TType               curveType      () const { return FCurveType; }
  bool                isNull         () const;
  TInterpolation      interpolType   () const { return FInterpolType; }
  void                setInterpolType(const TInterpolation AInterpolType);
  /*! @}*/


  /*! 16bit Lookup table between original channel value (array indexes)
      and curve processed value (array values). */
  std::array<uint16_t, 0x10000> Curve;

protected:
  TConfigStore doStoreConfig(const QString &APrefix) const;
  void         doLoadConfig(const TConfigStore &AConfig, const QString &APrefix);

private:
  void            calcCosineCurve();
  void            calcLinearCurve();
  void            calcSplineCurve();
  void            setCurveType(const TType AType);

  // Spline functions for spline interpolated anchor points.
  double *d3_np_fs        (int n, double a[], double b[]);
  double *spline_cubic_set(int n, const std::vector<double> t, const std::vector<double> y,
                           int ibcbeg, double ybcbeg, int ibcend, double ybcend );
  double  spline_cubic_val(int n, const std::vector<double> t, double tval,
                           const std::vector<double> y, double ypp[], double *ypval, double *yppval);

  TAnchorList         FAnchors;
  TAnchorList         FNullAnchors;
  TType               FCurveType;   // use setCurveType() to change it
  TMask               FCurrentMask;
  TMasks              FSupportedMasks;
  TInterpolation      FInterpolType;
  QString             FFileName;  // only used for the FullPrecalc curve type

public:
  static double Sigmoidal(double r, double Threshold, double Contrast);
  static double GammaTool(double r, double Gamma, double Linearity);
  static double DeltaGammaTool(double r, double Gamma, double Linearity);
  static double InverseGammaSRGB(double r, double Dummy1, double Dummy2);

#ifdef PT_CREATE_CURVES_PROJECT
  static double GammaBT709(double r, double Dummy1, double Dummy2);
  static double GammaSRGB(double r, double Dummy1, double Dummy2);
  static double GammaPure22(double r, double Dummy1, double Dummy2);
  // Read the anchors from a simple file in the format X0 Y0 \n X1 Y1 ...
  // Returns 0 on success.
  short ReadAnchors(const char *FileName);
  // More complete reading and writing function (compatible).
  // Header is a free text that is inserted as comment to describe
  // the curve.
  short WriteCurve(const char* FileName,const char *Header = NULL);
#endif

};

//------------------------------------------------------------------------------
/*! Qt macro that defines \c operator|() for \c TComponents. */
Q_DECLARE_OPERATORS_FOR_FLAGS(ptCurve::TMasks)

#endif // PTCURVE_H
