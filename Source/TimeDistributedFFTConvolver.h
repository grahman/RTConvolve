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
#include "RefCountedAudioBuffer.h"


/**
 The TimeDistributedFFTConvolver class computes the convolution of 
 the input with an impulse response using a slightly modified version 
 of the time-distributed fast Fourier Transform described in Jeffrey R. Hurchalla's 
 paper 'A Time Distributed FFT for Efficient Low Latency Convolution'.
 The convolved output will be available at the beginning of the output buffer
 8 'base time periods' (host audio buffer size) from when the corresponding input 
 was supplied in the call to 'processInput()'. There is therefore an input-output
 delay of eight times the number of samples in one base time period.
 
 */
template <typename FLOAT_TYPE>
class TimeDistributedFFTConvolver
{
public:
    
    /**
     Convenience enum for expressing the state of the object.
     */
    enum
    {
        kPhase0 = 0,
        kPhase1,
        kPhase2,
        kPhase3,
        kNumPhases
    };
    
    /**
     Construct a Time Distributed FFT-based object.
     @param impulseResponse
        A pointer to a buffer holding the impulse response with which 
        the subsequent input will be convolved.
     @param numSamplesImpulseResponse
        The length in samples of the impulse response.
     @param bufferSize
        The host audio application's audio buffer size, or 'base time period'.
     */
    TimeDistributedFFTConvolver(FLOAT_TYPE *impulseResponse, int numSamplesImpulseResponse, int bufferSize);
    
    /**
     Perform one base time period's worth of work for the convolution. The convolved
     output corresponding to this input will be ready 8 base time periods from when this
     method is called.
     @param input
        The input is expected to hold a number of samples equal to the 'bufferSize'
        specified in the constructor.
     */
    void processInput(FLOAT_TYPE *input);
    
    /**
     Obtain a pointer to one base time period's worth of output samples.
     @returns
        A pointer to the beginning of the output buffer.
     */
    const FLOAT_TYPE *getOutputBuffer() const
    {
        int startIndex = mCurrentPhase * mNumSamplesBaseTimePeriod;
        return mOutputReal->getReadPointer(0) + startIndex;
    }
    
private:
    int mNumSamplesBaseTimePeriod;
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mBuffersReal[3];
    juce::ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > mBuffersImag[3];
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
    
    /**
     Perform the 'decimation in frequency' forward decomposotition work for a quarter of the input.
     
     @param rex
        The real part of the length-N input.
     @param imx
        The imaginary part of the length-N input.
     @param N
        The length (in samples) of the input. The last N/2 samples are assumed to be
        a padding of zeros.
     @param whichQuarter <br />
        0 - Perform decomposition for 1st quarter of the non-pad input (first half of input buffer) <br />
        1 - Perform decomposition for 2nd quarter of the non-pad input (first half of input buffer) <br />
        2 - Perform decomposition for 3rd quarter of the non-pad input (first half of input buffer) <br />
        3 - Perform decomposition for 4th quarter of the non-pad input (first half of input buffer) <br />
        Parameter must be one of these four values, or an exception will be thrown.
     */
    void forwardDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int N, int whichQuarter);
    
    /**
     Convenience function to perform the forward decomposition work for all four quarters
     of the input buffer.
     
     @param rex
     The real part of the length-N input.
     @param imx
     The imaginary part of the length-N input.
     @param N
     The length (in samples) of the input. The last N/2 samples are assumed to be
     a padding of zeros.
     */
    void forwardDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int N);
    
    /**
     Perform the 'decimation in frequency' inverse decomposotition work for a quarter of the input.
     
     @param rex
     The real part of the length-N input.
     @param imx
     The imaginary part of the length-N input.
     @param N
     The length (in samples) of the input. The last N/2 samples are assumed to be
     a padding of zeros.
     @param whichQuarter <br />
     0 - Perform decomposition for 1st quarter of the non-pad input (first half of input buffer) <br />
     1 - Perform decomposition for 2nd quarter of the non-pad input (first half of input buffer) <br />
     2 - Perform decomposition for 3rd quarter of the non-pad input (first half of input buffer) <br />
     3 - Perform decomposition for 4th quarter of the non-pad input (first half of input buffer) <br />
     Parameter must be one of these four values, or an exception will be thrown.
     */
    void inverseDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int N, int whichQuarter);
    
    /**
     Convenience function to perform the inverse decomposition work for all four quarters
     of the input buffer.
     
     @param rex
     The real part of the length-N input.
     @param imx
     The imaginary part of the length-N input.
     @param N
     The length (in samples) of the input. The last N/2 samples are assumed to be
     a padding of zeros.
     */
    void inverseDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int N);
    
    /**
     Computes complex multiplications in the frequency domain for half of a sub-fft's
     convolutions.
     @param subArray <br />
        0 - the X(2k), ie. 'even' frequency bins
        1 - the X(2k+1), ie. 'odd' frequency bins
     @param whichHalf <br />
        0 - 1st half of 'subArray'.
        1 - 2nd half of 'subArray'.
     */
    void performConvolutions(int subArray, int whichHalf);
    
    /**
     Internal helper function for updating internal data structures for a new phase of the
     overall operation.
     */
    void promoteBuffers();
    
    /**
     Add the previous convolution tail to the output buffer.
     */
    void prepareOutput();
    
    /**
     Custom version of the fast fourier transform in which the frequency bins of the output are arranged
     in the order needed for frequency domain convolution implemented by this class.
     */
    void fft_priv(FLOAT_TYPE *rex, FLOAT_TYPE *imx, FLOAT_TYPE *trx, FLOAT_TYPE *tix, int N);
};

#include "TimeDistributedFFTConvolver.hpp"

#endif /* TimeDistributedFFTConvolver_h */
