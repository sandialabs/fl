#include "fl/canvas.h"


using namespace fl;
using namespace std;


// class Canvas ---------------------------------------------------------------

Canvas::~Canvas ()
{
}

void
Canvas::drawDone ()
{
  // Do nothing
}

void
Canvas::drawPoint (const Point & p, unsigned int color)
{
  throw "drawPoint not implemented for this type of Canvas";
}

void
Canvas::drawSegment (const Point & a, const Point & b, unsigned int color)
{
  throw "drawSegment not implemented for this type of Canvas";
}

void
Canvas::drawLine (const Point & a, const Point & b, unsigned int color)
{
  float l1 = b.y - a.y;
  float l2 = a.x - b.x;
  float l3 = - (l1 * a.x + l2 * a.y);
  drawLine (l1, l2, l3, color);
}

void
Canvas::drawLine (float a, float b, float c, unsigned int color)
{
  throw "drawLine not implemented for this type of Canvas";
}

void
Canvas::drawRay (const Point & p, float angle, unsigned int color)
{
  throw "drawRay not implemented for this type of Canvas";
}

void
Canvas::drawPolygon (const std::vector<Point> & points, unsigned int color)
{
  throw "drawPolygon not implemented for this type of Canvas";
}

void
Canvas::drawCircle (const Point & center, float radius, unsigned int color, float startAngle, float endAngle)
{
  Matrix2x2<double> A;
  A.identity ();
  drawEllipse (center, A, radius, color, startAngle, endAngle);
}

void
Canvas::drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius, unsigned int color, float startAngle, float endAngle, bool inverse)
{
  throw "drawEllipse not implemented for this type of Canvas";
}

void
Canvas::drawImage (const Image & image, Point & p, float width, float height)
{
  throw "drawImage not implemented for this type of Canvas";
}

void
Canvas::drawText (const std::string & text, const Point & point, float size, float angle, unsigned int color)
{
  throw "drawText not implemented for this type of Canvas";
}

void
Canvas::setTranslation (float x, float y)
{
  // Do nothing
}

void
Canvas::setScale (float x, float y)
{
  // Do nothing
}

void
Canvas::setLineWidth (float width)
{
  // Do nothing
}

void
Canvas::setPointSize (float radius)
{
  // Do nothing
}
