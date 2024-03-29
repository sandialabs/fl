/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef vector_sparse_h
#define vector_sparse_h


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <algorithm>
#include <vector>


namespace fl
{
  /**
	 Similar to an STL vector, but attempts to use memory only to store
	 non-zero elements.  Stores nearby non-zero elements in contiguous
	 blocks of memory, and does not store large contiguous groups of
	 zeros.  (Note the distinction between this strategy and associating
	 an index with every represented element.)  This class requires that its
	 element type T have the concept of a zero value.  That is,
	 <code>(T) 0</code> should be possible to create, and hopefully also
	 efficient.

	 <p>In addition to the standard iterators, this class provides a sparse
	 iterator that visits only stored elements and indicates the integer
	 index of its current position.

	 <p>This class exists primarily to support the implementation of sparse
	 matrices.  It is meant to be equivalent to std::vector<T>.  However,
	 we only implement particular functionality as the need becomes evident.

	 <p>Interface issue:  The functions at() and operator[] return references
	 to elements, which can be used as left or right values.  This is fine
	 for a dense vector.  However, the intended use has a big impact on how
	 a sparse vector should behave.  The programmer may wish to
	 <ul>
	 <li>Read the referenced memory.
	 <li>Assign a non-zero value to the referenced memory.
	 <li>Assign a zero value to the referenced memory.
	 </ul>
	 Assigning values (whether zero or non-zero) can change the structure
	 of the sparse vector.  Therefore, the const versions of at() and
	 operator[] will return a dummy object for any element that isn't stored
	 (a zero), on the assumption that programmer intends merely to read it.
	 The non-const versions will always allocate storage for an element if
	 it isn't already, on the assumption that the programmer intends to
	 assign a non-zero value to the element.  This means the non-const
	 versions can destroy sparsity if the programmer is not careful.  Similar
	 rules apply to iterators.  None of these functions can maintain sparsity
	 in the case where the programmer assigns a zero value, since they are
	 called only to deliver a reference to the element.  The function
	 sparsify() exists to rebuild the sparsity structure when the programmer
	 deems necessary.  The function clear(int) exists to explicitly set an
	 element to zero and incrementally maintain the sparsity structure.

	 <p>Warning: This class treats element type T as having a trivial copy
	 constructor.  IE: we assume we can simply move it around in memory.
	 This is inconsistent with std::vector<T>, which uses class traits to
	 select the copy method.  We need to determine if traits have a standard
	 interface across various implementations of STL before committing to
	 this aproach.  IE: This code must interoperate with several different
	 versions of STL, so it can't simply imitate any given one of them.
	 It may be useful to hack our own mini traits system just for use in
	 this code.
   **/
  template<class T>
  class vectorsparse
  {
  public:
	typedef T         value_type;
	typedef T *       pointer;
	typedef T &       reference;
	typedef const T * const_pointer;
	typedef const T & const_reference;
	typedef int32_t   size_type;
	typedef int32_t   difference_type;


	// Inner classes ----------------------------------------------------------

	/**
	   A block of memory that stores of a contiguous group of elements
	   (regardless of whether they are zero or not).  This class acts as
	   a header to a chunk of memory, which immediately follows it.

	   <p>Since this is strictly internal implementation, we take the
	   liberty to not use the STL typedefs from the outer class when the
	   actual type is obvious.
	 **/
	class Contig
	{
	public:
	  static Contig * create (int index, int count)
	  {
		assert (count >= 1);
		Contig * result = (Contig *) malloc (sizeof (Contig) + (count - 1) * sizeof (T));
		result->index = index;
		result->count = count;
		return result;
	  }

	  /**
		 Returns a new Contig containing the old elements, along with newly
		 constructed ones at the beginning, end, or both.  Destroys us in
		 the process, so calling routine must stop using "this".
	  **/
	  Contig * expand (int newIndex, int newCount, const T & value = zero)
	  {
		const int fCount = index - newIndex;
		assert (fCount >= 0);
		const int lCount = newCount - count - fCount;
		assert (lCount >= 0);
		Contig * result = create (newIndex, newCount);
		T * f    = result->start ();
		T * fEnd = f + fCount;
		T * l    = fEnd + count;
		T * lEnd = l + lCount;
		memcpy (fEnd, &data, count * sizeof (T));
		free (this);
		while (f < fEnd) ::new (f++) T (value);
		while (l < lEnd) ::new (l++) T (value);
		return result;
	  }

	  /**
		 Returns a new Contig containing any old elements that exist in the
		 new reduced range.  Destructs any elements outside the range.
		 Destoys us in the process, so calling routine must stop using "this".
	   **/
	  Contig * shrink (int newIndex, int newCount)
	  {
		const int fCount = newIndex - index;
		assert (fCount >= 0);
		T * f    = &data;
		T * fEnd = f + fCount;
		T * l    = fEnd + newCount;
		T * lEnd = f + count;
		assert (l <= lEnd);
		Contig * result = create (newIndex, newCount);
		memcpy (result->start (), fEnd, newCount * sizeof (T));
		while (f < fEnd) (*f++).~T ();
		while (l < lEnd) (*l++).~T ();
		free (this);
		return result;
	  }

	  Contig * duplicate () const
	  {
		Contig * result = create (index, count);
		const T * i = &data;
		T * o       = result->start ();
		T * end     = result->finish ();
		while (o < end) ::new (o++) T (*i++);
		return result;
	  }

	  void construct ()
	  {
		T * i = &data;
		T * end = i + count;
		while (i < end) ::new (i++) T (0);
	  }

	  void destruct ()
	  {
		T * i = &data;
		T * end = i + count;
		while (i < end) (i++)->~T ();
	  }

	  T * start  () {return &data;}
	  T * finish () {return &data + count;}

	  int index;  ///< array index of first stored element
	  int count;  ///< number of stored elements
	  T data;  ///< first stored element; all others immediately follow this one in memory, so it functions as the first element in an array
	};

	class iterator
	{
	public:
	  typedef std::bidirectional_iterator_tag        iterator_category;
	  typedef typename vectorsparse::value_type      value_type;
	  typedef typename vectorsparse::difference_type difference_type;
	  typedef typename vectorsparse::size_type       size_type;
	  typedef typename vectorsparse::pointer         pointer;
	  typedef typename vectorsparse::reference       reference;

	  iterator () : container (0), index (0) {}
	  iterator (vectorsparse * container, size_type index) : container (container), index (index) {}

	  reference operator * () const
	  {
		return (*container)[index];
	  }

	  pointer operator -> () const
	  {
		return & (*container)[index];
	  }

	  iterator & operator ++ ()  // prefix
	  {
		index++;
		return *this;
	  }

	  iterator operator ++ (int)  // postfix
	  {
		iterator result (container, index);
		index++;
		return result;
	  }

	  iterator & operator -- ()  // prefix
	  {
		index--;
		return *this;
	  }

	  iterator operator -- (int)  // postfix
	  {
		iterator result (container, index);
		index--;
		return result;
	  }

	  vectorsparse * container;
	  size_type index;
	};

	class const_iterator
	{
	public:
	  typedef std::bidirectional_iterator_tag        iterator_category;
	  typedef typename vectorsparse::value_type      value_type;
	  typedef typename vectorsparse::difference_type difference_type;
	  typedef typename vectorsparse::size_type       size_type;
	  typedef typename vectorsparse::const_pointer   pointer;
	  typedef typename vectorsparse::const_reference reference;

	  const_iterator () : container (0), index (0) {}
	  const_iterator (const vectorsparse * container, size_type index) : container (container), index (index) {}

	  reference operator * () const
	  {
		return (*container)[index];
	  }

	  pointer operator -> () const
	  {
		return & (*container)[index];
	  }

	  bool operator == (const const_iterator & that)
	  {
		return container == that.container  &&  index == that.index;
	  }

	  bool operator != (const const_iterator & that)
	  {
		return container != that.container  ||  index != that.index;
	  }

	  const_iterator & operator ++ ()  // prefix
	  {
		index++;
		return *this;
	  }

	  const_iterator operator ++ (int)  // postfix
	  {
		const_iterator result (container, index);
		index++;
		return result;
	  }

	  const_iterator & operator -- ()  // prefix
	  {
		index--;
		return *this;
	  }

	  const_iterator operator -- (int)  // postfix
	  {
		const_iterator result (container, index);
		index--;
		return result;
	  }

	  const vectorsparse * container;
	  size_type index;
	};

	class sparse_iterator
	{
	public:
	  typedef std::bidirectional_iterator_tag        iterator_category;
	  typedef typename vectorsparse::value_type      value_type;
	  typedef typename vectorsparse::difference_type difference_type;
	  typedef typename vectorsparse::size_type       size_type;
	  typedef typename vectorsparse::const_pointer   pointer;
	  typedef typename vectorsparse::const_reference reference;

	  sparse_iterator (vectorsparse * container, size_type index)
	  : container (container)
	  {
		c = container->contigs.begin ();
		i = 0;
		if (container->contigs.size () < 1) return;  // Any incrementing or decrementing of this iterator is not supported.
		int position = container->findContig (index);
		if (position < 0)
		{
		  i = (*c)->start ();
		}
		else
		{
		  c += position;
		  i = (*c)->start () + (index - (*c)->index);  // warning: no guard against going past end of buffer
		}
	  }

	  reference operator * () const
	  {
		return *i;
	  }

	  pointer operator -> () const
	  {
		return i;
	  }

	  sparse_iterator & operator ++ ()
	  {
		i++;
		if (i >= (*c)->finish ())
		{
		  c++;
		  if (c < container->contigs.end ())
		  {
			i = (*c)->start ();
		  }
		  else
		  {
			c--;
			i = (*c)->finish ();
		  }
		}
		return *this;
	  }

	  sparse_iterator & operator -- ()
	  {
		i--;
		if (i < (*c)->start ())
		{
		  c--;
		  if (c >= container->contigs.begin ())
		  {
			i = (*c)->finish () - 1;
		  }
		  else
		  {
			c = container->contigs.begin ();
			i = (*c)->start ();
		  }
		}
		return *this;
	  }

	  int index ()
	  {
		return (*c)->index + (i - (*c)->start ());
	  }

	  vectorsparse * container;
	  typename std::vector<Contig *>::iterator c;
	  T * i;
	};


	// Main class -------------------------------------------------------------

	vectorsparse ()
	{
	  threshold = 20;
	  lastIndex = -1;
	}

	/// Deep copy semantics.
	vectorsparse (const vectorsparse & that)
	{
	  threshold = that.threshold;
	  lastIndex = that.lastIndex;
	  typename std::vector<Contig *>::const_iterator it = that.contigs.begin ();
	  while (it < that.contigs.end ()) contigs.push_back ((*it++)->duplicate ());
	}

	~vectorsparse ()
	{
	  //std::cerr << "dtor" << std::endl;
	  typename std::vector<Contig *>::iterator it = contigs.begin ();
	  for (; it < contigs.end (); it++)
	  {
		//std::cerr << "  destructing " << (*it)->index << " " << (*it)->count << std::endl;
		(*it)->destruct ();
		free (*it);
		//std::cerr << "    done" << std::endl;
	  }
	  //std::cerr << "dtor done" << std::endl;
	}

	// Sweeping assignment operations are low priority, as they are unlikely for a sparse vector.
	//void assign (size_type count, const value_type & value);
	//template<class it> void assign (it first, it last);

	iterator begin ()
	{
	  return iterator (this, 0);
	}

	const_iterator begin () const
	{
	  return const_iterator (this, 0);
	}

	iterator end ()
	{
	  return iterator (this, size ());
	}

	const_iterator end () const
	{
	  return const_iterator (this, size ());
	}

	//reverse_iterator       rbegin ();
	//const_reverse_iterator rbegin () const;
	//reverse_iterator       rend ();
	//const_reverse_iterator rend () const;

	sparse_iterator sbegin ()
	{
	  return sparse_iterator (this, 0);
	}

	sparse_iterator send ()
	{
	  return sparse_iterator (this, size ());
	}

	bool empty () const
	{
	  return lastIndex == -1;
	}

	size_type size () const
	{
	  return lastIndex + 1;
	}

	size_type max_size () const
	{
	  return INT32_MAX / sizeof (value_type);
	}

	/**
	   Returns count that includes last represented element.  This violates
	   the STL definition of vector::capacity(), because memory allocation
	   may occur even if the index of an element falls within this range.
	   However, an allocation will certainly occur for a non-const access
	   to an element outside this range.
	 **/
	size_type capacity () const
	{
	  if (contigs.size () == 0) return 0;
	  const Contig * last = contigs.back ();
	  return last->index + last->count;
	}

	void resize (size_type n, const value_type & value = zero)
	{
	  n = std::max (n, (size_type) 0);  // since size_type is currently signed, we must enforce non-negativity.
	  size_type s = size ();
	  if (n == s) return;
	  if (n < s)
	  {

		size_type position = findContig (n - 1);
		if (position < 0)
		{
		  position = 0;  // effectively, we will destroy all contigs
		}
		else
		{
		  Contig * c = contigs[position];
		  if (c->index < n) position++;  // don't destroy contig if it starts at or before the new lastIndex
		}
		typename std::vector<Contig *>::iterator it;
		for (it = contigs.begin () + position; it < contigs.end (); it++)
		{
		  (*it)->destruct ();
		  free (*it);
		}
		contigs.resize (position);
	  }
	  else if (value != zero)  // and n > s
	  {
		// fill-in "value" from just after lastIndex up to (n-1)
		// We can safely assume that the start index of the last contig is
		// less than or equal to the lastIndex.  However, we cannot assume
		// that the last contig contains the lastIndex, so it may be necessary
		// to create a new contig.
		Contig * lastContig = contigs.back ();
		int last = lastContig->index + lastContig->count - 1;
		if (lastIndex - last > threshold)  // need to create a new lastContig
		{
		  // We will not actually represent the current lastIndex, since it
		  // is a zero element.  Instead, we will fill-in "value" starting
		  // just after the current lastIndex.
		  lastContig = Contig::create (lastIndex + 1, (n - 1) - lastIndex);
		  contigs.push_back (lastContig);
		  T * i   = lastContig->start ();
		  T * end = i + lastContig->count;
		  while (i < end) ::new (i++) T (value);
		}
		else  // just update current lastContig
		{
		  int needed = (n - 1) - last;
		  if (needed > 0)  // need to extend lastContig
		  {
			contigs[contigs.size () - 1] = lastContig = lastContig->expand (lastContig->index, lastContig->count + needed, value);
		  }
		  T * i   = lastContig->start () + (lastIndex + 1 - lastContig->index);
		  T * end = i + (last - lastIndex);  // because we actually start at lastIndex + 1, this range will include "last" rather than stopping just short of it
		  while (i < end) *i++ = value;
		}
	  }
	  lastIndex = n - 1;
	}

	/**
	   Does nothing.  Reserving space is meaningless for a sparse
	   vector.  This violates the STL definition of vector::reserve(), since
	   the value specified to this function does not affect the value
	   returned by capacity().
	 **/
	void reserve (size_type n) {}

	reference operator [] (size_type index)
	{
	  //std::cerr << "non-const reference " << index << std::endl;
	  int position = findContig (index);
	  //std::cerr << "  position = " << position << std::endl;
	  Contig * c;
	  if (position < 0)
	  {
		if (contigs.size ())
		{
		  c = contigs[0];
		  int count = c->index - index;
		  if (count > threshold)
		  {
			c = Contig::create (index, threshold);
			c->construct ();
			contigs.insert (contigs.begin (), c);
		  }
		  else
		  {
			contigs[0] = c = c->expand (index, count + c->count);
		  }
		}
		else
		{
		  c = Contig::create (index, threshold);
		  c->construct ();
		  contigs.push_back (c);
		}
	  }
	  else c = contigs[position];
	  //std::cerr << "  c = " << c << std::endl;
	  int last = c->index + c->count - 1;
	  int d1 = index - last;
	  //std::cerr << "  last, d1 = " << last << " " << d1 << std::endl;
	  if (d1 > 0)
	  {
		if (position < contigs.size () - 1)
		{
		  typename std::vector<Contig *>::iterator it = contigs.begin () + (position + 1);
		  Contig * c2 = *it;
		  int d2 = c2->index - index;
		  if (d1 > threshold  &&  d2 > threshold + 1)  // Add 1 to threshold for d2 so we don't create a contig that directly abuts the subsequent one.
		  {
			c = Contig::create (index, threshold);
			c->construct ();
			contigs.insert (it, c);
		  }
		  else if (d1 == 1  &&  d2 == 1)  // This element will join two contigs
		  {
			Contig * c1 = c;
			int offset = c2->index - c1->index;
			contigs[position] = c = Contig::create (c1->index, offset + c2->count);
			T * start = c->start ();
			memcpy (start,          c1->start (), c1->count * sizeof (T));
			::new (start + c1->count) T (0);
			memcpy (start + offset, c2->start (), c2->count * sizeof (T));
			contigs.erase (it);  // get rid of c2
			free (c1);
			free (c2);
		  }
		  else if (d1 < d2)
		  {
			contigs[position] = c = c->expand (c->index, index - c->index + 1);
		  }
		  else
		  {
			contigs[position+1] = c = c2->expand (index, c2->index - index + c2->count);
		  }
		}
		else if (d1 <= threshold)
		{
		  contigs[position] = c = c->expand (c->index, index - c->index + 1);
		}
		else  // d1 is more than threshold elements beyond last represented element
		{
		  c = Contig::create (index, threshold);
		  c->construct ();
		  contigs.push_back (c);
		}
	  }
	  lastIndex = std::max (lastIndex, index);
	  return *(c->start () + (index - c->index));
	}

	const_reference operator [] (size_type index) const
	{
	  int position = findContig (index);
	  if (position < 0) return zero;
	  Contig * c = contigs[position];
	  if (index >= c->index + c->count) return zero;
	  return *(c->start () + (index - c->index));
	}

	reference at (size_type index)
	{
	  return (*this)[index];
	}

	const_reference at (size_type index) const
	{
	  return (*this)[index];
	}

	reference front ()
	{
	  return (*this)[0];
	}

	const_reference front () const
	{
	  return (*this)[0];
	}

	reference back ()
	{
	  return (*this)[lastIndex];
	}

	const_reference back () const
	{
	  return (*this)[lastIndex];
	}

	void push_back (const value_type & value)
	{
	  (*this)[lastIndex + 1] = value;
	}

	void pop_back ()
	{
	  resize (lastIndex);
	}

	//iterator insert (iterator position, const value_type & value);
	//void insert (iterator position, size_type count, const value_type & value);
	//template<class it> void insert (it position, it first, it last);

	/**
	   Dispose of all storage, or set one element to zero.  This is the
	   function primarily responsible for incremental maintenance of
	   sparsity when a single element is set to zero.
	 **/
	void clear (size_type index = -1)
	{
	  if (index < 0)
	  {
		typename std::vector<Contig *>::iterator it;
		for (it = contigs.begin (); it < contigs.end (); it++)
		{
		  (*it)->destruct ();
		  free (*it);
		}
		contigs.clear ();
	  }
	  else
	  {
		int position = findContig (index);
		if (position < 0) return;
		Contig * c = contigs[position];
		int e = index - c->index;
		if (e >= c->count) return;

		// find contiguous zeros
		T * start = c->start ();
		T * finish = c->finish ();
		T * lastZero = start + (e + 1);
		while (lastZero < finish  &&  *lastZero == zero) lastZero++;
		T * firstZero = start + (e - 1);
		while (firstZero >= start  &&  *firstZero == zero) firstZero--;
		firstZero++;

		// split or truncate Contig if there are enough contiguous zeros
		if (lastZero - firstZero > threshold)
		{
		  T * kill = firstZero;
		  while (kill < lastZero) (*kill++).~T ();

		  Contig * afterContig = 0;
		  int count = finish - lastZero;
		  if (count)
		  {
			afterContig = Contig::create (c->index + (lastZero - start), count);
			memcpy (afterContig->start (), lastZero, count * sizeof (T));
		  }

		  Contig * beforeContig = 0;
		  count = firstZero - start;
		  if (count)
		  {
			beforeContig = Contig::create (c->index, count);
			memcpy (beforeContig->start (), start, count * sizeof (T));
		  }

		  free (c);
		  if (beforeContig) contigs[position] = beforeContig;
		  if (afterContig)
		  {
			if (beforeContig) contigs.insert (contigs.begin () + (position + 1), afterContig);
			else              contigs[position] = afterContig;
		  }
		  if (contigs[position] == c) contigs.erase (contigs.begin () + position);
		}
		else *(start + e) = zero;
	  }
	}

	//iterator erase (iterator position);
	//iterator erase (iterator first, iterator last);

	void swap (vectorsparse & that)
	{
	  std::swap (threshold, that.threshold);
	  std::swap (lastIndex, that.lastIndex);
	  std::swap (contigs,   that.contigs);
	}

	struct ContigRemap
	{
	  size_type index;
	  size_type count;
	  Contig *  contig;
	  typename std::vector<Contig *>::iterator it;
	};

	/**
	   Restore sparse structure.  Processes the storage contigs so that they
	   follow these rules:
	   <ul>
	   <li>Each contig begins and ends with a non-zero element.
	   <li>Within each contig, any sequence of zeros is shorter than
	   threshold elements.
	   <li>Between adjacent contigs, there are at least threshold zero elements.
	   </ul>
	**/
	void sparsify ()
	{
	  // Pass 1 -- Splits
	  std::vector<ContigRemap> splits;
	  splits.reserve (contigs.size ());
	  ContigRemap r;
	  typename std::vector<Contig *>::iterator it;
	  for (it = contigs.begin; it < contigs.end (); it++)
	  {
		r.contig = *it;
		r.it     = it;
		T * start = (*it)->start ();
		T * end   = (*it)->finish ();
		T * i     = start;
		// A block consists of a group of (mostly) non-zero elements, followed
		// by a group of zero elements.  The first block (and only the first
		// block) might have an empty set of nonzero elements.
		T * first = start;  // beginning element of block
		T * last  = 0;      // last non-zero element in a block
		while (i < end)
		{
		  if (*i != zero)
		  {
			if (i - last > threshold)
			{
			  r.count = last - first + 1;
			  if (r.count > 0)  // distinquishes between an initial set of zeros and one that follows a set of nonzeros
			  {
				r.index = first - start + (*it)->index;
				splits.push_back (r);
			  }
			  first = i;
			}
			last = i;
		  }
		  i++;
		}
		r.count = last - first + 1;
		if (r.count > 0)
		{
		  r.index = first - start + (*it)->index;
		  splits.push_back (r);
		}
	  }

	  // Pass 2 -- Merges
	  std::vector<Contig *> newContigs;
	  newContigs.reserve (contigs.size ());
	  typename std::vector<ContigRemap>::iterator i     = splits.begin ();
	  typename std::vector<ContigRemap>::iterator end   = splits.end ();
	  typename std::vector<ContigRemap>::iterator first = i;
	  typename std::vector<ContigRemap>::iterator last  = i;
	  i++;
	  while (i <= end)
	  {
		if (i == end  ||  last->index + last->count - i->index > threshold)
		{
		  // Combine and copy out contigs
		  if (first == last  &&  first->index == first->contig->index  &&  first->count == first->contig->count)
		  {
			// No change to the contig, so just keep it.
			newContigs.push_back (first->contig);
			*first->it = 0;  // Don't delete contig later
		  }
		  else
		  {
			Contig * c = Contig::create (first->index, last->index + last->count - first->index);
			pointer m = c->start ();
			pointer f = m;
			newContigs.push_back (c);
			typename std::vector<ContigRemap>::iterator r = first;
			while (r < last)
			{
			  memcpy (f,
					  r->contig->start () + (r->index - r->contig->index),
					  r->count);
			  f += r->count;
			  typename std::vector<ContigRemap>::iterator s = r + 1;
			  pointer e = m + (s->index - first->index);
			  while (f < e) ::new (f++) T (0);
			  f = e;
			  r = s;
			}
			memcpy (f,
					r->contig->start () + (r->index - r->contig->index),
					r->count);
		  }

		  first = i;
		}
		last = i;
		i++;
	  }

	  // Dispose of old memory
	  for (it = contigs.begin (); it < contigs.end (); it++)
	  {
		if (*it) delete (*it);
	  }
	  contigs = newContigs;
	}

	// Support functions for Contig style storage

	/**
	   Perform binary search for Contig that is responsible for the given
	   index.  The resulting Contig may not actually contain the element.
	   If the requested index occurs before any existing Contig, then this
	   function returns -1.  Otherwise, it returns the position of the
	   closest Contig that starts at or before requested index.
	 **/
	int findContig (size_type index) const
	{
	  int hi = contigs.size () - 1;
	  if (hi < 0  ||  contigs[0]->index > index) return -1;
	  if (contigs[hi]->index <= index) return hi;
	  int lo = 0;

	  while (true)
	  {
		// Invariants:
		// lo increases monotonically
		// hi decreases monotonically
		// lo->index <= index
		// hi->index > index
		// by implication, lo is strictly less than hi

		int mid = (hi + lo) / 2;

		// The == case is relatively rare, so we don't explicitly test for it.
		if (contigs[mid]->index <= index)
		{
		  if (contigs[mid+1]->index > index) return mid;
		  lo = mid + 1;
		}
		else
		{
		  hi = mid;
		}
	  }
	}


	static const T zero;  ///< A dummy value returned when an element is represented.  Also used for comparisons with "zero".  Assigned (T) 0 at construction time.
	size_type threshold;  ///< The number of contiguous zero elements before a contig is split or joined, with 1 element of hysteresis.
	size_type lastIndex;  ///< Index of nominal end of array.  Not necessarily the last represented element, or even a represented element at all.
	std::vector<Contig *> contigs;  ///< List of active contigs.  Managed by a real STL vector!
  };
  template <class T> const T vectorsparse<T>::zero = (T) 0;
}


#endif
