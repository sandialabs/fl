/*
Author: Fred Rothganger
Created 01/30/2006 to provide a general interface for measuring distances
in R^n, and to help separate numeric and image libraries.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_metric_h
#define fl_metric_h


#include "fl/matrix.h"

#include <iostream>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  /**
	 Reifies a distance function d(a,b).  This function must satisfy the three
	 axioms of a metric:
	 <ol>
	 <li>Positivity: d(a,b) = 0 if a == b, otherwise d(a,b) > 0
	 <li>Symmetry: d(a,b) == d(b,a)
	 <li>Triangle inequality: d(a,c) <= d(a,b) + d(b,c)
	 </ol>
   **/
  class SHARED Metric
  {
  public:
	virtual ~Metric ();  ///< Establishes that destructor is virtual, but doesn't do anything else.

	virtual float value (const Vector<float> & value1, const Vector<float> & value2) const = 0;

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;
  };
}


#endif
