/*
Author: Fred Rothganger
Created 2/26/2006


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"
#include "fl/string.h"
#include "fl/endian.h"
#include "fl/lapack.h"

#include <stdio.h>
#include <typeinfo>


using namespace std;
using namespace fl;


// NITF structure -------------------------------------------------------------

/**
   Record in maps used by nitfHeader to guide parsing.
 **/
struct nitfMapping
{
  const char * name;  ///< NITF standard field name.  If null, then end of table.
  int          size;  ///< byte count

  /**
	 A terse string that indicates the kind of data in this field.
	 <ul>
	 <li>"c" -- The name field refers to a combination of nitfItem subclass and nitfMapping table.
	 <li>"A" -- ASCII
	 <li>"N" -- Integer
	 <li>"F" -- Float
	 </ul>
  **/
  const char * type;
  const char * defaultValue;  ///< Blank if standard default for given type.
};

class nitfItem
{
public:
  nitfItem (nitfMapping * map)
  {
	this->map = map;
	data = 0;
  }

  virtual ~nitfItem ()
  {
	if (data) free (data);
  }

  virtual void read (istream & stream)
  {
	data = (char *) malloc (map->size);
	stream.read (data, map->size);

	//string bob (data, map->size);
	//cerr << "read item: " << map->name << " = " << bob << endl;
  }

  virtual void write (ostream & stream)
  {
	if (! data)
	{
	  data = (char *) malloc (map->size);
	  if (map->defaultValue[1])  // longer than a single character; must be exactly map->size chars long
	  {
		memcpy (data, map->defaultValue, map->size);
	  }
	  else  // one character long; use character to fill field
	  {
		memset (data, map->defaultValue[0], map->size);
	  }
	}
	stream.write ((char *) data, map->size);
  }

  virtual bool contains (const string & name)
  {
	return name == map->name;
  }

  virtual void get (const string & name, string & value)
  {
	value.assign (data, map->size);
  }

  virtual void get (const string & name, double & value)
  {
	string temp (data, map->size);
	value = atof (temp.c_str ());
  }

  virtual void get (const string & name, int & value)
  {
	string temp (data, map->size);
	value = atoi (temp.c_str ());
  }

  virtual void set (const string & name, const string & value)
  {
	int length = value.size ();
	if (length >= map->size)
	{
	  memcpy (data, (char *) value.c_str (), map->size);
	}
	else
	{
	  switch (map->type[0])
	  {
		case 'A':
		case 'F':
		  memcpy (data, (char *) value.c_str (), length);
		  memset (data + length, ' ', map->size - length);
		  break;
		case 'N':
		  int pad = map->size - length;
		  memset (data, '0', pad);
		  memcpy (data + pad, (char *) value.c_str (), length);
	  }
	}
  }

  virtual void set (const string & name, double value)
  {
	char buffer[256];
	if (map->type[0] == 'F')
	{
	  sprintf (buffer, "%lf", value);
	  set (name, buffer);
	}
	else
	{
	  long long v = (long long) roundp (value);
	  sprintf (buffer, "%0*lli", map->size, v);
	  memcpy (data, buffer, map->size);
	}
  }

  virtual void set (const string & name, int value)
  {
	char buffer[256];
	if (map->type[0] == 'F')
	{
	  sprintf (buffer, "%i", value);
	  set (name, buffer);
	}
	else
	{
	  sprintf (buffer, "%0*i", map->size, value);
	  memcpy (data, buffer, map->size);
	}
  }

  nitfMapping * map;
  char * data;

  static nitfItem * factory (nitfMapping * m);
};

/**
   Generic reader/writer of a contiguous section of NITF metadata items.
 **/
class nitfItemSet : public nitfItem
{
public:
  nitfItemSet (nitfMapping * map)
  : nitfItem (map)
  {
	nitfMapping * m = map;
	while (m->name)
	{
	  data.push_back (nitfItem::factory (m++));
	}
  }

  virtual ~nitfItemSet ()
  {
	for (int i = 0; i < data.size (); i++)
	{
	  delete data[i];
	}
  }

  virtual void read (istream & stream)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  data[i]->read (stream);
	}
  }

  virtual void write (ostream & stream)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  data[i]->write (stream);
	}
  }

  nitfItem * find (const string & name)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  if (data[i]->contains (name)) return data[i];
	}
	return 0;
  }

  virtual bool contains (const string & name)
  {
	if (find (name)) return true;
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void get (const string & name, double & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void get (const string & name, int & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void set (const string & name, const string & value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  virtual void set (const string & name, double value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  virtual void set (const string & name, int value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  vector<nitfItem *> data;
};

class nitfRepeat : public nitfItem
{
public:
  nitfRepeat (nitfMapping * map)
  : nitfItem (map)
  {
	countItem = nitfItem::factory (map);
  }

  virtual ~nitfRepeat ()
  {
	delete countItem;
	for (int i = 0; i < data.size (); i++)
	{
	  delete data[i];
	}
  }

  virtual void read (istream & stream)
  {
	countItem->read (stream);
	int count;
	countItem->get (map->name, count);

	for (int i = 0; i < count; i++)
	{
	  nitfItemSet * s = new nitfItemSet (&map[1]);
	  s->read (stream);
	  data.push_back (s);
	}
  }

  virtual void write (ostream & stream)
  {
	int count = data.size ();
	countItem->set (map->name, count);
	countItem->write (stream);

	for (int i = 0; i < count; i++)
	{
	  data[i]->write (stream);
	}
  }

  void split (const string & name, string & root, int & index)
  {
	int position = name.find_first_of ("0123456789");
	if (position == string::npos)
	{
	  root = name;
	  index = 0;
	}
	else
	{
	  root = name.substr (0, position);
	  index = atoi (name.substr (position).c_str ());
	}
  }

  virtual bool contains (const string & name)
  {
	string root;
	int index;
	split (name, root, index);

	if (countItem->contains (root)) return true;

	nitfMapping * m = &map[1];
	while (m->name)
	{
	  if (root == m->name) return true;
	  m++;
	}
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void get (const string & name, double & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= (int) data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void get (const string & name, int & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= (int) data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void set (const string & name, const string & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  virtual void set (const string & name, double value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  virtual void set (const string & name, int value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  nitfItem * countItem;
  vector<nitfItemSet *> data;
};

class nitfGeoloc : public nitfItemSet
{
public:
  nitfGeoloc (nitfMapping * map)
  : nitfItemSet (map)
  {
  }

  virtual void read (istream & stream)
  {
	data[0]->read (stream);
	if (data[0]->data[0] != map->defaultValue[0])
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->read (stream);
	  }
	}
  }

  virtual void write (ostream & stream)
  {
	data[0]->write (stream);
	if (data[1]->data)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->write (stream);
	  }
	}
  }
};

class nitfCompression : public nitfItemSet
{
public:
  nitfCompression (nitfMapping * map)
  : nitfItemSet (map),
	ICcodes ("C1|C3|C4|C5|C8|M1|M3|M4|M5|M8|I1")
  {
  }

  virtual void read (istream & stream)
  {
	data[0]->read (stream);
	string IC (data[0]->data, 2);
	if (ICcodes.find (IC) != string::npos)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->read (stream);
	  }
	}
  }

  virtual void write (ostream & stream)
  {
	data[0]->write (stream);
	string IC (data[0]->data, 2);
	if (ICcodes.find (IC) != string::npos)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->write (stream);
	  }
	}
  }

  string ICcodes;  ///< that require a COMRAT field
};

class nitfExtendableCount : public nitfItem
{
public:
  nitfExtendableCount (nitfMapping * map)
  : nitfItem (map)
  {
  }

  virtual void read (istream & stream)
  {
	char buffer[50];
	stream.read (buffer, map->size);
	buffer[map->size] = 0;
	count = atoi (buffer);
	if (! count)
	{
	  stream.read (buffer, map[1].size);
	  buffer[map[1].size] = 0;
	  count = atoi (buffer);
	}
  }

  virtual void write (ostream & stream)
  {
	char buffer[50];
	if (count < pow (10, map->size))
	{
	  sprintf (buffer, "%i", count);
	}
	else
	{
	  sprintf (buffer, "%0*i", map->size + map[1].size, count);
	}
	stream << buffer;
  }

  virtual bool contains (const string & name)
  {
	nitfMapping * m = map;
	while (m->name)
	{
	  if (name == m->name) return true;
	  m++;
	}
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	char buffer[50];
	sprintf (buffer, "%i", count);
	value = buffer;
  }

  virtual void get (const string & name, double & value)
  {
	value = count;
  }

  virtual void get (const string & name, int & value)
  {
	value = count;
  }

  virtual void set (const string & name, const string & value)
  {
	count = atoi (value.c_str ());
  }

  virtual void set (const string & name, double value)
  {
	count = (int) roundp (value);
  }

  virtual void set (const string & name, int value)
  {
	count = value;
  }

  int count;
};

class nitfLUT : public nitfItem
{
public:
  nitfLUT (nitfMapping * map)
  : nitfItem (map)
  {
	lut = 0;
  }

  virtual ~nitfLUT ()
  {
	if (lut) free (lut);
  }

  virtual void read (istream & stream)
  {
	char buffer[10];
	stream.read (buffer, 1);
	buffer[1] = 0;
	NLUTS = atoi (buffer);
	if (NLUTS)
	{
	  stream.read (buffer, 5);
	  buffer[5] = 0;
	  NELUT = atoi (buffer);

	  int count = NLUTS * NELUT;
	  lut = (unsigned char *) malloc (count);
	  stream.read ((char *) lut, count);
	}
  }

  virtual void write (ostream & stream)
  {
	stream << NLUTS;
	if (NLUTS)
	{
	  char buffer[10];
	  sprintf (buffer, "%05i", NELUT);
	  stream.write (buffer, 5);
	  int count = NLUTS * NELUT;
	  stream.write ((char *) lut, count);
	}
  }

  virtual bool contains (const string & name)
  {
	return name == "NLUTS"  ||  name == "NELUT"  ||  name == "LUTD";
  }

  virtual void get (const string & name, string & value)
  {
	char buffer[10];
	if (name == "NLUTS")
	{
	  sprintf (buffer, "%i", NLUTS);
	  value = buffer;
	}
	else if (name == "NELUT")
	{
	  sprintf (buffer, "%05i", NELUT);
	  value = buffer;
	}
	else if (name == "LUTD")
	{
	  value.assign ((char *) lut, NLUTS * NELUT);
	}
  }

  virtual void get (const string & name, double & value)
  {
	if (name == "NLUTS")
	{
	  value = NLUTS;
	}
	else if (name == "NELUT")
	{
	  value = NELUT;
	}
  }

  virtual void get (const string & name, int & value)
  {
	if (name == "NLUTS")
	{
	  value = NLUTS;
	}
	else if (name == "NELUT")
	{
	  value = NELUT;
	}
  }

  virtual void set (const string & name, const string & value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = atoi (value.c_str ());
	}
	else if (name == "NELUT")
	{
	  NELUT = atoi (value.c_str ());
	}
	throw "must resize lut";
  }

  virtual void set (const string & name, double value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = (int) roundp (value);
	}
	else if (name == "NELUT")
	{
	  NELUT = (int) roundp (value);
	}
	throw "must resize lut";
  }

  virtual void set (const string & name, int value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = value;
	}
	else if (name == "NELUT")
	{
	  NELUT = value;
	}
	throw "must resize lut";
  }

  int NLUTS;
  int NELUT;
  unsigned char * lut;  ///< an NELUT by NLUTS matrix
};

class nitfSecurityDowngrade : public nitfItemSet
{
public:
  nitfSecurityDowngrade (nitfMapping * map)
  : nitfItemSet (map)
  {
  }

  virtual void read (istream & stream)
  {
	data[0]->read (stream);
	string FSDWNG (data[0]->data, 6);
	if (FSDWNG == "999998")
	{
	  data[1]->read (stream);
	}
  }

  virtual void write (ostream & stream)
  {
	data[0]->write (stream);
	string FSDWNG (data[0]->data, 6);
	if (FSDWNG == "999998")
	{
	  data[1]->write (stream);
	}
  }
};

static nitfMapping mapIS[] =
{
  {"NUMI", 3,  "N", "0"},
  {"LISH", 6,  "N", "9"},
  {"LI",   10, "N", "9"},
  {0}
};

static nitfMapping mapGS[] =
{
  {"NUMS", 3, "N", "0"},
  {"LSSH", 4, "N", "9"},
  {"LS",   6, "N", "9"},
  {0}
};

static nitfMapping mapTS[] =
{
  {"NUMT", 3, "N", "0"},
  {"LTSH", 4, "N", "9"},
  {"LT",   5, "N", "9"},
  {0}
};

static nitfMapping mapDES[] =
{
  {"NUMDES", 3, "N", "0"},
  {"LDSH",   4, "N", "9"},
  {"LD",     9, "N", "9"},
  {0}
};

static nitfMapping mapRES[] =
{
  {"NUMRES", 3, "N", "0"},
  {"LRESH",  4, "N", "9"},
  {"LRE",    7, "N", "9"},
  {0}
};

static nitfMapping mapFileHeader0210[] =
{
  {"FHDR",    4, "A", "NITF"},
  {"FVER",    5, "A", "02.10"},
  {"CLEVEL",  2, "N", "9"},
  {"STYPE",   4, "A", "BF01"},
  {"OSTAID", 10, "A", " "},
  {"FDT",    14, "N", "9"},
  {"FTITLE", 80, "A", " "},
  {"FSCLAS",  1, "A", "U"},
  {"FSCLSY",  2, "A", " "},
  {"FSCODE", 11, "A", " "},
  {"FSCTLH",  2, "A", " "},
  {"FSREL",  20, "A", " "},
  {"FSDCTP",  2, "A", " "},
  {"FSDCDT",  8, "A", " "},
  {"FSDCXM",  4, "A", " "},
  {"FSDG",    1, "A", " "},
  {"FSDGDT",  8, "A", " "},
  {"FSCLTX", 43, "A", " "},
  {"FSCATP",  1, "A", " "},
  {"FSAUT",  40, "A", " "},
  {"FSCRSN",  1, "A", " "},
  {"FSSRDT",  8, "A", " "},
  {"FSCTLN", 15, "A", " "},
  {"FSCOP",   5, "N", "0"},
  {"FSCPYS",  5, "N", "0"},
  {"ENCRYP",  1, "N", "0"},
  {"FBKGC",   3, "A", "\0x00\0x00\0x00"},
  {"ONAME",  24, "A", " "},
  {"OPHONE", 18, "A", " "},
  {"FL",     12, "N", "9"},
  {"HL",      6, "N", "9"},
  {"IS",      0, "c"},
  {"GS",      0, "c"},
  {"NUMX",    3, "N", "0"},
  {"TS",      0, "c"},
  {"DES",     0, "c"},
  {"RES",     0, "c"},
  {0}
};

static nitfMapping mapDowngradeFile[] =
{
  {"FSDWNG",  6, "N", "9"},
  {"FSDEVT", 40, "A", " "},  // Must be filled explicitly.  Default value indicates error condition.
  {0}
};

static nitfMapping mapFileHeader0200[] =
{
  {"FHDR",    4, "A", "NITF"},
  {"FVER",    5, "A", "02.10"},
  {"CLEVEL",  2, "N", "9"},
  {"STYPE",   4, "A", "BF01"},
  {"OSTAID", 10, "A", " "},
  {"FDT",    14, "N", "9"},
  {"FTITLE", 80, "A", " "},
  {"FSCLAS",  1, "A", "U"},
  {"FSCODE", 40, "A", " "},
  {"FSCTLH", 40, "A", " "},
  {"FSREL",  40, "A", " "},
  {"FSAUT",  20, "A", " "},
  {"FSCTLN", 20, "A", " "},
  {"fdowngrade", 0, "c"},
  {"FSCOP",   5, "N", "0"},
  {"FSCPYS",  5, "N", "0"},
  {"ENCRYP",  1, "N", "0"},
  {"ONAME",  27, "A", " "},
  {"OPHONE", 18, "A", " "},
  {"FL",     12, "N", "9"},
  {"HL",      6, "N", "9"},
  {"IS",         0, "c"},
  // Add rest of header when needed.
  {0}
};

static nitfMapping mapGeoloc0210[] =
{
  {"ICORDS",   1, "A", " "},
  {"IGEOLO1", 15, "A", " "},
  {"IGEOLO2", 15, "A", " "},
  {"IGEOLO3", 15, "A", " "},
  {"IGEOLO4", 15, "A", " "},
  {0}
};

static nitfMapping mapIcom[] =
{
  {"NICOM",    1, "N", "0"},
  {"ICOM",    80, "A", " "},
  {0}
};

static nitfMapping mapCompression[] =
{
  {"IC",       2, "A", "NC"},
  {"COMRAT",   4, "A", " "},
  {0}
};

static nitfMapping mapBandCount[] =
{
  {"NBANDS",   1, "N", "0"},
  {"XBANDS",   5, "N", "0"},
  {0}
};

static nitfMapping mapBand0210[] =
{
  {"bandcount", 0, "c"},
  {"IREPBAND",  2, "A", " "},
  {"ISUBCAT",   6, "A", " "},
  {"IFC",       1, "A", "N"},
  {"IMFLT",     3, "A", " "},
  {"lut",       0, "c"},
  {0}
};

static nitfMapping mapImageHeader0210[] =
{
  {"IM",      2, "A", "IM"},
  {"IID1",   10, "A", " "},
  {"IDATIM", 14, "N", "9"},
  {"TGTID",  17, "A", " "},
  {"IID2",   80, "A", " "},
  {"ISCLAS",  1, "A", " "},
  {"ISCLSY",  2, "A", " "},
  {"ISCODE", 11, "A", " "},
  {"ISCTLH",  2, "A", " "},
  {"ISREL",  20, "A", " "},
  {"ISDCTP",  2, "A", " "},
  {"ISDCDT",  8, "A", " "},
  {"ISDCXM",  4, "A", " "},
  {"ISDG",    1, "A", " "},
  {"ISDGDT",  8, "A", " "},
  {"ISCLTX", 43, "A", " "},
  {"ISCATP",  1, "A", " "},
  {"ISCAUT", 40, "A", " "},
  {"ISCRSN",  1, "A", " "},
  {"ISSRDT",  8, "A", " "},
  {"ISCTLN", 15, "A", " "},
  {"ENCRYP",  1, "N", "0"},
  {"ISORCE", 42, "A", " "},
  {"NROWS",   8, "N", "9"},
  {"NCOLS",   8, "N", "9"},
  {"PVTYPE",  3, "A", "INT"},
  {"IREP",    8, "A", "MONO    "},
  {"ICAT",    8, "A", "VIS     "},
  {"ABPP",    2, "N", "9"},
  {"PJUST",   1, "A", "R"},
  {"geoloc0210",  0, "c"},
  {"icom",        0, "c"},
  {"compression", 0, "c"},
  {"band0210",    0, "c"},
  {"ISYNC",   1, "N", "0"},
  {"IMODE",   1, "A", "P"},
  {"NBPR",    4, "N", "1"},
  {"NBPC",    4, "N", "1"},
  {"NPPBH",   4, "N", "0"},
  {"NPPBV",   4, "N", "0"},
  {"NBPP",    2, "N", "9"},
  {"IDLVL",   3, "N", "001"},
  {"IALVL",   3, "N", "0"},
  {"ILOC",   10, "N", "0"},
  {"IMAG",    4, "F", "1.0 "},
  // user and extension headers go here
  {0}
};

static nitfMapping mapDowngradeImage[] =
{
  {"ISDWNG",  6, "N", "9"},
  {"ISDEVT", 40, "A", " "},
  {0}
};

static nitfMapping mapGeoloc0200[] =
{
  {"ICORDS",   1, "A", "N"},
  {"IGEOLO1", 15, "A", " "},
  {"IGEOLO2", 15, "A", " "},
  {"IGEOLO3", 15, "A", " "},
  {"IGEOLO4", 15, "A", " "},
  {0}
};

static nitfMapping mapBand0200[] =
{
  {"NBANDS",    1, "N", "1"},
  {"IREPBAND",  2, "A", " "},
  {"ISUBCAT",   6, "A", " "},
  {"IFC",       1, "A", "N"},
  {"IMFLT",     3, "A", " "},
  {"lut",       0, "c"},
  {0}
};

static nitfMapping mapImageHeader0200[] =
{
  {"IM",      2, "A", "IM"},
  {"IID1",   10, "A", " "},
  {"IDATIM", 14, "N", "9"},
  {"TGTID",  17, "A", " "},
  {"ITITLE", 80, "A", " "},
  {"ISCLAS",  1, "A", " "},
  {"ISCODE", 40, "A", " "},
  {"ISCTLH", 40, "A", " "},
  {"ISREL",  40, "A", " "},
  {"ISCAUT", 20, "A", " "},
  {"ISCTLN", 20, "A", " "},
  {"idowngrade",  0, "c"},
  {"ENCRYP",  1, "N", "0"},
  {"ISORCE", 42, "A", " "},
  {"NROWS",   8, "N", "9"},
  {"NCOLS",   8, "N", "9"},
  {"PVTYPE",  3, "A", "INT"},
  {"IREP",    8, "A", "MONO    "},
  {"ICAT",    8, "A", "VIS     "},
  {"ABPP",    2, "N", "9"},
  {"PJUST",   1, "A", "R"},
  {"geoloc0200",  0, "c"},
  {"icom",        0, "c"},
  {"compression", 0, "c"},
  {"band0200",    0, "c"},
  {"ISYNC",   1, "N", "0"},
  {"IMODE",   1, "A", "P"},
  {"NBPR",    4, "N", "1"},
  {"NBPC",    4, "N", "1"},
  {"NPPBH",   4, "N", "0"},
  {"NPPBV",   4, "N", "0"},
  {"NBPP",    2, "N", "9"},
  {"IDLVL",   3, "N", "001"},
  {"IALVL",   3, "N", "0"},
  {"ILOC",   10, "N", "0"},
  {"IMAG",    4, "F", "1.0 "},
  // Add rest of header when needed.
  {0}
};

enum nitfItemID
{
  idItem,
  idRepeat,
  idItemSet,
  idGeoloc,
  idCompression,
  idExtendableCount,
  idLUT,
  idDowngrade
};

struct nitfTypeMapping
{
  const char *  name;
  nitfItemID    id;
  nitfMapping * map;  ///< alternate map
};

nitfTypeMapping typeMap[] =
{
  {"IS",          idRepeat,          mapIS},
  {"GS",          idRepeat,          mapGS},
  {"TS",          idRepeat,          mapTS},
  {"DES",         idRepeat,          mapDES},
  {"RES",         idRepeat,          mapRES},
  {"geoloc0210",  idGeoloc,          mapGeoloc0210},
  {"geoloc0200",  idGeoloc,          mapGeoloc0200},
  {"icom",        idRepeat,          mapIcom},
  {"compression", idCompression,     mapCompression},
  {"band0210",    idRepeat,          mapBand0210},
  {"band0200",    idRepeat,          mapBand0200},
  {"bandcount",   idExtendableCount, mapBandCount},
  {"lut",         idLUT,             0},
  {"fdowngrade",  idDowngrade,       mapDowngradeFile},
  {"idowngrade",  idDowngrade,       mapDowngradeImage},
  {0}
};

nitfItem *
nitfItem::factory (nitfMapping * m)
{
  if (m->type[0] == 'A'  ||  m->type[0] == 'N'  ||  m->type[0] == 'F')
  {
	return new nitfItem (m);
  }

  if (m->type[0] != 'c') throw "Expected class type";  // error in map tables

  nitfTypeMapping * t = typeMap;
  while (t->name)
  {
	if (strcmp (t->name, m->name) == 0)
	{
	  nitfMapping * mm = t->map ? t->map : m;
	  switch (t->id)
	  {
		case idItem:            return new nitfItem              (mm);
		case idRepeat:          return new nitfRepeat            (mm);
		case idItemSet:         return new nitfItemSet           (mm);
		case idGeoloc:          return new nitfGeoloc            (mm);
		case idCompression:     return new nitfCompression       (mm);
		case idExtendableCount: return new nitfExtendableCount   (mm);
		case idLUT:             return new nitfLUT               (mm);
		case idDowngrade:       return new nitfSecurityDowngrade (mm);
	  }
	}
	t++;
  }

  throw "Can't find typeMap entry!";  // error in map tables
}

class nitfImageSection
{
public:
  nitfImageSection (const string & version)
  {
	BMRBND = 0;

	header = 0;
	if (version == "NITF02.10"  ||  version == "NSIF01.00")
	{
	  header = new nitfItemSet (mapImageHeader0210);
	}
	else if (version == "NITF02.00")
	{
	  header = new nitfItemSet (mapImageHeader0200);
	}
	else
	{
	  throw "Unknown file version";
	}
  }

  ~nitfImageSection ()
  {
	if (header) delete header;
	if (BMRBND) free (BMRBND);
  }

  bool contains (const string & name)
  {
	return header->contains (name);
  }

  void get (const string & name, string & value)
  {
	header->get (name, value);
  }

  void get (const string & name, int & value)
  {
	header->get (name, value);
  }

  void get (const string & name, double & value)
  {
	header->get (name, value);
  }

  void set (const string & name, const string & value)
  {
	header->set (name, value);
  }

  void set (const string & name, int value)
  {
	header->set (name, value);
  }

  void set (const string & name, double value)
  {
	header->set (name, value);
  }

  /**
	 \param band If IMODE is anything other than S, then this parameter must
	 always zero.
   **/
  void read (istream & stream, char * block, uint32_t blockSize, int bx, int by, int band)
  {
	int blockIndex = band * NBPR * NBPC + by * NBPR + bx;

	uint32_t blockAddress;
	if (BMRBND)
	{
	  blockAddress = BMRBND[blockIndex];
	  if (blockAddress == 0xFFFFFFFF)
	  {
		memset (block, 0, blockSize);
		return;
	  }
	}
	else
	{
	  blockAddress = blockIndex * blockSize;
	}

	stream.seekg (offset + blockAddress);
	stream.read (block, blockSize);
  }

  void read (istream & stream, Image & image, int x, int y, int width, int height)
  {
	if (IC[0] == 'N')  // no compression
	{
	  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
	  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
	  image.format = format;

	  if (! width ) width  = NCOLS - x;
	  if (! height) height = NROWS - y;
	  if (x < 0)
	  {
		width += x;
		x = 0;
	  }
	  if (y < 0)
	  {
		height += y;
		y = 0;
	  }
	  width  = min (width,  (int) NCOLS - x);
	  height = min (height, (int) NROWS - y);
	  width  = max (width,  0);
	  height = max (height, 0);
	  image.resize (width, height);
	  if (! width  ||  ! height) return;

	  char * imageMemory = (char *) buffer->base ();

	  // If the requested image is anything other than exactly the union of a
	  // set of blocks in the file, then must use temporary storage to read in
	  // blocks.
	  Image block (*image.format);
	  if (x % NPPBH  ||  y % NPPBV  ||  width != NPPBH  ||  height % NPPBV) block.resize (NPPBH, NPPBV);
	  int blockSize = (int) roundp (NPPBH * NPPBV * format->depth);
	  char * blockBuffer = (char *) ((PixelBufferPacked *) block.buffer)->base ();

	  for (int oy = 0; oy < height;)  // output y: position in output image
	  {
		int ry = oy + y;  // working y in raster
		int by = ry / NPPBV;  // vertical block number
		int iy = ry % NPPBV;  // input y: offset from top of block
		int h = min ((int) NPPBV - iy, height - oy);  // height of usable portion of block

		for (int ox = 0; ox < width;)
		{
		  int rx = ox + x;
		  int bx = rx / NPPBH;
		  int ix = rx % NPPBH;
		  int w = min ((int) NPPBH - ix, width - ox);

		  if (w == width  &&  w == NPPBH  &&  h == NPPBV)
		  {
			read (stream, imageMemory + oy * buffer->stride, blockSize, bx, by, 0);
		  }
		  else
		  {
			read (stream, blockBuffer, blockSize, bx, by, 0);
			image.bitblt (block, ox, oy, ix, iy, w, h);
		  }

		  ox += w;
		}

		oy += h;
	  }

#     if BYTE_ORDER == LITTLE_ENDIAN
	  // Must do endian conversion for gray formats
	  if (*format == GrayShort)
	  {
		bswap ((unsigned short *) imageMemory, width * height);
	  }
	  else if (*format == GrayFloat)
	  {
		bswap ((uint32_t *) imageMemory, width * height);
	  }
	  else if (*format == GrayDouble)
	  {
		bswap ((uint64_t *) imageMemory, width * height);
	  }
#     endif
	}
  }

  void read (istream & stream)
  {
	offset = stream.tellg ();
	offset += LISH;

	header->read (stream);

	header->get ("IC",     IC);
	header->get ("IMODE",  IMODE);
	header->get ("NBANDS", NBANDS);
	header->get ("NBPR",   NBPR);
	header->get ("NBPC",   NBPC);
	header->get ("NROWS",  NROWS);
	header->get ("NCOLS",  NCOLS);
	header->get ("NPPBH",  NPPBH);
	header->get ("NPPBV",  NPPBV);


	// Determine PixelFormat
	string IREP;
	get ("IREP", IREP);
	string PVTYPE;
	get ("PVTYPE", PVTYPE);
	int NBPP;
	get ("NBPP", NBPP);
	int ABPP;
	get ("ABPP", ABPP);
	string PJUST;
	get ("PJUST", PJUST);

	if (IREP == "MONO    ")
	{
	  if (PVTYPE == "INT"  ||  PVTYPE == "SI ")  // should distinguish signed from unsigned, but currently don't have signed PixelFormats
	  {
		switch (NBPP)
		{
		  case 8:
			format = &GrayChar;
			break;
		  case 16:
			if (ABPP == 16)
			{
			  format = &GrayShort;
			}
			else
			{
			  unsigned short grayMask = 0xFFFF;
			  if (PJUST == "L")
			  {
				grayMask <<= (16 - ABPP);
			  }
			  else
			  {
				grayMask >>= (16 - ABPP);
			  }
			  format = new PixelFormatGrayShort (grayMask);
			}
			break;
		}
	  }
	  else if (PVTYPE == "R  ")
	  {
		switch (NBPP)
		{
		  case 32:
			format = &GrayFloat;
			break;
		  case 64:
			format = &GrayDouble;
			break;
		}
	  }
	}
	else if (IREP == "RGB     ")
	{
	  // This is too simplistic.  Should take into account the band layout as well.

	  if (PVTYPE == "INT"  ||  PVTYPE == "SI ")  // should distinguish signed from unsigned, but currently don't have signed PixelFormats
	  {
		if (NBPP == 8) format = &RGBChar;
	  }
	}

	if (format == 0) throw "Can't match format";
	//cerr << "format = " << typeid (*format).name () << endl;


	// Parse block mask if it exists
	if (IC[0] == 'M'  ||  IC[1] == 'M')
	{
	  stream.seekg (offset);
	  IMDATOFF = 0;
	  stream.read ((char *) &IMDATOFF, 4);
	  if (! IMDATOFF) throw "failed to read IMDATOFF";
#     if BYTE_ORDER == LITTLE_ENDIAN
	  bswap (&IMDATOFF);
#     endif
	  offset += IMDATOFF;
	  cerr << "IMDATOFF = " << IMDATOFF << endl;

	  BMRLNTH = 0;
	  TMRLNTH = 0;
	  TPXCDLNTH = 0;
#     if BYTE_ORDER == LITTLE_ENDIAN
	  stream.read (((char *) &BMRLNTH) + 1, 1);
	  stream.read (((char *) &BMRLNTH) + 0, 1);
	  stream.read (((char *) &TMRLNTH) + 1, 1);
	  stream.read (((char *) &TMRLNTH) + 0, 1);
	  stream.read (((char *) &TPXCDLNTH) + 1, 1);
	  stream.read (((char *) &TPXCDLNTH) + 0, 1);
#     else
	  stream.read ((char *) &BMRLNTH, 2);
	  stream.read ((char *) &TMRLNTH, 2);
	  stream.read ((char *) &TPXCDLNTH, 2);
#     endif
	  cerr << "BMRLNTH = " << BMRLNTH << endl;
	  cerr << "TMRLNTH = " << TMRLNTH << endl;
	  cerr << "TPXCDLNTH = " << TPXCDLNTH << endl;

	  stream.seekg ((istream::off_type) ceil (TPXCDLNTH / 8.0), ios_base::cur);  // skip the TPXCD if it exists

	  if (BMRLNTH)
	  {
		int size = NBPR * NBPC;
		if (IMODE == "S") size *= NBANDS;
		BMRBND = (uint32_t *) malloc (size * sizeof (uint32_t));
		stream.read ((char *) BMRBND, size * sizeof (uint32_t));

#       if BYTE_ORDER == LITTLE_ENDIAN
		cerr << "BMRBND before = " << BMRBND[0] << endl;
		bswap (BMRBND, size);
		cerr << "BMRBND after = " << BMRBND[0] << endl;
#       endif
	  }
	}
  }

  int LISH;
  int LI;
  uint32_t offset;

  string IC;
  string IMODE;
  int NBPR;
  int NBPC;
  int NBANDS;
  int NROWS;
  int NCOLS;
  int NPPBH;
  int NPPBV;

  uint32_t IMDATOFF;
  uint16_t BMRLNTH;
  uint16_t TMRLNTH;
  uint16_t TPXCDLNTH;
  uint32_t * BMRBND;

  PointerPoly<PixelFormat> format;

  nitfItemSet * header;
};


// class ImageFileDelegateNITF ------------------------------------------------

class ImageFileDelegateNITF : public ImageFileDelegate
{
public:
  ImageFileDelegateNITF (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;

	header = 0;
	imageIndex = -1;
	if (in)
	{
	  // Probe for file version, but rewind so file header can parse it as well
	  char buffer[10];
	  int startOffset = in->tellg ();
	  in->read (buffer, 9);
	  in->seekg (startOffset);
	  version.assign (buffer, 9);

	  createHeader ();
	  header->read (*in);

	  int offset;
	  get ("HL", offset);

	  int NUMI;
	  get ("NUMI", NUMI);
	  for (int i = 0; i < NUMI; i++)
	  {
		nitfImageSection * h = new nitfImageSection (version);
		images.push_back (h);

		sprintf (buffer, "LISH%03i", i+1);
		get (buffer, h->LISH);
		sprintf (buffer, "LI%03i", i+1);
		get (buffer, h->LI);

		in->seekg (offset);
		h->read (*in);

		offset += h->LISH + h->LI;
	  }
	  if (images.size () > 0) imageIndex = 0;
	}
	else
	{
	  version = "NITF02.10";
	}
  }
  ~ImageFileDelegateNITF ();
  void createHeader ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name,       string & value);
  virtual void set (const string & name, const string & value);
  using Metadata::get;
  using Metadata::set;

  istream * in;
  ostream * out;
  bool ownStream;

  string version;
  nitfItemSet * header;
  vector<nitfImageSection *> images;
  int imageIndex;
};

ImageFileDelegateNITF::~ImageFileDelegateNITF ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }

  for (int i = 0; i < images.size (); i++)
  {
	delete images[i];
  }
  if (header) delete header;
}

void
ImageFileDelegateNITF::createHeader ()
{
  if (header) return;

  if (version == "NITF02.10"  ||  version == "NSIF01.00")
  {
	header = new nitfItemSet (mapFileHeader0210);
  }
  else if (version == "NITF02.00")
  {
	header = new nitfItemSet (mapFileHeader0200);
  }
  else
  {
	throw "Unrecognized file version";
  }
}

void
ImageFileDelegateNITF::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateNITF not open for reading";
  if (imageIndex < 0) throw "No image available";
  images[imageIndex]->read (*in, image, x, y, width, height);
}

void
ImageFileDelegateNITF::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateNITF not open for writing";
  throw "NITF write not yet implemented";
  // Must resize repeat sections in header to match number held
  // nitfRepeat needs work to allow direct change in count
}

static inline void
remapSpecialNames (const string & name, string & result)
{
  if      (name == "width")       result = "NCOLS";
  else if (name == "height")      result = "NROWS";
  else if (name == "blockWidth")  result = "NPPBH";
  else if (name == "blockHeight") result = "NPPBV";
  else                            result = name;
}

static inline void
geoToDecimal (const string & geo, double & x, double & y)
{
  y =   atof (geo.substr (0, 2).c_str ())
	  + atof (geo.substr (2, 2).c_str ()) / 60.0
 	  + atof (geo.substr (4, 2).c_str ()) / 3600.0;
  if (geo[6] == 'S') y *= -1.0;

  x =   atof (geo.substr (7, 3).c_str ())
	  + atof (geo.substr (10, 2).c_str ()) / 60.0
 	  + atof (geo.substr (12, 2).c_str ()) / 3600.0;
  if (geo[14] == 'W') x *= -1.0;
}

void
ImageFileDelegateNITF::get (const string & name, string & value)
{
  if (! header) return;

  if (name == "GeoTransformationMatrix")
  {
#   ifdef HAVE_LAPACK
	if (imageIndex < 0) return;

	string ICORDS;
	images[imageIndex]->get ("ICORDS", ICORDS);
	if (version == "NITF02.10"  ||  version == "NSIF01.00")
	{
	  if (ICORDS == " ") return;
	}
	else if (version == "NITF02.00")
	{
	  if (ICORDS == "N") return;
	  if (ICORDS == "C") ICORDS = "G";
	  // Not sure what NITF 2.0 "G" maps to in NITF 2.1
	}
	else return;  // meesa give up

	string IGEOLO1;
	string IGEOLO2;
	string IGEOLO3;
	string IGEOLO4;
	images[imageIndex]->get ("IGEOLO1", IGEOLO1);
	images[imageIndex]->get ("IGEOLO2", IGEOLO2);
	images[imageIndex]->get ("IGEOLO3", IGEOLO3);
	images[imageIndex]->get ("IGEOLO4", IGEOLO4);

	Matrix<double> XY (2, 4);
	if (ICORDS == "G")
	{
	  geoToDecimal (IGEOLO1, XY(0,0), XY(1,0));
	  geoToDecimal (IGEOLO2, XY(0,1), XY(1,1));
	  geoToDecimal (IGEOLO3, XY(0,2), XY(1,2));
	  geoToDecimal (IGEOLO4, XY(0,3), XY(1,3));
	}
	else if (ICORDS == "D")
	{
	}
	else return;

	int NROWS;
	int NCOLS;
	images[imageIndex]->get ("NROWS", NROWS);
	images[imageIndex]->get ("NCOLS", NCOLS);
	double lastX = NCOLS - 1;
	double lastY = NROWS - 1;

	// Solve for xform

	// Find center of each point cloud
	Vector<double> centerIJ (2);
	centerIJ[0] = lastX / 2.0;
	centerIJ[1] = lastY / 2.0;

	Vector<double> centerXY (2);
	centerXY.clear ();
	for (int c = 0; c < 4; c++)
	{
	  centerXY += XY.column (c);
	}
	centerXY /= 4;

	// Build least squares problem for 2x2 xform
	Matrix<double> IJ (2, 4);
	IJ(0,0) = 0     - centerIJ[0];
	IJ(1,0) = 0     - centerIJ[1];
	IJ(0,1) = lastX - centerIJ[0];
	IJ(1,1) = 0     - centerIJ[1];
	IJ(0,2) = lastX - centerIJ[0];
	IJ(1,2) = lastY - centerIJ[1];
	IJ(0,3) = 0     - centerIJ[0];
	IJ(1,3) = lastY - centerIJ[1];

	for (int c = 0; c < 4; c++)
	{
	  XY.column (c) -= centerXY;
	}
	Matrix<double> X;
	gelss (~IJ, X, ~XY, (double *) 0, true, true);

	// Assemble result
	Matrix<double> temp (4, 4);
	temp.clear ();
	temp.region (0, 0) = ~X;
	temp.region (0, 3) = centerXY - temp.region (0, 0, 1, 1) * centerIJ;
	temp(3,3) = 1;
	temp.toString (value);
#   endif

	return;
  }

  string n;
  remapSpecialNames (name, n);

  if (name == "imageIndex")
  {
	char buffer[10];
	sprintf (buffer, "%i", imageIndex);
	value = buffer;
  }
  else if (header->contains (n))
  {
	header->get (n, value);
  }
  else if (imageIndex >= 0  &&  images[imageIndex]->contains (n))
  {
	images[imageIndex]->get (n, value);
  }
}

void
ImageFileDelegateNITF::set (const string & name, const string & value)
{
  createHeader ();

  string n;
  remapSpecialNames (name, n);

  if (name == "imageIndex")
  {
	imageIndex = atoi (value.c_str ());
	imageIndex = min (imageIndex, 998);

	while (imageIndex >= images.size ())
	{
	  nitfImageSection * h = new nitfImageSection (version);
	  images.push_back (h);
	}
  }
  else if (header->contains (n))
  {
	header->set (n, value);
  }
  else if (imageIndex >= 0  &&  images[imageIndex]->contains (n))
  {
	images[imageIndex]->set (n, value);
  }
}


// class ImageFileFormatNITF --------------------------------------------------

void
ImageFileFormatNITF::use ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (ImageFileFormatNITF)) return;
  }
  formats.push_back (new ImageFileFormatNITF);
}

ImageFileDelegate *
ImageFileFormatNITF::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatNITF::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (0, &stream, ownStream);
}

float
ImageFileFormatNITF::isIn (std::istream & stream) const
{
  string magic = "         ";  // 9 spaces
  getMagic (stream, magic);
  if (magic.substr (0, 4) == "NITF")
  {
	if (magic.substr (4) == "02.10") return 1;
	if (magic.substr (4) == "02.00") return 1;
  }
  if (magic.substr (0, 4) == "NSIF")
  {
	if (magic.substr (4) == "01.00") return 1;
  }
  return 0;
}

float
ImageFileFormatNITF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "nitf") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "nsif") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "ntf") == 0)
  {
	return 0.9;
  }
  if (strcasecmp (formatName.c_str (), "nsf") == 0)
  {
	return 0.9;
  }
  return 0;
}
