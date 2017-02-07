//
//  TimeDistributedFFTConvolver.h
//  RTConvolve
//
//  Created by Graham Barab on 2/6/17.
//
//

#ifndef TimeDistributedFFTConvolver_h
#define TimeDistributedFFTConvolver_h

#include "../JuceLibraryCode/JuceHeader.h"

template <typename FLOAT_TYPE>
class RefCountedAudioBuffer: public juce::AudioBuffer<FLOAT_TYPE>, juce::ReferenceCountedObject
{
    
};

template <typename FLOAT_TYPE>
class TimeDistributedFFTConvolver
{
public:
    enum
    {
        kPhase0 = 0,
        kPhase1,
        kPhase2,
        kPhase3,
        kNumPhases
    };
    
    TimeDistributedFFTConvolver(FLOAT_TYPE *impulseResponse, int numSamplesImpulseResponse, int bufferSize);
    void processInput(FLOAT_TYPE *input);
    
private:
    int mNumSamplesBaseTimePeriod;
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mBuffersReal[3];
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mBuffersImag[3];
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mTempReal[3];
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mTempImag[3];
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mImpulsePartitionsReal;
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mImpulsePartitionsImag;
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mInputReal;
    juce::OwnedArray<juce::AudioBuffer<FLOAT_TYPE> > mInputImag;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutputReal;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mOutputImag;
    juce::ScopedPointer<juce::AudioBuffer<FLOAT_TYPE> > mPreviousTail;

    int mNumPartitions;
    int mCurrentPhase;
    int mCurrentInputIndex;
    
    void forwardDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, FLOAT_TYPE *tr, FLOAT_TYPE *ti, int N, int whichQuarter);
    void forwardDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, FLOAT_TYPE *tr, FLOAT_TYPE *ti, int N);

    void inverseDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int N, int whichQuarter);
    void inverseDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int N);
    
    void performConvolutions(int subArray, int whichHalf);
    void promoteBuffers();
    void prepareOutput();
    void bufferTail();
};

#include "TimeDistributedFFTConvolver.hpp"

#endif /* TimeDistributedFFTConvolver_h */
