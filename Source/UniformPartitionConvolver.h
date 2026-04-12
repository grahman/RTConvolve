//
//  UniformPartitionConvolver.hpp
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

#ifndef UniformPartitionConvolver_hpp
#define UniformPartitionConvolver_hpp

#include "util/DynamicArray.h"

#include <memory>
#include <vector>

/**
 The UPConvolver class computes the convolution via FFT of the input 
 with some impulse response using the uniform-partition method.
 */
template <typename FLOAT_TYPE>
class UPConvolver
{
public:
    
    /**
     Construct a UPConvolver object.
     @param impulseResponse
        A pointer to the samples of an impulse response.
     @param numSamples
        The number of samples in the impulse response.
     @param bufferSize
        The host audio applications buffer size.
     @param maxPartitions
        The maximum number of partitions to use. If the full impulse response
        requires a number of partions greater than 'maxPartitions', only the first
        'maxPartitions' sections of the IR will be used.
     */
    UPConvolver(FLOAT_TYPE *impulseResponse, size_t numSamples, int bufferSize, int maxPartitions);
    
    /**
     Perform one base time period's worth of work for the convolution.
     @param input
     The input is expected to hold a number of samples equal to the 'bufferSize'
     specified in the constructor.
     */
    void processInput(FLOAT_TYPE *input);
    
    /**
     @returns
        A pointer to the output buffer
     */
    const FLOAT_TYPE *getOutputBuffer() const
    {
        return mOutputReal.data();
    };
    
private:

    using SampleBuffer = DynamicArray<FLOAT_TYPE>;

    std::vector<SampleBuffer> mImpulsePartitionsReal;
    std::vector<SampleBuffer> mImpulsePartitionsImag;
    
    std::vector<SampleBuffer> mInputReal;
    std::vector<SampleBuffer> mInputImag;

    SampleBuffer mPreviousOutputTail;
    SampleBuffer mOutputReal;
    SampleBuffer mOutputImag;
    
    int mBufferSize;
    size_t mNumPartitions;
    
    int mCurrentInputSegment;
    
    void process();

};

#include "UniformPartitionConvolver.hpp"

#endif
