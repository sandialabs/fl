/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5  thru 1.9  Copyright 2005 Sandia Corporation.
Revisions 1.11 thru 1.32 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.32  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.31  2006/11/12 15:22:20  Fred
Lazy allocation of block buffer.

Remove underscore from XTIFFInitialize().

Revision 1.30  2006/03/16 04:26:26  Fred
Use GTIFValueName() to retrieve string describing key value.

Revision 1.29  2006/03/16 03:36:09  Fred
Add get(int) and set(int).  Rearrange order of accessors for prettiness sake.

Add dynamically created PixelFormat to handle different grayscale ranges. 
Unfortunately, this code doesn't work because can't find any tags that reliably
convey a shortened range.  May remove this code if nothing works out.

Ensure that when writing strips, the strip size will be an integral factor of
the given image size.  Otherwise, some pixels will be skipped in subsequent
writes.

Look up name of compression type when getting/setting as string.

Correct TIFFFieldInfo for \{S}\{Min|Max}SampleValue.  The field info follows
the standard, but libtiff returns just a single value in each case.

Revision 1.28  2006/03/13 13:35:27  Fred
Guard against direct reading of partial blocks.

Revision 1.27  2006/03/10 02:58:35  Fred
Fill partial blocks with black, rather than copying pixel values from ends of
rows.

Add x and y offset when automatically determining image size.

Fix bug in detecting partial blocks.

Revision 1.26  2006/03/06 14:22:04  Fred
Full support for ImageFile::write() interface, including tiled and stripped
organizations.  The method for adding fill pixels to blocks should change to
just write black to everything.  It will do little harm to the compression
efficiency, and will make the image more viewable if one happens to see the
fill.

Revision 1.25  2006/03/06 00:00:57  Fred
Full support for ImageFile::read() interface.  Selects arbitrary rectangular
region from either stripped or tiled image.  Still need to work on write().

Revision 1.24  2006/03/05 22:07:13  Fred
Fix issues with setting geokeys:  Properly detect file open for writing so that
we call GTIFWriteKeys() on close.  Add mapping between geokeys and data types,
since libgeotiff doesn't provide this information when keys aren't already in
the directory (a catch 22).

Revision 1.23  2006/03/05 14:28:41  Fred
Tiled and stripped reading and writing of whole images. This is an intermediate
step towards implementing the partial image interface.

Revision 1.22  2006/03/05 03:10:17  Fred
Add set() metadata methods.

Use libgeotiff methods to map names to keys.

Change strstream to ostrstream.

Use platform independent type names for library access.

Revision 1.21  2006/03/04 16:05:26  Fred
Build geokey table from geokeys.inc rather than hard-coded list.

Revision 1.20  2006/03/04 14:03:07  Fred
Use builtin tag table for libtiff to look up metadata.

Finish developing the handler for GeoTransformationMatrix.  It now has several
fallbacks, to the extent of computing an affine tranformation from a set of
GeoTiePoints.  Should consider computing a projective homography instead.

Revision 1.19  2006/03/03 17:48:15  Fred
Use tables for looking up metadata.  Includes TIFF standard tags and GeoTiff
metatags.  However, there appears to be a way to reflect on the tag information
in the tiff librarry, so will try to convert to that method.

Incomplete support for getting GeoTIFF model transformation matrix.

Revision 1.18  2006/03/02 03:28:39  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Initial work on using tables to map metadata.

Revision 1.17  2006/03/01 04:01:19  Fred
Basic support for GeoTiff.

Revision 1.16  2006/02/27 03:31:19  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.

Revision 1.15  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.14  2006/02/26 14:04:29  Fred
Use steam interface to libtiff.

Revision 1.13  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.12  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.11  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.10  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.9  2005/10/09 05:20:08  Fred
Add revision history and Sandia copyright notice.

Revision 1.8  2005/05/29 15:54:52  Fred
Add includes to overcome MSVC compilation issues.

Revision 1.7  2005/05/01 21:04:54  Fred
Adjust to more rigorous naming of PixelFormats.

Revision 1.6  2005/04/26 03:46:21  Fred
Add GrayShort to formats read and written.

Revision 1.5  2005/04/23 20:54:12  Fred
Finish code to write TIFFs.  Fix TIFF reading code to properly detect pixel
format.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/03/22 20:30:24  rothgang
Yet another attempt to extract info about alpha channel from TIFF file.  The
mystery continues...

Revision 1.2  2003/07/30 14:04:17  rothgang
Add another hack for TIFFs made by PaintShop.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/string.h"
#include "fl/math.h"
#include "fl/lapack.h"

#include <tiffio.h>
#include <tiffio.hxx>
#ifdef HAVE_GEOTIFF
#  include <xtiffio.h>
#  include <geotiff.h>
#endif

#include <strstream>

#include <fcntl.h>
// On Cygwin, O_WRONLY == 1, but fcntl() returns 2 for that state, so can't
// use O_WRONLY directly in comparisons.  Not sure if this is the case for
// all posix systems, or just peculiar to Cygwin.
#ifdef __CYGWIN__
#  undef O_WRONLY
#  define O_WRONLY _FWRITE
#endif


using namespace std;
using namespace fl;


// class ImageFileDelegateTIFF ------------------------------------------------

class ImageFileDelegateTIFF : public ImageFileDelegate
{
public:
  ImageFileDelegateTIFF (TIFF * tif, ios * stream)
  {
	this->tif = tif;
	this->stream = stream;

#   ifdef HAVE_GEOTIFF
	gtif = GTIFNew (tif);
#   endif

	format = 0;
	ownFormat = false;
  }
  virtual ~ImageFileDelegateTIFF ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name, string & value);
  virtual void get (const string & name, int & value);
  virtual void get (const string & name, double & value);
  virtual void get (const string & name, Matrix<double> & value);

  virtual void set (const string & name, const string & value);
  virtual void set (const string & name, int value);
  virtual void set (const string & name, double value);
  virtual void set (const string & name, const Matrix<double> & value);

  TIFF * tif;
  ios * stream;
# ifdef HAVE_GEOTIFF
  GTIF * gtif;
# endif

  const PixelFormat * format;
  bool ownFormat;
};

ImageFileDelegateTIFF::~ImageFileDelegateTIFF ()
{
  if (ownFormat  &&  format) delete format;

  if (tif)
  {
#   ifdef HAVE_GEOTIFF
	if (gtif)
	{
	  if (TIFFGetMode (tif) & O_WRONLY)
	  {
		GTIFWriteKeys (gtif);
	  }
	  GTIFFree (gtif);
	}
#   endif
	TIFFClose (tif);
  }
  if (stream) delete stream;
}

void
ImageFileDelegateTIFF::read (Image & image, int x, int y, int width, int height)
{
  if (! tif) throw "ImageFileDelegateTIFF not open";

  bool ok = true;

  uint32 imageWidth;
  uint32 imageHeight;
  ok &= TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  ok &= TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageHeight);

  uint16 samplesPerPixel;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
  uint16 bitsPerSample;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  uint16 sampleFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
  uint16 photometric;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &photometric);
  uint16 planarConfig;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PLANARCONFIG, &planarConfig);
  uint16 extra;
  uint16 * extraFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extra, &extraFormat);

  if (! ok)
  {
	throw "Unable to get needed tag values.";
  }
  if (photometric > 2)
  {
	throw "Can't handle color palettes, transparency masks, etc.";
	// But we should handle the YCbCr and L*a*b* color spaces.
  }
  if (planarConfig != PLANARCONFIG_CONTIG  &&  samplesPerPixel > 1)
  {
	throw "Can't yet handle planar formats";
  }
  if (extra > 1)
  {
	throw "No PixelFormats currently support more than one channel beyond three colors.";
  }

  format = 0;
  switch (bitsPerSample)
  {
	case 8:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayChar;
		  break;
		case 3:
		  format = &RGBChar;
		  break;
		case 4:
		  format = &RGBAChar;
	  }
	  break;
	case 16:
	  switch (samplesPerPixel)
	  {
		case 1:
		{
		  int maxValue = 0;
		  get ("SMaxSampleValue", maxValue);
		  if (maxValue == 0xFFFF  ||  maxValue == 0)
		  {
			format = &GrayShort;
		  }
		  else
		  {
			unsigned short mask = 0;
			while (mask < maxValue  &&  mask < 0xFFFF) mask = (mask << 1) | 0x1;
			format = new PixelFormatGrayShort (mask);
			ownFormat = true;
		  }
		  break;
		}
		case 3:
		  format = &RGBShort;
		  break;
		case 4:
		  format = &RGBAShort;
	  }
	  break;
	case 32:
	  if (sampleFormat == SAMPLEFORMAT_IEEEFP)
	  {
		switch (samplesPerPixel)
		{
		  case 1:
			format = &GrayFloat;
			break;
		  case 4:
			format = &RGBAFloat;
		}
	  }
  }
  if (! format)
  {
	throw "No PixelFormat available that matches file contents";
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.format = format;

  if (! width ) width  = imageWidth  - x;
  if (! height) height = imageHeight - y;
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
  width  = min (width,  (int) imageWidth  - x);
  height = min (height, (int) imageHeight - y);
  width  = max (width,  0);
  height = max (height, 0);
  image.resize (width, height);
  if (! width  ||  ! height) return;

  unsigned char * imageMemory = (unsigned char *) buffer->memory;
  int stride = image.format->depth * width;

  // If the requested image is anything other than exactly the union of a
  // vertical set of blocks in the file, then must use temporary storage
  // to read in blocks.
  Image block (*image.format);
  tdata_t blockBuffer = 0;

  if (TIFFIsTiled (tif))
  {
	uint32 blockWidth;
	TIFFGetField (tif, TIFFTAG_TILEWIDTH, &blockWidth);
	uint32 blockHeight;
	TIFFGetField (tif, TIFFTAG_TILELENGTH, &blockHeight);

	tsize_t blockSize = blockWidth * blockHeight * image.format->depth;

	for (int oy = 0; oy < height;)  // output y: position in output image
	{
	  int ry = oy + y;  // working y in raster
	  int iy = ry % blockHeight;  // input y: offset from top of block
	  int h = min ((int) blockHeight - iy, height - oy);  // height of usable portion of block

	  for (int ox = 0; ox < width;)
	  {
		int rx = ox + x;
		int ix = rx % blockWidth;
		int w = min ((int) blockWidth - ix, width - ox);

		ttile_t tile = TIFFComputeTile (tif, rx, ry, 0, 0);
		if (w == width  &&  w == blockWidth  &&  h == blockHeight)
		{
		  TIFFReadEncodedTile (tif, tile, imageMemory + oy * stride, blockSize);
		}
		else
		{
		  if (! blockBuffer)
		  {
			block.resize (blockWidth, blockHeight);
			blockBuffer = (tdata_t) ((PixelBufferPacked *) block.buffer)->memory;
		  }
		  TIFFReadEncodedTile (tif, tile, blockBuffer, blockSize);
		  image.bitblt (block, ox, oy, ix, iy, w, h);
		}

		ox += w;
	  }

	  oy += h;
	}
  }
  else  // strip organization
  {
	uint32 rowsPerStrip;
	TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);

	tsize_t blockSize = imageWidth * rowsPerStrip * image.format->depth;

	for (int oy = 0; oy < height;)
	{
	  int ry = oy + y;
	  int iy = ry % rowsPerStrip;
	  int strip = ry / rowsPerStrip;
	  int h = min ((int) rowsPerStrip - iy, height - oy);

	  if (width == imageWidth  &&  iy == 0)
	  {
		TIFFReadEncodedStrip (tif, strip, imageMemory + oy * stride, h * stride);
	  }
	  else
	  {
		if (! blockBuffer)
		{
		  block.resize (imageWidth, rowsPerStrip);
		  blockBuffer = (tdata_t) ((PixelBufferPacked *) block.buffer)->memory;
		}
		TIFFReadEncodedStrip (tif, strip, blockBuffer, blockSize);
		image.bitblt (block, 0, oy, x, iy, width, h);
	  }

	  oy += h;
	}
  }
}

inline void
fillBlock (unsigned char * block, int stride, int depth, int width, int height, int x1, int x2, int y1, int y2)
{
  // Fill top of block with black
  if (y1 > 0)
  {
	memset (block, 0, y1 * stride);
  }

  // Fill bottom of block with black
  int m = height - y2;
  if (m)
  {
	memset (block + y2 * stride, 0, m * stride);
  }

  // Fill left side with black
  if (x1 > 0)
  {
	unsigned char * a = block + y1 * stride;
	int count = x1 * depth;
	for (int i = y1; i < y2; i++)
	{
	  unsigned char * aa = a;
	  unsigned char * end = a + count;
	  while (aa < end) *aa++ = 0;
	  a += stride;
	}
  }

  // Fill right side with black
  int n = width - x2;
  if (n)
  {
	unsigned char * a = block + (y1 * stride + x2 * depth);
	int count = n * depth;
	for (int i = y1; i < y2; i++)
	{
	  unsigned char * aa = a;
	  unsigned char * end = a + count;
	  while (aa < end) *aa++ = 0;
	  a += stride;
	}
  }
}

/**
   \todo Allow negative coordinates for x and y.
**/
void
ImageFileDelegateTIFF::write (const Image & image, int x, int y)
{
  if (! tif) throw "ImageFileDelegateTIFF not open";
  if (x < 0  ||  y < 0) throw "Target coordinates must be non-negative";

  if (! format)  // signals need for one-time setup, including other tags besides pixel format
  {
	format = image.format;

	if (format->monochrome)
	{
	  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
	  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	  if (*format == GrayShort)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	  else if (*format == GrayFloat)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	  }
	  else if (*format == GrayDouble)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 64);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	  }
	  else
	  {
		format = &GrayChar;
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	}
	else if (format->hasAlpha)
	{
	  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	  if (*format == RGBAShort)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	  else if (*format == RGBAFloat)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	  }
	  else
	  {
		format = &RGBAChar;
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	}
	else  // Three color channels
	{
	  TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	  TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	  if (*format == RGBShort)
	  {
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	  else
	  {
		format = &RGBChar;
		TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	  }
	}

	TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  }

  Image work = image * *format;

  PixelBufferPacked * buffer = (PixelBufferPacked *) work.buffer;
  if (! buffer) throw "TIFF only handles packed buffers for now";
  unsigned char * workBuffer = (unsigned char *) buffer->memory;
  int stride = format->depth * work.width;

  uint32 imageWidth  = 0;
  uint32 imageHeight = 0;
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageHeight);
  if (! imageWidth)
  {
	imageWidth = x + work.width;
	TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, imageWidth);
  }
  if (! imageHeight)
  {
	imageHeight = y + work.height;
	TIFFSetField (tif, TIFFTAG_IMAGELENGTH, imageHeight);
  }

  int width  = min (work.width,  (int) imageWidth  - x);
  int height = min (work.height, (int) imageHeight - y);
  if (width <= 0  ||  height <= 0) return;

  Image block (*format);
  unsigned char * blockBuffer = 0;

  if (TIFFIsTiled (tif)  ||  imageWidth > work.width)  // non-clamped width, since it is the hint for block size
  {
	uint32 blockWidth = 0;
	uint32 blockHeight = 0;
	TIFFGetField (tif, TIFFTAG_TILEWIDTH, &blockWidth);
	TIFFGetField (tif, TIFFTAG_TILELENGTH, &blockHeight);
	if (! blockWidth  ||  ! blockHeight)
	{
	  if (! blockWidth)  blockWidth  = work.width;  // non-clamped value
	  if (! blockHeight) blockHeight = work.height;  // ditto
	  TIFFDefaultTileSize (tif, &blockWidth, &blockHeight);
	  TIFFSetField (tif, TIFFTAG_TILEWIDTH, blockWidth);
	  TIFFSetField (tif, TIFFTAG_TILELENGTH, blockHeight);
	}

	tsize_t blockSize = blockWidth * blockHeight * format->depth;
	int blockStride = blockWidth * format->depth;

	for (int iy = 0; iy < height;)  // input y: position in given image
	{
	  int ry = iy + y;  // working y in raster
	  int oy = ry % blockHeight;  // output y: offset from top of block
	  int h = min ((int) blockHeight - oy, height - iy);  // height of usable portion of block

	  for (int ix = 0; ix < width;)
	  {
		int rx = ix + x;
		int ox = rx % blockWidth;
		int w = min ((int) blockWidth - ox, width - ix);

		ttile_t tile = TIFFComputeTile (tif, rx, ry, 0, 0);
		if (w == work.width  &&  w == blockWidth  &&  h == blockHeight)
		{
		  TIFFWriteEncodedTile (tif, tile, workBuffer + iy * stride, blockSize);
		}
		else
		{
		  if (! blockBuffer)
		  {
			block.resize (blockWidth, blockHeight);
			blockBuffer = (unsigned char *) ((PixelBufferPacked *) block.buffer)->memory;
		  }
		  block.bitblt (work, ox, oy, ix, iy, w, h);
		  fillBlock (blockBuffer, blockStride, format->depth, blockWidth, blockHeight, ox, ox + w, oy, oy + h);
		  TIFFWriteEncodedTile (tif, tile, blockBuffer, blockSize);
		}

		ix += w;
	  }

	  iy += h;
	}
  }
  else  // strip organization
  {
	int stride = format->depth * buffer->stride;
	uint32 rowsPerStrip = 0;
	TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
	if (! rowsPerStrip)
	{
	  // There given image must be covered by an integral number of strips.
	  const int targetSize = 8192;  // Manual recommends 8K strips
	  int bestDistance = INT_MAX;
	  for (int rows = 1; rows <= work.height / 2; rows++)
	  {
		int size = rows * stride;
		int distance = abs (size - 8192);
		cerr << rows << " " << size << " " << distance << endl;
		if (work.height % rows == 0  &&  distance < bestDistance)
		{
		  cerr << "     accepted" << endl;
		  bestDistance = distance;
		  rowsPerStrip = rows;
		}
		if (rowsPerStrip  &&  size > targetSize) break;  // Since it won't get any better
	  }
	  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
	}

	int blockStride = imageWidth * format->depth;

	for (int iy = 0; iy < height;)
	{
	  int ry = iy + y;
	  int oy = ry % rowsPerStrip;
	  int strip = ry / rowsPerStrip;
	  int h = min ((int) rowsPerStrip - oy, height - iy);
	  int rows = strip * rowsPerStrip + h == imageHeight ? h : rowsPerStrip;
	  int blockSize = rows * blockStride;

	  if (work.width == imageWidth  &&  x == 0  &&  oy == 0)
	  {
		TIFFWriteEncodedStrip (tif, strip, workBuffer + iy * stride, blockSize);
	  }
	  else
	  {
		if (! blockBuffer)
		{
		  block.resize (imageWidth, rowsPerStrip);
		  blockBuffer = (unsigned char *) ((PixelBufferPacked *) block.buffer)->memory;
		}
		block.bitblt (work, x, oy, 0, iy, width, h);
		fillBlock (blockBuffer, blockStride, format->depth, imageWidth, rows, x, x + width, oy, oy + h);
		TIFFWriteEncodedStrip (tif, strip, blockBuffer, blockSize);
	  }

	  iy += h;
	}
  }
}

void
ImageFileDelegateTIFF::get (const string & name, string & value)
{
  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	if (fi->field_type == TIFF_ASCII)
	{
	  char * v;
	  if (TIFFGetFieldDefaulted (tif, fi->field_tag, &v)) value = v;
	}
	else
	{
	  if (name == "Compression")
	  {
		// Attempt to look up codec name
		uint16 v;
		if (TIFFGetFieldDefaulted (tif, fi->field_tag, &v))
		{
		  TIFFCodec * codec = TIFFGetConfiguredCODECs ();
		  while (codec->name)
		  {
			if (v == codec->scheme)
			{
			  value = codec->name;
			  return;
			}
			codec++;
		  }
		}
	  }
	  Matrix<double> v;
	  get (name, v);
	  if (v.rows () > 0  &&  v.columns () > 0)
	  {
		ostrstream sv;
		sv << v;
		value = sv.str ();
	  }
	}
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	char buffer[100];
	int size = 0;
	tagtype_t type = TYPE_UNKNOWN;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		uint16 v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  value = GTIFValueName ((geokey_t) key, v);
		}
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  sprintf (buffer, "%lf", v);
		  value = buffer;
		}
		return;
	  }
	  case TYPE_ASCII:
	  {
		value.resize (length);
		GTIFKeyGet (gtif, (geokey_t) key, (void *) value.c_str (), 0, length);
		return;
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::get (const string & name, int & value)
{
  Matrix<double> v;
  get (name, v);
  if (v.rows () > 0  &&  v.columns () > 0)
  {
	value = (int) rint (v(0,0));
  }
}

void
ImageFileDelegateTIFF::get (const string & name, double & value)
{
  Matrix<double> v;
  get (name, v);
  if (v.rows () > 0  &&  v.columns () > 0)
  {
	value = v(0,0);
  }
}

void
ImageFileDelegateTIFF::get (const string & name, Matrix<double> & value)
{
  if (name == "GeoTransformationMatrix")
  {
	uint16 count;
	double * v;
	bool found = TIFFGetField (tif, 34264, &count, &v);  // GeoTransformationMatrix
	if (! found)
	{
	  found = TIFFGetField (tif, 33920, &count, &v)  &&  count == 16;  // Intergraph TransformationMatrix
	}
	if (found)
	{
	  value = ~Matrix<double> (v, 4, 4);
	  return;
	}

	if (TIFFGetField (tif, 33922, &count, &v)) // ModelTiepoint
	{
	  int points = count / 6;
	  if (points == 1)  // exactly one point indicates direct computation of transformation
	  {
		double * o;
		if (TIFFGetField (tif, 33550, &count, &o)) // ModelPixelScale
		{
		  value.resize (4, 4);
		  value.clear ();
		  value(0,0) = o[0];
		  value(1,1) = -o[1];
		  value(2,2) = o[2];
		  value(0,3) = v[3] - v[0] * o[0];
		  value(1,3) = v[4] + v[1] * o[1];
		  value(2,3) = v[5] - v[2] * o[2];
		  value(3,3) = 1;
		}
	  }
	  else if (points >= 3)  // 3 or more tiepoints, so solve for transformation using least squares
	  {
		// First determine level of model to compute
		bool allzero = true;
		double * p = &v[2];
		for (int i = 0; i < 2 * points; i++)
		{
		  allzero &= *p == 0;
		  p += 3;
		}

		if (allzero)  // solve just 6 parameters
		{
		  // Find center of each point cloud
		  Vector<double> centerIJ (2);
		  Vector<double> centerXY (2);
		  centerIJ.clear ();
		  centerXY.clear ();
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			centerIJ[0] += *p++;
			centerIJ[1] += *p++;
			p++;
			centerXY[0] += *p++;
			centerXY[1] += *p++;
			p++;
		  }
		  centerIJ /= points;
		  centerXY /= points;

		  // Build least squares problem for 2x2 xform
		  Matrix<double> AT (points, 2);
		  Matrix<double> BT (points, 2);
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			AT(i,0) = *p++ - centerIJ[0];
			AT(i,1) = *p++ - centerIJ[1];
			p++;
			BT(i,0) = *p++ - centerXY[0];
			BT(i,1) = *p++ - centerXY[1];
			p++;
		  }
		  Matrix<double> X;
		  gelss (AT, X, BT, (double *) 0, true, true);

		  // Assemble result
		  value.resize (4, 4);
		  value.clear ();
		  value.region (0, 0) = ~X;
		  value.region (0, 3) = centerXY - value.region (0, 0, 1, 1) * centerIJ;
		  value(3,3) = 1;
		}
		else if (points >= 4)  // solve all 12 parameters
		{
		  // Find center of each point cloud
		  Vector<double> centerIJK (3);
		  Vector<double> centerXYZ (3);
		  centerIJK.clear ();
		  centerXYZ.clear ();
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			centerIJK[0] += *p++;
			centerIJK[1] += *p++;
			centerIJK[2] += *p++;
			centerXYZ[0] += *p++;
			centerXYZ[1] += *p++;
			centerXYZ[2] += *p++;
		  }
		  centerIJK /= points;
		  centerXYZ /= points;

		  // Build least squares problem for 3x3 xform
		  Matrix<double> AT (points, 3);
		  Matrix<double> BT (points, 3);
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			AT(i,0) = *p++ - centerIJK[0];
			AT(i,1) = *p++ - centerIJK[1];
			AT(i,2) = *p++ - centerIJK[2];
			BT(i,0) = *p++ - centerXYZ[0];
			BT(i,1) = *p++ - centerXYZ[1];
			BT(i,2) = *p++ - centerXYZ[2];
		  }
		  Matrix<double> X;
		  gelss (AT, X, BT, (double *) 0, true, true);

		  // Assemble result
		  value.resize (4, 4);
		  value.clear ();
		  value.region (0, 0) = ~X;
		  value.region (0, 3) = centerXYZ - value.region (0, 0, 2, 2) * centerIJK;
		  value(3,3) = 1;
		}
	  }
	}

	return;
  }

  if (name == "width")
  {
	get ("ImageWidth", value);
	return;
  }
  if (name == "height")
  {
	get ("ImageLength", value);
	return;
  }
  if (name == "blockWidth")
  {
	if (TIFFIsTiled (tif)) get ("TileWidth",  value);
	else                   get ("ImageWidth", value);
	return;
  }
  if (name == "blockHeight")
  {
	if (TIFFIsTiled (tif)) get ("TileLength",   value);
	else                   get ("RowsPerStrip", value);
	return;
  }

  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	int count = fi->field_readcount;
	TIFFDataType fieldType = fi->field_type;

	// Compensate for differences between field info and actual behavior of TIFFGetField().
	if (name == "SMinSampleValue"  ||  name == "SMaxSampleValue")
	{
	  count = 1;
	  fieldType = TIFF_DOUBLE;
	}
	else if (name == "MinSampleValue"  ||  name == "MaxSampleValue")
	{
	  count = 1;
	}

	if (count == 1)
	{
#     define grabSingle(type) \
	  { \
		type v; \
		if (TIFFGetFieldDefaulted (tif, fi->field_tag, &v)) \
		{ \
		  value.resize (1, 1); \
		  value(0,0) = v; \
		} \
		return; \
	  }

	  switch (fieldType)
	  {
		case TIFF_BYTE:
		  grabSingle (uint8);
		case TIFF_SBYTE:
		  grabSingle (int8);
		case TIFF_SHORT:
		  grabSingle (uint16);
		case TIFF_SSHORT:
		  grabSingle (int16);
		case TIFF_LONG:
		case TIFF_IFD:
		  grabSingle (uint32);
		case TIFF_SLONG:
		  grabSingle (int32);
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		case TIFF_FLOAT:
		  grabSingle (float);
		case TIFF_DOUBLE:
		  grabSingle (double);
	  }
	  return;
	}

	int rows = 1;
	if (fi->field_readcount == TIFF_SPP)
	{
	  uint16 spp;
	  TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	  rows = spp;
	}
	if (name == "GeoTiePoints") rows = 6;

	void * data;
	bool found = false;
	if (count > 1)
	{
	  found = TIFFGetFieldDefaulted (tif, fi->field_tag, &data);
	}
	else if (fi->field_passcount)
	{
	  if (fi->field_readcount == TIFF_VARIABLE2)
	  {
		uint32 c;
		found = TIFFGetFieldDefaulted (tif, fi->field_tag, &c, &data);
		count = c;
	  }
	  else
	  {
		uint16 c;
		found = TIFFGetFieldDefaulted (tif, fi->field_tag, &c, &data);
		count = c;
	  }
	}
	if (! found) return;

	if (fieldType != TIFF_DOUBLE)
	{
	  if (rows == 1) value.resize (count, 1);
	  else           value.resize (rows, count / rows);
	}

#   define readVector(type) \
	for (int i = 0; i < count; i++) \
	{ \
	  value[i] = ((type *) data)[i]; \
	} \
	return;

	switch (fieldType)
	{
	  case TIFF_BYTE:
		readVector (uint8);
	  case TIFF_SBYTE:
		readVector (int8);
	  case TIFF_SHORT:
		readVector (uint16);
	  case TIFF_SSHORT:
		readVector (int16);
	  case TIFF_LONG:
	  case TIFF_IFD:
		readVector (uint32);
	  case TIFF_SLONG:
		readVector (int32);
	  case TIFF_RATIONAL:
	  case TIFF_SRATIONAL:
	  case TIFF_FLOAT:
		readVector (float);
	  case TIFF_DOUBLE:
		if (rows == 1) value = Matrix<double> ((double *) data, count, 1);
		else           value = Matrix<double> ((double *) data, rows, count / rows);
		return;
	}
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	int size = 0;
	tagtype_t type = TYPE_UNKNOWN;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		uint16 v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TYPE_ASCII:
	  {
		string v;
		v.resize (length);
		if (GTIFKeyGet (gtif, (geokey_t) key, (void *) v.c_str (), 0, length))
		{
		  value.resize (1, 1);
		  value(0,0) = atof (v.c_str ());
		}
		return;
	  }
	}
  }
# endif
}

#ifdef HAVE_GEOTIFF

struct GTIFmapping
{
  geokey_t key;
  tagtype_t type;
};

/**
   \todo The GTIFKeyInfo() query does not work when writing, because it bails
   out if the key is not already in the directory.  This table is a hack to
   compensate for that lack.  It would be better is this functionality were
   moved into libgeotiff itself.
 **/
static GTIFmapping GTIFmap[] =
{
  {GTModelTypeGeoKey,              TYPE_SHORT},
  {GTRasterTypeGeoKey,             TYPE_SHORT},
  {GTCitationGeoKey,               TYPE_ASCII},
  {GeographicTypeGeoKey,           TYPE_SHORT},
  {GeogCitationGeoKey,             TYPE_ASCII},
  {GeogGeodeticDatumGeoKey,        TYPE_SHORT},
  {GeogPrimeMeridianGeoKey,        TYPE_SHORT},
  {GeogLinearUnitsGeoKey,          TYPE_SHORT},
  {GeogLinearUnitSizeGeoKey,       TYPE_SHORT},
  {GeogAngularUnitsGeoKey,         TYPE_SHORT},
  {GeogAngularUnitSizeGeoKey,      TYPE_SHORT},
  {GeogEllipsoidGeoKey,            TYPE_SHORT},
  {GeogSemiMajorAxisGeoKey,        TYPE_SHORT},
  {GeogSemiMinorAxisGeoKey,        TYPE_SHORT},
  {GeogInvFlatteningGeoKey,        TYPE_SHORT},
  {GeogAzimuthUnitsGeoKey,         TYPE_SHORT},
  {GeogPrimeMeridianLongGeoKey,    TYPE_SHORT},
  {ProjectedCSTypeGeoKey,          TYPE_SHORT},
  {PCSCitationGeoKey,              TYPE_ASCII},
  {ProjectionGeoKey,               TYPE_SHORT},
  {ProjCoordTransGeoKey,           TYPE_SHORT},
  {ProjLinearUnitsGeoKey,          TYPE_SHORT},
  {ProjLinearUnitSizeGeoKey,       TYPE_SHORT},
  {ProjStdParallel1GeoKey,         TYPE_SHORT},
  {ProjStdParallelGeoKey,          TYPE_SHORT},
  {ProjStdParallel2GeoKey,         TYPE_SHORT},
  {ProjNatOriginLongGeoKey,        TYPE_SHORT},
  {ProjOriginLongGeoKey,           TYPE_SHORT},
  {ProjNatOriginLatGeoKey,         TYPE_SHORT},
  {ProjOriginLatGeoKey,            TYPE_SHORT},
  {ProjFalseEastingGeoKey,         TYPE_SHORT},
  {ProjFalseNorthingGeoKey,        TYPE_SHORT},
  {ProjFalseOriginLongGeoKey,      TYPE_SHORT},
  {ProjFalseOriginLatGeoKey,       TYPE_SHORT},
  {ProjFalseOriginEastingGeoKey,   TYPE_SHORT},
  {ProjFalseOriginNorthingGeoKey,  TYPE_SHORT},
  {ProjCenterLongGeoKey,           TYPE_SHORT},
  {ProjCenterLatGeoKey,            TYPE_SHORT},
  {ProjCenterEastingGeoKey,        TYPE_SHORT},
  {ProjCenterNorthingGeoKey,       TYPE_SHORT},
  {ProjScaleAtNatOriginGeoKey,     TYPE_SHORT},
  {ProjScaleAtOriginGeoKey,        TYPE_SHORT},
  {ProjScaleAtCenterGeoKey,        TYPE_SHORT},
  {ProjAzimuthAngleGeoKey,         TYPE_SHORT},
  {ProjStraightVertPoleLongGeoKey, TYPE_SHORT},
  {VerticalCSTypeGeoKey,           TYPE_SHORT},
  {VerticalCitationGeoKey,         TYPE_ASCII},
  {VerticalDatumGeoKey,            TYPE_SHORT},
  {VerticalUnitsGeoKey,            TYPE_SHORT},
  {(geokey_t) 0}
};

static inline tagtype_t
findType (geokey_t key)
{
  GTIFmapping * map = GTIFmap;
  while (map->key  &&  map->key != key) map++;
  if (map->key) return map->type;
  return TYPE_UNKNOWN;
}

#endif

void
ImageFileDelegateTIFF::set (const string & name, const string & value)
{
  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	if (fi->field_type == TIFF_ASCII)
	{
	  TIFFSetField (tif, fi->field_tag, (char *) value.c_str ());
	}
	else
	{
	  if (name == "Compression")
	  {
		// Attempt to look up codec name
		TIFFCodec * codec = TIFFGetConfiguredCODECs ();
		while (codec->name)
		{
		  if (value == codec->name)
		  {
			set (name, codec->scheme);
			return;
		  }
		  codec++;
		}
	  }
	  set (name, atof (value.c_str ()));
	}
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	switch (findType ((geokey_t) key))
	{
	  case TYPE_SHORT:
	  {
		int v = GTIFValueCode ((geokey_t) key, (char *) value.c_str ());
		if (v < 0) v = atoi (value.c_str ());
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, (uint16) v);
		break;
	  }
	  case TYPE_DOUBLE:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, atof (value.c_str ()));
		break;
	  }
	  case TYPE_ASCII:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_ASCII, value.size (), value.c_str ());
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::set (const string & name, int value)
{
  Matrix<double> v (1, 1);
  v(0,0) = value;
  set (name, v);
}

void
ImageFileDelegateTIFF::set (const string & name, double value)
{
  Matrix<double> v (1, 1);
  v(0,0) = value;
  set (name, v);
}

void
ImageFileDelegateTIFF::set (const string & name, const Matrix<double> & value)
{
  if (name == "GeoTransformationMatrix")
  {
	Matrix<double> v = ~value;
	TIFFSetField (tif, 34264, (uint16) 16, &v(0,0));
	return;
  }

  if (name == "width")
  {
	set ("ImageWidth", value);
	return;
  }
  if (name == "height")
  {
	set ("ImageLength", value);
	return;
  }
  if (name == "blockWidth")
  {
	set ("TileWidth", value);
	return;
  }
  if (name == "blockHeight")
  {
	set ("TileLength", value);
	return;
  }

  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	int count = value.rows () * value.columns ();
	if (! count) throw "Emptry matrix";

	void * data = 0;
	ostrstream adata;

#   define writeVector(type) \
	data = malloc (count * sizeof (type)); \
	for (int i = 0; i < count; i++) \
	{ \
	  ((type *) data)[i] = (type) rint (value[i]); \
	} \
	break;

	switch (fi->field_type)
	{
	  case TIFF_BYTE:
		writeVector (uint8);
	  case TIFF_SBYTE:
		writeVector (int8);
	  case TIFF_SHORT:
		writeVector (uint16);
	  case TIFF_SSHORT:
		writeVector (int16);
	  case TIFF_LONG:
	  case TIFF_IFD:
		writeVector (uint32);
	  case TIFF_SLONG:
		writeVector (int32);
	  case TIFF_RATIONAL:
	  case TIFF_SRATIONAL:
	  case TIFF_FLOAT:
		data = malloc (count * sizeof (float));
		for (int i = 0; i < count; i++)
		{
		  ((float *) data)[i] = value[i];
		}
		break;
	  case TIFF_DOUBLE:
		data = &value(0,0);
		break;
	  case TIFF_ASCII:
		adata << value;
		data = adata.str ();
	}

	if (fi->field_passcount)
	{
	  if (fi->field_writecount == TIFF_VARIABLE2)
	  {
		TIFFSetField (tif, fi->field_tag, (uint32) count, data);
	  }
	  else
	  {
		TIFFSetField (tif, fi->field_tag, (uint16) count, data);
	  }
	}
	else
	{
	  if (fi->field_writecount == 1)
	  {
		switch (fi->field_type)
		{
		  case TIFF_BYTE:
			TIFFSetField (tif, fi->field_tag, *(uint8 *) data);
			break;
		  case TIFF_SBYTE:
			TIFFSetField (tif, fi->field_tag, *(int8 *) data);
			break;
		  case TIFF_SHORT:
			TIFFSetField (tif, fi->field_tag, *(uint16 *) data);
			break;
		  case TIFF_SSHORT:
			TIFFSetField (tif, fi->field_tag, *(int16 *) data);
			break;
		  case TIFF_LONG:
		  case TIFF_IFD:
			TIFFSetField (tif, fi->field_tag, *(uint32 *) data);
			break;
		  case TIFF_SLONG:
			TIFFSetField (tif, fi->field_tag, *(int32 *) data);
			break;
		  case TIFF_RATIONAL:
		  case TIFF_SRATIONAL:
		  case TIFF_FLOAT:
			TIFFSetField (tif, fi->field_tag, *(float *) data);
			break;
		  case TIFF_DOUBLE:
			TIFFSetField (tif, fi->field_tag, *(double *) data);
		}
	  }
	  else
	  {
		TIFFSetField (tif, fi->field_tag, data);
	  }
	}

	if (data  &&  fi->field_type != TIFF_DOUBLE  &&  fi->field_type != TIFF_ASCII) free (data);
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	switch (findType ((geokey_t) key))
	{
	  case TYPE_SHORT:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, (uint16) rint (value(0,0)));
		break;
	  }
	  case TYPE_DOUBLE:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_DOUBLE, 1, value(0,0));
		break;
	  }
	  case TYPE_ASCII:
	  {
		ostrstream data;
		data << value;
		GTIFKeySet (gtif, (geokey_t) key, TYPE_ASCII, data.pcount (), data.str ());
	  }
	}
  }
# endif
}


// class ImageFileFormatTIFF --------------------------------------------------

ImageFileFormatTIFF::ImageFileFormatTIFF ()
{
# ifdef HAVE_GEOTIFF
  XTIFFInitialize ();
# endif
}

ImageFileDelegate *
ImageFileFormatTIFF::open (istream & stream, bool ownStream) const
{
  TIFF * tif = TIFFStreamOpen ("", &stream);
  if (! tif)
  {
	throw "Unable to open file.";
  }
  return new ImageFileDelegateTIFF (tif, ownStream ? &stream : 0);
}

ImageFileDelegate *
ImageFileFormatTIFF::open (ostream & stream, bool ownStream) const
{
  TIFF * tif = TIFFStreamOpen ("", &stream);
  if (! tif)
  {
	throw "Unable to open file.";
  }
  return new ImageFileDelegateTIFF (tif, ownStream ? &stream : 0);
}

float
ImageFileFormatTIFF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);

  // Apparently, some implementation don't store the magic number 42 according
  // to the endian indicated by the first two bytes, so we have to handle the
  // wrong order as well.
  if (magic.substr (0, 2) == "II")  // little endian
  {
	if (magic[2] == '\x2A'  &&  magic[3] == '\x00') return 1;
	if (magic[2] == '\x00'  &&  magic[3] == '\x2A') return 0.8;
  }
  if (magic.substr (0, 2) == "MM")  // big endian
  {
	if (magic[2] == '\x2A'  &&  magic[3] == '\x00') return 0.8;
	if (magic[2] == '\x00'  &&  magic[3] == '\x2A') return 1;
  }

  return 0;
}

float
ImageFileFormatTIFF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "tiff") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "tif") == 0)
  {
	return 0.8;
  }
  return 0;
}
