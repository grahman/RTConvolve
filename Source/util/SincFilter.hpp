//
//  SincFilter.hpp
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

#ifndef SincFilter_hpp
#define SincFilter_hpp

#include <cmath>

template <typename FLOAT_TYPE>
void genSincFilter(FLOAT_TYPE *x, int N, FLOAT_TYPE normalizedCutoff)
{
    FLOAT_TYPE omega = 2.0 * M_PI * normalizedCutoff;
    FLOAT_TYPE N2 = N / 2.0;
    FLOAT_TYPE sum = 0;
    
    for (int i = 0; i < N; ++i)
    {
        if (i == N/2)
        {
            x[i] = 1;
        }
        else
        {
            x[i] = sin(omega * (i - N2)) / (i - N2);
        }

        sum += x[i];
    }
    
    /* Normalize */
    FLOAT_TYPE scale = 1.0 / sum;
    
    for (int i = 0; i < N; ++i)
    {
        x[i] *= scale;
    }
}

template <typename FLOAT_TYPE>
void genImpulse(FLOAT_TYPE *x, int N)
{
    x[0] = 1.0;
    
    for (int i = 1; i < N; ++i) { x[i] = 0; }
}

#endif /* SincFilter_hpp */
