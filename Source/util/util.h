//
//  util.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//
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

#ifndef util_hpp
#define util_hpp

#include <algorithm>
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
