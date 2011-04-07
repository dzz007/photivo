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
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef DLREFOCUSMATRIX_H
#define DLREFOCUSMATRIX_H

// C ++ includes.

#include <cstdio>

// CMat:
// @radius: Radius of the matrix.
//
// Centered matrix. This is a square matrix where
// the indices range from [-radius, radius].
// The matrix contains (2 * radius + 1) ** 2 elements.

typedef struct {
  int     Radius;                // Radius of the matrix
  int     RowStride;             // Size of one row = 2 * Radius + 1
  double *Data;                  // Contents of matrix
  double *Center;                // Points to element with index (0, 0)
} ptCMat;

// Mat:
// @rows: Number of rows in the matrix.
//
// Normal matrix type. Indices range from
// [0, rows -1 ] and [0, cols - 1].

typedef struct {
  int     Rows;                  // Number of Rows in the matrix
  int     Cols;                  // Number of columns in the matrix
  double *Data;                  // Content of the matrix
} ptMat;

class ptRefocusMatrix {

public:

static void FillMatrix(      ptCMat* Matrix,
                       const int     MatrixRadius,
                             double  f(int,int,double),
                       const double  fun_arg);

static void FillMatrix2(      ptCMat* Matrix,
                        const int     MatrixRadius,
                              double  f(const int,
                                        const int,
                                        const double,
                                        const double),
                        const double  fun_arg1,
                        const double  fun_arg2);

static void MakeCircleConvolution(const double  Radius,
                                        ptCMat* Convolution,
                                  const int     MatrixRadius);

static void MakeGaussianConvolution(const double  Alpha,
                                          ptCMat* Convolution,
                                    const int     MatrixRadius);

static void ConvolveStarMatrix(      ptCMat* Result,
                               const ptCMat* const MatrixA,
                               const ptCMat* const MatrixB);

static ptCMat* ComputeGMatrix(const ptCMat* const Convolution,
                              const int     MatrixRadius,
                              const double  Gamma,
                              const double  NoiseFactor,
                              const double  musq,
                              const short   Symmetric);

static void FinishMatrix(ptMat* Matrix);
static void FinishAndFreeMatrix(ptMat* Matrix);
static void InitCMatrix(ptCMat* Matrix, const int MatrixRadius);
static void FinishCMatrix(ptCMat* Matrix);

private:

static ptMat*  AllocateMatrix(int nRows, int nCols);
static double* mat_eltptr(ptMat * mat, const int r, const int c);
static double  mat_elt(const ptMat * mat, const int r, const int c);
static ptCMat* AllocateCMatrix(const int Radius);
static inline double *c_mat_eltptr(ptCMat* mat, const int col, const int row);
static inline double c_mat_elt(const ptCMat* const mat,
                               const int col,
                               const int row);
static void ConvolveMatrix(      ptCMat* Result,
                           const ptCMat* const MatrixA,
                           const ptCMat* const MatrixB);
static void ConvolveMatrixFunction(      ptCMat* Result,
                                   const ptCMat* const MatrixA,
                                         double(f)(int, int));
static int as_idx(const int k, const int l, const int m);
static int as_cidx(const int k, const int l);
static ptMat *MakeSMatrix(ptCMat* Matrix, int m, double NoiseFactor);
static ptMat *MakeSCMatrix(ptCMat* Matrix, int m, double NoiseFactor);
static double Correlation(const int    x,
                          const int    y,
                          const double Gamma,
                          const double musq);
static ptMat*  CopyVector(const ptCMat* const Matrix, const int m);
static ptMat*  CopyCVector(const ptCMat* const Matrix, const int m);
static ptCMat* CopyCVector2Matrix(const ptMat* const cvec, const int m);
static ptCMat* CopyVector2Matrix(const ptMat* const cvec, const int m);
static ptCMat* ComputeG(const ptCMat* const Convolution,
                        const int     m,
                        const double  Gamma,
                        const double  NoiseFactor,
                        const double  musq,
                        const short   Symmetric);
static double CircleIntegral(const double x, const double Radius);
static double CircleIntensity(const int x, const int y, const double Radius);
// CLapack interface.
static int dgesv (const int     N,
                  const int     NRHS,
                        double* A,
                  const int     lda,
                        double* B,
                  const int     ldb);

};

#endif

////////////////////////////////////////////////////////////////////////////////
