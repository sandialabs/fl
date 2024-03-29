/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/random.h"


using namespace std;
using namespace fl;


// class DescriptorSchmidScale ------------------------------------------------

DescriptorSchmidScale::DescriptorSchmidScale (float sigma)
{
  this->sigma = sigma;
  initialize ();
}

void
DescriptorSchmidScale::initialize ()
{
  G = Gaussian2D (sigma);

  Gx = GaussianDerivativeFirst (0, sigma);
  Gy = GaussianDerivativeFirst (1, sigma);

  Gxx = GaussianDerivativeSecond (0, 0, sigma);
  Gxy = GaussianDerivativeSecond (0, 1, sigma);
  Gyy = GaussianDerivativeSecond (1, 1, sigma);

  Gxxx = GaussianDerivativeThird (0, 0, 0, sigma);
  Gxxy = GaussianDerivativeThird (0, 0, 1, sigma);
  Gxyy = GaussianDerivativeThird (0, 1, 1, sigma);
  Gyyy = GaussianDerivativeThird (1, 1, 1, sigma);

  // Normalize scales: one sigma per level of derivation
  Gx *= sigma;
  Gy *= sigma;

  float sigma2 = sigma * sigma;
  Gxx *= sigma2;
  Gxy *= sigma2;
  Gyy *= sigma2;

  float sigma3 = sigma2 * sigma;
  Gxxx *= sigma3;
  Gxxy *= sigma3;
  Gxyy *= sigma3;
  Gyyy *= sigma3;
}

DescriptorSchmidScale::~DescriptorSchmidScale ()
{
}

Vector<float>
DescriptorSchmidScale::value (ImageCache & cache, const PointAffine & point)
{
  Image image = cache.get (new EntryPyramid (GrayFloat))->image;

  Vector<float> result (9);

  float L = G.response (image, point);
  float Ld[2];
  Ld[0] = Gx.response (image, point);
  Ld[1] = Gy.response (image, point);
  float Ldd[2][2];
  Ldd[0][0] = Gxx.response (image, point);
  Ldd[0][1] = Gxy.response (image, point);
  Ldd[1][0] = Ldd[0][1];
  Ldd[1][1] = Gyy.response (image, point);
  float Lddd[2][2][2];
  Lddd[0][0][0] = Gxxx.response (image, point);
  Lddd[0][0][1] = Gxxy.response (image, point);
  Lddd[0][1][0] = Lddd[0][0][1];
  Lddd[0][1][1] = Gxyy.response (image, point);
  Lddd[1][0][0] = Lddd[0][0][1];
  Lddd[1][0][1] = Lddd[0][1][1];
  Lddd[1][1][0] = Lddd[0][1][1];
  Lddd[1][1][1] = Gyyy.response (image, point);

  float e[2][2];
  e[0][0] = 0;
  e[0][1] = 1;
  e[1][0] = -1;
  e[1][1] = 0;

  result[0] = L;
  for (int i = 0; i < 2; i++)
  {
	result[1] += Ld[i] * Ld[i];
	result[3] += Ldd[i][i];
	for (int j = 0; j < 2; j++)
	{
	  result[2] += Ld[i] * Ldd[i][j] * Ld[j];
	  result[4] += Ldd[i][j] * Ldd[j][i];
	  for (int k = 0; k < 2; k++)
	  {
		result[6] += Lddd[i][i][j] * Ld[j] * Ld[k] * Ld[k] - Lddd[i][j][k] * Ld[i] * Ld[j] * Ld[k];
		result[8] += Lddd[i][j][k] * Ld[i] * Ld[j] * Ld[k];
		for (int l = 0; l < 2; l++)
		{
		  result[5] += e[i][j] * (Lddd[j][k][l] * Ld[i] * Ld[k] * Ld[l] - Lddd[j][k][k] * Ld[i] * Ld[l] * Ld[l]);
		  result[7] -= e[i][j] * Lddd[j][k][l] * Ld[i] * Ld[k] * Ld[l];
		}
	  }
	}
  }

  return result;
}

Image
DescriptorSchmidScale::patch (const Vector<float> & value)
{
  // This code is untested!
  // Replace all this code with a Search

  Image result (G.width, G.height, GrayFloat);

  Point center;
  center.x = result.width / 2;
  center.y = result.height / 2;

  int dimension = result.width * result.height;
  int levels = 10;
  float freezing =  1.0 / pow (2.0, levels);
  float temperature = 1.0;
  int patience = dimension / levels;
  int gotBetter = 0;
  int gotWorse = 0;
  float lastDistance = 1e30f;
  while (temperature > freezing)
  {
	// Generate a guess
	ImageOf<float> guess (result.width, result.height, GrayFloat);
	for (int x = 0; x < guess.width; x++)
	{
	  for (int y = 0; y < guess.height; y++)
	  {
		guess (x, y) = randGaussian ();
	  }
	}
	guess *= Normalize ();
	guess *= temperature;
	guess = result + guess;

	// Evaluate distance from guess to value
	Vector<float> guessValue = this->value (guess, center);
	guessValue = guessValue - value;
	float distance = guessValue.norm (1);  // should this be norm (2) instead?

	// If improved, keep guess
	if (distance <= lastDistance)
	{
	  gotBetter++;
	  gotWorse = 0;
	  result = guess;
	  lastDistance = distance;
	}
	else
	{
	  gotWorse++;
	  gotBetter = 0;
	}

	// Adjust temperature
	if (gotWorse > patience)
	{
	  temperature /= 2.0;
	  gotWorse = 0;
	}
	if (gotBetter > patience)
	{
	  temperature *= 2.0;
	  gotBetter = 0;
	}
  }

  return result;
}

int
DescriptorSchmidScale::dimension ()
{
  return 9;
}

void
DescriptorSchmidScale::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & sigma;

  if (archive.in) initialize ();
}
