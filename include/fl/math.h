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


#ifndef fl_math_h
#define fl_math_h


#ifdef _MSC_VER
#  define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _MSC_VER
#  include <float.h>
#  undef min
#  undef max
#endif

#ifndef M_PI
#  define M_PI     3.1415926535897932384626433832795029
#  define M_PIf    3.1415926535897932384626433832795029f
#endif

#define TWOPI  6.283185307179586476925286766559
#define TWOPIf 6.283185307179586476925286766559f


namespace std
{
  using ::abs;

  inline int
  sqrt (int a)
  {
	return (int) floorf (sqrtf ((float) a));
  }

  inline float
  pow (int a, float b)
  {
	return powf ((float) a, b);
  }

#if ! defined (_MSC_VER)  ||  _MSC_VER > 1310
  inline int
  pow (int a, int b)
  {
	return (int) floor (pow ((double) a, b));
  }
#endif

#ifdef _MSC_VER

  inline int
  isnan (double a)
  {
	return _isnan (a);
  }

  inline int
  isinf (double a)
  {
	return ! _finite (a);
  }

  inline double
  log (int a)
  {
	return log ((double) a);
  }

#endif

  /**
	 Four-way max.  Used mainly for finding limits of a set of four points in the plane.
   **/
  template<class T>
  inline const T &
  max (const T & a, const T & b, const T & c, const T & d)
  {
	return max (max (a, b), max (c, d));
  }

  /**
	 Four-way min.  Used mainly for finding limits of a set of four points in the plane.
   **/
  template<class T>
  inline const T &
  min (const T & a, const T & b, const T & c, const T & d)
  {
	return min (min (a, b), min (c, d));
  }
}


namespace fl
{
# ifndef INFINITY
  static union
  {
	unsigned char c[4];
	float f;
  }
  infinity = {0, 0, 0x80, 0x7f};
# define INFINITY fl::infinity.f
# endif

# ifndef NAN
  static union
  {
	unsigned char c[4];
	float f;
  }
  nan = {0, 0, 0xc0, 0x7f};
# define NAN fl::nan.f
# endif

# ifndef issubnormal
  inline bool
  issubnormal (float a)
  {
	return    ((*(uint32_t *) &a) & 0x7F800000) == 0
	       && ((*(uint32_t *) &a) &   0x7FFFFF) != 0;
  }

  inline bool
  issubnormal (double a)
  {
#   ifdef _MSC_VER
	int c = _fpclass (a);
	return c == _FPCLASS_ND  ||  c == _FPCLASS_PD;
#   else
	return    ((*(uint64_t *) &a) & 0x7FF0000000000000ll) == 0
	       && ((*(uint64_t *) &a) &    0xFFFFFFFFFFFFFll) != 0;
#   endif
  }
# endif

  /**
	 Same as round(), but when <code>|a - roundp(a)| = 0.5</code> the result will
	 be the more positive integer.
  **/
  inline float
  roundp (float a)
  {
	return floorf (a + 0.5f);
  }

  /**
	 Same as round(), but when <code>|a - roundp(a)| = 0.5</code> the result will
	 be the more positive integer.
  **/
  inline double
  roundp (double a)
  {
	return floor (a + 0.5);
  }

  inline float
  mod2pi (float a)
  {
	a = fmodf (a, TWOPIf);
	if (a < 0)
	{
	  a += TWOPIf;
	}
	return a;
  }

  inline double
  mod2pi (double a)
  {
	a = fmod (a, TWOPI);
	if (a < 0)
	{
	  a += TWOPI;
	}
	return a;
  }
}


#endif
