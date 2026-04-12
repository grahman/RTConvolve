/* Adapted from Stephen Smith's algorithm (www.dspguide.com) */

// MIT License
//
// Copyright (c) 2026 Graham Barab
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __FFT__
#define __FFT__

/* Fast Fourier Transform */
#ifdef _WIN32
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif
#include <cmath>

inline double _log2( double n )
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2 );  
}

template <typename T>
void fft(T *REX, T *IMX, size_t N)
{
	const size_t NM1 = N - 1;
	const size_t ND2 = N / 2;
	const size_t M = (size_t)_log2((float)N);
	size_t k;
	size_t LE, LE2;
	size_t IP;
	size_t j = ND2;
	size_t i, l;	/* Loop counters */
	size_t jm1;
	T TR, TI, UR, UI, SR, SI;

	/* Bit reversal sorting */
	for (i = 1; i < N - 1; ++i) {
		if (i >= j)
			goto SKIP;
		TR = REX[j];
		TI = IMX[j];
		REX[j] = REX[i];
		IMX[j] = IMX[i];
		REX[i] = TR;
		IMX[i] = TI;
SKIP:
		k = ND2;
		while (k <= j) {
			j = j-k;
			k /= 2;
		}
		j = j + k;
	}
	
	for (l = 1; l <= M; ++l) {
		LE = pow(2.0, l);
		LE2 = LE / 2;
		UR = 1;
		UI = 0;
		SR = cos(M_PI / LE2);
		SI = sin(M_PI / LE2);
		for (j = 1; j <= LE2; ++j) {
			jm1 = j - 1;
			for (i = jm1; i <= NM1; i += LE) {
				IP = i + LE2;
				TR = REX[IP] * UR - IMX[IP] * UI;
				TI = REX[IP] * UI + IMX[IP] * UR;
				REX[IP] = REX[i] - TR;
				IMX[IP] = IMX[i] - TI;
				REX[i] = REX[i] + TR;
				IMX[i] = IMX[i] + TI;
			}
			TR = UR;
			UR = TR * SR - UI * SI;
			UI = TR * SI + UI * SR;
		}
	}
}


/* Inverse Fast Fourier Transform */
template <typename T>
void ifft(T *REX, T *IMX, unsigned int N)
{
	int i, k;

	/* Change the sign of IMX[] */
	for (k = 0; k < N; ++k)
		IMX[k] *= -1;
	
    fft(REX, IMX, N);
	
	for (i=0; i < N; ++i) {
		REX[i] = REX[i] / N;
		IMX[i] = -1 * IMX[i] / N;
	}
}
#endif
