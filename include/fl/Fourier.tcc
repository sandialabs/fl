/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_fourier_tcc
#define fl_fourier_tcc


#include "fl/fourier.h"


namespace fl
{
  /**
	 Count the number of bits with value 0 that are less significant than the
	 least-significant bit with value 1 in the input word.
	 This code came from http://graphics.stanford.edu/~seander/bithacks.html.
	 That page states at the top that individual code fragments such as this
	 one are in the public domain.
  **/
  inline int
  trailingZeros (const uint32_t & a)
  {
	static const int Mod37BitPosition[] = // map a bit value mod 37 to its position
	{
	  32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
	  7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
	  20, 8, 19, 18
	};
	return Mod37BitPosition[(-a & a) % 37];
  }

  template<class T>
  Fourier<T>::Fourier (bool normalize, bool destroyInput, bool sizeFromOutput)
  : normalize (normalize),
	destroyInput (destroyInput),
	sizeFromOutput (sizeFromOutput)
  {
	cachedPlan = 0;
  }

  template<class T>
  Fourier<T>::~Fourier ()
  {
	if (cachedPlan) fftw_destroy_plan (cachedPlan);
  }

  template<class T>
  void
  Fourier<T>::dft (int direction, const MatrixStrided<std::complex<T> > & I, MatrixStrided<std::complex<T> > & O)
  {
	int rows;
	int cols;
	if (sizeFromOutput)
	{
	  rows = std::min (O.rows (),    I.rows ());
	  cols = std::min (O.columns (), I.columns ());
	}
	else
	{
	  rows = I.rows ();
	  cols = I.columns ();
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	fftw_iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != direction
          || cachedKind       != -1  // complex to complex
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  cachedPlan = fftw_plan_guru_dft
	  (
	    rank, dims,
		0, 0,
		(fftw_complex *) Idata, (fftw_complex *) Odata,
		direction, flags
	  );
	  cachedDirection = direction;
	  cachedKind      = -1;  // complex to complex
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft (cachedPlan, (fftw_complex *) Idata, (fftw_complex *) Odata);
	if (normalize) O /= sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (const MatrixStrided<T> & I, MatrixStrided<std::complex<T> > & O)
  {
	int rows;
	int cols;
	if (sizeFromOutput)
	{
	  rows = (O.rows () - 1) * 2;
	  cols =  O.columns ();
	}
	else
	{
	  rows = I.rows ();
	  cols = I.columns ();
	}
	const int Orows = rows / 2 + 1;
	if (O.rows () < Orows  ||  O.columns () < cols) O.resize (Orows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	fftw_iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != -1   // forward
          || cachedKind       != -2   // mixed complex-real
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  cachedPlan = fftw_plan_guru_dft_r2c
	  (
	    rank, dims,
		0, 0,
		(double *) Idata, (fftw_complex *) Odata,
		flags
	  );
	  cachedDirection = -1;  // forward
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft_r2c (cachedPlan, (double *) Idata, (fftw_complex *) Odata);
	if (normalize) O /= sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (const MatrixStrided<std::complex<T> > & I, MatrixStrided<T> & O)
  {
	int rows;
	int cols;
	if (sizeFromOutput)
	{
	  rows = O.rows ();
	  cols = O.columns ();
	}
	else
	{
	  rows = (I.rows () - 1) * 2;
	  cols =  I.columns ();
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	fftw_iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != 1  // reverse
          || cachedKind       != -2  // mixed complex-real
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  cachedPlan = fftw_plan_guru_dft_c2r
	  (
	    rank, dims,
		0, 0,
		(fftw_complex *) Idata, (double *) Odata,
		flags
	  );
	  cachedDirection = 1;   // reverse
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft_c2r (cachedPlan, (fftw_complex *) Idata, (double *) Odata);
	if (normalize) O /= sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (int kind, const MatrixStrided<T> & I, MatrixStrided<T> & O)
  {
	int rows;
	int cols;
	if (sizeFromOutput)
	{
	  rows = std::min (O.rows (),    I.rows ());
	  cols = std::min (O.columns (), I.columns ());
	}
	else
	{
	  rows = I.rows ();
	  cols = I.columns ();
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	fftw_iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != 0   // none
          || cachedKind       != kind
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  fftw_r2r_kind kinds[2];
	  kinds[0] = (fftw_r2r_kind) kind;
	  kinds[1] = (fftw_r2r_kind) kind;
	  cachedPlan = fftw_plan_guru_r2r
	  (
	    rank, dims,
		0, 0,
		(double *) Idata, (double *) Odata,
		kinds, flags
	  );
	  cachedDirection = 0;   // none
	  cachedKind      = kind;
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_r2r (cachedPlan, (double *) Idata, (double *) Odata);
	if (normalize)
	{
	  T N;
	  switch (kind)
	  {
		case FFTW_R2HC:
		case FFTW_HC2R:
		case FFTW_DHT:
		  N = rows * cols;
		  break;
		case FFTW_REDFT00:
		  N = 4 * (rows - 1) * (cols - 1);
		  break;
		case FFTW_RODFT00:
		  N = 4 * (rows + 1) * (cols + 1);
		  break;
		case FFTW_REDFT10:
		case FFTW_REDFT01:
		case FFTW_REDFT11:
		case FFTW_RODFT10:
		case FFTW_RODFT01:
		case FFTW_RODFT11:
		  N = 4 * rows * cols;
		  break;
	  }
	  O /= sqrt (N);
	}
  }
}


#endif
