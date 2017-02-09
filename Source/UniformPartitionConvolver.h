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

template <typename FLOAT_TYPE>
class UPConvolver
{
public:
    UPConvolver(FLOAT_TYPE *impulseResponse, int numSamples, int bufferSize, int maxPartitions);
    
    void processInput(FLOAT_TYPE *input);
    
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
