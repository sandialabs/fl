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


#include "fl/interest.h"

#include <math.h>

// Include for debugging
//#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHarris -------------------------------------------------------

InterestHarris::InterestHarris (int neighborhood, int maxPoints, float thresholdFactor)
: nms (neighborhood),
  filter (1.0, 1.4, GrayFloat)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;
}

void
InterestHarris::run (ImageCache & cache, PointSet & result)
{
  int offset = filter.offset;

  Image image = cache.get (new EntryPyramid (GrayFloat))->image;
  ImageOf<float> i = image * filter;
  i *= nms;
  float threshold = nms.average * thresholdFactor;

  multiset<PointInterest> sorted;

  for (int y = 0; y < i.height; y++)
  {
	for (int x = 0; x < i.width; x++)
	{
	  float pixel = i(x,y);
	  if (pixel > threshold)
	  {
		PointInterest p;
		p.x = x + offset;
		p.y = y + offset;
		p.weight = pixel;
		p.detector = PointInterest::Corner;
		sorted.insert (p);
		if (sorted.size () > maxPoints)
		{
		  sorted.erase (sorted.begin ());
		}
	  }
	}
  }

  result.add (sorted);
}

void
InterestHarris::serialize (Archive & archive, uint32_t version)
{
  archive & *((InterestOperator *) this);
  archive & maxPoints;
  archive & thresholdFactor;
  archive & nms.half;
}
