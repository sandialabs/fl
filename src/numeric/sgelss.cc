/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"

#include <assert.h>


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & B, float * residual, bool destroyA, bool destroyB)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = B.columns ();
	int ldx = std::max (m, n);
	assert (B.rows () == m);

	Matrix<float> tempA;
	const Matrix<float> * p;
	if (destroyA  &&  (p = dynamic_cast<const Matrix<float> *> (&A)))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	p = dynamic_cast<const Matrix<float> *> (&B);
	if (destroyB  &&  ldx == m  &&  p)
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  float * xp = & x(0,0);
	  int step = ldx - m;
	  if (p)
	  {
		float * bp = & B(0,0);
		float * end = bp + m * nrhs;
		while (bp < end)
		{
		  float * rowEnd = bp + m;
		  while (bp < rowEnd)
		  {
			*xp++ = *bp++;
		  }
		  xp += step;
		}
	  }
	  else
	  {
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = 0; r < m; r++)
		  {
			*xp++ = B(r,c);
		  }
		  xp += step;
		}
	  }
	}

	Matrix<float> s (std::min (m, n), 1);

	int rank;
	int ldwork = 5 * std::max (ldx, nrhs);
	float * work = (float *) malloc (ldwork * sizeof (float));
	int info = 0;

	sgelss_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldx,
			 & s[0],
			 -1.0f,  // use machine precision
			 rank,
			 work,
			 ldwork,
			 info);

	free (work);

	if (info)
	{
	  throw info;
	}

	if (ldx > n)
	{
	  Matrix<float> tempX (n, nrhs);
	  float * xp = & x(0,0);
	  float * tp = & tempX(0,0);
	  float * end = tp + n * nrhs;
	  int step = ldx - n;
	  while (tp < end)
	  {
		float * rowEnd = tp + n;
		while (tp < rowEnd)
		{
		  *tp++ = *xp++;
		}
		xp += step;
	  }

	  if (residual)
	  {
		float total = 0;
		float * xp = & x(0,0);
		float * end = xp + ldx * nrhs;
		while (xp < end)
		{
		  float * rowEnd = xp + ldx;
		  xp += n;
		  while (xp < rowEnd)
		  {
			total += *xp * *xp++;
		  }
		}
		*residual = total;
	  }

	  x = tempX;
	}
	else
	{
	  if (residual)
	  {
		*residual = 0;
	  }
	}
  }
}
