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

#include <array>
#include <cmath>

#include <QFile>
#include <QTextStream>

#include "ptCalloc.h"
#include "ptError.h"
#include "ptCurve.h"
#include "ptInfo.h"
#include "ptConstants.h"
#include "ptMessageBox.h"

//==============================================================================

ptCurve::ptCurve(const TAnchorList    &ANullAnchors,
                 const TMasks         &ASupportedMasks,
                 const TMask          &ACurrentMask,
                 const TInterpolation &AInterpolType)
: FCurveType(AnchorType),
  FCurrentMask(ACurrentMask),
  FSupportedMasks(ASupportedMasks),
  FInterpolType(AInterpolType)
{
  setNullCurve(ANullAnchors);  // setNullCurve() might change the anchor list.
  FAnchors = FNullAnchors;     // Use FNullAnchors from this point on, never ANullAnchors!
  calcCurve();
}

//==============================================================================

ptCurve::ptCurve(const TAnchorList &AAnchors, const bool AImmediateCalc)
: FAnchors(AAnchors),
  FCurveType(AnchorType),
  FCurrentMask(NoMask),
  FSupportedMasks(NoMask),
  FInterpolType(SplineInterpol)
{
  if (AImmediateCalc)
    calcCurve();
}

//==============================================================================

ptCurve::ptCurve()
: FCurveType(AnchorType),
  FCurrentMask(NoMask),
  FSupportedMasks(NoMask),
  FInterpolType(SplineInterpol)
{}

//==============================================================================

ptCurve::~ptCurve() {
/*
  Resources managed by Qt parent or other objects. Do not delete manually.
    currently none
*/
}

//==============================================================================

void ptCurve::set(const ptCurve &AOther) {
  Curve           = AOther.Curve;
  FAnchors        = AOther.FAnchors;
  FNullAnchors    = AOther.FNullAnchors;
  FCurveType      = AOther.FCurveType;
  FSupportedMasks = AOther.FSupportedMasks;
  FInterpolType   = AOther.FInterpolType;
  FFileName       = AOther.FFileName;
  calcCurve();
}

//==============================================================================

void ptCurve::setNullCurve(const TAnchorList &AAnchors) {
  this->setCurveType(AnchorType);
  FNullAnchors    = AAnchors;

  // make sure we have at least two anchors.
  if (FNullAnchors.empty())
    FNullAnchors.push_back(TAnchor(0.0, 0.0));

  if (FNullAnchors.size() == 1)
    FNullAnchors.push_back(TAnchor(1.0, 1.0));
}

//==============================================================================

void ptCurve::reset() {
  FCurveType = AnchorType;
  FAnchors = FNullAnchors;
  calcCurve();
}

//==============================================================================

void ptCurve::setFromAnchors(const TAnchorList &AAnchors) {
  this->setCurveType(AnchorType);
  FAnchors = AAnchors;

  // make sure we have at least two anchors.
  if (FAnchors.empty())
    FAnchors.push_back(TAnchor(0.0, 0.0));

  if (FAnchors.size() == 1)
    FAnchors.push_back(TAnchor(1.0, 1.0));

  calcCurve();
}

//==============================================================================

void ptCurve::setAnchor(const int AIdx, const TAnchor &APoint) {
  FAnchors[AIdx] = APoint;
}

//==============================================================================

void ptCurve::setAnchorY(const int AIdx, const float y) {
  FAnchors[AIdx].second = y;
}

//==============================================================================

void ptCurve::calcCurve() {
  if ((FAnchors.size() < 2) || (FCurveType == FullPrecalcType)) return;

  switch (FInterpolType) {
    case SplineInterpol: calcSplineCurve(); break;
    case CosineInterpol: calcCosineCurve(); break;
    case LinearInterpol: calcLinearCurve(); break;
  default:
    GInfo->Raise(QString("Unhandled interpolation type: ") + QString::number(FInterpolType), AT);
  }
}

//==============================================================================

void ptCurve::calcCosineCurve() {
  for(uint32_t a = 0; a < FAnchors[0].first * 0xffff; a++)
    Curve[a] = FAnchors[0].second * 0xffff;

  for(size_t b = 0; b < FAnchors.size()-1; b++) {
    float Factor = FAnchors[b+1].second - FAnchors[b].second;
    float Scale  = ptPI / (float)0xffff / (FAnchors[b+1].first-FAnchors[b].first);

    for(uint32_t i = 0; i < FAnchors[b+1].first * 0xffff - FAnchors[b].first * 0xffff; i++) {
      Curve[i + (int32_t)(FAnchors[b].first * 0xffff)] =
          (int32_t)((( (1-cosf(i*Scale))/2 ) * Factor + FAnchors[b].second)*0xffff);
    }
  }

  auto hMaxAnchorIdx = FAnchors.size()-1;
  for(uint32_t c = FAnchors[hMaxAnchorIdx].first * 0xffff; c < 0x10000; c++) {
    Curve[c] = FAnchors[hMaxAnchorIdx].second * 0xffff;
  }
}

//==============================================================================

void ptCurve::calcLinearCurve() {
  for(uint32_t a = 0; a < FAnchors[0].first * 0xffff; a++) {
    Curve[a] = FAnchors[0].second * 0xffff;
  }

  for(size_t b = 0; b < FAnchors.size()-1; b++) {
    float Factor = (float)(FAnchors[b+1].second - FAnchors[b].second) /
                   (float)(FAnchors[b+1].first - FAnchors[b].first);
    for(uint32_t i = 0; i < FAnchors[b+1].first * 0xffff - FAnchors[b].first * 0xffff; i++) {
      Curve[i+(int32_t)(FAnchors[b].first * 0xffff)] =
         FAnchors[b].second * 0xffff + (int32_t)(Factor*i);
    }
  }

  auto hMaxAnchorIdx = FAnchors.size()-1;
  for(uint32_t c = FAnchors[hMaxAnchorIdx].first * 0xffff; c < 0x10000; c++) {
    Curve[c] = FAnchors[hMaxAnchorIdx].second * 0xffff;
  }
}

//==============================================================================

void ptCurve::calcSplineCurve() {
  std::vector<double> hXAnchors;
  std::vector<double> hYAnchors;
  for (auto hPoint: FAnchors) {
    hXAnchors.push_back(hPoint.first);
    hYAnchors.push_back(hPoint.second);
  }

  double *ypp = spline_cubic_set(FAnchors.size(), hXAnchors, hYAnchors, 2, 0.0, 2, 0.0);
  GInfo->Assert(ypp, "spline_cubic_set() returned a nullptr.", AT);

  double ypval = 0;   //first derivative at a point
  double yppval = 0;  //second derivate at a point

  //Now build a table
  uint16_t firstPointX = (uint16_t) (hXAnchors[0] * 0xffff);
  uint16_t firstPointY = (uint16_t) (hYAnchors[0] * 0xffff);
  uint16_t lastPointX  = (uint16_t) (hXAnchors[FAnchors.size()-1] * 0xffff);
  uint16_t lastPointY  = (uint16_t) (hYAnchors[FAnchors.size()-1] * 0xffff);

  double Resolution = 1.0/(double)(0xffff);

  for(uint32_t i = 0; i < 0x10000; i++) {
    if (i < firstPointX) {
      Curve[i] = firstPointY;
    } else if (i > lastPointX) {
      Curve[i] = lastPointY;
    } else  {
      int32_t Value = (int32_t) (spline_cubic_val(FAnchors.size(), hXAnchors, i*Resolution,
                                                  hYAnchors, ypp, &ypval, &yppval)
                                 * 0xffff + 0.5);
      Curve[i] = CLIP(Value);
    }
  }
  FREE(ypp);
}

//==============================================================================

void ptCurve::setCurveType(const ptCurve::TType AType) {
  FCurveType = AType;
  if (AType == AnchorType) FFileName.clear();
}

//==============================================================================

TConfigStore ptCurve::filterConfig(const QString &APrefix) const {
  TConfigStore hStore;

  hStore.insert(APrefix+"CurveType",     (int)FCurveType);
  hStore.insert(APrefix+"Mask",          (int)FCurrentMask);
  hStore.insert(APrefix+"Interpolation", (int)FInterpolType);
  hStore.insert(APrefix+"FileName",      FFileName);

  if (this->isNull()) {
    hStore.insert(APrefix+"Anchor/size", 0);

  } else if (FCurveType == AnchorType) {
    hStore.insert(APrefix+"Anchor/size", (int)FAnchors.size());
    int i = 0;
    for (TAnchor hAnchor: FAnchors) {
      hStore.insert(QString(APrefix+"Anchor/%1/X").arg(i), hAnchor.first);
      hStore.insert(QString(APrefix+"Anchor/%1/Y").arg(i), hAnchor.second);
      ++i;
    }
  }

  return hStore;
}

//==============================================================================

void ptCurve::setFromFilterConfig(const TConfigStore &AConfig, const QString &APrefix) {
  FCurveType    = (TType)AConfig.value(APrefix+"CurveType", FCurveType).toInt();
  FCurrentMask  = (TMask)AConfig.value(APrefix+"Mask", FCurrentMask).toInt();
  FInterpolType = (TInterpolation)AConfig.value(APrefix+"Interpolation", FInterpolType).toInt();
  FFileName     = AConfig.value(APrefix+"FileName", "").toString();

  if (FCurveType == AnchorType) {
    FAnchors.clear();
    int hSize = AConfig.value(APrefix+"Anchor/size", 0).toInt();

    if (hSize < 2) {  // no/not enough anchors, fall back to null curve
      FAnchors = FNullAnchors;

    } else {  // read anchors
      for (int i = 0; i < hSize; ++i) {
        FAnchors.push_back(TAnchor(AConfig.value(QString(APrefix+"Anchor/%1/X").arg(i), 0.0).toDouble(),
                                   AConfig.value(QString(APrefix+"Anchor/%1/Y").arg(i), 0.0).toDouble()));
      }
      FAnchors.shrink_to_fit();
    }

    calcCurve();

  } else {  // curve from old-style curve file
    int hError = readCurveFile(FFileName, false);
    if (hError != 0) {
      QString hErrMsg = QString(QObject::tr("Failed to load curve file %1.")).arg(FFileName);
      if (hError > 0) hErrMsg += QString(QObject::tr("\nThe error occurred in line %1.")).arg(hError);
      ptMessageBox::critical(nullptr, QObject::tr("Load curve file"), hErrMsg);
      this->reset();
    }
  }
}

//==============================================================================

void ptCurve::setFromFunc(double(*Function)(double r, double Arg1, double Arg2),
                               double Arg1, double Arg2)
{
  this->setCurveType(FullPrecalcType);
  FFileName.clear();

  for (uint32_t i=0; i<0x10000; i++) {
    double r = (double(i) / 0xffff);
    int32_t Value = (int32_t) (0xffff * Function(r,Arg1,Arg2));
    Curve[i] = CLIP(Value);
  }

  calcCurve();
}

//==============================================================================

int ptCurve::readCurveFile(const QString &AFileName, const bool AOnlyAnchors) {
  // No members are updated until the curve file is completely and successfully read.
  // That way we can error-return safely at any point.

  // read the file into a string list
  QStringList hCurveData;
  QFile       hCurveFile(AFileName);

  hCurveFile.open(QIODevice::ReadOnly);
  QTextStream hTextStream(&hCurveFile);

  while (!hTextStream.atEnd()) {
    hCurveData.append(hTextStream.readLine());
  }

  // read and check curve type (precalced or anchors)
  int hNextIdx = hCurveData.indexOf(QRegExp("^CurveType  *[0-1]$"));
  if (hNextIdx == -1) return -1;
  bool ok = false;
  int hCurveType = hCurveData.at(hNextIdx).right(1).toInt(&ok);
  if (!ok || (hCurveType < 0) || (hCurveType > 1))
    return hNextIdx;

  if (hCurveType == 0 && AOnlyAnchors)
    return -2;

  // temporary containers for the actual data
  std::array<uint16_t, 0x10000> hCurveArray;
  TAnchorList          hAnchors;
  hCurveArray.fill(0);

  // Assume data starts immediately after the "CurveType" line
  if (++hNextIdx >= hCurveData.size()) return hNextIdx;

  // read key/value pairs
  for (; hNextIdx < hCurveData.size(); ++hNextIdx) {
    if (hCurveData.at(hNextIdx).trimmed().isEmpty()) continue;  // ignore empty lines
    QStringList hLine = hCurveData.at(hNextIdx).split(" ");     // split line into key and value
    if (hLine.size() != 2) return hNextIdx;

    if (hCurveType == 0) {  // full precalc
      bool ok;
      uint16_t hKey = hLine.at(0).mid(2).toUInt(&ok, 16);
      if (!ok) return hNextIdx;
      uint16_t hVal = hLine.at(1).mid(2).toUInt(&ok, 16);
      if (!ok) return hNextIdx;
      hCurveArray[hKey] = hVal;

    } else {  // anchors
      bool ok;
      uint16_t hKey = hLine.at(0).toUInt(&ok);
      if (!ok) return hNextIdx;
      uint16_t hVal = hLine.at(1).toUInt(&ok);
      if (!ok) return hNextIdx;
      if (hKey > 1000 || hVal > 1000) return hNextIdx;
      hAnchors.push_back(TAnchor((float)hKey/1000.0f, (float)hVal/1000.0f));
    }
  }

  // When we arrive here the file was successfully and completely read.
  if (!AOnlyAnchors) {
    FFileName  = AFileName;
  }
  this->setCurveType((TType)hCurveType);

  if (FCurveType == FullPrecalcType) {
    Curve = hCurveArray;
  } else {
    FInterpolType = SplineInterpol;
    FAnchors = hAnchors;
  }

  calcCurve();
  return 0;
}

//==============================================================================

void ptCurve::setMask(const ptCurve::TMask AMask) {
  if (FSupportedMasks & AMask) {
    FCurrentMask = AMask;
  }
}

//==============================================================================

bool ptCurve::isNull() const {
  return (FCurveType == AnchorType) && (FAnchors == FNullAnchors);
}

//==============================================================================

void ptCurve::setInterpolType(const ptCurve::TInterpolation AInterpolType) {
  if (FInterpolType != AInterpolType) {
    FInterpolType = AInterpolType;
    calcCurve();
  }
}

//==============================================================================

TAnchorList ptCurve::diagonalNull() {
  return TAnchorList( { TAnchor(0.0, 0.0), TAnchor(0.5, 0.5), TAnchor(1.0, 1.0) } );
}

//==============================================================================

TAnchorList ptCurve::horizontalMidNull() {
  return TAnchorList( { TAnchor(0.0, 0.5), TAnchor(0.5, 0.5), TAnchor(1.0, 0.5) } );
}

//==============================================================================

TAnchorList ptCurve::horizontalQuarterNull() {
  return TAnchorList( { TAnchor(0.0, 0.25), TAnchor(0.5, 0.25), TAnchor(1.0, 0.25) } );
}

//==============================================================================

/*  Purpose:
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
*/
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

//==============================================================================

/*  Purpose:
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
*/
double *ptCurve::spline_cubic_set (int n, const std::vector<double> t, const std::vector<double> y, int ibcbeg,
                                   double ybcbeg, int ibcend, double ybcend )
{
  double *a   = nullptr;
  double *b   = nullptr;
  int     i   = 0;
  double *ypp = nullptr;

  for (; i < n - 1; i++ ) {
    if ( t[i+1] <= t[i] ) {
      GInfo->Raise(QString("The knots must be strictly increasing, but T(%1) = %2, T(%3) = %4")
                     .arg(i).arg(t[i]).arg(i+1).arg(t[i+1]), AT);
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
    FREE(a);
    FREE(b);
    GInfo->Raise(QString("IBCBEG must be 0, 1 or 2, but the input value is %1.").arg(ibcbeg), AT);
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
    FREE(a);
    FREE(b);
    GInfo->Raise(QString("IBCEND must be 0, 1 or 2, but the input value is %1.").arg(ibcend), AT);
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
      FREE(a);
      FREE(b);
      GInfo->Raise("The linear system could not be solved.", AT);
    }
  }

  FREE(a);
  FREE(b);
  return ypp;
}

//==============================================================================

/*  Purpose:
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
*/
double ptCurve::spline_cubic_val(int n, const std::vector<double> t, double tval, const std::vector<double> y,
                                 double ypp[], double *ypval, double *yppval )
{
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

//==============================================================================

double ptCurve::GammaTool(double r, double Gamma, double Linearity) {
  const double g = Gamma * (1 - Linearity) / (1-Gamma*Linearity);
  const double a = 1/(1 + Linearity*(g-1));
  const double b = Linearity * (g-1)*a;
  const double c = pow((a*Linearity+b),g) / Linearity;
  return r<Linearity ? c*r : pow (a*r+b,g);
}

//==============================================================================

double ptCurve::DeltaGammaTool(double r, double Gamma, double Linearity) {
  return InverseGammaSRGB(GammaTool(r,Gamma,Linearity),0,0);
}

//==============================================================================

double ptCurve::InverseGammaSRGB(double r, double, double) {
  return (r <= 0.04045 ? r/12.92 : pow((r+0.055)/1.055,2.4) );
}

//==============================================================================

double ptCurve::Sigmoidal(double r, double Threshold, double Contrast) {
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

//==============================================================================

#ifdef PT_CREATE_CURVES_PROJECT
double ptCurve::GammaBT709(double r, double, double) {
  return (r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099 );
}

double ptCurve::GammaSRGB(double r, double, double) {
  return (r <= 0.00304 ? r*12.92 : pow(r,2.5/6)*1.055-0.055 );
}

double ptCurve::GammaPure22(double r, double, double) {
  return (pow(r,2.2));
}


//==============================================================================

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
        fprintf(OutFile,"0x%04X 0x%04X\n",i,Lookup[i]);
      }
      break;
  }
  FCLOSE(OutFile);
  return 0;
}

//==============================================================================

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
#endif // PT_CREATE_CURVES_PROJECT

//==============================================================================

