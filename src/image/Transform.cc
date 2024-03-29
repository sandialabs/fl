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


#include "fl/convolve.h"
#include "fl/lapack.h"


using namespace std;
using namespace fl;


// class Transform ------------------------------------------------------------

Transform::Transform (const Matrix<double> & A, bool inverse)
{
  initialize (A, inverse);
}

Transform::Transform (const Matrix<double> & IA, const double scale)
{
  Matrix<double> temp (3, 3);
  temp.identity ();
  int r = min (2, IA.rows () - 1);
  int c = min (2, IA.columns () - 1);
  temp.region (0, 0) = IA.region (0, 0, r, c);
  temp.region (0, 0, 2, 1) /= scale;

  initialize (temp, true);
}

Transform::Transform (double angle)
{
  Matrix<double> temp (2, 2);
  temp(0,0) = cos (angle);
  temp(1,0) = sin (angle);
  temp(0,1) = -temp(1,0);
  temp(1,1) = temp(0,0);

  initialize (temp, false);
}

Transform::Transform (double scaleX, double scaleY)
{
  Matrix<double> temp (2, 2);
  temp(0,0) = scaleX;
  temp(0,1) = 0;
  temp(1,0) = 0;
  temp(1,1) = scaleY;

  initialize (temp, false);
}

void
Transform::initialize (const Matrix<double> & A, bool inverse)
{
  MatrixFixed<double,3,3> temp;
  temp.identity ();
  int r = min (2, A.rows () - 1);
  int c = min (2, A.columns () - 1);
  temp.region (0, 0) = A.region (0, 0, r, c);
  temp /= temp(2,2);

  this->inverse = inverse;
  if (inverse)
  {
	IA      =  temp;
	this->A = !temp;
	this->A /= this->A(2,2);
  }
  else
  {
	this->A =  temp;
	IA      = !temp;
	IA /= IA(2,2);
  }

  defaultViewport = true;
}

Image
Transform::filter (const Image & image)
{
  if (image.format->monochrome)
  {
	if (*image.format != GrayFloat  &&  *image.format != GrayDouble)
	{
	  return filter (image * GrayFloat);
	}
  }
  else  // color format
  {
	if (*image.format != RGBAFloat)
	{
	  return filter (image * RGBAFloat);
	}
  }

  MatrixFixed<double,3,3> H;  // homography from destination image to source image
  int w;
  int h;
  int lo;
  int hi;
  prepareResult (image, w, h, H, lo, hi);

  const int    iLastX1 = image.width  - 1;
  const int    iLastY1 = image.height - 1;
  const double lastX5  = image.width  - 0.5;
  const double lastY5  = image.height - 0.5;

  const double H00 = H(0,0);
  const double H10 = H(1,0);
  const double H20 = H(2,0);
  const double H01 = H(0,1);
  const double H11 = H(1,1);
  const double H21 = H(2,1);
  const double H02 = H(0,2);
  const double H12 = H(1,2);
  // no need for H22 because it is guaranteed to be 1 by prepareResult()

  // One row + one pixel before beginning of destination image
  double tx = -H00 - H01 + H02;
  double ty = -H10 - H11 + H12;
  double tz = -H20 - H21 + 1.0;

  // There are two versions of the transformation code.  One handles all
  // 8 degrees of freedom of a homography, and the other is optimized for
  // the case when the homography is a pure affine transformation (6-DOF).

  if (tz != 1.0)  // full 8-DOF homography
  {
	if (*image.format == GrayFloat)
	{
	  ImageOf<float> result (w, h, GrayFloat);
	  ImageOf<float> that (image);
	  float * r = (float *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		// There are two versions of the inner loop, depending on whether we will
		// encounter the edge of the image during this row or not.
		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) cx;  // cast to int should always be equivalent to floor()
			int fromY = (int) cy;
			float * p00 = & that(fromX,fromY);
			float * p01 = p00 + 1;
			float * p10 = p00 + that.width;
			float * p11 = p10 + 1;
			float dx = cx - fromX;
			float dy = cy - fromY;
			float a = *p00 + dx * (*p01 - *p00);
			float b = *p10 + dx * (*p11 - *p10);
			*r++ = a + dy * (b - a);
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			if (cx >= -0.5  &&  cx < lastX5  &&  cy >= -0.5  &&  cy < lastY5)
			{
			  int fromX = (int) cx;
			  int fromY = (int) cy;
			  float * p00 = & that(fromX,fromY);
			  float * p01 = p00 + 1;
			  float * p10 = p00 + that.width;
			  float * p11 = p10 + 1;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  float dx = cx - fromX;
			  float dy = cy - fromY;
			  float a = *p00 + dx * (*p01 - *p00);
			  float b = *p10 + dx * (*p11 - *p10);
			  *r++ = a + dy * (b - a);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == GrayDouble)
	{
	  ImageOf<double> result (w, h, GrayDouble);
	  ImageOf<double> that (image);
	  double * r = (double *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) cx;
			int fromY = (int) cy;
			double * p00 = & that(fromX,fromY);
			double * p01 = p00 + 1;
			double * p10 = p00 + that.width;
			double * p11 = p10 + 1;
			double dx = cx - fromX;
			double dy = cy - fromY;
			double a = *p00 + dx * (*p01 - *p00);
			double b = *p10 + dx * (*p11 - *p10);
			*r++ = a + dy * (b - a);
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			if (cx >= -0.5  &&  cx < lastX5  &&  cy >= -0.5  &&  cy < lastY5)
			{
			  int fromX = (int) cx;
			  int fromY = (int) cy;
			  double * p00 = & that(fromX,fromY);
			  double * p01 = p00 + 1;
			  double * p10 = p00 + that.width;
			  double * p11 = p10 + 1;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  double dx = cx - fromX;
			  double dy = cy - fromY;
			  double a = *p00 + dx * (*p01 - *p00);
			  double b = *p10 + dx * (*p11 - *p10);
			  *r++ = a + dy * (b - a);
			}
			else
			{
			  *r++ = 0.0;
			}
		  }
		}
	  }
	  return result;
	}
	else  // RGBAFloat  (guaranteed by format check at start of this function)
	{
	  ImageOf<float[4]> result (w, h, RGBAFloat);
	  ImageOf<float[4]> that (image);
	  float * r = result (0, 0);

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) cx;
			int fromY = (int) cy;
			float * p00 = that(fromX,fromY);
			float * p01 = p00 + 4;
			float * p10 = p00 + that.width * 4;
			float * p11 = p10 + 4;
			float dx = cx - fromX;
			float dy = cy - fromY;
			float dx1 = 1.0f - dx;
			float dy1 = 1.0f - dy;
			float d00 = dx1 * dy1;
			float d01 = dx  * dy1;
			float d10 = dx1 * dy;
			float d11 = dx  * dy;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00   * d00 + *p01   * d01 + *p10   * d10 + *p11   * d11;  // Don't advance p## pointers on last step, because uneeded.  However, r must advance.
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			if (cx >= -0.5  &&  cx < lastX5  &&  cy >= -0.5  &&  cy < lastY5)
			{
			  int fromX = (int) cx;
			  int fromY = (int) cy;
			  float * p00 = that(fromX,fromY);
			  float * p01 = p00 + 4;
			  float * p10 = p00 + that.width * 4;
			  float * p11 = p10 + 4;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  float dx = cx - fromX;
			  float dy = cy - fromY;
			  float dx1 = 1.0f - dx;
			  float dy1 = 1.0f - dy;
			  float d00 = dx1 * dy1;
			  float d01 = dx  * dy1;
			  float d10 = dx1 * dy;
			  float d11 = dx  * dy;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00   * d00 + *p01   * d01 + *p10   * d10 + *p11   * d11;
			}
			else
			{
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
  }
  else  // tz == 1.0, meaning only 6-DOF homography, allowing slightly fewer computations for coordinates
  {
	if (*image.format == GrayFloat)
	{
	  ImageOf<float> result (w, h, GrayFloat);
	  ImageOf<float> that (image);
	  float * r = (float *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) x;
			int fromY = (int) y;
			float * p00 = & that(fromX,fromY);
			float * p01 = p00 + 1;
			float * p10 = p00 + that.width;
			float * p11 = p10 + 1;
			float dx = x - fromX;
			float dy = y - fromY;
			float a = *p00 + dx * (*p01 - *p00);
			float b = *p10 + dx * (*p11 - *p10);
			*r++ = a + dy * (b - a);
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			if (x >= -0.5  &&  x < lastX5  &&  y >= -0.5  &&  y < lastY5)
			{
			  int fromX = (int) x;
			  int fromY = (int) y;
			  float * p00 = & that(fromX,fromY);
			  float * p01 = p00 + 1;
			  float * p10 = p00 + that.width;
			  float * p11 = p10 + 1;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  float dx = x - fromX;
			  float dy = y - fromY;
			  float a = *p00 + dx * (*p01 - *p00);
			  float b = *p10 + dx * (*p11 - *p10);
			  *r++ = a + dy * (b - a);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == GrayDouble)
	{
	  ImageOf<double> result (w, h, GrayDouble);
	  ImageOf<double> that (image);
	  double * r = (double *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) x;
			int fromY = (int) y;
			double * p00 = & that(fromX,fromY);
			double * p01 = p00 + 1;
			double * p10 = p00 + that.width;
			double * p11 = p10 + 1;
			double dx = x - fromX;
			double dy = y - fromY;
			double a = *p00 + dx * (*p01 - *p00);
			double b = *p10 + dx * (*p11 - *p10);
			*r++ = a + dy * (b - a);
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			if (x >= -0.5  &&  x < lastX5  &&  y >= -0.5  &&  y < lastY5)
			{
			  int fromX = (int) x;
			  int fromY = (int) y;
			  double * p00 = & that(fromX,fromY);
			  double * p01 = p00 + 1;
			  double * p10 = p00 + that.width;
			  double * p11 = p10 + 1;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  double dx = x - fromX;
			  double dy = y - fromY;
			  double a = *p00 + dx * (*p01 - *p00);
			  double b = *p10 + dx * (*p11 - *p10);
			  *r++ = a + dy * (b - a);
			}
			else
			{
			  *r++ = 0.0;
			}
		  }
		}
	  }
	  return result;
	}
	else  // RGBAFloat  (guaranteed by format check at start of this function)
	{
	  ImageOf<float[4]> result (w, h, RGBAFloat);
	  ImageOf<float[4]> that (image);
	  float * r = result (0, 0);

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) x;
			int fromY = (int) y;
			float * p00 = that(fromX,fromY);
			float * p01 = p00 + 4;
			float * p10 = p00 + that.width * 4;
			float * p11 = p10 + 4;
			float dx = x - fromX;
			float dy = y - fromY;
			float dx1 = 1.0f - dx;
			float dy1 = 1.0f - dy;
			float d00 = dx1 * dy1;
			float d01 = dx  * dy1;
			float d10 = dx1 * dy;
			float d11 = dx  * dy;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			*r++ = *p00   * d00 + *p01   * d01 + *p10   * d10 + *p11   * d11;  // Don't advance p## pointers on last step, because uneeded.  However, r must advance.
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			if (x >= -0.5  &&  x < lastX5  &&  y >= -0.5  &&  y < lastY5)
			{
			  int fromX = (int) x;
			  int fromY = (int) y;
			  float * p00 = that(fromX,fromY);
			  float * p01 = p00 + 4;
			  float * p10 = p00 + that.width * 4;
			  float * p11 = p10 + 4;
			  if (x < 0  ||  fromX == iLastX1)
			  {
				p01 = p00;
				p11 = p10;
			  }
			  if (y < 0  ||  fromY == iLastY1)
			  {
				p10 = p00;
				p11 = p01;
			  }
			  float dx = x - fromX;
			  float dy = y - fromY;
			  float dx1 = 1.0f - dx;
			  float dy1 = 1.0f - dy;
			  float d00 = dx1 * dy1;
			  float d01 = dx  * dy1;
			  float d10 = dx1 * dy;
			  float d11 = dx  * dy;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00++ * d00 + *p01++ * d01 + *p10++ * d10 + *p11++ * d11;
			  *r++ = *p00   * d00 + *p01   * d01 + *p10   * d10 + *p11   * d11;
			}
			else
			{
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
  }
}

/**
   Set up viewport (of resulting image) so its center hits at a specified
   point in source image.
   \param centerX If NAN, then use center of original image.
   \param centerY If NAN, then use center of original image.
   \param width   If <= 0, then use width of original image.
   \param height  If <= 0, then use width of original image.
**/
void
Transform::setPeg (float centerX, float centerY, int width, int height)
{
  peg = true;
  defaultViewport = false;

  this->centerX = centerX;
  this->centerY = centerY;
  this->width   = width;
  this->height  = height;
}

/**
   Set up viewport so its center hits a specified point in what would
   otherwise be the resulting image.
 **/
void
Transform::setWindow (float centerX, float centerY, int width, int height)
{
  peg = false;
  defaultViewport = false;

  this->centerX = centerX;
  this->centerY = centerY;
  this->width   = width;
  this->height  = height;
}

/**
   Set up viewport in destination image in terms of its edges.
 **/
void
Transform::setWindowEdges (int left, int top, int right, int bottom)
{
  peg = false;
  defaultViewport = false;

  centerX = (left + right) / 2.0;
  centerY = (top + bottom) / 2.0;
  width  = right  - left + 1;
  height = bottom - top  + 1;
}

Transform
Transform::operator * (const Transform & that) const
{
  if (! inverse  &&  ! that.inverse)
  {
	return Transform (A * that.A, false);
  }
  else
  {
	return Transform (that.IA * IA, true);
  }
}

inline void
Transform::twistCorner (const double inx, const double iny, double & l, double & r, double & t, double & b)
{
  double outz =  A(2,0) * inx + A(2,1) * iny + A(2,2);
  if (outz <= 0.0f) throw "Negative scale factor.  Image too large or homography too distorting.";
  double outx = (A(0,0) * inx + A(0,1) * iny + A(0,2)) / outz;
  double outy = (A(1,0) * inx + A(1,1) * iny + A(1,2)) / outz;
  l = min (l, outx);
  r = max (r, outx);
  t = min (t, outy);
  b = max (b, outy);
}

/**
   Subroutine of prepareResult() which helps find range of destination image
   rows that don't require bounds checking because they fall fully in the
   source image.
   (dx0,dy0) should be associated with the point on y=0 (that is, at the top)
   in the destination image.
   (sx1,sy1) - (sx0,sy0) should define a vector whose positive (that is, right)
   side is the inside of the image.
 **/
inline void
Transform::clip (const double dx0, const double dy0, const double dx1, const double dy1,
				 const double sx0, const double sy0, const double sx1, const double sy1,
				 const bool open,
				 double & dLo, double & dHi, bool & openLo, bool & openHi)
{
  double sx   = sx1 - sx0;
  double sy   = sy1 - sy0;
  double sdx0 = dx0 - sx0;
  double sdy0 = dy0 - sy0;
  double sdx1 = dx1 - sx0;
  double sdy1 = dy1 - sy0;
  double det0 = sx * sdy0 - sy * sdx0;
  double det1 = sx * sdy1 - sy * sdx1;
  bool inside0 = open ? det0 > 0 : det0 >= 0;
  bool inside1 = open ? det1 > 0 : det1 >= 0;
  if (inside0  &&  inside1) return;  // Segment is entirely inside image, so don't touch dLo or dHi.
  if (! inside0  &&  ! inside1)
  {
	// Force an empty interval
	dHi = -1;
	dLo = 2;
	return;
  }

  double dx    = dx1 - dx0;
  double dy    = dy1 - dy0;
  double denom = dy * sx - dx * sy;
  // Don't worry about parallel lines (denom == 0), since already determined that endpoints of destination edge are on opposite sides of source edge.
  double t = -det0 / denom;
  if (inside0)  // &&  ! inside1
  {
	if (dHi > t)
	{
	  dHi = t;
	  openHi = open;
	}
	else if (t - dHi < 1e-6)
	{
	  openHi |= open;
	}
  }
  else  // ! inside0  &&  inside1
  {
	if (dLo < t)
	{
	  dLo = t;
	  openLo = open;
	}
	else if (dLo - t < 1e-6)
	{
	  openLo |= open;
	}
  }
}

void
Transform::prepareResult (const Image & image, int & w, int & h, MatrixFixed<double,3,3> & C, int & lo, int & hi)
{
  if (defaultViewport)
  {
	double l = INFINITY;
	double r = -INFINITY;
	double t = INFINITY;
	double b = -INFINITY;

	twistCorner (-0.5,                -0.5,                 l, r, t, b);  // Upper  left  corner
	twistCorner ((image.width - 0.5), -0.5,                 l, r, t, b);  // Upper  right corner
	twistCorner (-0.5,                (image.height - 0.5), l, r, t, b);  // Bottom left  corner
	twistCorner ((image.width - 0.5), (image.height - 0.5), l, r, t, b);  // Bottom right corner

	peg = false;
	centerX = (l + r) / 2.0f;
	centerY = (t + b) / 2.0f;
	width = (int) ceil (r - l);
	height = (int) ceil (b - t);
  }

  w = width  <= 0 ? image.width  : width;
  h = height <= 0 ? image.height : height;

  Vector<double> cd (3);
  cd[2] = 1.0f;
  if (peg)
  {
	// Use cd as temporary storage for source image center.
	cd[0] = isnan (centerX) ? (image.width - 1)  / 2.0 : centerX;
	cd[1] = isnan (centerY) ? (image.height - 1) / 2.0 : centerY;

	// Transform center of source image into a point in virtual destination image.
	cd = A * cd;
	cd /= cd[2];
  }
  else
  {
	cd[0] = centerX;
	cd[1] = centerY;
  }

  // Combine center of real destination image with virtual destination point.
  cd[0] -= (w - 1) / 2.0;
  cd[1] -= (h - 1) / 2.0;

  // Use cd to construct C
  C = IA;  // since Matrix3x3 stores its data directly, this is a deep copy
  C.column (2) = IA * cd;
  C /= C(2,2);  // guarantee that C(2,2) == 1, so we can ommit it from calculations


  // Compute bounds where rows of source pixels are completely within image.
  double dLo = 0.0;
  double dHi = 1.0;
  bool openLo = false;
  bool openHi = false;

  //   Project destination end-points into source image
  double lx0 = C(0,2);
  double ly0 = C(1,2);

  double h1 = h - 1;
  double lx1 = C(0,1) * h1 + C(0,2);
  double ly1 = C(1,1) * h1 + C(1,2);
  double lz1 = C(2,1) * h1 + 1.0;
  lx1 /= lz1;
  ly1 /= lz1;

  double w1 = w - 1;
  double rx0 = C(0,0) * w1 + C(0,2);
  double ry0 = C(1,0) * w1 + C(1,2);
  double rz0 = C(2,0) * w1 + 1.0;
  rx0 /= rz0;
  ry0 /= rz0;

  double rx1 = C(0,0) * w1 + C(0,1) * h1 + C(0,2);
  double ry1 = C(1,0) * w1 + C(1,1) * h1 + C(1,2);
  double rz1 = C(2,0) * w1 + C(2,1) * h1 + 1.0;
  rx1 /= rz1;
  ry1 /= rz1;

  //   Clip test left and right destination edges vs. all 4 source edges
  double lastX = image.width - 1;
  double lastY = image.height - 1;
  clip (lx0, ly0, lx1, ly1, 0,     0,     lastX, 0,     false, dLo, dHi, openLo, openHi);
  clip (lx0, ly0, lx1, ly1, lastX, 0,     lastX, lastY, true,  dLo, dHi, openLo, openHi);
  clip (lx0, ly0, lx1, ly1, lastX, lastY, 0,     lastY, true,  dLo, dHi, openLo, openHi);
  clip (lx0, ly0, lx1, ly1, 0,     lastY, 0,     0,     false, dLo, dHi, openLo, openHi);
  clip (rx0, ry0, rx1, ry1, 0,     0,     lastX, 0,     false, dLo, dHi, openLo, openHi);
  clip (rx0, ry0, rx1, ry1, lastX, 0,     lastX, lastY, true,  dLo, dHi, openLo, openHi);
  clip (rx0, ry0, rx1, ry1, lastX, lastY, 0,     lastY, true,  dLo, dHi, openLo, openHi);
  clip (rx0, ry0, rx1, ry1, 0,     lastY, 0,     0,     false, dLo, dHi, openLo, openHi);

  //   Set lo and hi based on parametric values dLo and dHi
  if (h1 > 0)  // guard against height <= 1
  {
	dLo *= h1;
	dHi *= h1;
  }
  double iLo = ceil (dLo);
  double iHi = floor (dHi);
  if (openLo  &&  iLo - dLo < 1e-6) iLo++;
  if (openHi  &&  dHi - iHi < 1e-6) iHi--;
  lo = (int) iLo;
  hi = (int) iHi;
}
