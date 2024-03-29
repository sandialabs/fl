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


#ifndef fl_reduce_h
#define fl_reduce_h


#include <fl/matrix.h>

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
	 Dimensionality reduction methods.
  **/
  class SHARED DimensionalityReduction
  {
  public:
	// A derived class must override at least one analyze() method.  Otherwise,
	// the default implementations will create an infinite loop.
	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual void analyze (const std::vector< Vector<float> > & data, const std::vector<int> & classAssignments);
	virtual Vector<float> reduce (const Vector<float> & datum) = 0;

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;
  };

  class SHARED PCA : public DimensionalityReduction
  {
  public:
	PCA (int targetDimension = 1);

	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual Vector<float> reduce (const Vector<float> & datum);

	void serialize (Archive & archive, uint32_t version);

	int targetDimension;
	Matrix<float> W;  ///< Basis matrix for reduced space.
  };

  class SHARED MDA : public DimensionalityReduction
  {
  public:
	MDA () {}

	virtual void analyze (const std::vector< Vector<float> > & data, const std::vector<int> & classAssignments);
	virtual Vector<float> reduce (const Vector<float> & datum);

	void serialize (Archive & archive, uint32_t version);

	Matrix<float> W;  ///< Basis matrix for reduced space.
  };
}


#endif
