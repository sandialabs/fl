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


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorSpin -------------------------------------------------------

DescriptorSpin::DescriptorSpin (int binsRadial, int binsIntensity, float supportRadial, float supportIntensity)
{
  this->binsRadial       = binsRadial;
  this->binsIntensity    = binsIntensity;
  this->supportRadial    = supportRadial;
  this->supportIntensity = supportIntensity;
}

Vector<float>
DescriptorSpin::value (ImageCache & cache, const PointAffine & point)
{
  ImageOf<float> image = cache.get (new EntryPyramid (GrayFloat))->image;

  // Determine square region in source image to scan

  Matrix<double> R = point.rectification ();
  Matrix<double> S = !R;

  Vector<double> tl (3);
  tl[0] = -supportRadial;
  tl[1] = supportRadial;
  tl[2] = 1;
  Vector<double> tr (3);
  tr[0] = supportRadial;
  tr[1] = supportRadial;
  tr[2] = 1;
  Vector<double> bl (3);
  bl[0] = -supportRadial;
  bl[1] = -supportRadial;
  bl[2] = 1;
  Vector<double> br (3);
  br[0] = supportRadial;
  br[1] = -supportRadial;
  br[2] = 1;

  Point ptl = S * tl;
  Point ptr = S * tr;
  Point pbl = S * bl;
  Point pbr = S * br;

  int sourceL = (int) roundp (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0));
  int sourceR = (int) roundp (min (max (ptl.x, ptr.x, pbl.x, pbr.x), image.width - 1.0));
  int sourceT = (int) roundp (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0));
  int sourceB = (int) roundp (min (max (ptl.y, ptr.y, pbl.y, pbr.y), image.height - 1.0));

  R.region (0, 0, 1, 2) *= binsRadial / supportRadial;  // Now R maps directly to radial bin values

  // Determine mapping between pixel values and intensity bins
  float average = 0;
  float count = 0;
  Point q;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).norm (2);
	  if (radius < binsRadial)
	  {
		float weight = 1.0f - radius / binsRadial;
		average += image(x,y) * weight;
		count += weight;
	  }
	}
  }
  average /= count;
  float deviation = 0;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).norm (2);
	  if (radius < binsRadial)
	  {
		float d = image(x,y) - average;
		float weight = 1.0f - radius / binsRadial;
		deviation += d * d * weight;
	  }
	}
  }
  deviation = sqrt (deviation / count);
  float range = 2.0f * supportIntensity * deviation;
  if (range == 0.0f)  // In case the image is completely flat
  {
	range = 1.0f;
  }
  float quantum = range / binsIntensity;
  float minIntensity = average - range / 2 + 0.5f * quantum;

  // Bin up all the pixels
  Matrix<float> result (binsIntensity, binsRadial);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float rf = (R * q).norm (2) - 0.5f;
	  if (rf < binsRadial)
	  {
		int rl = (int) floorf (rf);
		int rh = rl + 1;
		rf -= rl;
		float rf1 = 1.0f - rf;
		if (rh > binsRadial - 1)
		{
		  rh = binsRadial - 1;
		  rf = 0.0f;
		}
		rl = max (rl, 0);

		float df = (image(x,y) - minIntensity) / quantum;
		int dl = (int) floorf (df);
		int dh = dl + 1;
		df -= dl;
		float df1 = 1.0f - df;
		if (dl < 0)
		{
		  dl = 0;
		  dh = 0;
		}
		if (dh > binsIntensity - 1)
		{
		  dl = binsIntensity - 1;
		  dh = binsIntensity - 1;
		}

		result(dl,rl) += df1 * rf1;
		result(dl,rh) += df1 * rf;
		result(dh,rl) += df  * rf1;
		result(dh,rh) += df  * rf;
	  }
	}
  }

  // Convert to probabilities
  for (int r = 0; r < binsRadial; r++)
  {
	float sum = result.column (r).norm (1);
	result.column (r) /= sum;
  }

  return result;
}

Image
DescriptorSpin::patch (const Vector<float> & value)
{
  ImageOf<float> result (binsRadial, binsIntensity, GrayFloat);

  for (int r = 0; r < binsRadial; r++)
  {
	for (int d = 0; d < binsIntensity; d++)
	{
	  result (r, d) = 1.0 - value[(r * binsIntensity) + (binsIntensity - d - 1)];
	}
  }

  return result;
}

Comparison *
DescriptorSpin::comparison ()
{
  //return new HistogramIntersection;
  return new ChiSquared;
}

int
DescriptorSpin::dimension ()
{
  return binsRadial * binsIntensity;
}

void
DescriptorSpin::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & binsRadial;
  archive & binsIntensity;
  archive & supportRadial;
  archive & supportIntensity;
}
