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
#include "util/SincFilter.hpp"

static const int DEFAULT_NUM_SAMPLES = 1024;
static const int DEFAULT_BUFFER_SIZE = 1024;

template <typename FLOAT_TYPE>
class ConvolutionManager
{
public:
    ConvolutionManager(FLOAT_TYPE *impulseResponse = nullptr, int numSamples = 0, int bufferSize = 0)
    : mBufferSize(bufferSize)
    , mTimeDistributedConvolver(nullptr)
    {
        if (impulseResponse == nullptr)
        {
            mBufferSize = DEFAULT_BUFFER_SIZE;
            mImpulseResponse = new juce::AudioBuffer<FLOAT_TYPE>(1, DEFAULT_NUM_SAMPLES);
            checkNull(mImpulseResponse);
            
            FLOAT_TYPE *ir = mImpulseResponse->getWritePointer(0);
            genImpulse(ir, DEFAULT_NUM_SAMPLES);
            
            init(ir, DEFAULT_NUM_SAMPLES);
        }
        else
        {
            mImpulseResponse = new juce::AudioBuffer<FLOAT_TYPE>(1, numSamples);
            checkNull(mImpulseResponse);
            mImpulseResponse->clear();
            FLOAT_TYPE *ir = mImpulseResponse->getWritePointer(0);
            
            memcpy(ir, impulseResponse, numSamples * sizeof(FLOAT_TYPE));
            init(ir, numSamples);
        }
    }
    
    void processInput(FLOAT_TYPE *input)
    {
        mUniformConvolver->processInput(input);
        const FLOAT_TYPE *out1 = mUniformConvolver->getOutputBuffer();
        FLOAT_TYPE *output = mOutput->getWritePointer(0);
        
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
        return mOutput->getReadPointer(0);
    }
    
    void setBufferSize(int bufferSize)
    {
        FLOAT_TYPE *ir = mImpulseResponse->getWritePointer(0);
        int numSamples = mImpulseResponse->getNumSamples();
        mBufferSize = bufferSize;
        
        init(ir, numSamples);
    }
    
    void setImpulseResponse(const FLOAT_TYPE *impulseResponse, int numSamples)
    {
        mImpulseResponse = new juce::AudioBuffer<FLOAT_TYPE>(1, numSamples);
        checkNull(mImpulseResponse);
        
        mImpulseResponse->clear();
        
        FLOAT_TYPE *ir = mImpulseResponse->getWritePointer(0);
        memcpy(ir, impulseResponse, numSamples * sizeof(FLOAT_TYPE));
        init(ir, numSamples);
    }
    
private:
    int mBufferSize;
    juce::ScopedPointer<UPConvolver<FLOAT_TYPE> > mUniformConvolver;
    juce::ScopedPointer<TimeDistributedFFTConvolver<FLOAT_TYPE> >  mTimeDistributedConvolver;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutput;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mImpulseResponse;
    
    void init(FLOAT_TYPE *impulseResponse, int numSamples)
    {
        mUniformConvolver = new UPConvolver<FLOAT_TYPE>(impulseResponse, numSamples, mBufferSize, 8);
        checkNull(mUniformConvolver);
        
        FLOAT_TYPE *subIR = impulseResponse + (8 * mBufferSize);
        int subNumSamples = numSamples - (8 * mBufferSize);
        
        if (subNumSamples > 0)
        {
            mTimeDistributedConvolver = new TimeDistributedFFTConvolver<FLOAT_TYPE>(subIR, subNumSamples, mBufferSize);
            checkNull(mTimeDistributedConvolver);
        }
        else
        {
            mTimeDistributedConvolver = nullptr;
        }
        
        mOutput = new juce::AudioBuffer<FLOAT_TYPE>(1, mBufferSize);
        checkNull(mOutput);
    }
};

#endif /* ConvolutionManager_h */
