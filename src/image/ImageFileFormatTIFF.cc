/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/image.h"


#include <tiffio.h>


using namespace std;
using namespace fl;


// class ImageFileFormatTIFF --------------------------------------------------

void
ImageFileFormatTIFF::read (const std::string & fileName, Image & image) const
{
  TIFF * tif = TIFFOpen (fileName.c_str (), "r");
  if (! tif)
  {
	throw "Unable to open file.";
  }

  bool ok = true;

  uint32 w, h;
  ok &= TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
  ok &= TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);

  uint16 samplesPerPixel;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
  uint16 bitsPerSample;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  uint16 sampleFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
  uint16 photometric;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &photometric);
  uint16 orientation;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_ORIENTATION, &orientation);
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
  if (planarConfig != PLANARCONFIG_CONTIG)
  {
	throw "Can't handle planar formats";
  }
  if (extra > 1)
  {
	throw "No PixelFormats currently support more than one channel beyond three colors.";
  }

  PixelFormat * format = 0;
  switch (bitsPerSample)
  {
	case 8:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayChar;
		  break;
		case 3:
		  format = &BGRChar;
		  break;
		case 4:
		  format = &ABGRChar;
	  }
	  break;
	case 16:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayShort;
		  break;
		case 3:
		  format = &RGBShort;  // misnamed format
		  break;
		case 4:
		  format = &RGBAShort;  // misnamed format
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
  image.format = format;
  image.resize (w, h);

  unsigned char * buffer = image.buffer;
  int stride = image.format->depth * w;
  for (int y = 0; y < h; y++)
  {
	TIFFReadScanline (tif, buffer, y);
	buffer += stride;
  }

  TIFFClose(tif);
}

void
ImageFileFormatTIFF::read (std::istream & stream, Image & image) const
{
  throw "Can't read TIFF on stream (limitation of libtiff and standard).";
}

void
ImageFileFormatTIFF::write (const std::string & fileName, const Image & image) const
{
  if (image.format->monochrome)
  {
	if (*image.format != GrayChar  &&  *image.format != GrayShort  &&  *image.format != GrayFloat  &&  *image.format != GrayDouble)
	{
	  write (fileName, image * GrayChar);
	  return;
	}
  }
  else if (image.format->hasAlpha)
  {
	if (*image.format != ABGRChar  &&  *image.format != RGBAShort  &&  *image.format != RGBAFloat)
	{
	  write (fileName, image * ABGRChar);
	  return;
	}
  }
  else  // Three color channels
  {
	if (*image.format != BGRChar  &&  *image.format != RGBShort)
	{
	  write (fileName, image * BGRChar);
	  return;
	}
  }


  TIFF * tif = TIFFOpen (fileName.c_str (), "w");

  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, image.width);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, image.height);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (image.format->monochrome)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	if (*image.format == GrayChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayFloat)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
	else if (*image.format == GrayDouble)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 64);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else if (image.format->hasAlpha)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == ABGRChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == RGBAShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBAFloat
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else  // 3 color channels
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == BGRChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBShort
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
  }

  TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

  unsigned char * buffer = image.buffer;
  int stride = image.format->depth * image.width;
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (int) ceil (8.0 * 1024 / stride));  // Manual recommends 8K strips
  for (int y = 0; y < image.height; y++)
  {
	TIFFWriteScanline (tif, buffer, y, 0);
	buffer += stride;
  }

  TIFFClose (tif);
}

void
ImageFileFormatTIFF::write (std::ostream & stream, const Image & image) const
{
  throw "Can't write TIFF on stream (limitation of libtiff and standard).";

  // Actually, we could write entire TIFF to a temporary file, and then dump
  // to stream.  Read could be modified to interpret the header and directory
  // blocks.  It could then estimate how many bytes to grab from stream.
}

bool
ImageFileFormatTIFF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);

  bool result = magic.substr (0, 2) == "II"  ||  magic.substr (0, 2) == "MM";  // endian indicator
  // Apparently, some implementation don't store the magic number 42 according
  // to the endian indicated by the first two bytes, so we have to handle the
  // wrong order as well.
  result &= (magic[2] == '\x00'  ||  magic[2] == '\x2A')  &&  (magic[3] == '\x2A'  ||  magic[3] == '\x00');

  return result;
}

bool
ImageFileFormatTIFF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "tiff") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "tif") == 0)
  {
	return true;
  }
  return false;
}
