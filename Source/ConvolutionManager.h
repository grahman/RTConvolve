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
    {
        mUniformConvolver = new UPConvolver<FLOAT_TYPE>(impulseResponse, numSamples, 4);
        checkNull(mUniformConvolver);
        
        FLOAT_TYPE *subIR = impulseResponse + (4 * bufferSize);
        int subNumSamples = numSamples - (4 * bufferSize);
        
        mTimeDistributedConvolver = new TimeDistributedFFTConvolver<FLOAT_TYPE>(subIR, subNumSamples, bufferSize);
        checkNull(mTimeDistributedConvolver);
    }
    
    void processInput(FLOAT_TYPE *input);
    FLOAT_TYPE *getOutput();
    
private:
    juce::ScopedPointer<UPConvolver<FLOAT_TYPE> > mUniformConvolver;
    juce::ScopedPointer<TimeDistributedFFTConvolver<FLOAT_TYPE> >  mTimeDistributedConvolver;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutput;
};

#endif /* ConvolutionManager_h */
