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


#include "fl/image.h"
#include "fl/string.h"
#include "fl/math.h"
#include "fl/lapack.h"
#include "fl/binary.h"

#include <tiffio.h>
#ifdef HAVE_GEOTIFF
#  include <xtiffio.h>
#  include <geotiff.h>
#endif

#include <sstream>
#include <typeinfo>

#include <errno.h>
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


// Extend TIFF tags -----------------------------------------------------------

static const TIFFFieldInfo privateFieldInfo[] =
{
  {65000, TIFF_VARIABLE, TIFF_VARIABLE, TIFF_ASCII, FIELD_CUSTOM, 1, 0, (char *) "flMetadata"}
};

static TIFFExtendProc previousExtender = 0;

static void registerCustomTags (TIFF * tif)
{
  TIFFMergeFieldInfo (tif, privateFieldInfo, sizeof (privateFieldInfo) / sizeof (privateFieldInfo[0]));
  if (previousExtender) (*previousExtender)(tif);
}


// class ImageFileDelegateTIFF ------------------------------------------------

class ImageFileDelegateTIFF : public ImageFileDelegate
{
public:
  ImageFileDelegateTIFF (istream * in, ostream * out, bool ownStream = false);
  virtual ~ImageFileDelegateTIFF ();
  void open ();  ///< Enable lazy opening of file, so user can specify BigTIFF via metadata interface.

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name,       string & value);
  virtual void set (const string & name, const string & value);
  using Metadata::get;
  using Metadata::set;

  static tsize_t tiffRead  (thandle_t handle, tdata_t data, tsize_t size);
  static tsize_t tiffWrite (thandle_t handle, tdata_t data, tsize_t size);
  static toff_t  tiffSeek  (thandle_t handle, toff_t offset, int direction);
  static int     tiffClose (thandle_t handle);
  static toff_t  tiffSize  (thandle_t handle);
  static int     tiffMap   (thandle_t handle, tdata_t * data, toff_t * offset);
  static void    tiffUnmap (thandle_t handle, tdata_t data, toff_t offset);

  static void tiffErrorHandler (const char * module, const char * format, va_list arguments);

  istream * in;
  ostream * out;
  bool ownStream;
  toff_t startPosition;
  bool bigtiff;
  NamedValueSet metadata;

  TIFF * tif;
# ifdef HAVE_GEOTIFF
  GTIF * gtif;
# endif

  PointerPoly<const PixelFormat> format;
};

ImageFileDelegateTIFF::ImageFileDelegateTIFF (istream * in, ostream * out, bool ownStream)
{
  this->in        = in;
  this->out       = out;
  this->ownStream = ownStream;
  if (in) startPosition = (toff_t) in->tellg ();
  else    startPosition = (toff_t) out->tellp ();
  bigtiff = false;

  tif = 0;
# ifdef HAVE_GEOTIFF
  gtif = 0;
# endif
}

void
ImageFileDelegateTIFF::open ()
{
  string mode = in ? "r" : "w";
  if (bigtiff) mode += "8";

  tif = TIFFClientOpen
  (
    "",
	mode.c_str (),
	(thandle_t) this,
	tiffRead,
	tiffWrite,
	tiffSeek,
	tiffClose,
	tiffSize,
	tiffMap,
	tiffUnmap
  );
  if (! tif)
  {
	throw "Unable to open file.";
  }

# ifdef HAVE_GEOTIFF
  gtif = GTIFNew (tif);
# endif
}

ImageFileDelegateTIFF::~ImageFileDelegateTIFF ()
{
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

	if (metadata.namedValues.size () > 0)
	{
	  string value;
	  metadata.write (value);
	  const TIFFField * fi = TIFFFieldWithName (tif, "flMetadata");
	  TIFFSetField (tif, TIFFFieldTag (fi), (char *) value.c_str ());
	}

	TIFFClose (tif);
  }

  if (ownStream)
  {
	if (in)  delete in;
	if (out) delete out;
  }
}

struct FormatMapping
{
  PixelFormat * format;   // Which pre-fab format to use.  A value of zero ends the format map.  If this casts to a low-valued integer, it selectes a special construction method.
  uint16_t exact;           // A 1 in a bit position indicates the associated field is necessary for matching the format, while a 0 indicates wild-card.  Bit position 6 refers to the first field below, and lower positions refer to subsequent fields in order.
  uint16_t samplesPerPixel;
  uint16_t bitsPerSample;
  uint16_t sampleFormat;
  uint16_t photometric;
  uint16_t planarConfig;
  uint16_t extraCount;
  uint16_t extraFormat;     // In general, should be an array, but we will only examine first element.
};

static FormatMapping formatMap[] =
{
  {(PixelFormat *) 2, B8(0001000), 1,  0, 1, 3, 1, 0, 0},  // palette
  {(PixelFormat *) 3, B8(1110000), 1,  1, 1, 0, 1, 0, 0},  // GrayBits
  {(PixelFormat *) 3, B8(1110000), 1,  2, 1, 0, 1, 0, 0},  // GrayBits
  {(PixelFormat *) 3, B8(1110000), 1,  4, 1, 0, 1, 0, 0},  // GrayBits
  {&GrayChar,         B8(1110000), 1,  8, 1, 0, 1, 0, 0},  // No-care on photometric: we don't distinguish positive from negative images at this time.
  {(PixelFormat *) 1, B8(1110000), 1, 16, 1, 0, 1, 0, 0},  // GrayShort, with consideration for SMaxSampleValue
  {&GrayShortSigned,  B8(1110000), 1, 16, 2, 0, 1, 0, 0},
  {&GrayFloat,        B8(1110000), 1, 32, 3, 0, 1, 0, 0},
  {&GrayDouble,       B8(1110000), 1, 64, 3, 0, 1, 0, 0},
  {&RGBChar,          B8(1111100), 3,  8, 1, 2, 1, 0, 0},
  {&RGBPlanar,        B8(1111100), 3,  8, 1, 2, 2, 0, 0},
  {(PixelFormat *) 4, B8(1111100), 3,  8, 1, 6, 1, 0, 0},
  {&RGBShort,         B8(1110100), 3, 16, 1, 2, 1, 0, 0},
  {&RGBAChar,         B8(1110100), 4,  8, 1, 2, 1, 1, 2},  // No-care on alpha channel type.  We always treat it as unassociated.
  {&RGBAShort,        B8(1110100), 4, 16, 1, 2, 1, 1, 2},
  {&RGBAFloat,        B8(1110100), 4, 32, 3, 2, 1, 1, 2},
  {0}
};

void
ImageFileDelegateTIFF::read (Image & image, int x, int y, int width, int height)
{
  if (! tif) open ();

  bool ok = true;

  uint32_t imageWidth;
  uint32_t imageHeight;
  ok &= TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
  ok &= TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imageHeight);
  if (! ok) throw "Unable to get needed tag values.";

  if (format == 0)
  {
	uint16_t samplesPerPixel;
	uint16_t bitsPerSample;
	uint16_t sampleFormat;
	uint16_t photometric;
	uint16_t planarConfig;
	uint16_t extraCount;
	uint16_t * extraFormat;
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &photometric);
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PLANARCONFIG, &planarConfig);
	ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extraCount, &extraFormat);
	if (! ok) throw "Unable to get needed tag values.";
	//cerr << "tif pix fmt info: " << samplesPerPixel << " " << bitsPerSample << " " << sampleFormat << " " << photometric << " " << planarConfig << " " << extraCount;
	//for (int i = 0; i < extraCount; i++) cerr << " " << extraFormat[i];
	//cerr << endl;

	for (FormatMapping * m = formatMap; m->format; m++)
	{
	  // Filter on format tags
	  if (m->exact & B8(1000000)  &&  m->samplesPerPixel != samplesPerPixel) continue;
	  if (m->exact & B8(0100000)  &&  m->bitsPerSample   != bitsPerSample  ) continue;
	  if (m->exact & B8(0010000)  &&  m->sampleFormat    != sampleFormat   ) continue;
	  if (m->exact & B8(0001000)  &&  m->photometric     != photometric    ) continue;
	  if (m->exact & B8(0000100)  &&  m->planarConfig    != planarConfig   ) continue;
	  if (m->exact & B8(0000010)  &&  m->extraCount      != extraCount     ) continue;
	  if (m->exact & B8(0000001)  &&  extraCount  &&  m->extraFormat != extraFormat[0]) continue;

	  // Assign the chosen format, or construct one as indicated.
	  switch ((ptrdiff_t) m->format)
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
		  }
		  break;
		}
		case 2:
		{
		  unsigned char * reds;
		  unsigned char * greens;
		  unsigned char * blues;
		  ok = TIFFGetFieldDefaulted (tif, TIFFTAG_COLORMAP, &reds, &greens, &blues);  // As far as I know, there is no default color map, but why not give it a try?
		  if (! ok) throw "ColorMap tag is missing, but required for paletted images.";

		  // libtiff always returns data in machine word order
#         if BYTE_ORDER == LITTLE_ENDIAN
		  reds++;
		  greens++;
		  blues++;
#         elif BYTE_ORDER != BIG_ENDIAN
#           error This endian is not currently handled.
#         endif

		  format = new PixelFormatPalette (reds, greens, blues, sizeof (uint16_t), bitsPerSample);
		  break;
		}
		case 3:
		{
		  format = new PixelFormatGrayBits (bitsPerSample);
		  break;
		}
		case 4:
		{
		  uint16_t ratioH;
		  uint16_t ratioV;
		  TIFFGetFieldDefaulted (tif, TIFFTAG_YCBCRSUBSAMPLING, &ratioH, &ratioV);
		  PixelFormatPackedYUV::YUVindex * table = new PixelFormatPackedYUV::YUVindex[ratioH * ratioV + 1];
		  PixelFormatPackedYUV::YUVindex * t = table;
		  for (int v = 0; v < ratioV; v++)
		  {
			for (int h = 0; h < ratioH; h++)
			{
			  t->y = v * ratioH + h;
			  t->u = ratioV * ratioH;
			  t->v = ratioV * ratioH + 1;
			  t++;
			}
		  }
		  t->y = -1;
		  format = new PixelFormatPackedYUV (table, ratioV);
		  delete[] table;  // format ctor makes a deep copy of the table
		  break;
		}
		default:
		  format = m->format;  // Rather risky, but as long as we keep formatMap consistent, we are OK.
	  }
	  break;
	}

	if (format == 0) throw "No PixelFormat available that matches file contents";
  }
  //cerr << "format = " << typeid (*format).name () << endl;

  image.format = format;
  if (image.buffer == 0  ||  image.buffer->planes != image.format->planes) image.buffer = image.format->buffer ();

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

  // TODO: Size of memory arrays below should be the number of planes. For now, we hard-code to 3.
  uint8_t * imageMemory[3];
  int stride[3];
  if (PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer)
  {
	imageMemory[0] = (uint8_t *) buffer->base ();
	stride[0] = buffer->stride;
  }
  else if (PixelBufferGroups * buffer = (PixelBufferGroups *) image.buffer)
  {
	imageMemory[0] = (uint8_t *) buffer->memory;
	stride[0] = buffer->stride;
  }
  else if (PixelBufferPlanar * buffer = (PixelBufferPlanar *) image.buffer)
  {
	imageMemory[0] = (uint8_t *) buffer->plane0;
	imageMemory[1] = (uint8_t *) buffer->plane1;
	imageMemory[2] = (uint8_t *) buffer->plane2;
	stride[0] = buffer->stride0;
	stride[1] = buffer->stride12;
	stride[2] = buffer->stride12;
  }
  else throw "We don't handle this type of buffer.";

  // If the requested image is anything other than exactly the union of a
  // vertical set of blocks in the file, then must use temporary storage
  // to read in blocks.
  Image block (*image.format);
  tdata_t blockBuffer[3];
  blockBuffer[0] = 0;

  if (TIFFIsTiled (tif))
  {
	uint32_t blockWidth;
	TIFFGetField (tif, TIFFTAG_TILEWIDTH, &blockWidth);
	uint32_t blockHeight;
	TIFFGetField (tif, TIFFTAG_TILELENGTH, &blockHeight);

	tsize_t blockSize = TIFFTileSize (tif);

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

		if (w == width  &&  w == blockWidth  &&  h == blockHeight)
		{
		  for (int p = 0; p < format->planes; p++)
		  {
			ttile_t tile = TIFFComputeTile (tif, rx, ry, 0, p);
			TIFFReadEncodedTile (tif, tile, imageMemory[p] + oy * stride[p], blockSize);
		  }
		}
		else
		{
		  if (! blockBuffer[0])
		  {
			block.resize (blockWidth, blockHeight);
			if (PixelBufferPacked * buffer = (PixelBufferPacked *) block.buffer)
			{
			  blockBuffer[0] = (tdata_t) buffer->base ();
			}
			else if (PixelBufferGroups * buffer = (PixelBufferGroups *) block.buffer)
			{
			  blockBuffer[0] = (tdata_t) buffer->memory;
			}
			else if (PixelBufferPlanar * buffer = (PixelBufferPlanar *) block.buffer)
			{
			  blockBuffer[0] = (tdata_t) buffer->plane0;
			  blockBuffer[1] = (tdata_t) buffer->plane1;
			  blockBuffer[2] = (tdata_t) buffer->plane2;
			}
			assert (blockBuffer[0]);
		  }
		  for (int p = 0; p < format->planes; p++)
		  {
			ttile_t tile = TIFFComputeTile (tif, rx, ry, 0, p);
			TIFFReadEncodedTile (tif, tile, blockBuffer[p], blockSize);
		  }
		  image.bitblt (block, ox, oy, ix, iy, w, h);
		}

		ox += w;
	  }

	  oy += h;
	}
  }
  else  // strip organization
  {
	uint32_t rowsPerStrip;
	TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);

	tsize_t blockSize = TIFFStripSize (tif);

	for (int oy = 0; oy < height;)
	{
	  int ry = oy + y;
	  int iy = ry % rowsPerStrip;
	  int h = min ((int) rowsPerStrip - iy, height - oy);

	  if (width == imageWidth  &&  iy == 0)
	  {
		for (int p = 0; p < format->planes; p++)
		{
		  tstrip_t strip = TIFFComputeStrip (tif, ry, p);
		  TIFFReadEncodedStrip (tif, strip, imageMemory[p] + oy * stride[p], h * stride[p]);
		}
	  }
	  else
	  {
		if (! blockBuffer[0])
		{
		  block.resize (imageWidth, rowsPerStrip);
		  if (PixelBufferPacked * buffer = (PixelBufferPacked *) block.buffer)
		  {
			blockBuffer[0] = (tdata_t) buffer->base ();
		  }
		  else if (PixelBufferGroups * buffer = (PixelBufferGroups *) block.buffer)
		  {
			blockBuffer[0] = (tdata_t) buffer->memory;
		  }
		  else if (PixelBufferPlanar * buffer = (PixelBufferPlanar *) block.buffer)
		  {
			blockBuffer[0] = (tdata_t) buffer->plane0;
			blockBuffer[1] = (tdata_t) buffer->plane1;
			blockBuffer[2] = (tdata_t) buffer->plane2;
		  }
		  assert (blockBuffer[0]);
		}
		for (int p = 0; p < format->planes; p++)
		{
		  tstrip_t strip = TIFFComputeStrip (tif, ry, p);
		  TIFFReadEncodedStrip (tif, strip, blockBuffer[p], blockSize);
		}
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
  if (! tif) open ();

  if (x < 0  ||  y < 0) throw "Target coordinates must be non-negative";

  if (format == 0)  // signals need for one-time setup, including other tags besides pixel format
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
  unsigned char * workBuffer = (unsigned char *) buffer->base ();

  uint32_t imageWidth  = 0;
  uint32_t imageHeight = 0;
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
	uint32_t blockWidth = 0;
	uint32_t blockHeight = 0;
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

	int blockStride = (int) roundp (blockWidth * format->depth);
	tsize_t blockSize = blockHeight * blockStride;

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
		  TIFFWriteEncodedTile (tif, tile, workBuffer + iy * buffer->stride, blockSize);
		}
		else
		{
		  if (! blockBuffer)
		  {
			block.resize (blockWidth, blockHeight);
			blockBuffer = (unsigned char *) ((PixelBufferPacked *) block.buffer)->base ();
		  }
		  block.bitblt (work, ox, oy, ix, iy, w, h);
		  fillBlock (blockBuffer, blockStride, (int) format->depth, blockWidth, blockHeight, ox, ox + w, oy, oy + h);
		  TIFFWriteEncodedTile (tif, tile, blockBuffer, blockSize);
		}

		ix += w;
	  }

	  iy += h;
	}
  }
  else  // strip organization
  {
	uint32_t rowsPerStrip = 0;
	TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
	if (! rowsPerStrip)
	{
	  // There given image must be covered by an integral number of strips.
	  const int targetSize = 8192;  // Manual recommends 8K strips
	  int bestDistance = INT_MAX;
	  for (int rows = 1; rows <= work.height / 2; rows++)
	  {
		int size = rows * buffer->stride;
		int distance = abs (size - 8192);
		if (work.height % rows == 0  &&  distance < bestDistance)
		{
		  bestDistance = distance;
		  rowsPerStrip = rows;
		}
		if (rowsPerStrip  &&  size > targetSize) break;  // Since it won't get any better
	  }
	  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
	}

	int blockStride = (int) roundp (imageWidth * format->depth);

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
		TIFFWriteEncodedStrip (tif, strip, workBuffer + iy * buffer->stride, blockSize);
	  }
	  else
	  {
		if (! blockBuffer)
		{
		  block.resize (imageWidth, rowsPerStrip);
		  blockBuffer = (unsigned char *) ((PixelBufferPacked *) block.buffer)->base ();
		}
		block.bitblt (work, x, oy, 0, iy, width, h);
		fillBlock (blockBuffer, blockStride, (int) format->depth, imageWidth, rows, x, x + width, oy, oy + h);
		TIFFWriteEncodedStrip (tif, strip, blockBuffer, blockSize);
	  }

	  iy += h;
	}
  }
}

void
ImageFileDelegateTIFF::get (const string & name, string & value)
{
  if (name == "bigtiff")
  {
	value = bigtiff ? "1" : "0";
	return;
  }

  if (! tif) open ();

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

  if (name == "GeoTransformationMatrix")
  {
	uint16_t count;
	double * v;
	bool found = TIFFGetField (tif, 34264, &count, &v);  // GeoTransformationMatrix
	if (! found)
	{
	  found = TIFFGetField (tif, 33920, &count, &v)  &&  count == 16;  // Intergraph TransformationMatrix
	}
	if (found)
	{
	  (~Matrix<double> (v, 4, 4)).toString (value);
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
		  Matrix<double> temp (4, 4);
		  temp.clear ();
		  temp(0,0) = o[0];
		  temp(1,1) = -o[1];
		  temp(2,2) = o[2];
		  temp(0,3) = v[3] - v[0] * o[0];
		  temp(1,3) = v[4] + v[1] * o[1];
		  temp(2,3) = v[5] - v[2] * o[2];
		  temp(3,3) = 1;
		  temp.toString (value);
		}
	  }
#     ifdef HAVE_LAPACK
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
		  Matrix<double> temp (4, 4);
		  temp.clear ();
		  temp.region (0, 0) = ~X;
		  temp.region (0, 3) = centerXY - temp.region (0, 0, 1, 1) * centerIJ;
		  temp(3,3) = 1;
		  temp.toString (value);
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
		  Matrix<double> temp (4, 4);
		  temp.clear ();
		  temp.region (0, 0) = ~X;
		  temp.region (0, 3) = centerXYZ - temp.region (0, 0, 2, 2) * centerIJK;
		  temp(3,3) = 1;
		  temp.toString (value);
		}
	  }
#     endif
	}

	return;
  }

  const TIFFField * fi = TIFFFieldWithName (tif, name.c_str ());
  if (fi)
  {
	if (TIFFFieldDataType (fi) == TIFF_ASCII)
	{
	  char * v;
	  if (TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &v)) value = v;
	  return;
	}
	if (name == "Compression")
	{
	  // Attempt to look up codec name
	  uint16_t v;
	  if (TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &v))
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
	  return;  // We failed to find a name for the compression scheme, but no need to process further.
	}

	int count = TIFFFieldReadCount (fi);
	TIFFDataType fieldType = TIFFFieldDataType (fi);

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
	  std::ostringstream formatted;
#     define grabSingle(type) \
	  { \
		type v; \
		if (TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &v)) \
		{ \
		  formatted << v; \
		  value = formatted.str (); \
		} \
		return; \
	  }

	  switch (fieldType)
	  {
		case TIFF_BYTE:
		  grabSingle (uint8_t);
		case TIFF_SBYTE:
		  grabSingle (int8_t);
		case TIFF_SHORT:
		  grabSingle (uint16_t);
		case TIFF_SSHORT:
		  grabSingle (int16_t);
		case TIFF_LONG:
		case TIFF_IFD:
		  grabSingle (uint32_t);
		case TIFF_SLONG:
		  grabSingle (int32_t);
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
	if (TIFFFieldReadCount (fi) == TIFF_SPP)
	{
	  uint16_t spp;
	  TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	  rows = spp;
	}
	if (name == "GeoTiePoints") rows = 6;

	void * data;
	bool found = false;
	if (count > 1)
	{
	  found = TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &data);
	}
	else if (TIFFFieldPassCount (fi))
	{
	  if (TIFFFieldReadCount (fi) == TIFF_VARIABLE2)
	  {
		uint32_t c;
		found = TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &c, &data);
		count = c;
	  }
	  else
	  {
		uint16_t c;
		found = TIFFGetFieldDefaulted (tif, TIFFFieldTag (fi), &c, &data);
		count = c;
	  }
	}
	if (! found) return;

	Matrix<double> temp;
	if (rows == 1) temp.resize (count, 1);
	else           temp.resize (rows, count / rows);

	switch (fieldType)
	{
	  case TIFF_BYTE:
		for (int i = 0; i < count; i++) temp[i] = ((uint8_t * ) data)[i];
		break;
	  case TIFF_SBYTE:
		for (int i = 0; i < count; i++) temp[i] = ((int8_t *  ) data)[i];
		break;
	  case TIFF_SHORT:
		for (int i = 0; i < count; i++) temp[i] = ((uint16_t *) data)[i];
		break;
	  case TIFF_SSHORT:
		for (int i = 0; i < count; i++) temp[i] = ((int16_t * ) data)[i];
		break;
	  case TIFF_LONG:
	  case TIFF_IFD:
		for (int i = 0; i < count; i++) temp[i] = ((uint32_t *) data)[i];
		break;
	  case TIFF_SLONG:
		for (int i = 0; i < count; i++) temp[i] = ((int32_t * ) data)[i];
		break;
	  case TIFF_RATIONAL:
	  case TIFF_SRATIONAL:
	  case TIFF_FLOAT:
		for (int i = 0; i < count; i++) temp[i] = ((float * ) data)[i];
		break;
	  case TIFF_DOUBLE:
		for (int i = 0; i < count; i++) temp[i] = ((double *) data)[i];
		break;
	  default:
		return;
	}
	temp.toString (value);
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
		uint16_t v;
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

	return;
  }
# endif

  // Final resort: see if the key is in our custom tag for generic metadata
  if (metadata.namedValues.size () == 0)
  {
	const TIFFField * fi = TIFFFieldWithName (tif, "flMetadata");
	char * v;
	if (TIFFGetField (tif, TIFFFieldTag (fi), &v)) metadata.read (v);
  }
  metadata.get (name, value);
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
  if (name == "bigtiff")
  {
	if (tif) return;  // no longer settable
	bigtiff = ! (value.size () == 0  ||  value == "0"  ||  value == "false");
	return;
  }

  if (! tif) open ();

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

  if (name == "GeoTransformationMatrix")
  {
	Matrix<double> v = ~Matrix<double> (value);
	TIFFSetField (tif, 34264, (uint16_t) 16, &v(0,0));
	return;
  }

  const TIFFField * fi = TIFFFieldWithName (tif, name.c_str ());
  if (fi)
  {
	if (TIFFFieldDataType (fi) == TIFF_ASCII)
	{
	  TIFFSetField (tif, TIFFFieldTag (fi), (char *) value.c_str ());
	  return;
	}
	if (name == "Compression")
	{
	  // Attempt to look up codec name
	  TIFFCodec * codec = TIFFGetConfiguredCODECs ();
	  while (codec->name)
	  {
		if (value == codec->name)
		{
		  TIFFSetField (tif, TIFFFieldTag (fi), codec->scheme);
		  return;
		}
		codec++;
	  }
	  return;
	}

	// If we get this far, then it is a numeric field, not text, so
	// just convert it to a matrix.
	Matrix<double> temp;
	if (value.find_first_of ('[') == string::npos)
	{
	  temp.resize (1, 1);
	  temp(0,0) = atof (value.c_str ());
	}
	else
	{
	  temp = Matrix<double> (value);
	}

	const int count = temp.rows () * temp.columns ();
	if (! count) throw "Empty matrix";

	const void * data = 0;

#   define writeVector(type) \
	data = malloc (count * sizeof (type)); \
	for (int i = 0; i < count; i++) \
	{ \
	  ((type *) data)[i] = (type) roundp (temp[i]); \
	} \
	break;

	switch (TIFFFieldDataType (fi))
	{
	  case TIFF_BYTE:
		writeVector (uint8_t);
	  case TIFF_SBYTE:
		writeVector (int8_t);
	  case TIFF_SHORT:
		writeVector (uint16_t);
	  case TIFF_SSHORT:
		writeVector (int16_t);
	  case TIFF_LONG:
	  case TIFF_IFD:
		writeVector (uint32_t);
	  case TIFF_SLONG:
		writeVector (int32_t);
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
		data = &temp(0,0);
		break;
	}

	if (TIFFFieldPassCount (fi))
	{
	  if (TIFFFieldWriteCount (fi) == TIFF_VARIABLE2)
	  {
		TIFFSetField (tif, TIFFFieldTag (fi), (uint32_t) count, data);
	  }
	  else
	  {
		TIFFSetField (tif, TIFFFieldTag (fi), (uint16_t) count, data);
	  }
	}
	else
	{
	  if (TIFFFieldWriteCount (fi) == 1)
	  {
		switch (TIFFFieldDataType (fi))
		{
		  case TIFF_BYTE:
			TIFFSetField (tif, TIFFFieldTag (fi), *(uint8_t * ) data);
			break;
		  case TIFF_SBYTE:
			TIFFSetField (tif, TIFFFieldTag (fi), *(int8_t *  ) data);
			break;
		  case TIFF_SHORT:
			TIFFSetField (tif, TIFFFieldTag (fi), *(uint16_t *) data);
			break;
		  case TIFF_SSHORT:
			TIFFSetField (tif, TIFFFieldTag (fi), *(int16_t * ) data);
			break;
		  case TIFF_LONG:
		  case TIFF_IFD:
			TIFFSetField (tif, TIFFFieldTag (fi), *(uint32_t *) data);
			break;
		  case TIFF_SLONG:
			TIFFSetField (tif, TIFFFieldTag (fi), *(int32_t * ) data);
			break;
		  case TIFF_RATIONAL:
		  case TIFF_SRATIONAL:
		  case TIFF_FLOAT:
			TIFFSetField (tif, TIFFFieldTag (fi), *(float * ) data);
			break;
		  case TIFF_DOUBLE:
			TIFFSetField (tif, TIFFFieldTag (fi), *(double *) data);
		}
	  }
	  else
	  {
		TIFFSetField (tif, TIFFFieldTag (fi), data);
	  }
	}

	if (data  &&  TIFFFieldDataType (fi) != TIFF_DOUBLE) free ((void *) data);
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
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, (uint16_t) v);
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

	return;
  }
# endif

  // All unrecognized tags get stashed in nameValues
  metadata.set (name, value);
}

tsize_t
ImageFileDelegateTIFF::tiffRead (thandle_t handle, tdata_t data, tsize_t size)
{
  ImageFileDelegateTIFF * me = (ImageFileDelegateTIFF *) handle;
  if (me->out) return 0;
  me->in->read ((char *) data, size);
  return me->in->gcount ();
}

tsize_t
ImageFileDelegateTIFF::tiffWrite (thandle_t handle, tdata_t data, tsize_t size)
{
  ImageFileDelegateTIFF * me = (ImageFileDelegateTIFF *) handle;
  if (me->in) return 0;
  me->out->write ((char *) data, size);
  if (me->out->fail ()) return -1;
  return size;  // If we succeed at all, then it will be a complete write.
}

toff_t
ImageFileDelegateTIFF::tiffSeek (thandle_t handle, toff_t offset, int direction)
{
  ImageFileDelegateTIFF * me = (ImageFileDelegateTIFF *) handle;

  ios::seekdir dir;
  switch (direction)
  {
	case SEEK_CUR:
	  dir = ios::cur;
	  break;
	case SEEK_SET:
	  dir = ios::beg;
	  offset += me->startPosition;
	  break;
	case SEEK_END:
	  dir = ios::end;
	  break;
	default:
	  errno = EINVAL;
	  return (toff_t) -1;
  }

  if (me->in)
  {
	me->in->seekg (offset, dir);
	if (! me->in->fail ()) return (toff_t) me->in->tellg () - me->startPosition;
  }
  else
  {
	me->out->seekp (offset, dir);
	if (! me->out->fail ()) return (toff_t) me->out->tellp () - me->startPosition;
	// String streams will fail to seek past end.  Solution is to zero-fill.
  }

  errno = EINVAL;
  return (toff_t) -1;
}

int
ImageFileDelegateTIFF::tiffClose (thandle_t handle)
{
  return 0;
}

toff_t
ImageFileDelegateTIFF::tiffSize (thandle_t handle)
{
  ImageFileDelegateTIFF * me = (ImageFileDelegateTIFF *) handle;

  if (me->in)
  {
	toff_t position = me->in->tellg ();
	me->in->seekg (0, ios::end);
	toff_t result = (toff_t) me->in->tellg () - me->startPosition;
	me->in->seekg (position);
	return result;
  }
  else
  {
	toff_t position = me->out->tellp ();
	me->out->seekp (0, ios::end);
	toff_t result = (toff_t) me->out->tellp () - me->startPosition;
	me->out->seekp (position);
	return result;
  }
}

int
ImageFileDelegateTIFF::tiffMap (thandle_t handle, tdata_t * data, toff_t * offset)
{
  return 0;  // No-can-do
}

void
ImageFileDelegateTIFF::tiffUnmap (thandle_t handle, tdata_t data, toff_t offset)
{
}

void
ImageFileDelegateTIFF::tiffErrorHandler (const char * module, const char * format, va_list arguments)
{
  if (module  &&  strcmp (module, "TIFFFieldWithName") == 0  &&  strncmp (format, "Internal error, unknown tag", 27) == 0) return;
  if (module) fprintf (stderr, "%s: ", module);
  vfprintf (stderr, format, arguments);
  cerr << endl;
}


// class ImageFileFormatTIFF --------------------------------------------------

void
ImageFileFormatTIFF::use ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (ImageFileFormatTIFF)) return;
  }
  formats.push_back (new ImageFileFormatTIFF);
}

ImageFileFormatTIFF::ImageFileFormatTIFF ()
{
  TIFFSetWarningHandler (0);  // suppress warning messagse
  TIFFSetErrorHandler (ImageFileDelegateTIFF::tiffErrorHandler);  // filter error messages

  // Install our extender
  // Online examples use a boolean flag to guard against multiple installs,
  // but we theoretically don't need one, because ImageFileFormats are
  // singletons and this constructor should only be called once.
  previousExtender = TIFFSetTagExtender (registerCustomTags);

# ifdef HAVE_GEOTIFF
  XTIFFInitialize ();
# endif
}

ImageFileDelegate *
ImageFileFormatTIFF::open (istream & stream, bool ownStream) const
{
  return new ImageFileDelegateTIFF (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatTIFF::open (ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateTIFF (0, &stream, ownStream);
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
	if (magic[2] == '\x00'  &&  (magic[3] == '\x2A'  ||  magic[3] == '\x2B')) return 0.8;
	if (magic[3] == '\x00'  &&  (magic[2] == '\x2A'  ||  magic[2] == '\x2B')) return 1;
  }
  if (magic.substr (0, 2) == "MM")  // big endian
  {
	if (magic[2] == '\x00'  &&  (magic[3] == '\x2A'  ||  magic[3] == '\x2B')) return 1;
	if (magic[3] == '\x00'  &&  (magic[2] == '\x2A'  ||  magic[2] == '\x2B')) return 0.8;
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
