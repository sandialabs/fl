/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class GaussianDerivativeFirst ----------------------------------------------

GaussianDerivativeFirst::GaussianDerivativeFirst (int xy, double sigmaX, double sigmaY, double angle, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  if (sigmaY < 0)
  {
	sigmaY = sigmaX;
  }

  const double C = 1.0 / (TWOPI * sigmaX * sigmaY);
  int half = (int) roundp (Gaussian2D::cutoff * max (sigmaX, sigmaY));
  int size = 2 * half + 1;

  ImageOf<double> temp (size, size, GrayDouble);

  double s = sin (- angle);
  double c = cos (- angle);

  double sigmaX2 = sigmaX * sigmaX;
  double sigmaY2 = sigmaY * sigmaY;

  for (int row = 0; row < size; row++)
  {
	for (int column = 0; column < size; column++)
	{
	  double u = column - half;
	  double v = row - half;
	  double x = u * c - v * s;
	  double y = u * s + v * c;

	  double value = C * exp (-0.5 * (x * x / sigmaX2 + y * y / sigmaY2));
	  if (xy)  // Gy
	  {
		value *= - y / sigmaY2;
	  }
	  else  // Gx
	  {
		value *= - x / sigmaX2;
	  }
	  temp (column, row) = value;
	}
  }

  *((Image *) this) = temp * format;
  normalFloats ();
}
