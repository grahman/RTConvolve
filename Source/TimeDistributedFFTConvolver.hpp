//
//  TimeDistributedFFTConvolver.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/6/17.
//
//

//#include "TimeDistributedFFTConvolver.h"

#include "util/util.h"
#include "util/fft.hpp"
#include <stdexcept>

static const float TWOPI = 2.0 * M_PI;
static const float NTWOPI = -2.0 * M_PI;

template <typename FLOAT_TYPE>
TimeDistributedFFTConvolver<FLOAT_TYPE>::TimeDistributedFFTConvolver(FLOAT_TYPE *impulseResponse, int numSamplesImpulseResponse, int bufferSize)
 : mCurrentInputIndex(0)
 , mCurrentPhase(kPhase3)
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
        
        /* Calculate transform */
        fft(partition, partitionImag, 2 * partitionSize);
        
        /* Allocate an input buffer */
        mInputReal.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        mInputImag.add(new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize));
        mInputReal[i]->clear();
        mInputImag[i]->clear();
    }
    
    mTempReal = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    checkNull(mTempReal);
    mTempReal->clear();
    mTempImag = new juce::AudioBuffer<FLOAT_TYPE>(1, 2 * partitionSize);
    checkNull(mTempImag);
    mTempReal->clear();
    
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
    
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::processInput(FLOAT_TYPE *input)
{
    ReferenceCountedObjectPtr<RefCountedAudioBuffer<FLOAT_TYPE> > temp;
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    int N4 = partitionSize >> 1;    /* 1/4 of FFT size */
    int Q = mCurrentPhase * N4;
    
    switch (mCurrentPhase)
    {
    case kPhase0:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            FLOAT_TYPE *tr = mTempReal[2] -> getWritePointer(0);
            FLOAT_TYPE *ti = mTempImag[2] -> getWritePointer(0);
            
            forwardDecomposition(cr, ci, tr, ti, 2 * partitionSize, 0);
            
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
            
            tr = mTempReal[0]->getWritePointer(0);
            ti = mTempImag[0]->getWritePointer(0);
            inverseDecomposition(ar, ai, tr, ti, 2 * partitionSize, 0);
            
            prepareOutput();
            bufferTail();
            
            break;
        }
        case kPhase1:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            FLOAT_TYPE *tr = mTempReal[2] -> getWritePointer(0);
            FLOAT_TYPE *ti = mTempImag[2] -> getWritePointer(0);
            
            forwardDecomposition(cr, ci, tr, ti, 2 * partitionSize, 1);
            
            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            performConvolutions(0, 1);
            ifft(br, bi, partitionSize);    /* Y(2k) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            tr = mTempReal[0]->getWritePointer(0);
            ti = mTempImag[0]->getWritePointer(0);
            inverseDecomposition(ar, ai, tr, ti, 2 * partitionSize, 1);
            
            prepareOutput();
            bufferTail();
            break;
        }
        case kPhase2:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            FLOAT_TYPE *tr = mTempReal[2] -> getWritePointer(0);
            FLOAT_TYPE *ti = mTempImag[2] -> getWritePointer(0);
            
            forwardDecomposition(cr, ci, tr, ti, 2 * partitionSize, 2);
            
            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            
            fft(br + partitionSize, bi + partitionSize, partitionSize);
            performConvolutions(1, 0);
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            tr = mTempReal[0]->getWritePointer(0);
            ti = mTempImag[0]->getWritePointer(0);
            inverseDecomposition(ar, ai, tr, ti, 2 * partitionSize, 2);
            
            prepareOutput();
            bufferTail();
            
            break;
        }
        case kPhase3:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->getWritePointer(0);
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->getWritePointer(0);
            FLOAT_TYPE *tr = mTempReal[2] -> getWritePointer(0);
            FLOAT_TYPE *ti = mTempImag[2] -> getWritePointer(0);
            
            forwardDecomposition(cr, ci, tr, ti, 2 * partitionSize, 3);
            
            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->getWritePointer(0);
            FLOAT_TYPE *bi = mBuffersImag[1]->getWritePointer(0);
            performConvolutions(1, 1);
            ifft(br + partitionSize, bi + partitionSize, partitionSize);    /* Y(2k) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->getWritePointer(0);
            FLOAT_TYPE *ai = mBuffersImag[0]->getWritePointer(0);
            
            tr = mTempReal[0]->getWritePointer(0);
            ti = mTempImag[0]->getWritePointer(0);
            inverseDecomposition(ar, ai, tr, ti, 2 * partitionSize, 3);
            
            prepareOutput();
            bufferTail();
            
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
    int startIndex = mCurrentPhase * mNumSamplesBaseTimePeriod;
    
    for (int i = 0; i < mNumSamplesBaseTimePeriod; ++i)
    {
        int j = startIndex + i;
        out[j] = ar[j] + tail[j];
        
        
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::bufferTail()
{
    FLOAT_TYPE *tail = mPreviousTail->getWritePointer(0);
    int startIndex = mCurrentPhase * mNumSamplesBaseTimePeriod;
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    FLOAT_TYPE *out = mOutputReal->getWritePointer(0);

    for (int i = 0; i < mNumSamplesBaseTimePeriod; ++i)
    {
        int j = startIndex + i;
        int k = j + partitionSize;
        
        tail[j] = out[k];
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
}


template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int length, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    int N = length;
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    FLOAT_TYPE *tr = trx;
    FLOAT_TYPE *ti = tix;
    
   
    int N8 = N >> 3;
    int N2 = N >> 1;
    int Q = whichQuarter * N8;
    
    /* Even half decomposition */
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        tr[j] = re[j];
        ti[j] = im[j];
        re[j] = re[j] + re[j + N2];
        im[j] = im[j] + im[j + N2];
    }
    
    /* Odd half decomposition */
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        float frac = j / (float)N;
        float zr = cos(TWOPI * frac);
        float zi = sin(NTWOPI * frac);
        
        tr[N2 + j] = (tr[j] - re[N2 + j]);
        ti[N2 + j] = (ti[j] - im[N2 + j]);
        
        re[N2 + j] = (tr[N2 + j] * zr) - (ti[N2 + j] * zi);
        im[N2 + j] = (tr[N2 + j] * zi) + (ti[N2 + j] * zr);
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int length)
{
    for (int i = 0; i < 4; ++i)
    {
        forwardDecomposition(rex, imx, trx, tix, length, i);
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int N, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    FLOAT_TYPE *tr = trx;
    FLOAT_TYPE *ti = tix;
    
    
    int N8 = N >> 3;
    int N2 = N >> 1;
    int Q = whichQuarter * N8;
    
    /* Even half decomposition */
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        tr[j] = re[j];
        ti[j] = im[j];
        re[j] = (re[j] + re[j + N2]) * 0.5;
        im[j] = (im[j] + im[j + N2]) * 0.5;
    }
    
    /* Odd half decomposition */
    for (int i = 0; i < N8; ++i)
    {
        int j = i + Q;
        float frac = j / (float)N;
        float zr = cos(TWOPI * frac);
        float zi = sin(TWOPI * frac);
        
        tr[N2 + j] = (tr[j] - re[N2 + j]);
        ti[N2 + j] = (ti[j] - im[N2 + j]);
        
        re[N2 + j] = ((tr[N2 + j] * zr) - (ti[N2 + j] * zi)) * 0.5;
        im[N2 + j] = ((tr[N2 + j] * zi) + (ti[N2 + j] * zr)) * 0.5;
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx,FLOAT_TYPE *trx, FLOAT_TYPE *tix, int length)
{
    for (int i = 0; i < 4; ++i)
    {
        inverseDecomposition(rex, imx, trx, tix, length, i);
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::performConvolutions(int subArray, int whichHalf)
{
    int N = 2 * mNumSamplesBaseTimePeriod;
    int startIndex = (subArray * 2 * N) + (whichHalf * N);

    FLOAT_TYPE *rex = nullptr;
    FLOAT_TYPE *imx = nullptr;
    
    FLOAT_TYPE rey = mBuffersReal[1]->getWritePointer(0) + startIndex;
    FLOAT_TYPE imy = mBuffersImag[1]->getWritePointer(0) + startIndex;
    FLOAT_TYPE reh = nullptr;
    FLOAT_TYPE imh = nullptr;
    
    memset(rey, 0, N * sizeof(FLOAT_TYPE));
    memset(imy, 0, N * sizeof(FLOAT_TYPE));
    
    for (int i = 0; i < mNumPartitions; ++i)
    {
        int k = trueMod((mCurrentInputIndex - i + 1), mNumPartitions);
       
        rex = mInputReal[k]->getWritePointer(0) + startIndex;
        imx = mInputImag[k]->getWritePointer(0) + startIndex;
        reh = mImpulsePartitionsReal[i]->getWritePointer(0) + startIndex;
        imh = mImpulsePartitionsImag[i]->getWritePointer(0) + startIndex;

        for (int j = 0; j < N; ++j)
        {
            rey[i] += (rex[i] * reh[i]) - (imx[i] * imh[i]);
            imy[i] += (rex[i] * imh[i]) + (imx[i] * reh[i]);
        }
    }
}







