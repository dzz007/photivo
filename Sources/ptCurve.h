/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2010-2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

//==============================================================================

#include <array>
#include <vector>
#include <utility>

#include <filters/ptFilterConfig.h>

class ptImage;

//==============================================================================

typedef std::pair<double,double> TAnchor;
typedef std::vector<TAnchor>     TAnchorList;

//==============================================================================

class ptCurve {
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
  typedef QFlags<TMask>   TMasks;


public:
  /*! \group Static helper functions that return common null curves. */
  ///@{
  static TAnchorList diagonalNull();
  static TAnchorList horizontalMidNull();
  static TAnchorList horizontalQuarterNull();
  ///@}


public:
  /*! Creates a \c ptCurve instance.
      \param ANullAnchors     A list of anchor points defining the null curve.
      \param ASupportedMasks  An OR combination of \c TMask flags.
      \param ACurrentMask     The initially active mask.
      \param AInterpolType    The inital interpolation type.
   */
  ptCurve(const TAnchorList    &ANullAnchors,
          const TMasks         &ASupportedMasks,
          const TMask          &ACurrentMask,
          const TInterpolation &AInterpolType   = SplineInterpol);

  ptCurve(const TAnchorList &AAnchors, const bool AImmediateCalc = true);
  ptCurve();

  ~ptCurve();


  /*! Calculates the values for the lookup table from the current anchors. Most functions that
      change the curve automatically trigger \c calcCurve(). The exceptions are \c setAnchor()
      and \c setAnchorY() to avoid excessive calculations when you change several anchors
      in a row.
  */
  void            calcCurve();

  /*! Clones all values from another curves. */
  void            set(const ptCurve &AOther);

  /*! Resets the curve to the null curve. Does nothing for \c FullPrecalcType curves.
      \see setNullCurve()
  */
  void            reset();

  /*! Sets the anchors for the null curve (the reset/fallback curve that does nothing).
      Only sets the null curve; to activate it call \c reset().
  */
  void            setNullCurve(const TAnchorList &AAnchors);

  /*! Replaces the current anchors with new ones. Then recalculates the curve.
      \param AAnchors
        The list of anchor points. Coordinates must be normalised to 0.0-1.0 range.
        When the list is empty the default start (0.0,0.0) and end (1.0,1.0) are assumed.
        When the list contains only a single entry, it is assumed to be the start and the
        default end is added automatically.
   */
  void            setFromAnchors(const TAnchorList &AAnchors);

  /*! Changes one anchor. Does *not* automatically recalculate the curve. */
  void            setAnchor(const int AIdx, const TAnchor &APoint);

  /*! Changes one anchor’s Y coordinate. Does *not* automatically recalculate the curve. */
  void            setAnchorY(const int AIdx, const float y);

  /*! Sets a curve from a mathematical function over the (0,0)..(1,1) box.
      The function is passed as parameter Function who's first argument
      will be the x (0..1) of the function.
      Optional arguments in ... are passed to the Args part of Function.
  */
  void            setFromFunc(double(*Function)(double r, double Arg1, double Arg2),
                              double Arg1,
                              double Arg2);

  /*! Sets the config store associated with this curve and updates the curve’s configuration
      from the store. I.e. the store must contain a proper set of curve config items.
      \param AConfig
        A pointer to the config store. Must be valid for the rest of the curve’s lifetime.
      \see assignFilterConfig()
   */
  void            setFromFilterConfig(const TConfigStore &AConfig, const QString &APrefix = "");

  /*! Returns the curve’s configuration in config store format usable by \c ptFilterConfig. */
  TConfigStore    filterConfig(const QString &APrefix = "") const;

  /*! Read a curve from an old-style curve file. This function is the only way to load a
      curve with type \c FullPrecalcType, i.e. a file where the complete 16bit lookup table
      is pre-calculated and stored in the file. Such curves are read-only.
      \param AFileName
        The full path to the .ptc curve file.
      \return
        0 when successful, -1 when an undetermined error occured, or the line number
        in the file where parsing failed.
   */
  int             readCurveFile(const QString &AFileName,
                                const bool     AOnlyAnchors);


  /*! \group Standard setters and getters. */
  ///@{
  TAnchorList        *anchors        ()       { return &FAnchors; }
  int                 anchorCount    () const { return FAnchors.size(); }
  TMask               mask           () const { return FCurrentMask; }
  TMasks              supportedMasks () const { return FSupportedMasks; }
  void                setMask        (const TMask AMask);
  TType               curveType      () const { return FCurveType; }
  bool                isNull         () const;
  TInterpolation      interpolType   () const { return FInterpolType; }
  void                setInterpolType(const TInterpolation AInterpolType);
  ///@}


  /*! 16bit Lookup table between original channel value (array indexes)
      and curve processed value (array values). */
  std::array<uint16_t, 0x10000> Curve;


private:
  void            calcCosineCurve();
  void            calcLinearCurve();
  void            calcSplineCurve();
  void            setCurveType(const TType AType);

  /*! \group Spline functions for spline interpolated anchor points. */
  ///@{
  double *d3_np_fs        (int n, double a[], double b[]);
  double *spline_cubic_set(int n, const std::vector<double> t, const std::vector<double> y,
                           int ibcbeg, double ybcbeg, int ibcend, double ybcend );
  double  spline_cubic_val(int n, const std::vector<double> t, double tval,
                           const std::vector<double> y, double ypp[], double *ypval, double *yppval);
  ///@}


  TAnchorList         FAnchors;
  TAnchorList         FNullAnchors;
  TType               FCurveType;   // use setCurveType() to change it
  TMask               FCurrentMask;
  TMasks              FSupportedMasks;
  TInterpolation      FInterpolType;
  QString             FFileName;  // only used for the FullPrecalc curve type


public:
  /*! 'Sigmoidal' function operating in the (0,0)(1,1) box. */
  static double Sigmoidal(double r, double Threshold, double Contrast);

  /*! 'gamma' functions operating in the (0,0)(1,1) box.
     * GammaSRGB        is the correct one for sRGB encoding.
     * GammaBT709       is the ITU-R BT.709 one, used by dcraw.
     * GammaPure22      is a pure 2.2 gamma curve.
     * GammaTool        based on Gamma and Linearity as used in ufraw.
     * DeltaGammaTool   is the gamma function as used in ufraw, but
                        sRGB 'subtracted' as it is added afterwards as standard part of the flow.
     * InverseGammaSRGB is the inverse for sRGB encoding.
     Args is sometimes dummy but required for matching general signature of
     SetCurveFromFunction.
  */
  static double GammaTool(double r, double Gamma, double Linearity);
  static double DeltaGammaTool(double r, double Gamma, double Linearity);
  static double InverseGammaSRGB(double r, double Dummy1, double Dummy2);
#ifdef PT_CREATE_CURVES_PROJECT
  static double GammaBT709(double r, double Dummy1, double Dummy2);
  static double GammaSRGB(double r, double Dummy1, double Dummy2);
  static double GammaPure22(double r, double Dummy1, double Dummy2);
#endif

#ifdef PT_CREATE_CURVES_PROJECT
public:
  // Read the anchors from a simple file in the format X0 Y0 \n X1 Y1 ...
  // Returns 0 on success.
  short ReadAnchors(const char *FileName);
  // More complete reading and writing function (compatible).
  // Header is a free text that is inserted as comment to describe
  // the curve.
  short WriteCurve(const char* FileName,const char *Header = NULL);
#endif

};

//==============================================================================

/*! Qt macro that defines \c operator|() for \c TComponents. */
Q_DECLARE_OPERATORS_FOR_FLAGS(ptCurve::TMasks)


#endif // PTCURVE_H
