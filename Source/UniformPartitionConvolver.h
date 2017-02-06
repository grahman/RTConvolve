//
//  UniformPartitionConvolver.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/5/17.
//
//

#ifndef UniformPartitionConvolver_hpp
#define UniformPartitionConvolver_hpp

#include <stdio.h>
#include "../JuceLibraryCode/JuceHeader.h"

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
    UPConvolver(FLOAT_TYPE *impulseResponse, int numSamples, int bufferSize, int maxPartitions);
    
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
        return mOutputReal->getReadPointer(0);
    };
    
private:
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mImpulsePartitionsReal;
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mImpulsePartitionsImag;
    
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mInputReal;
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mInputImag;

    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mPreviousOutputTail;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutputReal;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutputImag;
    
    int mBufferSize;
    int mNumPartitions;
    
    int mCurrentInputSegment;
    
    void process();

};

#include "UniformPartitionConvolver.hpp"

#endif
