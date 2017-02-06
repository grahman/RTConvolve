//
//  TimeDistributedFFTConvolver.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/6/17.
//
//

#include "util/util.h"
#include "util/fft.hpp"
#include <stdexcept>

static const float TWOPI = 2.0 * M_PI;
static const float NTWOPI = -2.0 * M_PI;

template <typename FLOAT_TYPE>
TimeDistributedFFTConvolver<FLOAT_TYPE>::TimeDistributedFFTConvolver(FLOAT_TYPE *impulseResponse, int numSamplesImpulseResponse, int bufferSize)
 : mCurrentPhase(kPhase3)
 , mCurrentInputIndex(0)
{
    mNumSamplesBaseTimePeriod = bufferSize;
    
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    
    if (isPowerOfTwo(bufferSize) == false)
    {
        throw std::invalid_argument("bufferSize must be a power of 2");
    }
    
    mNumPartitions = (numSamplesImpulseResponse / partitionSize) + !!(numSamplesImpulseResponse % partitionSize);
    
    for (int i = 0; i < mNumPartitions; ++i)
    {
        int samplesToCopy = std::min((numSamplesImpulseResponse - (i * partitionSize)), partitionSize);
        
        /* Allocate and fill real array */
        
        mImpulsePartitionsReal.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        checkNull(mImpulsePartitionsReal[i]);
        mImpulsePartitionsReal[i]->clear();
        
        FLOAT_TYPE *partition = mImpulsePartitionsReal[i]->getWritePointer(0);
        memcpy(partition, impulseResponse + (i * partitionSize), samplesToCopy * sizeof(FLOAT_TYPE));
        
        /* Allocate imag array */
        
        mImpulsePartitionsImag.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        checkNull(mImpulsePartitionsReal[i]);
        
        mImpulsePartitionsImag[i]->clear();
        FLOAT_TYPE *partitionImag = mImpulsePartitionsImag[i]->getWritePointer(0);
        
        FLOAT_TYPE *tempReal = new FLOAT_TYPE[2 * partitionSize];
        checkNull(tempReal);
        memset(tempReal, 0, 2 * partitionSize * sizeof(FLOAT_TYPE));
        
        FLOAT_TYPE *tempImag = new FLOAT_TYPE[2 * partitionSize];
        checkNull(tempImag);
        memset(tempImag, 0, 2 * partitionSize * sizeof(FLOAT_TYPE));
        
        /* Calculate transform */
        fft_priv(partition, partitionImag, tempReal, tempImag, 2 * partitionSize);
        
        delete [] tempReal;
        delete [] tempImag;
        
        /* Allocate an input buffer */
        mInputReal.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        mInputImag.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        mInputReal[i]->clear();
        mInputImag[i]->clear();
    }
    
    mOutputReal = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    checkNull(mOutputReal);
    mOutputReal->clear();
    mOutputImag = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    checkNull(mOutputImag);
    mOutputImag->clear();
    
    mBuffersReal[0] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    mBuffersReal[1] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    mBuffersReal[2] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);

    mBuffersImag[0] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    mBuffersImag[1] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    mBuffersImag[2] = new RefCountedAudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);

    for (int i = 0; i < 3; ++i)
    {
        mBuffersReal[i]->clear();
        mBuffersImag[i]->clear();
    }
    
    mPreviousTail = new juce::AudioBuffer<FLOAT_TYPE>(1, partitionSize);
    checkNull(mPreviousTail);
    mPreviousTail->clear();
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::processInput(FLOAT_TYPE *input)
{
    ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > temp;
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    mCurrentPhase = trueMod((mCurrentPhase + 1), 4);
    int Q = mCurrentPhase * mNumSamplesBaseTimePeriod;
    
    switch (mCurrentPhase)
    {
    case kPhase0:
        {
            promoteBuffers();

            /* Buffer 'C' */
            mBuffersReal[2]->clear();
            mBuffersImag[2]->clear();
            
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);

            forwardDecomposition(cr, ci, 2 * partitionSize, 0);
            
            /* Buffer 'B' */
            
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            
            fft(br, bi, partitionSize); /* X(2k) */
            
            FLOAT_TYPE *rex0 = mInputReal[mCurrentInputIndex]->getWritePointer(0);
            FLOAT_TYPE *imx0 = mInputImag[mCurrentInputIndex]->getWritePointer(0);
            
            memcpy(rex0, br, partitionSize * sizeof(FLOAT_TYPE));
            memcpy(imx0, bi, partitionSize * sizeof(FLOAT_TYPE));
            
            performConvolutions(0, 0);  /* Perform first half of convolutions for vector Y(2k) */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 0);
            prepareOutput();
            break;
        }
        case kPhase1:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            forwardDecomposition(cr, ci, 2 * partitionSize, 1);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            performConvolutions(0, 1);
            ifft(br, bi, partitionSize);    /* Y(2k) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 1);
            prepareOutput();
            break;
        }
        case kPhase2:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            forwardDecomposition(cr, ci, 2 * partitionSize, 2);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            FLOAT_TYPE *rex0 = mInputReal[mCurrentInputIndex]->getWritePointer(0);
            FLOAT_TYPE *imx0 = mInputImag[mCurrentInputIndex]->getWritePointer(0);

            fft(br + partitionSize, bi + partitionSize, partitionSize);
            memcpy(rex0 + partitionSize, br + partitionSize, partitionSize * sizeof(FLOAT_TYPE));
            memcpy(imx0 + partitionSize, bi + partitionSize, partitionSize * sizeof(FLOAT_TYPE));
            performConvolutions(1, 0);
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 2);
            prepareOutput();
            break;
        }
        case kPhase3:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            forwardDecomposition(cr, ci, 2 * partitionSize, 3);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            performConvolutions(1, 1);
            ifft(br + partitionSize, bi + partitionSize, partitionSize);    /* Y(2k+1) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);

            inverseDecomposition(ar, ai, 2 * partitionSize, 3);
            
            prepareOutput();
            break;
        }
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::prepareOutput()
{
    FLOAT_TYPE *out = mOutputReal->getWritePointer(0);
    FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
    FLOAT_TYPE *tail = mPreviousTail->getWritePointer(0);
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    int startIndex = mCurrentPhase * mNumSamplesBaseTimePeriod;
    
    for (int i = 0; i < mNumSamplesBaseTimePeriod; ++i)
    {
        int j = startIndex + i;
        out[j] = ar[j] + tail[j];
        tail[j] = ar[j + partitionSize];
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::promoteBuffers()
{
    ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > temp = mBuffersReal[0];
    mBuffersReal[0] = mBuffersReal[1];
    mBuffersReal[1] = mBuffersReal[2];
    mBuffersReal[2] = temp;
    
    temp = mBuffersImag[0];
    mBuffersImag[0] = mBuffersImag[1];
    mBuffersImag[1] = mBuffersImag[2];
    mBuffersImag[2] = temp;
    
    mCurrentInputIndex = trueMod((mCurrentInputIndex + 1), mNumPartitions);
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int length, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    int N = length;
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    
    int N8 = N >> 3;
    int N2 = N >> 1;
    int Q = whichQuarter * N8;
    
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        FLOAT_TYPE frac = j / (float)N;
        FLOAT_TYPE zr = cos(TWOPI * frac);  /* Twiddle factor real part */
        FLOAT_TYPE zi = sin(NTWOPI * frac); /* Twiddle factor imag part */
        FLOAT_TYPE rea;     /* an real part */
        FLOAT_TYPE ima;     /* an imag part */
        FLOAT_TYPE reb;     /* bn real part */
        FLOAT_TYPE imb;     /* bn imag part */
        
        rea = re[j] + re[j+N2];
        ima = im[j] + im[j+N2];
        reb = re[j] - re[j+N2];
        imb = im[j] - im[j+N2];
        
        re[j] = rea;
        im[j] = ima;
        re[j+N2] = (reb * zr) - (imb * zi);
        im[j+N2] = (reb * zi) - (imb * zr);
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int length)
{
    for (int i = 0; i < 4; ++i)
    {
        forwardDecomposition(rex, imx, length, i);
    }
}


template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int N, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    
    int N8 = N >> 3;
    int N2 = N >> 1;
    int Q = whichQuarter * N8;
        
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        FLOAT_TYPE frac = j / (float)N;
        FLOAT_TYPE twr = cos(TWOPI * frac);
        FLOAT_TYPE twi = sin(TWOPI * frac);
        FLOAT_TYPE rea = re[j];
        FLOAT_TYPE ima = im[j];
        FLOAT_TYPE reb = (re[j + N2] * twr) - (im[j + N2] * twi);
        FLOAT_TYPE imb = (re[j + N2] * twi) + (im[j + N2] * twr);
        re[j] = (rea + reb) * 0.5;
        im[j] = (ima + imb) * 0.5;
        re[j+N2] = (rea - reb) * 0.5;
        im[j+N2] = (ima - imb) * 0.5;
    }
}


template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, int length)
{
    for (int i = 0; i < 4; ++i)
    {
        inverseDecomposition(rex, imx, length, i);
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::performConvolutions(int subArray, int whichHalf)
{
    int N = 2 * mNumSamplesBaseTimePeriod;
    int startIndex = (subArray * 2 * N) + (whichHalf * N);

    FLOAT_TYPE *rex = nullptr;
    FLOAT_TYPE *imx = nullptr;
    
    FLOAT_TYPE *rey = mBuffersReal[1]->getWritePointer(0) + startIndex;
    FLOAT_TYPE *imy = mBuffersImag[1]->getWritePointer(0) + startIndex;
    FLOAT_TYPE *reh = nullptr;
    FLOAT_TYPE *imh = nullptr;
    
    memset(rey, 0, N * sizeof(FLOAT_TYPE));
    memset(imy, 0, N * sizeof(FLOAT_TYPE));
    
    for (int i = 0; i < mNumPartitions; ++i)
    {
        int k = trueMod((mCurrentInputIndex - i), mNumPartitions);
       
        rex = mInputReal[k]->getWritePointer(0) + startIndex;
        imx = mInputImag[k]->getWritePointer(0) + startIndex;
        reh = mImpulsePartitionsReal[i]->getWritePointer(0) + startIndex;
        imh = mImpulsePartitionsImag[i]->getWritePointer(0) + startIndex;

        for (int j = 0; j < N; ++j)
        {
            rey[j] += (rex[j] * reh[j]) - (imx[j] * imh[j]);
            imy[j] += (rex[j] * imh[j]) + (imx[j] * reh[j]);
        }
    }
}

/**
 This function computes the FFT with the frequency domain bins arranged in
 the specific order needed for the frequency domain convolutions 
 */
template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::fft_priv(FLOAT_TYPE *rex, FLOAT_TYPE *imx, FLOAT_TYPE *trx, FLOAT_TYPE *tix, int N)
{
    forwardDecompositionComplete(rex, imx, N);
    int N2 = N >> 1;
    
    fft(rex, imx, N2);
    fft(rex+N2, imx+N2, N2);
}

