/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix2x2_tcc
#define fl_matrix2x2_tcc


#include "fl/MatrixFixed.tcc"


namespace fl
{
  template <class T>
  MatrixFixed<T,2,2>
  operator ! (const MatrixFixed<T,2,2> & A)
  {
	MatrixFixed<T,2,2> result;
	T q = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];
	if (q == 0)
	{
	  throw "invert: Matrix is singular!";
	}
	result.data[0][0] = A.data[1][1] / q;
	result.data[0][1] = A.data[0][1] / -q;
	result.data[1][0] = A.data[1][0] / -q;
	result.data[1][1] = A.data[0][0] / q;
	return result;
  }

  template<class T>
  void
  geev (const MatrixFixed<T,2,2> & A, Matrix<T> & eigenvalues)
  {
	// a = 1  :)
	T b = A.data[0][0] + A.data[1][1];  // trace
	T c = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];  // determinant
	T b4c = b * b - 4 * c;
	if (b4c < 0)
	{
	  throw "eigen: no real eigenvalues!";
	}
	if (b4c > 0)
	{
	  b4c = sqrt (b4c);
	}
	eigenvalues.resize (2, 1);
	eigenvalues (0, 0) = (b - b4c) / 2.0;
	eigenvalues (1, 0) = (b + b4c) / 2.0;
  }

  template<class T>
  void
  geev (const MatrixFixed<T,2,2> & A, Matrix<std::complex<T> > & eigenvalues)
  {
	eigenvalues.resize (2, 1);

	// a = 1  :)
	T b = -(A.data[0][0] + A.data[1][1]);  // trace
	T c = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];  // determinant
	T b4c = b * b - 4 * c;
	bool imaginary = b4c < 0;
	if (b4c != 0)
	{
	  b4c = sqrt (fabs (b4c));
	}
	if (imaginary)
	{
	  b /= -2.0;
	  b4c /= 2.0;
	  eigenvalues(0,0) = std::complex<T> (b, b4c);
	  eigenvalues(1,0) = std::complex<T> (b, -b4c);
	}
	else
	{
	  eigenvalues(0,0) = (-b - b4c) / T (2);
	  eigenvalues(1,0) = (-b + b4c) / T (2);
	}
  }
}


#endif
