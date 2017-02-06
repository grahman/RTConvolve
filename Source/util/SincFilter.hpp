//
//  SincFilter.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//

#ifndef SincFilter_hpp
#define SincFilter_hpp

#include <stdio.h>
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
