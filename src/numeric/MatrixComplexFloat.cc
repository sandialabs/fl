/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include <complex>
/// A small cheat to make the basic norm() template work.
namespace std
{
  inline complex<float>
  max (const float & a, const complex<float> & b)
  {
	return complex<float> (max (a, b.real ()));
  }
}

#define flNumeric_MS_EVIL
#include "fl/Matrix.tcc"


using namespace fl;
using namespace std;


template class MatrixAbstract<complex<float> >;
template class MatrixStrided<complex<float> >;
template class Matrix<complex<float> >;
template class MatrixTranspose<complex<float> >;
template class MatrixRegion<complex<float> >;

namespace fl
{
  template SHARED std::ostream & operator << (std::ostream & stream, const MatrixAbstract<complex<float> > & A);
  template SHARED std::istream & operator >> (std::istream & stream, MatrixAbstract<complex<float> > & A);
  template SHARED MatrixAbstract<complex<float> > & operator << (MatrixAbstract<complex<float> > & A, const std::string & source);
}
