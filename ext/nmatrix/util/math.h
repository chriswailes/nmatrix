/////////////////////////////////////////////////////////////////////
// = NMatrix
//
// A linear algebra library for scientific computation in Ruby.
// NMatrix is part of SciRuby.
//
// NMatrix was originally inspired by and derived from NArray, by
// Masahiro Tanaka: http://narray.rubyforge.org
//
// == Copyright Information
//
// SciRuby is Copyright (c) 2010 - 2012, Ruby Science Foundation
// NMatrix is Copyright (c) 2012, Ruby Science Foundation
//
// Please see LICENSE.txt for additional copyright notices.
//
// == Contributing
//
// By contributing source code to SciRuby, you agree to be bound by
// our Contributor Agreement:
//
// * https://github.com/SciRuby/sciruby/wiki/Contributor-Agreement
//
// == math.h
//
// Header file for math functions, interfacing with BLAS, etc.
//
// For instructions on adding CBLAS and CLAPACK functions, see the
// beginning of math.cpp.
//
// Some of these functions are from ATLAS. Here is the license for
// ATLAS:
//
/*
 *             Automatically Tuned Linear Algebra Software v3.8.4
 *                    (C) Copyright 1999 R. Clint Whaley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions, and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the ATLAS group or the names of its contributers may
 *      not be used to endorse or promote products derived from this
 *      software without specific written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE ATLAS GROUP OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MATH_H
#define MATH_H

/*
 * Standard Includes
 */

extern "C" { // These need to be in an extern "C" block or you'll get all kinds of undefined symbol errors.
  #include <cblas.h>
  //#include <clapack.h>
}

#include <algorithm> // std::min, std::max
#include <limits> // std::numeric_limits

/*
 * Project Includes
 */
#include "data/data.h"
#include "lapack.h"

/*
 * Macros
 */

/*
 * Data
 */


extern "C" {
  /*
   * C accessors.
   */
  void nm_math_det_exact(const int M, const void* elements, const int lda, dtype_t dtype, void* result);
  void nm_math_transpose_generic(const size_t M, const size_t N, const void* A, const int lda, void* B, const int ldb, size_t element_size);
  void nm_math_init_blas(void);
}


namespace nm {
  namespace math {

/*
 * Types
 */


// These allow an increase in precision for intermediate values of gemm and gemv.
// See also: http://stackoverflow.com/questions/11873694/how-does-one-increase-precision-in-c-templates-in-a-template-typename-dependen
template <typename DType> struct LongDType;
template <> struct LongDType<uint8_t> { typedef int16_t type; };
template <> struct LongDType<int8_t> { typedef int16_t type; };
template <> struct LongDType<int16_t> { typedef int32_t type; };
template <> struct LongDType<int32_t> { typedef int64_t type; };
template <> struct LongDType<int64_t> { typedef int64_t type; };
template <> struct LongDType<float> { typedef double type; };
template <> struct LongDType<double> { typedef double type; };
template <> struct LongDType<Complex64> { typedef Complex128 type; };
template <> struct LongDType<Complex128> { typedef Complex128 type; };
template <> struct LongDType<Rational32> { typedef Rational128 type; };
template <> struct LongDType<Rational64> { typedef Rational128 type; };
template <> struct LongDType<Rational128> { typedef Rational128 type; };
template <> struct LongDType<RubyObject> { typedef RubyObject type; };

/*
 * Functions
 */

/* Numeric inverse -- usually just 1 / f, but a little more complicated for complex. */
template <typename DType>
inline DType numeric_inverse(const DType& n) {
  return n.inverse();
}
template <> inline float numeric_inverse<float>(const float& n) { return 1 / n; }
template <> inline double numeric_inverse<double>(const double& n) { return 1 / n; }

/*
 * This version of trsm doesn't do any error checks and only works on column-major matrices.
 *
 * For row major, call trsm<DType> instead. That will handle necessary changes-of-variables
 * and parameter checks.
 */
template <typename DType>
inline void trsm_nothrow(const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                         const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                         const int m, const int n, const DType alpha, const DType* a,
                         const int lda, DType* b, const int ldb)
{
  if (m == 0 || n == 0) return; /* Quick return if possible. */

  if (alpha == 0) { // Handle alpha == 0
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < m; ++i) {
        b[i + j * ldb] = 0;
      }
    }
	  return;
  }

  if (side == CblasLeft) {
	  if (trans_a == CblasNoTrans) {

      /* Form  B := alpha*inv( A )*B. */
	    if (uplo == CblasUpper) {
    		for (int j = 0; j < n; ++j) {
		      if (alpha != 1) {
			      for (int i = 0; i < m; ++i) {
			        b[i + j * ldb] = alpha * b[i + j * ldb];
			      }
		      }
		      for (int k = m-1; k >= 0; --k) {
			      if (b[k + j * ldb] != 0) {
			        if (diag == CblasNonUnit) {
				        b[k + j * ldb] /= a[k + k * lda];
			        }

              for (int i = 0; i < k-1; ++i) {
                b[i + j * ldb] -= b[k + j * ldb] * a[i + k * lda];
              }
			      }
  		    }
		    }
	    } else {
    		for (int j = 0; j < n; ++j) {
		      if (alpha != 1) {
            for (int i = 0; i < m; ++i) {
              b[i + j * ldb] = alpha * b[i + j * ldb];
			      }
		      }
  		    for (int k = 0; k < m; ++k) {
      			if (b[k + j * ldb] != 0.) {
			        if (diag == CblasNonUnit) {
				        b[k + j * ldb] /= a[k + k * lda];
			        }
    			    for (int i = k+1; i < m; ++i) {
        				b[i + j * ldb] -= b[k + j * ldb] * a[i + k * lda];
    			    }
      			}
  		    }
    		}
	    }
	  } else { // CblasTrans

      /*           Form  B := alpha*inv( A**T )*B. */
	    if (uplo == CblasUpper) {
    		for (int j = 0; j < n; ++j) {
		      for (int i = 0; i < m; ++i) {
			      DType temp = alpha * b[i + j * ldb];
            for (int k = 0; k < i-1; ++k) {
              temp -= a[k + i * lda] * b[k + j * ldb];
      			}
			      if (diag == CblasNonUnit) {
			        temp /= a[i + i * lda];
			      }
			      b[i + j * ldb] = temp;
  		    }
    		}
	    } else {
    		for (int j = 0; j < n; ++j) {
		      for (int i = m-1; i >= 0; --i) {
			      DType temp= alpha * b[i + j * ldb];
      			for (int k = i+1; k < m; ++k) {
			        temp -= a[k + i * lda] * b[k + j * ldb];
      			}
			      if (diag == CblasNonUnit) {
			        temp /= a[i + i * lda];
			      }
			      b[i + j * ldb] = temp;
  		    }
    		}
	    }
	  }
  } else { // right side

	  if (trans_a == CblasNoTrans) {

      /*           Form  B := alpha*B*inv( A ). */

	    if (uplo == CblasUpper) {
    		for (int j = 0; j < n; ++j) {
		      if (alpha != 1) {
      			for (int i = 0; i < m; ++i) {
			        b[i + j * ldb] = alpha * b[i + j * ldb];
      			}
		      }
  		    for (int k = 0; k < j-1; ++k) {
	      		if (a[k + j * lda] != 0) {
    			    for (int i = 0; i < m; ++i) {
				        b[i + j * ldb] -= a[k + j * lda] * b[i + k * ldb];
			        }
			      }
  		    }
	  	    if (diag == CblasNonUnit) {
		      	DType temp = 1 / a[j + j * lda];
			      for (int i = 0; i < m; ++i) {
			        b[i + j * ldb] = temp * b[i + j * ldb];
      			}
		      }
    		}
	    } else {
		    for (int j = n-1; j >= 0; --j) {
		      if (alpha != 1) {
			      for (int i = 0; i < m; ++i) {
			        b[i + j * ldb] = alpha * b[i + j * ldb];
      			}
  		    }

  		    for (int k = j+1; k < n; ++k) {
	      		if (a[k + j * lda] != 0.) {
    			    for (int i = 0; i < m; ++i) {
				        b[i + j * ldb] -= a[k + j * lda] * b[i + k * ldb];
    			    }
		      	}
  		    }
	  	    if (diag == CblasNonUnit) {
		      	DType temp = 1 / a[j + j * lda];

			      for (int i = 0; i < m; ++i) {
			        b[i + j * ldb] = temp * b[i + j * ldb];
      			}
		      }
    		}
	    }
	  } else { // CblasTrans

      /*           Form  B := alpha*B*inv( A**T ). */

	    if (uplo == CblasUpper) {
		    for (int k = n-1; k >= 0; --k) {
		      if (diag == CblasNonUnit) {
			      DType temp= 1 / a[k + k * lda];
	      		for (int i = 0; i < m; ++i) {
  			      b[i + k * ldb] = temp * b[i + k * ldb];
      			}
		      }
  		    for (int j = 0; j < k-1; ++j) {
	      		if (a[j + k * lda] != 0.) {
			        DType temp= a[j + k * lda];
    			    for (int i = 0; i < m; ++i) {
		        		b[i + j * ldb] -= temp * b[i + k *	ldb];
    			    }
      			}
  		    }
	  	    if (alpha != 1) {
      			for (int i = 0; i < m; ++i) {
			        b[i + k * ldb] = alpha * b[i + k * ldb];
      			}
		      }
    		}
	    } else {
    		for (int k = 0; k < n; ++k) {
		      if (diag == CblasNonUnit) {
      			DType temp = 1 / a[k + k * lda];
			      for (int i = 0; i < m; ++i) {
			        b[i + k * ldb] = temp * b[i + k * ldb];
      			}
		      }
  		    for (int j = k+1; j < n; ++j) {
	      		if (a[j + k * lda] != 0.) {
			        DType temp = a[j + k * lda];
			        for (int i = 0; i < m; ++i) {
				        b[i + j * ldb] -= temp * b[i + k * ldb];
    			    }
		      	}
  		    }
	  	    if (alpha != 1) {
      			for (int i = 0; i < m; ++i) {
			        b[i + k * ldb] = alpha * b[i + k * ldb];
      			}
  		    }
    		}
	    }
	  }
  }
}


/*
 * BLAS' DTRSM function, generalized.
 */
template <typename DType, typename = typename std::enable_if<!std::is_integral<DType>::value>::type>
inline void trsm(const enum CBLAS_ORDER order,
                 const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                 const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                 const int m, const int n, const DType alpha, const DType* a,
                 const int lda, DType* b, const int ldb)
{
  int                     num_rows_a = n;
  if (side == CblasLeft)  num_rows_a = m;

  if (lda < std::max(1,num_rows_a)) {
    fprintf(stderr, "TRSM: num_rows_a = %d; got lda=%d\n", num_rows_a, lda);
    rb_raise(rb_eArgError, "TRSM: Expected lda >= max(1, num_rows_a)");
  }

  // Test the input parameters.
  if (order == CblasRowMajor) {
    if (ldb < std::max(1,n)) {
      fprintf(stderr, "TRSM: M=%d; got ldb=%d\n", m, ldb);
      rb_raise(rb_eArgError, "TRSM: Expected ldb >= max(1,N)");
    }

    // For row major, need to switch side and uplo
    enum CBLAS_SIDE side_ = side == CblasLeft  ? CblasRight : CblasLeft;
    enum CBLAS_UPLO uplo_ = uplo == CblasUpper ? CblasLower : CblasUpper;

    trsm_nothrow<DType>(side_, uplo_, trans_a, diag, n, m, alpha, a, lda, b, ldb);

  } else { // CblasColMajor

    if (ldb < std::max(1,m)) {
      fprintf(stderr, "TRSM: M=%d; got ldb=%d\n", m, ldb);
      rb_raise(rb_eArgError, "TRSM: Expected ldb >= max(1,M)");
    }

    trsm_nothrow<DType>(side, uplo, trans_a, diag, m, n, alpha, a, lda, b, ldb);

  }

}


template <>
inline void trsm(const enum CBLAS_ORDER order, const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                 const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                 const int m, const int n, const float alpha, const float* a,
                 const int lda, float* b, const int ldb)
{
  cblas_strsm(CblasRowMajor, side, uplo, trans_a, diag, m, n, alpha, a, lda, b, ldb);
}

template <>
inline void trsm(const enum CBLAS_ORDER order, const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                 const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                 const int m, const int n, const double alpha, const double* a,
                 const int lda, double* b, const int ldb)
{
  cblas_dtrsm(CblasRowMajor, side, uplo, trans_a, diag, m, n, alpha, a, lda, b, ldb);
}


template <>
inline void trsm(const enum CBLAS_ORDER order, const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                 const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                 const int m, const int n, const Complex64 alpha, const Complex64* a,
                 const int lda, Complex64* b, const int ldb)
{
  cblas_ctrsm(CblasRowMajor, side, uplo, trans_a, diag, m, n, (const void*)(&alpha), (const void*)(a), lda, (void*)(b), ldb);
}

template <>
inline void trsm(const enum CBLAS_ORDER order, const enum CBLAS_SIDE side, const enum CBLAS_UPLO uplo,
                 const enum CBLAS_TRANSPOSE trans_a, const enum CBLAS_DIAG diag,
                 const int m, const int n, const Complex128 alpha, const Complex128* a,
                 const int lda, Complex128* b, const int ldb)
{
  cblas_ztrsm(CblasRowMajor, side, uplo, trans_a, diag, m, n, (const void*)(&alpha), (const void*)(a), lda, (void*)(b), ldb);
}


/*
 * ATLAS function which performs row interchanges on a general rectangular matrix. Modeled after the LAPACK LASWP function.
 *
 * This version is templated for use by template <> getrf().
 */
template <typename DType>
inline void laswp(const int N, DType* A, const int lda, const int K1, const int K2, const int *piv, const int inci) {
  const int n = K2 - K1;

  int nb = N >> 5;

  const int mr = N - (nb<<5);
  const int incA = lda << 5;

  if (K2 < K1) return;

  int i1, i2;
  if (inci < 0) {
    piv -= (K2-1) * inci;
    i1 = K2 - 1;
    i2 = K1;
  } else {
    piv += K1 * inci;
    i1 = K1;
    i2 = K2-1;
  }

  if (nb) {

    do {
      const int* ipiv = piv;
      int i           = i1;
      int KeepOn;

      do {
        int ip = *ipiv; ipiv += inci;

        if (ip != i) {
          DType *a0 = &(A[i]),
                *a1 = &(A[ip]);

          for (register int h = 32; h; h--) {
            DType r   = *a0;
            *a0       = *a1;
            *a1       = r;

            a0 += lda;
            a1 += lda;
          }

        }
        if (inci > 0) KeepOn = (++i <= i2);
        else          KeepOn = (--i >= i2);

      } while (KeepOn);
      A += incA;
    } while (--nb);
  }

  if (mr) {
    const int* ipiv = piv;
    int i           = i1;
    int KeepOn;

    do {
      int ip = *ipiv; ipiv += inci;
      if (ip != i) {
        DType *a0 = &(A[i]),
              *a1 = &(A[ip]);

        for (register int h = mr; h; h--) {
          DType r   = *a0;
          *a0       = *a1;
          *a1       = r;

          a0 += lda;
          a1 += lda;
        }
      }

      if (inci > 0) KeepOn = (++i <= i2);
      else          KeepOn = (--i >= i2);

    } while (KeepOn);
  }
}


/*
 * GEneral Matrix Multiplication: based on dgemm.f from Netlib.
 *
 * This is an extremely inefficient algorithm. Recommend using ATLAS' version instead.
 *
 * Template parameters: LT -- long version of type T. Type T is the matrix dtype.
 *
 * This version throws no errors. Use gemm<DType> instead for error checking.
 */
template <typename DType>
inline void gemm_nothrow(const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
                 const DType* alpha, const DType* A, const int lda, const DType* B, const int ldb, const DType* beta, DType* C, const int ldc)
{

  typename LongDType<DType>::type temp;

  // Quick return if possible
  if (!M or !N or ((*alpha == 0 or !K) and *beta == 1)) return;

  // For alpha = 0
  if (*alpha == 0) {
    if (*beta == 0) {
      for (int j = 0; j < N; ++j)
        for (int i = 0; i < M; ++i) {
          C[i+j*ldc] = 0;
        }
    } else {
      for (int j = 0; j < N; ++j)
        for (int i = 0; i < M; ++i) {
          C[i+j*ldc] *= *beta;
        }
    }
    return;
  }

  // Start the operations
  if (TransB == CblasNoTrans) {
    if (TransA == CblasNoTrans) {
      // C = alpha*A*B+beta*C
      for (int j = 0; j < N; ++j) {
        if (*beta == 0) {
          for (int i = 0; i < M; ++i) {
            C[i+j*ldc] = 0;
          }
        } else if (*beta != 1) {
          for (int i = 0; i < M; ++i) {
            C[i+j*ldc] *= *beta;
          }
        }

        for (int l = 0; l < K; ++l) {
          if (B[l+j*ldb] != 0) {
            temp = *alpha * B[l+j*ldb];
            for (int i = 0; i < M; ++i) {
              C[i+j*ldc] += A[i+l*lda] * temp;
            }
          }
        }
      }

    } else {

      // C = alpha*A**DType*B + beta*C
      for (int j = 0; j < N; ++j) {
        for (int i = 0; i < M; ++i) {
          temp = 0;
          for (int l = 0; l < K; ++l) {
            temp += A[l+i*lda] * B[l+j*ldb];
          }

          if (*beta == 0) {
            C[i+j*ldc] = *alpha*temp;
          } else {
            C[i+j*ldc] = *alpha*temp + *beta*C[i+j*ldc];
          }
        }
      }

    }

  } else if (TransA == CblasNoTrans) {

    // C = alpha*A*B**T + beta*C
    for (int j = 0; j < N; ++j) {
      if (*beta == 0) {
        for (int i = 0; i < M; ++i) {
          C[i+j*ldc] = 0;
        }
      } else if (*beta != 1) {
        for (int i = 0; i < M; ++i) {
          C[i+j*ldc] *= *beta;
        }
      }

      for (int l = 0; l < K; ++l) {
        if (B[j+l*ldb] != 0) {
          temp = *alpha * B[j+l*ldb];
          for (int i = 0; i < M; ++i) {
            C[i+j*ldc] += A[i+l*lda] * temp;
          }
        }
      }

    }

  } else {

    // C = alpha*A**DType*B**T + beta*C
    for (int j = 0; j < N; ++j) {
      for (int i = 0; i < M; ++i) {
        temp = 0;
        for (int l = 0; l < K; ++l) {
          temp += A[l+i*lda] * B[j+l*ldb];
        }

        if (*beta == 0) {
          C[i+j*ldc] = *alpha*temp;
        } else {
          C[i+j*ldc] = *alpha*temp + *beta*C[i+j*ldc];
        }
      }
    }

  }

  return;
}



template <typename DType>
inline void gemm(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
                 const DType* alpha, const DType* A, const int lda, const DType* B, const int ldb, const DType* beta, DType* C, const int ldc)
{
  if (Order == CblasRowMajor) {
    if (TransA == CblasNoTrans) {
      if (lda < std::max(K,1)) {
        rb_raise(rb_eArgError, "lda must be >= MAX(K,1): lda=%d K=%d", lda, K);
      }
    } else {
      if (lda < std::max(M,1)) { // && TransA == CblasTrans
        rb_raise(rb_eArgError, "lda must be >= MAX(M,1): lda=%d M=%d", lda, M);
      }
    }

    if (TransB == CblasNoTrans) {
      if (ldb < std::max(N,1)) {
        rb_raise(rb_eArgError, "ldb must be >= MAX(N,1): ldb=%d N=%d", ldb, N);
      }
    } else {
      if (ldb < std::max(K,1)) {
        rb_raise(rb_eArgError, "ldb must be >= MAX(K,1): ldb=%d K=%d", ldb, K);
      }
    }

    if (ldc < std::max(N,1)) {
      rb_raise(rb_eArgError, "ldc must be >= MAX(N,1): ldc=%d N=%d", ldc, N);
    }
  } else { // CblasColMajor
    if (TransA == CblasNoTrans) {
      if (lda < std::max(M,1)) {
        rb_raise(rb_eArgError, "lda must be >= MAX(M,1): lda=%d M=%d", lda, M);
      }
    } else {
      if (lda < std::max(K,1)) { // && TransA == CblasTrans
        rb_raise(rb_eArgError, "lda must be >= MAX(K,1): lda=%d K=%d", lda, K);
      }
    }

    if (TransB == CblasNoTrans) {
      if (ldb < std::max(K,1)) {
        rb_raise(rb_eArgError, "ldb must be >= MAX(K,1): ldb=%d N=%d", ldb, K);
      }
    } else {
      if (ldb < std::max(N,1)) { // NOTE: This error message is actually wrong in the ATLAS source currently. Or are we wrong?
        rb_raise(rb_eArgError, "ldb must be >= MAX(N,1): ldb=%d N=%d", ldb, N);
      }
    }

    if (ldc < std::max(M,1)) {
      rb_raise(rb_eArgError, "ldc must be >= MAX(M,1): ldc=%d N=%d", ldc, M);
    }
  }

  /*
   * Call SYRK when that's what the user is actually asking for; just handle beta=0, because beta=X requires
   * we copy C and then subtract to preserve asymmetry.
   */

  if (A == B && M == N && TransA != TransB && lda == ldb && beta == 0) {
    rb_raise(rb_eNotImpError, "syrk and syreflect not implemented");
    /*syrk<DType>(CblasUpper, (Order == CblasColMajor) ? TransA : TransB, N, K, alpha, A, lda, beta, C, ldc);
    syreflect(CblasUpper, N, C, ldc);
    */
  }

  if (Order == CblasRowMajor)    gemm_nothrow<DType>(TransB, TransA, N, M, K, alpha, B, ldb, A, lda, beta, C, ldc);
  else                           gemm_nothrow<DType>(TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);

}


template <>
inline void gemm(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
          const float* alpha, const float* A, const int lda, const float* B, const int ldb, const float* beta, float* C, const int ldc) {
  cblas_sgemm(Order, TransA, TransB, M, N, K, *alpha, A, lda, B, ldb, *beta, C, ldc);
}

template <>
inline void gemm(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
          const double* alpha, const double* A, const int lda, const double* B, const int ldb, const double* beta, double* C, const int ldc) {
  cblas_dgemm(Order, TransA, TransB, M, N, K, *alpha, A, lda, B, ldb, *beta, C, ldc);
}

template <>
inline void gemm(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
          const Complex64* alpha, const Complex64* A, const int lda, const Complex64* B, const int ldb, const Complex64* beta, Complex64* C, const int ldc) {
  cblas_cgemm(Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
}

template <>
inline void gemm(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA, const enum CBLAS_TRANSPOSE TransB, const int M, const int N, const int K,
          const Complex128* alpha, const Complex128* A, const int lda, const Complex128* B, const int ldb, const Complex128* beta, Complex128* C, const int ldc) {
  cblas_zgemm(Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
}


/*
 * GEneral Matrix-Vector multiplication: based on dgemv.f from Netlib.
 *
 * This is an extremely inefficient algorithm. Recommend using ATLAS' version instead.
 *
 * Template parameters: LT -- long version of type T. Type T is the matrix dtype.
 */
template <typename DType>
inline bool gemv(const enum CBLAS_TRANSPOSE Trans, const int M, const int N, const DType* alpha, const DType* A, const int lda,
          const DType* X, const int incX, const DType* beta, DType* Y, const int incY) {
  int lenX, lenY, i, j;
  int kx, ky, iy, jx, jy, ix;

  typename LongDType<DType>::type temp;

  // Test the input parameters
  if (Trans < 111 || Trans > 113) {
    rb_raise(rb_eArgError, "GEMV: TransA must be CblasNoTrans, CblasTrans, or CblasConjTrans");
    return false;
  } else if (lda < std::max(1, N)) {
    fprintf(stderr, "GEMV: N = %d; got lda=%d", N, lda);
    rb_raise(rb_eArgError, "GEMV: Expected lda >= max(1, N)");
    return false;
  } else if (incX == 0) {
    rb_raise(rb_eArgError, "GEMV: Expected incX != 0\n");
    return false;
  } else if (incY == 0) {
    rb_raise(rb_eArgError, "GEMV: Expected incY != 0\n");
    return false;
  }

  // Quick return if possible
  if (!M or !N or (*alpha == 0 and *beta == 1)) return true;

  if (Trans == CblasNoTrans) {
    lenX = N;
    lenY = M;
  } else {
    lenX = M;
    lenY = N;
  }

  if (incX > 0) kx = 0;
  else          kx = (lenX - 1) * -incX;

  if (incY > 0) ky = 0;
  else          ky =  (lenY - 1) * -incY;

  // Start the operations. In this version, the elements of A are accessed sequentially with one pass through A.
  if (*beta != 1) {
    if (incY == 1) {
      if (*beta == 0) {
        for (i = 0; i < lenY; ++i) {
          Y[i] = 0;
        }
      } else {
        for (i = 0; i < lenY; ++i) {
          Y[i] *= *beta;
        }
      }
    } else {
      iy = ky;
      if (*beta == 0) {
        for (i = 0; i < lenY; ++i) {
          Y[iy] = 0;
          iy += incY;
        }
      } else {
        for (i = 0; i < lenY; ++i) {
          Y[iy] *= *beta;
          iy += incY;
        }
      }
    }
  }

  if (*alpha == 0) return false;

  if (Trans == CblasNoTrans) {

    // Form  y := alpha*A*x + y.
    jx = kx;
    if (incY == 1) {
      for (j = 0; j < N; ++j) {
        if (X[jx] != 0) {
          temp = *alpha * X[jx];
          for (i = 0; i < M; ++i) {
            Y[i] += A[j+i*lda] * temp;
          }
        }
        jx += incX;
      }
    } else {
      for (j = 0; j < N; ++j) {
        if (X[jx] != 0) {
          temp = *alpha * X[jx];
          iy = ky;
          for (i = 0; i < M; ++i) {
            Y[iy] += A[j+i*lda] * temp;
            iy += incY;
          }
        }
        jx += incX;
      }
    }

  } else { // TODO: Check that indices are correct! They're switched for C.

    // Form  y := alpha*A**DType*x + y.
    jy = ky;

    if (incX == 1) {
      for (j = 0; j < N; ++j) {
        temp = 0;
        for (i = 0; i < M; ++i) {
          temp += A[j+i*lda]*X[j];
        }
        Y[jy] += *alpha * temp;
        jy += incY;
      }
    } else {
      for (j = 0; j < N; ++j) {
        temp = 0;
        ix = kx;
        for (i = 0; i < M; ++i) {
          temp += A[j+i*lda] * X[ix];
          ix += incX;
        }

        Y[jy] += *alpha * temp;
        jy += incY;
      }
    }
  }

  return true;
}  // end of GEMV

template <>
inline bool gemv(const enum CBLAS_TRANSPOSE Trans, const int M, const int N, const float* alpha, const float* A, const int lda,
          const float* X, const int incX, const float* beta, float* Y, const int incY) {
  cblas_sgemv(CblasRowMajor, Trans, M, N, *alpha, A, lda, X, incX, *beta, Y, incY);
  return true;
}

template <>
inline bool gemv(const enum CBLAS_TRANSPOSE Trans, const int M, const int N, const double* alpha, const double* A, const int lda,
          const double* X, const int incX, const double* beta, double* Y, const int incY) {
  cblas_dgemv(CblasRowMajor, Trans, M, N, *alpha, A, lda, X, incX, *beta, Y, incY);
  return true;
}

template <>
inline bool gemv(const enum CBLAS_TRANSPOSE Trans, const int M, const int N, const Complex64* alpha, const Complex64* A, const int lda,
          const Complex64* X, const int incX, const Complex64* beta, Complex64* Y, const int incY) {
  cblas_cgemv(CblasRowMajor, Trans, M, N, alpha, A, lda, X, incX, beta, Y, incY);
  return true;
}

template <>
inline bool gemv(const enum CBLAS_TRANSPOSE Trans, const int M, const int N, const Complex128* alpha, const Complex128* A, const int lda,
          const Complex128* X, const int incX, const Complex128* beta, Complex128* Y, const int incY) {
  cblas_zgemv(CblasRowMajor, Trans, M, N, alpha, A, lda, X, incX, beta, Y, incY);
  return true;
}


// Yale: numeric matrix multiply c=a*b
template <typename DType, typename IType>
inline void numbmm(const unsigned int n, const unsigned int m, const IType* ia, const IType* ja, const DType* a, const bool diaga,
            const IType* ib, const IType* jb, const DType* b, const bool diagb, IType* ic, IType* jc, DType* c, const bool diagc) {
  IType next[m];
  DType sums[m];

  DType v;

  IType head, length, temp, ndnz = 0;
  IType jj_start, jj_end, kk_start, kk_end;
  IType i, j, k, kk, jj;
  IType minmn = std::min(m,n);

  for (i = 0; i < m; ++i) { // initialize scratch arrays
    next[i] = std::numeric_limits<IType>::max();
    sums[i] = 0;
  }

  for (i = 0; i < n; ++i) { // walk down the rows
    head = std::numeric_limits<IType>::max()-1; // head gets assigned as whichever column of B's row j we last visited
    length = 0;

    jj_start = ia[i];
    jj_end   = ia[i+1];

    for (jj = jj_start; jj <= jj_end; ++jj) { // walk through entries in each row

      if (jj == jj_end) { // if we're in the last entry for this row:
        if (!diaga || i >= minmn) continue;
        j   = i;      // if it's a new Yale matrix, and last entry, get the diagonal position (j) and entry (ajj)
        v   = a[i];
      } else {
        j   = ja[jj]; // if it's not the last entry for this row, get the column (j) and entry (ajj)
        v   = a[jj];
      }

      kk_start = ib[j];   // Find the first entry of row j of matrix B
      kk_end   = ib[j+1];
      for (kk = kk_start; kk <= kk_end; ++kk) {

        if (kk == kk_end) { // Get the column id for that entry
          if (!diagb || j >= minmn) continue;
          k  = j;
          sums[k] += v*b[k];
        } else {
          k  = jb[kk];
          sums[k] += v*b[kk];
        }

        if (next[k] == std::numeric_limits<IType>::max()) {
          next[k] = head;
          head    = k;
          ++length;
        }
      }
    }

    for (jj = 0; jj < length; ++jj) {
      if (sums[head] != 0) {
        if (diagc && head == i) {
          c[head] = sums[head];
        } else {
          jc[n+1+ndnz] = head;
          c[n+1+ndnz]  = sums[head];
          ++ndnz;
        }
      }

      temp = head;
      head = next[head];

      next[temp] = std::numeric_limits<IType>::max();
      sums[temp] = 0;
    }

    ic[i+1] = n+1+ndnz;
  }
} /* numbmm_ */



// Yale: Symbolic matrix multiply c=a*b
template <typename IType>
inline void symbmm(const unsigned int n, const unsigned int m, const IType* ia, const IType* ja, const bool diaga,
            const IType* ib, const IType* jb, const bool diagb, IType* ic, const bool diagc) {
  IType mask[m];
  IType j, k, ndnz = n; /* Local variables */


  for (j = 0; j < m; ++j)
    mask[j] = std::numeric_limits<IType>::max();

  if (diagc)  ic[0] = n+1;
  else        ic[0] = 0;

  IType minmn = std::min(m,n);

  for (IType i = 0; i < n; ++i) { // MAIN LOOP: through rows

    for (IType jj = ia[i]; jj <= ia[i+1]; ++jj) { // merge row lists, walking through columns in each row

      // j <- column index given by JA[jj], or handle diagonal.
      if (jj == ia[i+1]) { // Don't really do it the last time -- just handle diagonals in a new yale matrix.
        if (!diaga || i >= minmn) continue;
        j = i;
      } else j = ja[jj];

      for (IType kk = ib[j]; kk <= ib[j+1]; ++kk) { // Now walk through columns of row J in matrix B.
        if (kk == ib[j+1]) {
          if (!diagb || j >= minmn) continue;
          k = j;
        } else k = jb[kk];

        if (mask[k] != i) {
          mask[k] = i;
          ++ndnz;
        }
      }
    }

    if (diagc && !mask[i]) --ndnz;

    ic[i+1] = ndnz;
  }
} /* symbmm_ */


//TODO: More efficient sorting algorithm than selection sort would be nice, probably.
// Remember, we're dealing with unique keys, which simplifies things.
// Doesn't have to be in-place, since we probably just multiplied and that wasn't in-place.
template <typename DType, typename IType>
inline void smmp_sort_columns(const size_t n, const IType* ia, IType* ja, DType* a) {
  IType jj, min, min_jj;
  DType temp_val;

  for (size_t i = 0; i < n; ++i) {
    // No need to sort if there are 0 or 1 entries in the row
    if (ia[i+1] - ia[i] < 2) continue;

    for (IType jj_start = ia[i]; jj_start < ia[i+1]; ++jj_start) {

      // If the previous min is just current-1, this key/value pair is already in sorted order.
      // This follows from the unique condition on our column keys.
      if (jj_start > ia[i] && min+1 == ja[jj_start]) {
        min    = ja[jj_start];
        continue;
      }

      // find the minimum key (column index) between jj_start and ia[i+1]
      min    = ja[jj_start];
      min_jj = jj_start;
      for (jj = jj_start+1; jj < ia[i+1]; ++jj) {
        if (ja[jj] < min) {
          min_jj = jj;
          min    = ja[jj];
        }
      }

      // if min is already first, skip this iteration
      if (min_jj == jj_start) continue;

      for (jj = jj_start; jj < ia[i+1]; ++jj) {
        // swap minimum key/value pair with key/value pair in the first position.
        if (min_jj != jj) {
          // min already = ja[min_jj], so use this as temp_key
          temp_val = a[min_jj];

          ja[min_jj] = ja[jj];
          a[min_jj] = a[jj];

          ja[jj] = min;
          a[jj] = temp_val;
        }
      }
    }
  }
}


/*
 * Transposes a generic Yale matrix (old or new). Specify new by setting diaga = true.
 *
 * Based on transp from SMMP (same as symbmm and numbmm).
 *
 * This is not named in the same way as most yale_storage functions because it does not act on a YALE_STORAGE
 * object.
 */
template <typename DType, typename IType>
void transpose_yale(const size_t n, const size_t m, const void* ia_, const void* ja_, const void* a_,
                             const bool diaga, void* ib_, void* jb_, void* b_, const bool move)
{
  const IType *ia = reinterpret_cast<const IType*>(ia_),
              *ja = reinterpret_cast<const IType*>(ja_);
  const DType *a  = reinterpret_cast<const DType*>(a_);

  IType *ib = reinterpret_cast<IType*>(ib_),
        *jb = reinterpret_cast<IType*>(jb_);
  DType *b  = reinterpret_cast<DType*>(b_);



  size_t index;

  // Clear B
  for (size_t i = 0; i < m+1; ++i) ib[i] = 0;

  if (move)
    for (size_t i = 0; i < m+1; ++i) b[i] = 0;

  if (diaga) ib[0] = m + 1;
  else       ib[0] = 0;

  /* count indices for each column */

  for (size_t i = 0; i < n; ++i) {
    for (size_t j = ia[i]; j < ia[i+1]; ++j) {
      ++(ib[ja[j]+1]);
    }
  }

  for (size_t i = 0; i < m; ++i) {
    ib[i+1] = ib[i] + ib[i+1];
  }

  /* now make jb */

  for (size_t i = 0; i < n; ++i) {

    for (size_t j = ia[i]; j < ia[i+1]; ++j) {
      index = ja[j];
      jb[ib[index]] = i;

      if (move)
        b[ib[index]] = a[j];

      ++(ib[index]);
    }
  }

  /* now fixup ib */

  for (size_t i = m; i >= 1; --i) {
    ib[i] = ib[i-1];
  }


  if (diaga) {
    if (move) {
      size_t j = std::min(n,m);

      for (size_t i = 0; i < j; ++i) {
        b[i] = a[i];
      }
    }
    ib[0] = m + 1;

  } else {
    ib[0] = 0;
  }
}


/*
 * Templated version of row-order and column-order getrf, derived from ATL_getrfR.c (from ATLAS 3.8.0).
 *
 * 1. Row-major factorization of form
 *   A = L * U * P
 * where P is a column-permutation matrix, L is lower triangular (lower
 * trapazoidal if M > N), and U is upper triangular with unit diagonals (upper
 * trapazoidal if M < N).  This is the recursive Level 3 BLAS version.
 *
 * 2. Column-major factorization of form
 *   A = P * L * U
 * where P is a row-permutation matrix, L is lower triangular with unit diagonal
 * elements (lower trapazoidal if M > N), and U is upper triangular (upper
 * trapazoidal if M < N).  This is the recursive Level 3 BLAS version.
 *
 * Template argument determines whether 1 or 2 is utilized.
 */
template <bool RowMajor, typename DType>
inline int getrf_nothrow(const int M, const int N, DType* A, const int lda, int* ipiv) {
  const int MN = std::min(M, N);
  int ierr = 0;

  // Symbols used by ATLAS:
  // Row   Col      Us
  // Nup   Nleft    N_ul
  // Ndown Nright   N_dr
  // We're going to use N_ul, N_dr

  DType neg_one = -1, one = 1;

  if (MN > 1) {
    int N_ul = MN >> 1;

    // FIXME: Figure out how ATLAS #defines NB
#ifdef NB
    if (N_ul > NB) N_ul = ATL_MulByNB(ATL_DivByNB(N_ul));
#endif

    int N_dr = M - N_ul;

    int i = RowMajor ? getrf_nothrow<true,DType>(N_ul, N, A, lda, ipiv) : getrf_nothrow<false,DType>(M, N_ul, A, lda, ipiv);

    if (i) if (!ierr) ierr = i;

    DType *Ar, *Ac, *An;
    if (RowMajor) {
      Ar = &(A[N_ul * lda]),
      Ac = &(A[N_ul]);
      An = &(Ar[N_ul]);

      nm::math::laswp<DType>(N_dr, Ar, lda, 0, N_ul, ipiv, 1);

      nm::math::trsm<DType>(CblasRowMajor, CblasRight, CblasUpper, CblasNoTrans, CblasUnit, N_dr, N_ul, one, A, lda, Ar, lda);
      nm::math::gemm<DType>(CblasRowMajor, CblasNoTrans, CblasNoTrans, N_dr, N-N_ul, N_ul, &neg_one, Ar, lda, Ac, lda, &one, An, lda);

      i = getrf_nothrow<true,DType>(N_dr, N-N_ul, An, lda, ipiv+N_ul);
    } else {
      Ar = NULL;
      Ac = &(A[N_ul * lda]);
      An = &(Ac[N_ul]);

      nm::math::laswp<DType>(N_dr, Ac, lda, 0, N_ul, ipiv, 1);

      nm::math::trsm<DType>(CblasColMajor, CblasLeft, CblasLower, CblasNoTrans, CblasUnit, N_ul, N_dr, one, A, lda, Ac, lda);
      nm::math::gemm<DType>(CblasColMajor, CblasNoTrans, CblasNoTrans, M-N_ul, N_dr, N_ul, &neg_one, An, lda, Ac, lda, &one, An, lda);

      i = getrf_nothrow<false,DType>(M-N_ul, N_dr, An, lda, ipiv+N_ul);
    }

    if (i) if (!ierr) ierr = N_ul + i;

    for (i = N_ul; i != MN; i++) {
      ipiv[i] += N_ul;
    }

    nm::math::laswp<DType>(N_ul, A, lda, N_ul, MN, ipiv, 1);  /* apply pivots */

  } else if (MN == 1) { // there's another case for the colmajor version, but i don't know that it's that critical. Calls ATLAS LU2, who knows what that does.

    int i = *ipiv = nm::math::lapack::idamax<DType>(N, A, 1); // cblas_iamax(N, A, 1);

    DType tmp = A[i];
    if (tmp != 0) {

      nm::math::lapack::scal<DType>((RowMajor ? N : M), nm::math::numeric_inverse(tmp), A, 1);
      A[i] = *A;
      *A   = tmp;

    } else ierr = 1;

  }
  return(ierr);
}


/*
 * From ATLAS 3.8.0:
 *
 * Computes one of two LU factorizations based on the setting of the Order
 * parameter, as follows:
 * ----------------------------------------------------------------------------
 *                       Order == CblasColMajor
 * Column-major factorization of form
 *   A = P * L * U
 * where P is a row-permutation matrix, L is lower triangular with unit
 * diagonal elements (lower trapazoidal if M > N), and U is upper triangular
 * (upper trapazoidal if M < N).
 *
 * ----------------------------------------------------------------------------
 *                       Order == CblasRowMajor
 * Row-major factorization of form
 *   A = P * L * U
 * where P is a column-permutation matrix, L is lower triangular (lower
 * trapazoidal if M > N), and U is upper triangular with unit diagonals (upper
 * trapazoidal if M < N).
 *
 * ============================================================================
 * Let IERR be the return value of the function:
 *    If IERR == 0, successful exit.
 *    If (IERR < 0) the -IERR argument had an illegal value
 *    If (IERR > 0 && Order == CblasColMajor)
 *       U(i-1,i-1) is exactly zero.  The factorization has been completed,
 *       but the factor U is exactly singular, and division by zero will
 *       occur if it is used to solve a system of equations.
 *    If (IERR > 0 && Order == CblasRowMajor)
 *       L(i-1,i-1) is exactly zero.  The factorization has been completed,
 *       but the factor L is exactly singular, and division by zero will
 *       occur if it is used to solve a system of equations.
 */
template <typename DType>
inline int getrf(const enum CBLAS_ORDER Order, const int M, const int N, DType* A, int lda, int* ipiv) {
  if (Order == CblasRowMajor) {
    if (lda < std::max(1,N)) {
      rb_raise(rb_eArgError, "GETRF: lda must be >= MAX(N,1): lda=%d N=%d", lda, N);
      return -6;
    }

    return getrf_nothrow<true,DType>(M, N, A, lda, ipiv);
  } else {
    if (lda < std::max(1,M)) {
      rb_raise(rb_eArgError, "GETRF: lda must be >= MAX(M,1): lda=%d M=%d", lda, M);
      return -6;
    }

    return getrf_nothrow<false,DType>(M, N, A, lda, ipiv);
    //rb_raise(rb_eNotImpError, "column major getrf not implemented");
  }
}




/*
 * Macro for declaring LAPACK specializations of the getrf function.
 *
 * type is the DType; call is the specific function to call; cast_as is what the DType* should be
 * cast to in order to pass it to LAPACK.
 */
#define LAPACK_GETRF(type, call, cast_as)                                     \
template <>                                                                   \
inline int getrf(const enum CBLAS_ORDER Order, const int M, const int N, type * A, const int lda, int* ipiv) { \
  int info = call(Order, M, N, reinterpret_cast<cast_as *>(A), lda, ipiv);    \
  if (!info) return info;                                                     \
  else {                                                                      \
    rb_raise(rb_eArgError, "getrf: problem with argument %d\n", info);        \
    return info;                                                              \
  }                                                                           \
}

/* Specialize for ATLAS types */
/*LAPACK_GETRF(float,      clapack_sgetrf, float)
LAPACK_GETRF(double,     clapack_dgetrf, double)
LAPACK_GETRF(Complex64,  clapack_cgetrf, void)
LAPACK_GETRF(Complex128, clapack_zgetrf, void)
*/


/*
* Function signature conversion for calling LAPACK's getrf functions as directly as possible.
*
* For documentation: http://www.netlib.org/lapack/double/dgetrf.f
*
* This function should normally go in math.cpp, but we need it to be available to nmatrix.cpp.
*/
template <typename DType>
inline int clapack_getrf(const enum CBLAS_ORDER order, const int m, const int n, void* a, const int lda, int* ipiv) {
  return getrf<DType>(order, m, n, reinterpret_cast<DType*>(a), lda, ipiv);
}


}} // end namespace nm::math


#endif // MATH_H
