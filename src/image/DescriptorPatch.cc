#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorPatch ------------------------------------------------------

DescriptorPatch::DescriptorPatch (int width, float supportRadial)
{
  this->width = width;
  this->supportRadial = supportRadial;
}

DescriptorPatch::DescriptorPatch (std::istream & stream)
{
  read (stream);
}

DescriptorPatch::~DescriptorPatch ()
{
}

Vector<float>
DescriptorPatch::value (const Image & image, const PointAffine & point)
{
  float half = width / 2.0;

  Matrix2x2<double> R;
  R(0,0) = cos (point.angle);
  R(1,0) = sin (point.angle);
  R(0,1) = -R(1,0);
  R(1,1) = R(0,0);

  TransformGauss reduce (point.A * R / (half / (supportRadial * point.scale)), true);
  reduce.setPeg (point.x, point.y, width, width);
  ImageOf<float> patch = image * GrayFloat * reduce;

  Vector<float> result (patch.buffer, width * width);

  return result;
}

Image
DescriptorPatch::patch (const Vector<float> & value)
{
  Image result (width, width, GrayFloat);
  result.buffer = value.data;

  return result;
}

Comparison *
DescriptorPatch::comparison ()
{
  return new NormalizedCorrelation (true, 11.0f);
}

int
DescriptorPatch::dimension ()
{
  return width * width;
}

void
DescriptorPatch::read (std::istream & stream)
{
  stream.read ((char *) &width, sizeof (width));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
}

void
DescriptorPatch::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
  stream.write ((char *) &width, sizeof (width));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}

