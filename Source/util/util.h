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

void checkNull(void *x);
#endif /* util_hpp */
