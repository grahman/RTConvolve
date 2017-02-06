//
//  util.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//

#ifndef util_hpp
#define util_hpp

#include <iostream>
#include <exception>

#define TRUEMOD(x, m) ( (x >= 0) ? (x % m) : (m + (x % m)) )

int isPowerOfTwo (unsigned int x);

template <typename INT_TYPE>
INT_TYPE trueMod(INT_TYPE x, INT_TYPE m)
{
    return ( (x >= 0) ? (x % m) : (m + (x % m)) );
}

template <typename T>
T *throwIfNull(T *x)
{
    if (x == nullptr)
    {
        std::cerr << "Error: could not allocate memory" << std::endl;
        throw std::exception();
    }
    return x;
}
void *checkNull(void *x);

template <typename FLOAT_TYPE>
FLOAT_TYPE summation(FLOAT_TYPE *x, int N)
{
    FLOAT_TYPE sum = 0.0;
    
    for (int i = 0; i < N; ++i)
    {
        sum += fabs(x[i]);
    }
    return sum;
}

template <typename FLOAT_TYPE>
void scaleArray(FLOAT_TYPE *x, int N, FLOAT_TYPE amount)
{
    for (int i = 0; i < N; ++i)
    {
        x[i] *= amount;
    }
}

template <typename FLOAT_TYPE>
void normalizeStereoImpulseResponse(FLOAT_TYPE *left, FLOAT_TYPE* right, int numSamples)
{
    FLOAT_TYPE sumL = summation(left, numSamples);
    FLOAT_TYPE sumR = summation(right, numSamples);
    
    FLOAT_TYPE scale = fabs(20.0f/std::max(sumL, sumR));
    
    scaleArray(left, numSamples, scale);
    scaleArray(right, numSamples, scale);
}

template <typename FLOAT_TYPE>
void normalizeMonoImpulseResponse(FLOAT_TYPE *x, int numSamples)
{
    FLOAT_TYPE sum = fabs(summation(x, numSamples));
    scaleArray(x, numSamples, (20.0f/sum));
}
#endif /* util_hpp */
