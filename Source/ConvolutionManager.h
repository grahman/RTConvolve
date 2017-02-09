//
//  ConvolutionManager.h
//  RTConvolve
//
//  Created by Graham Barab on 2/7/17.
//
//

#ifndef ConvolutionManager_h
#define ConvolutionManager_h

#include "UniformPartitionConvolver.h"
#include "TimeDistributedFFTConvolver.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "util/util.h"

template <typename FLOAT_TYPE>
class ConvolutionManager
{
public:
    ConvolutionManager(FLOAT_TYPE *impulseResponse, int numSamples, int bufferSize)
    : mBufferSize(bufferSize)
    {
        mUniformConvolver = new UPConvolver<FLOAT_TYPE>(impulseResponse, numSamples, mBufferSize, 8);
        checkNull(mUniformConvolver);
        
        FLOAT_TYPE *subIR = impulseResponse + (8 * mBufferSize);
        int subNumSamples = numSamples - (8 * mBufferSize);
        
        mTimeDistributedConvolver = new TimeDistributedFFTConvolver<FLOAT_TYPE>(subIR, subNumSamples, mBufferSize);
        checkNull(mTimeDistributedConvolver);
        
        mOutput = new juce::AudioBuffer<FLOAT_TYPE>(1, mBufferSize);
        checkNull(mOutput);
    }
    
    void processInput(FLOAT_TYPE *input)
    {
        mUniformConvolver->processInput(input);
        mTimeDistributedConvolver->processInput(input);
        const FLOAT_TYPE *out1 = mUniformConvolver->getOutputBuffer();
        const FLOAT_TYPE *out2 = mTimeDistributedConvolver->getOutputBuffer();
        FLOAT_TYPE *output = mOutput->getWritePointer(0);
        
        /* Prepare output */
        for (int i = 0; i < mBufferSize; ++i)
        {
            output[i] = out1[i] + out2[i];
//            output[i] = out2[i];
        }
    }
    
    const FLOAT_TYPE *getOutputBuffer() const
    {
        return mOutput->getReadPointer(0);
    }
    
private:
    juce::ScopedPointer<UPConvolver<FLOAT_TYPE> > mUniformConvolver;
    juce::ScopedPointer<TimeDistributedFFTConvolver<FLOAT_TYPE> >  mTimeDistributedConvolver;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutput;
    int mBufferSize;
};

#endif /* ConvolutionManager_h */
