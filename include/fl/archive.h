/*
Author: Fred Rothganger

Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_archive_h
#define fl_archive_h

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <iostream>
#include <fstream>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flBase_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  class Archive;
  typedef void * createFunction    ();
  typedef void   serializeFunction (void * me, Archive & archive, uint32_t version);

  class ClassDescription
  {
  public:
	createFunction *    create;
	serializeFunction * serialize;
	std::string         name;     ///< name to write to stream
	uint32_t            index;    ///< serial # of class in archive; 0xFFFFFFFF means not assigned yet
	uint32_t            version;
  };

  /**
	 Manages all bookkeeping needed to read and write object structures
	 on a stream.
	 This is not the most sophisticated serialization scheme.  It can't do
	 everything.  If you want to do everything, use Boost.
	 The rules for this method are:
	 * Everything is either a primitive type or a subclass of Serializable.
	 * Serializable is always polymorphic, with at least one virtual function:
	   serialize(Archive).
	 * Select STL classes get special treatment, and are effectively primitive:
	   string and vector.
	 * Serializables are numbered sequentially in the archive, starting at zero.
	 * Pointers are written as the index of the referenced Serializable.
	 * Just before a Serializable class is used for the first time, a record
	   describing it appears in the archive.  This record contains only
	   information that might be unknown at that point.  In particular,
	   a reference to Serializable will only cause a version number to be
	   written, while a pointer to Serializable will cause a class name to
	   be written, followed by a version number.
	 * Classes are numbered sequentially in the archive, starting at zero.
	 * If a pointer appears and its referenced Serializable has not yet
	   appeared, then the referenced Serializable appears immediately after.
	 * When a Serializable is written out to fulfill a pointer, the record
	   begins with a class index.
	 * No reference class members are allowed.  Therefore, a Serializable is
	   either already instantiated, or is instantiated based on its class
	   index (using a static factory function).
	 * To implement all this, every class'es serialize() function must begin
	   with a call that registers both the static factory function and the
	   version number of the class.
   **/
  class Archive
  {
  public:
	SHARED Archive (std::istream & in,  bool ownStream = false);
	SHARED Archive (std::ostream & out, bool ownStream = false);
	SHARED Archive (const std::string & fileName, const std::string & mode);
	SHARED ~Archive ();

	SHARED void open (std::istream & in, bool ownStream = false);
	SHARED void open (std::ostream & out, bool ownStream = false);
	SHARED void open (const std::string & fileName, const std::string & mode);
	SHARED void close ();

	/**
	   Create a class description record in memory.
	   Only use for classes that can be instantiated (have a default
	   constructor and are not abstract).  Note that all classes that are
	   serialized polymorphically must be intantiable.
	   @param name Specifies the string actually stored in the stream that
	   identifies this class.  Can be called several times with different
	   values.  When reading a stream, all values act as aliases for the same
	   class.  When writing a stream, only the value given in the last call
	   is used.  If no value is given, defaults to the RTTI name of the class.
	   This isn't necessarily the best thing, because RTTI names can vary
	   between compilers, and even between versions of the same compiler.
	 **/
	template<class T>
	void registerClass (const std::string & name = "")
	{
	  // Inner-class used to lay down a function that can construct an object
	  // of type T.
	  struct Product
	  {
		static void * create ()
		{
		  return new T;  // default constructor
		}
		static void serialize (void * me, Archive & archive, uint32_t version)
		{
		  ((T *) me)->serialize (archive, version);
		}
	  };

	  // Create or locate basic registration record
	  std::string typeidName = typeid (T).name ();
	  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
	  if (c == classesOut.end ())
	  {
		ClassDescription * info = new ClassDescription;
		info->index   = 0xFFFFFFFF;
		info->version = T::serializeVersion;
		c = classesOut.insert (make_pair (typeidName, info)).first;
	  }
	  c->second->create    = Product::create;
	  c->second->serialize = Product::serialize;
	  c->second->name      = (name.size () == 0) ? typeidName : name;

	  // Add or reassign alias for initial lookup when reading from stream
	  alias.insert (make_pair (c->second->name, c->second));
	}

	template<class T>
	Archive & operator & (T & data)
	{
	  // Auto-register if necessary
	  std::string typeidName = typeid (T).name ();  // Because reference members are not allowed, we assume that T is exactly the class we are working with, rather than a parent thereof.
	  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
	  if (c == classesOut.end ())
	  {
		ClassDescription * info = new ClassDescription;
		info->create    = 0;
		info->serialize = 0;
		info->name      = typeidName;
		info->index     = 0xFFFFFFFF;
		info->version   = T::serializeVersion;
		c = classesOut.insert (make_pair (typeidName, info)).first;
		// No need for alias, unless class is also treated polymorphically.
	  }

	  // Assign class index if necessary
	  if (c->second->index == 0xFFFFFFFF)
	  {
		c->second->index = classesIn.size ();
		classesIn.push_back (c->second);
		// Serialize the class description.  Only need version #.  Don't need
		// class name because it is implicit.
		(*this) & c->second->version;
	  }

	  // Record the pointer
	  // It is an error to record an object through a pointer, and then
	  // later record it through a reference.  The problem with this is
	  // that two (or more) copies of the object will be created, and
	  // the pointers will not address the same object as the reference.
	  // It is possible to detect this situation at write-time (by keeping
	  // track of which class an object was last written under), but we
	  // currently don't want to spend the space resources on it.
	  if (pointersOut.find (&data) == pointersOut.end ())
	  {
		pointersOut.insert (make_pair (&data, pointersOut.size ()));
		if (in) pointersIn.push_back (&data);
	  }

	  data.serialize (*this, c->second->version);
	  return *this;
	}

	template<class T>
	Archive & operator & (T * & data)
	{
	  if (in)
	  {
		uint32_t pointer;
		(*this) & pointer;
		if      (pointer == 0xFFFFFFFF) data = 0;
		else if (pointer > pointersIn.size ()) throw "pointer index out of range in archive";
		else if (pointer < pointersIn.size ()) data = (T *) pointersIn[pointer];
		else  // new pointer, so serialize associated object
		{
		  uint32_t classIndex;
		  (*this) & classIndex;
		  if (classIndex >  classesIn.size ()) throw "class index out of range in archive";
		  if (classIndex == classesIn.size ())  // new class, so deserialize class info
		  {
			std::string name;
			(*this) & name;

			std::map<std::string, ClassDescription *>::iterator a = alias.find (name);
			if (a == alias.end ())
			{
			  sprintf (message, "Unregistered alias: %s", name.c_str ());
			  throw message;
			}
			a->second->index = classIndex;
			classesIn.push_back (a->second);

			(*this) & a->second->version;
		  }

		  ClassDescription * d = classesIn[classIndex];
		  if (d->create == 0  ||  d->serialize == 0)
		  {
			sprintf (message, "Please explicitly register: %s", d->name.c_str ());
			throw message;
		  }
		  data = (T *) d->create ();
		  pointersOut.insert (make_pair (data, pointersOut.size ()));
		  pointersIn.push_back (data);
		  d->serialize (data, *this, d->version);
		}
	  }
	  else
	  {
		if (data == 0)
		{
		  uint32_t nullPointer = 0xFFFFFFFF;
		  return (*this) & nullPointer;
		}
		std::map<void *, uint32_t>::iterator p = pointersOut.find (data);
		if (p != pointersOut.end ()) (*this) & p->second;
		else
		{
		  p = pointersOut.insert (std::make_pair (data, pointersOut.size ())).first;
		  (*this) & p->second;

		  std::string typeidName = typeid (*data).name ();
		  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
		  if (c == classesOut.end ())
		  {
			sprintf (message, "Unregistered class %s", typeidName.c_str ());
			throw message;
		  }
		  if (c->second->index < 0xFFFFFFFF) (*this) & c->second->index;
		  else
		  {
			c->second->index = classesIn.size ();
			classesIn.push_back (c->second);
			(*this) & c->second->index;
			(*this) & c->second->name;
			(*this) & c->second->version;
		  }

		  if (c->second->serialize == 0)
		  {
			sprintf (message, "Please explicitly register: %s", typeidName.c_str ());
			throw message;
		  }
		  c->second->serialize (data, *this, c->second->version);
		}
	  }
	  return *this;
	}

	template<class T>
	Archive & operator & (std::vector<T> & data)
	{
	  uint32_t count = data.size ();
	  (*this) & count;
	  if (in)
	  {
		if (in->bad ()) throw "stream bad";
		data.resize (count);  // possibly blow away existing data
	  }
	  for (uint32_t i = 0; i < count; i++) (*this) & data[i];
	  return (*this);
	}

	std::istream * in;
	std::ostream * out;
	bool ownStream;

	std::vector<void *>        pointersIn;   ///< mapping from serial # to pointer
	std::map<void *, uint32_t> pointersOut;  ///< mapping from pointer to serial #; @todo change this to unordered_map when broadly available

	std::vector<ClassDescription *>           classesIn;   ///< mapping from serial # to class description; ClassDescription objects are held by classesOut
	std::map<std::string, ClassDescription *> classesOut;  ///< mapping from RTTI name to class description; one-to-one
	std::map<std::string, ClassDescription *> alias;       ///< mapping from user-assigned name to class description; can be many-to-one

	char message[256];  ///< buffer for constructing detailed exceptions
  };

  // These are necessary, because any type that does not have an explicit
  // operator&() function must have a serialize() function.  The programmer
  // may define others locally as needed.
  template<> SHARED Archive & Archive::operator & (std::string    & data);
  template<> SHARED Archive & Archive::operator & (uint8_t        & data);
  template<> SHARED Archive & Archive::operator & (uint16_t       & data);
  template<> SHARED Archive & Archive::operator & (uint32_t       & data);
  template<> SHARED Archive & Archive::operator & (uint64_t       & data);
  template<> SHARED Archive & Archive::operator & (int8_t         & data);
  template<> SHARED Archive & Archive::operator & (int16_t        & data);
  template<> SHARED Archive & Archive::operator & (int32_t        & data);
  template<> SHARED Archive & Archive::operator & (int64_t        & data);
  template<> SHARED Archive & Archive::operator & (float          & data);
  template<> SHARED Archive & Archive::operator & (double         & data);
  template<> SHARED Archive & Archive::operator & (bool           & data);
}


#endif
