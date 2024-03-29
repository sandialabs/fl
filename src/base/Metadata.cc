/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/metadata.h"

#include <stdlib.h>

using namespace fl;
using namespace std;


// class Metadata -------------------------------------------------------------

Metadata::~Metadata ()
{
}

void
Metadata::get (const string & name, string & value)
{
}

void
Metadata::get (const string & name, int32_t & value)
{
  string temp;
  get (name, temp);
  if (temp.size ()) value = atoi (temp.c_str ());
}

void
Metadata::get (const string & name, uint32_t & value)
{
  string temp;
  get (name, temp);
  if (temp.size ()) value = strtoul (temp.c_str (), 0, 0);
}

void
Metadata::get (const string & name, double & value)
{
  string temp;
  get (name, temp);
  if (temp.size ()) value = atof (temp.c_str ());
}

// Metadata::get (string, Matrix<double>) is implemented in src/numeric/MatrixDouble.cc

void
Metadata::set (const string & name, const string & value)
{
}

void
Metadata::set (const string & name, int32_t value)
{
  char buffer[32];
  sprintf (buffer, "%i", value);
  set (name, buffer);
}

void
Metadata::set (const string & name, uint32_t value)
{
  char buffer[32];
  sprintf (buffer, "%u", value);
  set (name, buffer);
}

void
Metadata::set (const string & name, double value)
{
  char buffer[32];
  sprintf (buffer, "%g", value);
  set (name, buffer);
}

// Metadata::set (string, Matrix<double>) is implemented in src/numeric/MatrixDouble.cc
