#include "fl/descriptor.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class Comparison -----------------------------------------------------------

Factory<Comparison>::productMap Factory<Comparison>::products;

Comparison::~Comparison ()
{
}

Vector<float>
Comparison::preprocess (const Vector<float> & value) const
{
  return Vector<float> (value);
}

void
Comparison::read (istream & stream)
{
}

void
Comparison::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}

/**
   Since at present all Comparisons (other than ComparisonCombo) are
   lightweight classes implemented in this single source file, it makes sense
   to just register them all at once.  Another alternative would be to hard
   code a factory just for Comparisons.  However, the Factory template is
   more flexible.
 **/
void
Comparison::addProducts ()
{
  Product<Comparison, NormalizedCorrelation>::add ();
  Product<Comparison, MetricEuclidean>::add ();
  Product<Comparison, HistogramIntersection>::add ();
  Product<Comparison, ChiSquared>::add ();
}


// class NormalizedCorrelation ------------------------------------------------

NormalizedCorrelation::NormalizedCorrelation (bool subtractMean)
{
  this->subtractMean = subtractMean;
}

NormalizedCorrelation::NormalizedCorrelation (istream & stream)
{
  read (stream);
}

Vector<float>
NormalizedCorrelation::preprocess (const Vector<float> & value) const
{
  if (subtractMean)
  {
	const int n = value.rows ();
	Vector<float> result (n);
	float norm = 0;
	float mean = value.frob (1) / n;
	for (int r = 0; r < n; r++)
	{
	  float t = value[r] - mean;
	  result[r] = t;
	  norm += t * t;
	}
	result /= sqrtf (norm);
	return result;
  }
  else
  {
	return value / value.frob (2);
  }
}

float
NormalizedCorrelation::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result;
  if (preprocessed)
  {
	result = value1.dot (value2);
  }
  else if (subtractMean)
  {
	const int n = value1.rows ();

	Vector<float> v1 (n);
	Vector<float> v2 (n);

	float mean1 = value1.frob (1) / n;
	float mean2 = value2.frob (1) / n;

	float norm1 = 0;
	float norm2 = 0;
	for (int r = 0; r < n; r++)
	{
	  float t1 = value1[r] - mean1;
	  float t2 = value2[r] - mean2;
	  v1[r] = t1;
	  v2[r] = t2;
	  norm1 += t1 * t1;
	  norm2 += t2 * t2;
	}
	norm1 = sqrt (norm1);
	norm2 = sqrt (norm2);
	result = v1.dot (v2) / (norm1 * norm2);
  }
  else
  {
	result = value1.dot (value2) / (value1.frob (2) * value2.frob (2));
  }

  result = max (0.0f, result);  // Default mode is to consider everything below zero as zero probability.

  return result;
}

void
NormalizedCorrelation::read (istream & stream)
{
  stream.read ((char *) &subtractMean, sizeof (subtractMean));
}

void
NormalizedCorrelation::write (ostream & stream, bool withName)
{
  Comparison::write (stream, withName);
  stream.write ((char *) &subtractMean, sizeof (subtractMean));
}


// class MetricEuclidean ------------------------------------------------------

MetricEuclidean::MetricEuclidean (istream & stream)
{
  read (stream);
}

float
MetricEuclidean::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  return 1.0f / coshf ((value1 - value2).frob (2));
}


// class HistogramIntersection ------------------------------------------------

HistogramIntersection::HistogramIntersection (istream & stream)
{
  read (stream);
}

float
HistogramIntersection::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result = 0;
  const int m = value1.rows ();
  int count1 = 0;
  int count2 = 0;
  for (int i = 0; i < m; i++)
  {
	float minimum = min (value1[i], value2[i]);
	float maximum = max (value1[i], value2[i]);
	if (maximum != 0.0f)
	{
	  result += minimum / maximum;
	}
	if (value1[i] >= 0)
	{
	  count1++;
	}
	if (value2[i] >= 0)
	{
	  count2++;
	}
  }
  result /= max (count1, count2);
  return result;
}


// class ChiSquared -----------------------------------------------------------

ChiSquared::ChiSquared (istream & stream)
{
  read (stream);
}

float
ChiSquared::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result = 0;
  const int m = value1.rows ();
  for (int i = 0; i < m; i++)
  {
	float sum = value1[i] + value2[i];
	if (sum != 0.0f)
	{
	  float d = value1[i] - value2[i];
	  result += d * d / sum;
	}
  }
  //result = (m - result) / m;
  //return result;
  return 1.0f / coshf (result * 100.0f / m);  // scale to make all lengths of feature vectors come out as if they have 100 elements.
}
