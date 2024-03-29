/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"
#include "fl/string.h"

#include <typeinfo>


using namespace std;
using namespace fl;


// class ImageFileDelegateRRIF -------------------------------------------------

class ImageFileDelegateRRIF : public ImageFileDelegate
{
public:
  ImageFileDelegateRRIF (istream * in, ostream * out, bool ownStream = false);
  ~ImageFileDelegateRRIF ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name,       string & value);
  virtual void set (const string & name, const string & value);

  istream * in;
  ostream * out;
  bool ownStream;

  uint16_t height;
  uint16_t width;
};

ImageFileDelegateRRIF::ImageFileDelegateRRIF (istream * in, ostream * out, bool ownStream)
{
  this->in = in;
  this->out = out;
  this->ownStream = ownStream;
  height = 0;
  width  = 0;

  // Scan header if we are opened for read.
  if (! in) return;
  in->ignore (4);  // magic string
  in->read ((char *) & height, sizeof (height));
  in->read ((char *) & width,  sizeof (width));
}

ImageFileDelegateRRIF::~ImageFileDelegateRRIF ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegateRRIF::read (Image & image, int x, int y, int ignorewidth, int ignoreheight)
{
  if (! in) throw "ImageFileDelegateRRIF not open for reading";

  // Read data
  if (! in->good ()) throw "Unable to finish reading image: stream bad.";
  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.format = &GrayChar;
  image.resize (width, height);
  in->read ((char *) buffer->base (), width * height);
}

void
ImageFileDelegateRRIF::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateRRIF not open for writing";

  Image work = image * GrayChar;
  PixelBufferPacked * buffer = (PixelBufferPacked *) work.buffer;
  if (! buffer) throw "RRIF can only handle packed buffers for now";

  // Header
  height = work.height;
  width  = work.width;
  (*out) << "RRIF";
  out->write ((char *) & height, sizeof (height));
  out->write ((char *) & width,  sizeof (width));

  // Body
  if (buffer->stride == width)
  {
	out->write ((char *) buffer->base (), width * height);
  }
  else
  {
	char * row = (char *) buffer->base ();
	for (uint32_t y = 0; y < height; y++)
	{
	  out->write (row, width);
	  row += buffer->stride;
	}
  }
}

void
ImageFileDelegateRRIF::get (const string & name, string & value)
{
  char buffer[32];
  if (name == "width")
  {
	sprintf (buffer, "%i", width);
	value = buffer;
  }
  else if (name == "height")
  {
	sprintf (buffer, "%i", height);
	value = buffer;
  }
}

void
ImageFileDelegateRRIF::set (const string & name, const string & value)
{
}


// class ImageFileFormatRRIF ---------------------------------------------------

void
ImageFileFormatRRIF::use ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (ImageFileFormatRRIF)) return;
  }
  formats.push_back (new ImageFileFormatRRIF);
}

ImageFileDelegate *
ImageFileFormatRRIF::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateRRIF (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatRRIF::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateRRIF (0, &stream, ownStream);
}

float
ImageFileFormatRRIF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);
  if (magic == "RRIF")
  {
	return 1.0;
  }
  return 0;
}

float
ImageFileFormatRRIF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "rrif") == 0)
  {
	return 1.0;
  }
  if (strcasecmp (formatName.c_str (), "raw") == 0)
  {
	return 0.9;
  }
  return 0;
}
