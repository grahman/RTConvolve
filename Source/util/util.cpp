//
//  util.cpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//

#include "util.h"

int isPowerOfTwo (unsigned int x)
{
    return ((x != 0) && !(x & (x - 1)));
}


void* checkNull(void *x)
{
    if (x == nullptr)
    {
        std::cerr << "Error: could not allocate memory" << std::endl;
        throw std::exception();
    }
    return x;
}