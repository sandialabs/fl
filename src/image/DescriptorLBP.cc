#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"

// for debugging
#include "fl/time.h"


using namespace fl;
using namespace std;


// class DescriptorLBP --------------------------------------------------------

DescriptorLBP::DescriptorLBP (int P, float R, float supportRadial, int supportPixel)
{
  this->P             = P;
  this->R             = R;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;

  initialize ();
}

DescriptorLBP::DescriptorLBP (istream & stream)
{
  read (stream);
}

void
DescriptorLBP::initialize ()
{
  lastImage = 0;

  interpolates.resize (P);
  for (int i = 0; i < P; i++)
  {
	Interpolate & t = interpolates[i];
	float angle = i * 2 * PI / P;
	float xf = R * cosf (angle);
	float yf = R * sinf (angle);
	t.xl = (int) floorf (xf);
	t.yl = (int) floorf (yf);
	xf -= t.xl;
	yf -= t.yl;
	if ((xf < 0.01f  ||  xf > 0.99f)  &&  (yf < 0.01f  ||  yf > 0.99f))
	{
	  t.exact = true;
	  if (xf > 0.5f) t.xl++;
	  if (yf > 0.5f) t.yl++;
	}
	else
	{
	  t.exact = false;
	  t.xh = t.xl + 1;
	  t.yh = t.yl + 1;
	  float xf1 = 1.0f - xf;
	  float yf1 = 1.0f - yf;
	  t.wll = xf1 * yf1;
	  t.wlh = xf1 * yf;
	  t.whl = xf  * yf1;
	  t.whh = xf  * yf;
	}
	//cerr << i << " " << t.exact << " " << t.xl << " " << t.yl << endl;
  }
}

inline void
DescriptorLBP::preprocess (const Image & image)
{
  if (lastImage == &image  &&  lastBuffer == (void *) image.buffer)
  {
	return;
  }
  lastImage = &image;
  lastBuffer = (void *) image.buffer;

  grayImage = image * GrayFloat;
}

inline void
DescriptorLBP::add (const int x, const int y, Vector<float> & result)
{
  vector<bool> bits (P);
  int ones = 0;

  float center = grayImage(x,y);
  for (int i = 0; i < P; i++)
  {
	float p;
	Interpolate & t = interpolates[i];
	if (t.exact)
	{
	  p = grayImage(x + t.xl, y + t.yl);
	}
	else
	{
	  int xl = x + t.xl;
	  int yl = y + t.yl;
	  int xh = x + t.xh;
	  int yh = y + t.yh;
	  p = grayImage(xl,yl) * t.wll + grayImage(xh,yl) * t.whl + grayImage(xl,yh) * t.wlh + grayImage(xh,yh) * t.whh;
	}
	bool sign = p >= center;
	bits[i] = sign;
	if (sign) ones++;
  }

  int transitions = bits.back () == bits.front () ? 0 : 1;
  for (int i = 1; i < P; i++)
  {
	if (bits[i-1] != bits[i])
	{
	  transitions++;
	}
  }

  if (transitions > 2)
  {
	ones = P + 1;
  }

  result[ones]++;
}

Vector<float>
DescriptorLBP::value (const Image & image, const PointAffine & point)
{
  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  if (S(0,1) == 0  &&  S(1,0) == 0)  // Special case: indicates that point describes a rectangular region in the image, rather than a patch.
  {
	double h = fabs (S(0,0) * supportRadial);
	double v = fabs (S(1,1) * supportRadial);
	sourceL = (int) rint (S(0,2) - h >? R);
	sourceR = (int) rint (S(0,2) + h <? image.width - 1 - R);
	sourceT = (int) rint (S(1,2) - v >? R);
	sourceB = (int) rint (S(1,2) + v <? image.height - 1 - R);
	preprocess (image);
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (S, scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * t;
	preprocess (patch);

	sourceT = (int) ceilf (R);
	sourceL = sourceT;
	sourceB = (int) floorf (patchSize - 1 - R);
	sourceR = sourceB;
  }

  // Gather LBP values into histogram
  Vector<float> result (P + 2);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  add (x, y, result);
	}
  }
  result /= result.frob (1);
  return result;
}

Vector<float>
DescriptorLBP::value (const Image & image)
{
  double start = getTimestamp ();
  preprocess (image);
  int sourceL = (int) ceilf (R);
  int sourceR = (int) floorf (image.width - 1 - R);
  int sourceT = sourceL;
  int sourceB = (int) floorf (image.height - 1 - R);
  cerr << sourceL << " " << sourceR << " " << sourceT << " " << sourceB << endl;
  Vector<float> result (P + 2);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  if (image.getAlpha (x, y))
	  {
		add (x, y, result);
	  }
	}
  }
  cerr << "before normalization = " << result << " = " << result.frob (1) << endl;
  result /= result.frob (1);
  cerr << "time = " << getTimestamp () - start << endl;
  return result;
}

Image
DescriptorLBP::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorLBP::comparison ()
{
  return new ChiSquared;
}

int
DescriptorLBP::dimension ()
{
  return P + 2;
}

void
DescriptorLBP::read (std::istream & stream)
{
  stream.read ((char *) &P,             sizeof (P));
  stream.read ((char *) &R,             sizeof (R));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));

  initialize ();
}

void
DescriptorLBP::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &P,             sizeof (P));
  stream.write ((char *) &R,             sizeof (R));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
}
