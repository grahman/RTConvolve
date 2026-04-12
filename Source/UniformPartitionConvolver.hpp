//
//  UniformPartitionConvolver.cpp
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

#include "util/util.h"
#include "util/fft.hpp"
#include <stdexcept>

template <typename FLOAT_TYPE>
UPConvolver<FLOAT_TYPE>::UPConvolver(FLOAT_TYPE *impulseResponse, size_t numSamples, int bufferSize, int maxPartitions)
: mCurrentInputSegment(0)
{
    if (isPowerOfTwo(bufferSize) == false)
    {
        throw std::invalid_argument("bufferSize must be a power of 2");
    }
    
    size_t numPartitions = (numSamples / bufferSize) + !!(numSamples % bufferSize);

    if (numPartitions > maxPartitions)
    {
        numPartitions = maxPartitions;
    }

    mNumPartitions = numPartitions;
    mBufferSize = bufferSize;

    for (int i = 0; i < numPartitions; ++i)
    {
        size_t samplesToCopy = std::min((numSamples - (i * mBufferSize)), (size_t)mBufferSize);

        /* Allocate and fill real array */
        mImpulsePartitionsReal.emplace_back(2 * mBufferSize);

        checkNull(mImpulsePartitionsReal[i].data());
        std::fill(mImpulsePartitionsReal[i].begin(), mImpulsePartitionsReal[i].end(), 0);
        
        FLOAT_TYPE *partition = mImpulsePartitionsReal[i].data();
        memcpy(partition, impulseResponse + (i * mBufferSize), samplesToCopy * sizeof(FLOAT_TYPE));
        
        /* Allocate imag array */
        mImpulsePartitionsImag.emplace_back(2 * mBufferSize);
        checkNull(mImpulsePartitionsReal[i].data());
        
        std::fill(mImpulsePartitionsImag[i].begin(), mImpulsePartitionsImag[i].end(), 0);

        FLOAT_TYPE *partitionImag = mImpulsePartitionsImag[i].data();
        
        /* Calculate transform */
        fft(partition, partitionImag, 2 * mBufferSize);
        
        /* Allocate an input buffer */
        mInputReal.emplace_back(2 * mBufferSize);
        mInputImag.emplace_back(2 * mBufferSize);

        std::fill(mInputReal[i].begin(), mInputReal[i].end(), 0);
        std::fill(mInputImag[i].begin(), mInputImag[i].end(), 0);
    }
    
    mOutputReal.reset(2 * mBufferSize);
    checkNull(mOutputReal.data());
    std::fill(mOutputReal.begin(), mOutputReal.end(), 0);

    mOutputImag.reset(2 * mBufferSize);
    checkNull(mOutputImag.data());
    std::fill(mOutputImag.begin(), mOutputImag.end(), 0);
    
    mPreviousOutputTail.reset(mBufferSize);
    checkNull(mPreviousOutputTail.data());
    std::fill(mPreviousOutputTail.begin(), mPreviousOutputTail.end(), 0);
}

template <typename FLOAT_TYPE>
void UPConvolver<FLOAT_TYPE>::processInput(FLOAT_TYPE *input)
{
    FLOAT_TYPE *currentSegmentReal = mInputReal[mCurrentInputSegment].data();
    FLOAT_TYPE *currentSegmentImag = mInputImag[mCurrentInputSegment].data();
    
    std::fill(mInputReal[mCurrentInputSegment].begin(), mInputReal[mCurrentInputSegment].end(), 0);
    std::fill(mInputImag[mCurrentInputSegment].begin(), mInputImag[mCurrentInputSegment].end(), 0);

    memcpy(currentSegmentReal, input, mBufferSize * sizeof(FLOAT_TYPE));
    fft(currentSegmentReal, currentSegmentImag, 2 * mBufferSize);
    
    process();
}


template <typename FLOAT_TYPE>
void UPConvolver<FLOAT_TYPE>::process()
{
    std::fill(mOutputReal.begin(), mOutputReal.end(), 0);
    std::fill(mOutputImag.begin(), mOutputImag.end(), 0);

    FLOAT_TYPE *rey = mOutputReal.data();
    FLOAT_TYPE *imy = mOutputImag.data();
    
    for (size_t j = 0; j < mNumPartitions; ++j)
    {
        size_t k = trueMod((size_t)(mCurrentInputSegment - j), mNumPartitions);

        const FLOAT_TYPE *rex = mInputReal[k].data();
        const FLOAT_TYPE *imx = mInputImag[k].data();
        const FLOAT_TYPE *reh = mImpulsePartitionsReal[j].data();
        const FLOAT_TYPE *imh = mImpulsePartitionsImag[j].data();
        
        for (int i = 0; i < (2 * mBufferSize); ++i)
        {
            rey[i] += (rex[i] * reh[i]) - (imx[i] * imh[i]);
            imy[i] += (rex[i] * imh[i]) + (imx[i] * reh[i]);
        }
    }
    
    ifft(rey, imy, 2 * mBufferSize);
    FLOAT_TYPE *tail = mPreviousOutputTail.data();
    
    for (int i = 0; i < mBufferSize; ++i)
    {
        rey[i] += tail[i];
        tail[i] = rey[i + mBufferSize];
    }
    
    mCurrentInputSegment = (mCurrentInputSegment + 1) % mNumPartitions;
}
