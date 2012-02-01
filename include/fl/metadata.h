/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_metadata_h
#define fl_metadata_h


#include "fl/matrix.h"

#undef SHARED_BASE
#ifdef _MSC_VER
#  ifdef flBase_EXPORTS
#    define SHARED_BASE __declspec(dllexport)
#  else
#    define SHARED_BASE __declspec(dllimport)
#  endif
#else
#  define SHARED_BASE
#endif

#undef SHARED_NUMERIC
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED_NUMERIC __declspec(dllexport)
#  else
#    define SHARED_NUMERIC __declspec(dllimport)
#  endif
#else
#  define SHARED_NUMERIC
#endif


namespace fl
{
  class SHARED_BASE NamedValueSet
  {
  public:
	void serialize (fl::Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	void get (const std::string & name,       std::string & value);
	void set (const std::string & name, const std::string & value);

	std::map<std::string, std::string> namedValues;
  };
  SHARED_BASE std::ostream & operator << (std::ostream & out, const NamedValueSet & data);

  /**
	 Inheritable set of convenience functions for accessing and converting
	 named values to various other types.  This goes in the numeric library
	 because we create a dependency on Matrix, and most of the ancilliary
	 data types are numeric in any case.

	 <p>To make use of the class properly, do the following steps in the
	 child class:
	 <ul>
	 <li>Inherit from Metadata.
	 <li>Implement get(string,string) and set(string,string)
	 <li>Pull all the other implementations in with "using Metadata::get" and
	 "using Metadata::set".  This step is important, because the standard
	 behavior of C++ is to hide all the inherited virtual functions when
	 you create a get or set function in the child class.  Alternately,
	 you can explicitly qualify any call: "object->Metadata::get (...)".
	 </ul>
  **/
  class SHARED_NUMERIC Metadata
  {
  public:
	virtual ~Metadata ();

	virtual void get (const std::string & name, std::string &    value);
	virtual void get (const std::string & name, int32_t &        value);
	virtual void get (const std::string & name, uint32_t &       value);
	virtual void get (const std::string & name, double &         value);
	virtual void get (const std::string & name, Matrix<double> & value);

	virtual void set (const std::string & name, const std::string &    value);
	virtual void set (const std::string & name, int32_t                value);
	virtual void set (const std::string & name, uint32_t               value);
	virtual void set (const std::string & name, double                 value);
	virtual void set (const std::string & name, const Matrix<double> & value);
  };
}


#endif