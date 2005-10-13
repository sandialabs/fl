/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class GaussianDerivative1D -------------------------------------------------

GaussianDerivative1D::GaussianDerivative1D (double sigma, const BorderMode mode, const PixelFormat & format, const Direction direction)
: ConvolutionDiscrete1D (mode, GrayDouble, direction)
{
  double sigma2 = sigma * sigma;
  double C = 1.0 / (sqrt (2.0 * PI) * sigma);

  int h = (int) rint (Gaussian2D::cutoff * sigma);
  width = 2 * h + 1;
  height = 1;
  buffer.grow (width * GrayDouble.depth);

  ((double *) buffer)[h] = 0;
  for (int i = 1; i <= h; i++)
  {
	double x = i;
	double value = C * exp (- x * x / (2 * sigma2)) * (- x / sigma2);
	((double *) buffer)[h + i] = value;
	((double *) buffer)[h - i] = -value;
  }

  *this *= format;
  normalFloats ();
}
