/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class GaussianDerivative1D -------------------------------------------------

GaussianDerivative1D::GaussianDerivative1D (double sigma, const BorderMode mode, const PixelFormat & format, const Direction direction)
: ConvolutionDiscrete1D (mode, GrayDouble, direction)
{
  double sigma2 = sigma * sigma;
  double C = 1.0 / (sqrt (TWOPI) * sigma);

  int h = (int) roundp (Gaussian2D::cutoff * sigma);
  resize (2 * h + 1, 1);

  double * kernel = (double *) ((PixelBufferPacked *) buffer)->base ();
  kernel[h] = 0;
  for (int i = 1; i <= h; i++)
  {
	double x = i;
	double value = C * exp (- x * x / (2 * sigma2)) * (- x / sigma2);
	kernel[h + i] = value;
	kernel[h - i] = -value;
  }

  *this *= format;
  normalFloats ();
}
