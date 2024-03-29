/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_fourier_h
#define fl_fourier_h

#ifdef HAVE_FFTW


#include "fl/matrix.h"
#include "fl/fftwproto.h"

#include <complex>
#include <pthread.h>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  template<class T>
  class SHARED Fourier
  {
  public:
	Fourier (bool normalize = true, bool destroyInput = false, bool sizeFromOutput = true);
	~Fourier ();

	void dft (int direction, const MatrixStrided<std::complex<T> > & input, MatrixStrided<std::complex<T> > & output);
	void dft (               const MatrixStrided<std::complex<T> > & input, MatrixStrided<T>                & output);
	void dft (               const MatrixStrided<T>                & input, MatrixStrided<std::complex<T> > & output);
 	void dft (int kind,      const MatrixStrided<T>                & input, MatrixStrided<T>                & output);

 	void dht  (const MatrixStrided<T> & input, MatrixStrided<T> & output) {dft (FFTW_DHT,     input, output);}
 	void dct  (const MatrixStrided<T> & input, MatrixStrided<T> & output) {dft (FFTW_REDFT10, input, output);}
 	void idct (const MatrixStrided<T> & input, MatrixStrided<T> & output) {dft (FFTW_REDFT01, input, output);}
 	void dst  (const MatrixStrided<T> & input, MatrixStrided<T> & output) {dft (FFTW_RODFT10, input, output);}
 	void idst (const MatrixStrided<T> & input, MatrixStrided<T> & output) {dft (FFTW_RODFT01, input, output);}

	bool normalize;       ///< Indicates to apply a balanced normalization so that round-trip transformations result in same values as original input.
	bool destroyInput;    ///< Indicates that the input matrix can be overwritten by the process.
	bool sizeFromOutput;  ///< Indicates to determine the logical size of the problem from the output matrix rather than the input matrix.

	// Cached plan (internal to implementation).
	static pthread_mutex_t        mutexPlan;
	typename traitsFFTW<T>::plan  cachedPlan;
	int                           cachedDirection;
	int                           cachedKind;
	unsigned int                  cachedFlags;
	typename traitsFFTW<T>::iodim cachedDims[2];
	int                           cachedAlignment;
	bool                          cachedInPlace;
  };
  template<class T> pthread_mutex_t Fourier<T>::mutexPlan = PTHREAD_MUTEX_INITIALIZER;
}


#endif  // HAVE_FFTW

#endif
