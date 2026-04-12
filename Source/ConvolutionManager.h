//
//  ConvolutionManager.h
//  RTConvolve
//
//  Created by Graham Barab on 2/7/17.
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

#ifndef ConvolutionManager_h
#define ConvolutionManager_h

#include "UniformPartitionConvolver.h"
#include "TimeDistributedFFTConvolver.h"
#include "util/util.h"
#include "util/SincFilter.hpp"

#include <memory>

static const int DEFAULT_NUM_SAMPLES = 512;
static const int DEFAULT_BUFFER_SIZE = 512;

template <typename FLOAT_TYPE>
class ConvolutionManager
{
    using SampleBuffer = DynamicArray<FLOAT_TYPE>;

public:
    ConvolutionManager(FLOAT_TYPE *impulseResponse = nullptr, int numSamples = 0, int bufferSize = 0)
    : mBufferSize(bufferSize)
    , mTimeDistributedConvolver(nullptr)
    {
        if (impulseResponse == nullptr)
        {
            mBufferSize = DEFAULT_BUFFER_SIZE;
            mImpulseResponse.reset(DEFAULT_NUM_SAMPLES);
            checkNull(mImpulseResponse.data());
            
            FLOAT_TYPE *ir = mImpulseResponse.data();
            genImpulse(ir, DEFAULT_NUM_SAMPLES);
            
            init(ir, DEFAULT_NUM_SAMPLES);
        }
        else
        {
            mImpulseResponse.reset(numSamples);
            checkNull(mImpulseResponse.data());
            std::fill(mImpulseResponse.begin(), mImpulseResponse.end(), 0);
            FLOAT_TYPE *ir = mImpulseResponse.data();
            
            memcpy(ir, impulseResponse, numSamples * sizeof(FLOAT_TYPE));
            init(ir, numSamples);
        }
    }
    
    /**
     Perform one base time period's worth of work for the convolution.
     @param input
     The input is expected to hold a number of samples equal to the 'bufferSize'
     specified in the constructor.
     */
    void processInput(FLOAT_TYPE *input)
    {
        mUniformConvolver->processInput(input);
        const FLOAT_TYPE *out1 = mUniformConvolver->getOutputBuffer();
        FLOAT_TYPE *output = mOutput.data();
        
        /* Prepare output */
        
        if (mTimeDistributedConvolver != nullptr)
        {
            mTimeDistributedConvolver->processInput(input);
            const FLOAT_TYPE *out2 = mTimeDistributedConvolver->getOutputBuffer();
            
            for (int i = 0; i < mBufferSize; ++i)
            {
                output[i] = out1[i] + out2[i];
            }
        }
        else
        {
            for (int i = 0; i < mBufferSize; ++i)
            {
                output[i] = out1[i];
            }
        }
    }
    
    const FLOAT_TYPE *getOutputBuffer() const
    {
        return mOutput.data();
    }
    
    void setBufferSize(int bufferSize)
    {
        FLOAT_TYPE *ir = mImpulseResponse.data();
        auto numSamples = mImpulseResponse.size();
        mBufferSize = bufferSize;
        
        init(ir, numSamples);
    }
    
    void setImpulseResponse(const FLOAT_TYPE *impulseResponse, size_t numSamples)
    {
        mImpulseResponse.reset(numSamples);
        checkNull(mImpulseResponse.data());
        
        std::fill(mImpulseResponse.begin(), mImpulseResponse.end(), 0);
        
        FLOAT_TYPE *ir = mImpulseResponse.data();
        memcpy(ir, impulseResponse, numSamples * sizeof(FLOAT_TYPE));
        init(ir, numSamples);
    }
    
private:
    int mBufferSize;
    std::unique_ptr<UPConvolver<FLOAT_TYPE> > mUniformConvolver;
    std::unique_ptr<TimeDistributedFFTConvolver<FLOAT_TYPE> >  mTimeDistributedConvolver;
    SampleBuffer mOutput;
    SampleBuffer mImpulseResponse;
    
    void init(FLOAT_TYPE *impulseResponse, size_t numSamples)
    {
        mUniformConvolver.reset(new UPConvolver<FLOAT_TYPE>(impulseResponse, numSamples, mBufferSize, 8));
        checkNull(mUniformConvolver.get());
        
        FLOAT_TYPE *subIR = impulseResponse + (8 * mBufferSize);
        ssize_t subNumSamples = numSamples - (8 * mBufferSize);
        
        if (subNumSamples > 0)
        {
            mTimeDistributedConvolver.reset(new TimeDistributedFFTConvolver<FLOAT_TYPE>(subIR, subNumSamples, mBufferSize));
            checkNull(mTimeDistributedConvolver.get());
        }
        else
        {
            mTimeDistributedConvolver.reset();
        }
        
        mOutput.reset(mBufferSize);
        checkNull(mOutput.data());
    }
};

#endif /* ConvolutionManager_h */
