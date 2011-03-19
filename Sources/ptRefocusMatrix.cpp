/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
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
********************************************************************************
**
** This file is largely based on the digiKam project.
** http://www.digikam.org
** Copyright (C) 2005-2007 by Gilles Caulier
**   <caulier dot gilles at gmail dot com>
** Original implementation from Refocus Gimp plug-in
** Copyright (C) 1999-2003 Ernst Lippe
**
*******************************************************************************/

// Uncomment this line to debug matrix computation.
//#define RF_DEBUG 1

#include "ptDefines.h"
#include "ptRefocusMatrix.h"

#include <cassert>
#include <cstring>
#include <cmath>

extern "C"
{
#include "clapack/f2c.h"
#include "clapack/clapack.h"
}

ptMat *ptRefocusMatrix::AllocateMatrix(int nRows, int nCols) {
  ptMat *Result = new ptMat;
  memset (Result, 0, sizeof(Result));

  Result->Cols = nCols;
  Result->Rows = nRows;
  Result->Data = new double [nRows * nCols];
  memset(Result->Data, 0, nRows * nCols * sizeof(double));

  return (Result);
}

void ptRefocusMatrix::FinishMatrix (ptMat* mat) {
  delete [] mat->Data;
}

void ptRefocusMatrix::FinishAndFreeMatrix(ptMat* mat) {
  delete [] mat->Data;
  delete mat;
}

double *ptRefocusMatrix::mat_eltptr(ptMat* mat,const int r,const int c) {
  assert ((r >= 0) && (r < mat->Rows));
  assert ((c >= 0) && (c < mat->Rows));
  return (&(mat->Data[mat->Rows * c + r]));
}

double ptRefocusMatrix::mat_elt(const ptMat* mat,const int r,const int c) {
  assert ((r >= 0) && (r < mat->Rows));
  assert ((c >= 0) && (c < mat->Rows));
  return (mat->Data[mat->Rows * c + r]);
}

void ptRefocusMatrix::InitCMatrix(ptCMat*   Matrix,
                                  const int MatrixRadius) {
  Matrix->Radius = MatrixRadius;
  Matrix->RowStride = 2 * MatrixRadius + 1;
  Matrix->Data = new double [SQR (Matrix->RowStride)];
  memset (Matrix->Data, 0, SQR (Matrix->RowStride) * sizeof(double));
  Matrix->Center =
    Matrix->Data + Matrix->RowStride * Matrix->Radius + Matrix->Radius;
}

ptCMat *ptRefocusMatrix::AllocateCMatrix(const int MatrixRadius) {
  ptCMat *Result = new ptCMat;
  memset(Result, 0, sizeof(Result));
  InitCMatrix(Result, MatrixRadius);
  return (Result);
}

void ptRefocusMatrix::FinishCMatrix(ptCMat* mat) {
  delete [] mat->Data;
  mat->Data = NULL;
}

inline double *ptRefocusMatrix::c_mat_eltptr(ptCMat* mat,
                                             const int col,
                                             const int row) {
  assert ((abs (row) <= mat->Radius) && (abs (col) <= mat->Radius));
  return (mat->Center + mat->RowStride * row + col);
}

inline double ptRefocusMatrix::c_mat_elt(const ptCMat* const mat,
                                         const int col, const int row) {
  assert ((abs (row) <= mat->Radius) && (abs (col) <= mat->Radius));
  return (mat->Center[mat->RowStride * row + col]);
}

void ptRefocusMatrix::ConvolveMatrix(      ptCMat* Result,
                                     const ptCMat* const mata,
                                     const ptCMat* const matb) {
  register int xr, yr, xa, ya;

  for (yr = -Result->Radius; yr <= Result->Radius; yr++) {
      for (xr = -Result->Radius; xr <= Result->Radius; xr++) {
          const int ya_low  = MAX (-mata->Radius, yr - matb->Radius);
          const int ya_high = MIN (mata->Radius, yr + matb->Radius);
          const int xa_low  = MAX (-mata->Radius, xr - matb->Radius);
          const int xa_high = MIN (mata->Radius, xr + matb->Radius);
          register double val = 0.0;

          for (ya = ya_low; ya <= ya_high; ya++) {
              for (xa = xa_low; xa <= xa_high; xa++) {
                  val += c_mat_elt (mata, xa, ya) *
                      c_mat_elt (matb, xr - xa, yr - ya);
              }
          }

          *c_mat_eltptr (Result, xr, yr) = val;
      }
  }
}

void ptRefocusMatrix::ConvolveStarMatrix(      ptCMat* Result,
                                         const ptCMat* const mata,
                                         const ptCMat* const matb) {
  register int xr, yr, xa, ya;

  for (yr = -Result->Radius; yr <= Result->Radius; yr++) {
      for (xr = -Result->Radius; xr <= Result->Radius; xr++) {
          const int ya_low = MAX (-mata->Radius, -matb->Radius - yr);
          const int ya_high = MIN (mata->Radius, matb->Radius - yr);
          const int xa_low = MAX (-mata->Radius, -matb->Radius - xr);
          const int xa_high = MIN (mata->Radius, matb->Radius - xr);
          register double val = 0.0;

          for (ya = ya_low; ya <= ya_high; ya++) {
              for (xa = xa_low; xa <= xa_high; xa++) {
                  val += c_mat_elt (mata, xa, ya) *
                      c_mat_elt (matb, xr + xa, yr + ya);
              }
          }

          *c_mat_eltptr (Result, xr, yr) = val;
      }
  }
}

void ptRefocusMatrix::ConvolveMatrixFunction(      ptCMat* Result,
                                             const ptCMat* const mata,
                                             double (f) (int, int)) {
  register int xr, yr, xa, ya;

  for (yr = -Result->Radius; yr <= Result->Radius; yr++) {
      for (xr = -Result->Radius; xr <= Result->Radius; xr++) {
          register double val = 0.0;

          for (ya = -mata->Radius; ya <= mata->Radius; ya++) {
              for (xa = -mata->Radius; xa <= mata->Radius; xa++) {
                  val += c_mat_elt (mata, xa, ya) * f (xr - xa, yr - ya);
              }
          }

          *c_mat_eltptr (Result, xr, yr) = val;
      }
  }
}

int ptRefocusMatrix::as_idx (const int k, const int l, const int m) {
  return ((k + m) * (2 * m + 1) + (l + m));
}

int ptRefocusMatrix::as_cidx (const int k, const int l) {
  const int a = MAX (abs (k), abs (l));
  const int b = MIN (abs (k), abs (l));
  return ((a * (a + 1)) / 2 + b);
}

ptMat *ptRefocusMatrix::MakeSMatrix(ptCMat* Matrix,
                                    int     MatrixRadius,
                                    double  NoiseFactor) {
  const int mat_size = SQR (2 * MatrixRadius + 1);
  ptMat *Result = AllocateMatrix (mat_size, mat_size);
  register int yr, yc, xr, xc;

  for (yr = -MatrixRadius; yr <= MatrixRadius; yr++) {
      for (xr = -MatrixRadius; xr <= MatrixRadius; xr++) {
          for (yc = -MatrixRadius; yc <= MatrixRadius; yc++) {
              for (xc = -MatrixRadius; xc <= MatrixRadius; xc++) {
                  *mat_eltptr(Result,as_idx(xr,yr,MatrixRadius),
                              as_idx(xc,yc,MatrixRadius)) =
                      c_mat_elt(Matrix, xr - xc, yr - yc);
                  if ((xr == xc) && (yr == yc)) {
                      *mat_eltptr (Result,as_idx(xr,yr,MatrixRadius),
                                  as_idx(xc,yc,MatrixRadius)) += NoiseFactor;
                  }
              }
          }
      }
  }

  return (Result);
}

ptMat *ptRefocusMatrix::MakeSCMatrix(ptCMat* Matrix,
                                     int     MatrixRadius,
                                     double  NoiseFactor) {
  const int mat_size = as_cidx (MatrixRadius + 1, 0);
  ptMat *Result = AllocateMatrix (mat_size, mat_size);
  register int yr, yc, xr, xc;

  for (yr = 0; yr <=MatrixRadius; yr++) {
      for (xr = 0; xr <= yr; xr++) {
          for (yc = -MatrixRadius; yc <= MatrixRadius; yc++) {
              for (xc = -MatrixRadius; xc <= MatrixRadius; xc++) {
                  *mat_eltptr (Result, as_cidx (xr, yr), as_cidx (xc, yc)) +=
                      c_mat_elt (Matrix, xr - xc, yr - yc);
                  if ((xr == xc) && (yr == yc)) {
                      *mat_eltptr (Result, as_cidx (xr, yr),
                                  as_cidx (xc, yc)) += NoiseFactor;
                  }
              }
          }
      }
  }

  return (Result);
}

double ptRefocusMatrix::Correlation(const int    x,
                                    const int    y,
                                    const double Gamma,
                                    const double musq) {
  return (musq + pow (Gamma, sqrt (SQR (x) + SQR (y))));
}

ptMat *ptRefocusMatrix::CopyVector(const ptCMat* const mat, const int m) {
  ptMat *Result = AllocateMatrix (SQR (2 * m + 1), 1);
  register int x, y, index = 0;

  for (y = -m; y <= m; y++) {
      for (x = -m; x <= m; x++) {
          *mat_eltptr (Result, index, 0) = c_mat_elt (mat, x, y);
          index++;
      }
  }

  assert (index == SQR (2 * m + 1));
  return (Result);
}

ptMat *ptRefocusMatrix::CopyCVector(const ptCMat* const mat, const int m) {
  ptMat *Result = AllocateMatrix (as_cidx (m + 1, 0), 1);
  register int x, y, index = 0;

  for (y = 0; y <= m; y++) {
      for (x = 0; x <= y; x++) {
          *mat_eltptr (Result, index, 0) = c_mat_elt (mat, x, y);
          index++;
      }
  }

  assert (index == as_cidx (m + 1, 0));
  return (Result);
}

ptCMat *ptRefocusMatrix::CopyCVector2Matrix(const ptMat* const cvec,
                                            const int m) {
  ptCMat *Result = AllocateCMatrix (m);
  register int x, y;

  for (y = -m; y <= m; y++) {
      for (x = -m; x <= m; x++) {
          *c_mat_eltptr (Result, x, y) = mat_elt (cvec, as_cidx (x, y), 0);
      }
  }

  return (Result);
}

ptCMat *ptRefocusMatrix::CopyVector2Matrix(const ptMat* const cvec,
                                           const int m) {
  ptCMat *Result = AllocateCMatrix (m);
  register int x, y;

  for (y = -m; y <= m; y++) {
      for (x = -m; x <= m; x++) {
          *c_mat_eltptr (Result, x, y) = mat_elt (cvec, as_idx (x, y, m), 0);
      }
  }

  return (Result);
}

ptCMat *ptRefocusMatrix::ComputeG(const ptCMat* const Convolution,
                                  const int     MatrixRadius,
                                  const double  Gamma,
                                  const double  NoiseFactor,
                                  const double  musq,
                                  const short   symmetric) {
  ptCMat h_conv_ruv, a, corr;
  ptCMat *Result;
  ptMat *b;
  ptMat *s;
  int status;

  InitCMatrix (&h_conv_ruv, 3 * MatrixRadius);
  FillMatrix2 (&corr, 4 * MatrixRadius, Correlation, Gamma, musq);
  ConvolveMatrix (&h_conv_ruv, Convolution, &corr);
  InitCMatrix (&a, 2 * MatrixRadius);
  ConvolveStarMatrix (&a, Convolution, &h_conv_ruv);

  if (symmetric) {
      s = MakeSCMatrix (&a, MatrixRadius, NoiseFactor);
      b = CopyCVector (&h_conv_ruv, MatrixRadius);
  } else {
      s = MakeSMatrix (&a, MatrixRadius, NoiseFactor);
      b = CopyVector (&h_conv_ruv, MatrixRadius);
  }

  assert (s->Cols == s->Rows);
  assert (s->Rows == b->Rows);
  status = dgesv (s->Rows, 1, s->Data, s->Rows, b->Data, b->Rows);

  if (symmetric) {
      Result = CopyCVector2Matrix (b, MatrixRadius);
  } else {
      Result = CopyVector2Matrix (b, MatrixRadius);
  }

  FinishCMatrix (&a);
  FinishCMatrix (&h_conv_ruv);
  FinishCMatrix (&corr);
  FinishAndFreeMatrix (s);
  FinishAndFreeMatrix (b);
  return (Result);
}

ptCMat *ptRefocusMatrix::ComputeGMatrix(const ptCMat* const Convolution,
                                        const int     MatrixRadius,
                                        const double  Gamma,
                                        const double  NoiseFactor,
                                        const double  musq,
                                        const short   symmetric) {

  ptCMat *g = ComputeG(Convolution,
                       MatrixRadius,
                       Gamma,
                       NoiseFactor,
                       musq,
                       symmetric);
  int r, c;
  double sum = 0.0;

  /* Determine sum of array */
  for (r = -g->Radius; r <= g->Radius; r++) {
      for (c = -g->Radius; c <= g->Radius; c++) {
          sum += c_mat_elt (g, r, c);
      }
  }

  for (r = -g->Radius; r <= g->Radius; r++) {
      for (c = -g->Radius; c <= g->Radius; c++) {
          *c_mat_eltptr (g, r, c) /= sum;
      }
  }

  return (g);
}

void ptRefocusMatrix::FillMatrix(      ptCMat* Matrix,
                                 const int     MatrixRadius,
                                       double  f(const int,
                                                 const int,
                                                 const double),
                                 const double  fun_arg) {
  register int x, y;
  InitCMatrix(Matrix,MatrixRadius);

  for (y = -MatrixRadius; y <= MatrixRadius; y++) {
    for (x = -MatrixRadius; x <= MatrixRadius; x++) {
      *c_mat_eltptr(Matrix,x,y) = f(x,y,fun_arg);
    }
  }
}

void ptRefocusMatrix::FillMatrix2(      ptCMat* Matrix,
                                  const int     MatrixRadius,
                                        double  f (const int,
                                                   const int,
                                                   const double,
                                                   const double),
                                  const double  fun_arg1,
                                  const double  fun_arg2) {
  register int x, y;
  InitCMatrix (Matrix,MatrixRadius);

  for (y = -MatrixRadius; y <= MatrixRadius; y++) {
      for (x = -MatrixRadius; x <= MatrixRadius; x++) {
          *c_mat_eltptr(Matrix,x,y) = f(x,y,fun_arg1,fun_arg2);
      }
  }
}

void ptRefocusMatrix::MakeGaussianConvolution(const double gRadius,
                                              ptCMat*      Convolution,
                                              const int    MatrixRadius) {
  register int x, y;

  InitCMatrix(Convolution,MatrixRadius);

  if (SQR (gRadius) <= 1 / 3.40282347e38F) {
      for (y = -MatrixRadius; y <= MatrixRadius; y++) {
          for (x = -MatrixRadius; x <= MatrixRadius; x++) {
              *c_mat_eltptr(Convolution,x,y) = 0;
          }
      }

      *c_mat_eltptr (Convolution,0,0) = 1;
  } else {
      const double alpha = log (2.0) / SQR (gRadius);

      for (y = -MatrixRadius; y <= MatrixRadius; y++) {
          for (x = -MatrixRadius; x <= MatrixRadius; x++) {
              *c_mat_eltptr(Convolution,x,y) =
                  exp (-alpha * (SQR (x) + SQR (y)));
          }
      }
  }
}

/** Return the integral of sqrt(Radius^2 - z^2) for z = 0 to x. */

double ptRefocusMatrix::CircleIntegral(const double x, const double Radius) {
  if (Radius == 0) {
      // Perhaps some epsilon must be added here.
      return (0);
  } else {
      const double sin = x / Radius;
      const double sq_diff = SQR (Radius) - SQR (x);
      // From a mathematical point of view the following is redundant.
      // Numerically they are not equivalent!

      if ((sq_diff < 0.0) || (sin < -1.0) || (sin > 1.0)) {
          if (sin < 0) {
              return (-0.25 * SQR (Radius) * M_PI);
          } else {
              return (0.25 * SQR (Radius) * M_PI);
          }
      } else {
          return (0.5 * x * sqrt (sq_diff) + 0.5 * SQR (Radius) * asin (sin));
      }
  }
}

double ptRefocusMatrix::CircleIntensity(const int x,
                                        const int y,
                                        const double Radius) {
  double ReturnValue;
  if (Radius == 0) {
      ReturnValue = (((x == 0) && (y == 0)) ? 1 : 0);
  } else {
      register double xlo = abs (x) - 0.5, xhi = abs (x) + 0.5,
          ylo = abs (y) - 0.5, yhi = abs (y) + 0.5;
      register double symmetry_factor = 1, xc1, xc2;

      if (xlo < 0) {
          xlo = 0;
          symmetry_factor *= 2;
      }

      if (ylo < 0) {
          ylo = 0;
          symmetry_factor *= 2;
      }

      if (SQR (xlo) + SQR (yhi) > SQR (Radius)) {
          xc1 = xlo;
      } else if (SQR (xhi) + SQR (yhi) > SQR (Radius)) {
          xc1 = sqrt (SQR (Radius) - SQR (yhi));
      } else {
          xc1 = xhi;
      }

      if (SQR (xlo) + SQR (ylo) > SQR (Radius)) {
          xc2 = xlo;
      } else if (SQR (xhi) + SQR (ylo) > SQR (Radius)) {
          xc2 = sqrt (SQR (Radius) - SQR (ylo));
      } else {
          xc2 = xhi;
      }

      ReturnValue = (((yhi - ylo) * (xc1 - xlo) +
              CircleIntegral(xc2, Radius) - CircleIntegral(xc1, Radius) -
              (xc2 - xc1) * ylo) * symmetry_factor / (M_PI * SQR (Radius)));
  }
  return ReturnValue;
}

void ptRefocusMatrix::MakeCircleConvolution(const double  Radius,
                                                  ptCMat* Convolution,
                                            const int     MatrixRadius) {
  FillMatrix(Convolution,MatrixRadius,CircleIntensity,Radius);
}

int ptRefocusMatrix::dgesv (const int     N,
                            const int     NRHS,
                                  double* A,
                            const int     lda,
                                  double* B,
                            const int     ldb) {
  int Result = 0;
  integer i_N = N, i_NHRS = NRHS, i_lda = lda, i_ldb = ldb, info;
  integer *ipiv = new integer[N];

  // Clapack call.
  dgesv_ (&i_N, &i_NHRS, A, &i_lda, ipiv, B, &i_ldb, &info);

  delete [] ipiv;
  Result = info;
  return (Result);
}

////////////////////////////////////////////////////////////////////////////////
