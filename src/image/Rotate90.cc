#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Rotate90 ------------------------------------------------------------

Rotate90::Rotate90 (bool clockwise)
{
  this->clockwise = clockwise;
}

struct triad
{
  unsigned char channel[3];
};

Image
Rotate90::filter (const Image & image)
{
  #define transfer(size) \
  { \
	ImageOf<size> input = image; \
	ImageOf<size> result (image.height, image.width, *image.format); \
	if (clockwise) \
	{ \
	  for (int y = 0; y < result.height; y++) \
	  { \
		for (int x = 0; x < result.width; x++) \
		{ \
		  result (x, y) = input (input.width - y - 1, x); \
		} \
	  } \
	} \
	else \
	{ \
	  for (int y = 0; y < result.height; y++) \
	  { \
		for (int x = 0; x < result.width; x++) \
		{ \
		  result (x, y) = input (y, input.height - x - 1); \
		} \
	  } \
	} \
    return result; \
  }

  switch (image.format->depth)
  {
    case 8:
	  transfer (double);
	  break;
    case 4:
	  transfer (unsigned int);
	  break;
    case 3:
	  transfer (triad);
	  break;
    case 2:
	  transfer (unsigned short);
	  break;
    case 1:
    default:
	  transfer (unsigned char);
  }
}