/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Normalize ------------------------------------------------------------

Normalize::Normalize (double length)
{
  this->length = length;
}

Image
Normalize::filter (const Image & image)
{
  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;
	ImageOf<float> that (image);
	float sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (image.width, image.height, GrayDouble);
	result.timestamp = image.timestamp;
	ImageOf<double> that (image);
	double sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else
  {
	throw "Normalize::filter: unimplemented format";
  }
}
