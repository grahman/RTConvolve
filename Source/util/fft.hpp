/* Adapted from Stephen Smith's algorithm (www.dspguide.com) */

#ifndef __FFT__
#define __FFT__

/* Fast Fourier Transform */
#ifdef _WIN32
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static double log2( double n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2 );  
}
#endif
#include <cmath>
template <typename T>
void fft(T *REX, T *IMX, unsigned int N)
{
	const unsigned int NM1 = N - 1;
	const unsigned int ND2 = N / 2;
	const unsigned int M = (unsigned int)log2((float)N);
	unsigned int k;
	unsigned int LE, LE2;
	unsigned int IP;
	int j = ND2;
	int i, l;	/* Loop counters */
	int jm1;
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