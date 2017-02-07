//
//  UniformPartitionConvolver.cpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//

#include "util/util.h"
#include "util/fft.hpp"
#include <stdexcept>

template <typename FLOAT_TYPE>
UPConvolver<FLOAT_TYPE>::UPConvolver(FLOAT_TYPE *impulseResponse, int numSamples, int bufferSize, int maxPartitions)
: mCurrentInputSegment(0)
{
    if (isPowerOfTwo(bufferSize) == false)
    {
        throw std::invalid_argument("bufferSize must be a power of 2");
    }
    
    int numPartitions = (numSamples / bufferSize) + !!(numSamples % bufferSize);

    if (numPartitions > maxPartitions)
    {
        numPartitions = maxPartitions;
    }

    mNumPartitions = numPartitions;
    mBufferSize = bufferSize;

    for (int i = 0; i < numPartitions; ++i)
    {
        int samplesToCopy = std::min((numSamples - (i * mBufferSize)), mBufferSize);

        /* Allocate and fill real array */
        
        mImpulsePartitionsReal.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize));
        checkNull(mImpulsePartitionsReal[i]);
        mImpulsePartitionsReal[i]->clear();
        
        FLOAT_TYPE *partition = mImpulsePartitionsReal[i]->getWritePointer(0);
        memcpy(partition, impulseResponse + (i * mBufferSize), samplesToCopy * sizeof(FLOAT_TYPE));
        
        /* Allocate imag array */
        
        mImpulsePartitionsImag.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize));
        checkNull(mImpulsePartitionsReal[i]);
        
        mImpulsePartitionsImag[i]->clear();
        FLOAT_TYPE *partitionImag = mImpulsePartitionsImag[i]->getWritePointer(0);
        
        /* Calculate transform */
        fft(partition, partitionImag, 2 * mBufferSize);
        
        /* Allocate an input buffer */
        mInputReal.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize));
        mInputImag.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize));
        mInputReal[i]->clear();
        mInputImag[i]->clear();
    }
    
    mOutputReal = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize);
    checkNull(mOutputReal);
    
    mOutputImag = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * mBufferSize);
    checkNull(mOutputImag);
    mOutputImag->clear();
    
    mPreviousOutputTail = new juce::AudioBuffer<FLOAT_TYPE>(1, mBufferSize);
    checkNull(mPreviousOutputTail);
    mPreviousOutputTail->clear();
}

template <typename FLOAT_TYPE>
void UPConvolver<FLOAT_TYPE>::processInput(FLOAT_TYPE *input)
{
    FLOAT_TYPE *currentSegmentReal = mInputReal[mCurrentInputSegment]->getWritePointer(0);
    FLOAT_TYPE *currentSegmentImag = mInputImag[mCurrentInputSegment]->getWritePointer(0);
    
    mInputReal[mCurrentInputSegment]->clear();
    mInputImag[mCurrentInputSegment]->clear();
    
    memcpy(currentSegmentReal, input, mBufferSize * sizeof(FLOAT_TYPE));
    fft(currentSegmentReal, currentSegmentImag, 2 * mBufferSize);
    
    process();
}


template <typename FLOAT_TYPE>
void UPConvolver<FLOAT_TYPE>::process()
{
    mOutputReal->clear();
    mOutputImag->clear();
    
    FLOAT_TYPE *rey = mOutputReal->getWritePointer(0);
    FLOAT_TYPE *imy = mOutputImag->getWritePointer(0);
    
    for (int j = 0; j < mNumPartitions; ++j)
    {
        int k = trueMod(mCurrentInputSegment - j, mNumPartitions);

        const FLOAT_TYPE *rex = mInputReal[k]->getReadPointer(0);
        const FLOAT_TYPE *imx = mInputImag[k]->getReadPointer(0);
        const FLOAT_TYPE *reh = mImpulsePartitionsReal[j]->getReadPointer(0);
        const FLOAT_TYPE *imh = mImpulsePartitionsImag[j]->getReadPointer(0);
        
        for (int i = 0; i < (2 * mBufferSize); ++i)
        {
            rey[i] += (rex[i] * reh[i]) - (imx[i] * imh[i]);
            imy[i] += (rex[i] * imh[i]) + (imx[i] * reh[i]);
        }
    }
    
    ifft(rey, imy, 2 * mBufferSize);
    FLOAT_TYPE *tail = mPreviousOutputTail->getWritePointer(0);
    
    for (int i = 0; i < mBufferSize; ++i)
    {
        rey[i] += tail[i];
        tail[i] = rey[i + mBufferSize];
    }
    
    mCurrentInputSegment = (mCurrentInputSegment + 1) % mNumPartitions;
}
