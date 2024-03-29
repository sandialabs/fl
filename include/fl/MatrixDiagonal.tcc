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


#ifndef fl_matrix_diagonal_tcc
#define fl_matrix_diagonal_tcc


#include "fl/matrix.h"


namespace fl
{
  // MatrixDiagonal -----------------------------------------------------------

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal ()
  {
	rows_ = 0;
	columns_ = 0;
  }

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal (const int rows, const int columns)
  {
	rows_ = rows;
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}
	data.grow (std::min (rows_, columns_) * sizeof (T));
  }

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal (const Vector<T> & that, const int rows, const int columns)
  {
	if (rows == -1)
	{
	  rows_ = that.rows ();
	}
	else
	{
	  rows_ = rows;
	}
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}

	data = that.data;
  }

  template<class T>
  uint32_t
  MatrixDiagonal<T>::classID () const
  {
	return MatrixAbstractID | MatrixDiagonalID;
  }

  template <class T>
  MatrixAbstract<T> *
  MatrixDiagonal<T>::clone (bool deep) const
  {
	if (deep)
	{
	  MatrixDiagonal * result = new MatrixDiagonal (rows_, columns_);
	  result->data.copyFrom (data);
	  return result;
	}
	return new MatrixDiagonal (*this);
  }

  template <class T>
  T &
  MatrixDiagonal<T>::operator () (const int row, const int column) const
  {
	if (row == column)
	{
	  return ((T *) data)[row];
	}
	else
	{
	  static T zero;
	  zero = (T) 0;
	  return zero;
	}
  }

  template <class T>
  T &
  MatrixDiagonal<T>::operator [] (const int row) const
  {
	return ((T *) data)[row];
  }

  template <class T>
  int
  MatrixDiagonal<T>::rows () const
  {
	return rows_;
  }

  template <class T>
  int
  MatrixDiagonal<T>::columns () const
  {
	return columns_;
  }

  template <class T>
  void
  MatrixDiagonal<T>::resize (const int rows, const int columns)
  {
	rows_ = rows;
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}
	data.grow (std::min (rows_, columns_) * sizeof (T));
  }

  template <class T>
  void
  MatrixDiagonal<T>::clear (const T scalar)
  {
	if (scalar == (T) 0)
	{
	  data.clear ();
	}
	else
	{
	  T * i = (T *) data;
	  T * end = i + std::min (rows_, columns_);
	  while (i < end)
	  {
		*i++ = scalar;
	  }
	}	  
  }
}


#endif
