//
//  TimeDistributedFFTConvolver.hpp
//  RTConvolve
//
//  Created by Graham Barab on 2/6/17.
//
//
// MIT License
//
// Copyright (c) 2026 Graham Barab
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "util/util.h"
#include "util/fft.hpp"

#include <algorithm>
#include <stdexcept>

static const float TWOPI = 2.0 * M_PI;
static const float NTWOPI = -2.0 * M_PI;

template <typename FLOAT_TYPE>
TimeDistributedFFTConvolver<FLOAT_TYPE>::TimeDistributedFFTConvolver(FLOAT_TYPE *impulseResponse, size_t numSamplesImpulseResponse, int bufferSize)
 : mCurrentPhase(kPhase3)
 , mCurrentInputIndex(0)
{
    mNumSamplesBaseTimePeriod = bufferSize;
    
    size_t partitionSize = 4 * mNumSamplesBaseTimePeriod;
    
    if (isPowerOfTwo(bufferSize) == false)
    {
        throw std::invalid_argument("bufferSize must be a power of 2");
    }
    
    mNumPartitions = (numSamplesImpulseResponse / partitionSize) + !!(numSamplesImpulseResponse % partitionSize);
    
    mImpulsePartitionsReal.reserve(mNumPartitions);
    mImpulsePartitionsImag.reserve(mNumPartitions);
    mInputReal.reserve(mNumPartitions);
    mInputImag.reserve(mNumPartitions);
    
    for (size_t i = 0; i < mNumPartitions; ++i)
    {
        size_t samplesToCopy = std::min((numSamplesImpulseResponse - (i * partitionSize)), partitionSize);
        
        /* Allocate and fill real array */
        mImpulsePartitionsReal.emplace_back(2 * partitionSize);
        checkNull(mImpulsePartitionsReal[i].data());
        std::fill(mImpulsePartitionsReal[i].begin(), mImpulsePartitionsReal[i].end(), 0);
        
        FLOAT_TYPE *partition = mImpulsePartitionsReal[i].data();

        memcpy(partition, impulseResponse + (i * partitionSize), samplesToCopy * sizeof(FLOAT_TYPE));
        
        /* Allocate imag array */
        
        mImpulsePartitionsImag.emplace_back(2 * partitionSize);

        checkNull(mImpulsePartitionsImag[i].data());
        std::fill(mImpulsePartitionsImag[i].begin(), mImpulsePartitionsImag[i].end(), 0);
        FLOAT_TYPE *partitionImag = mImpulsePartitionsImag[i].data();
        
        std::unique_ptr<FLOAT_TYPE[]> tempReal {new FLOAT_TYPE[2 * partitionSize]};
        checkNull(tempReal.get());
        memset(tempReal.get(), 0, 2 * partitionSize * sizeof(FLOAT_TYPE));
        
        std::unique_ptr<FLOAT_TYPE[]> tempImag {new FLOAT_TYPE[2 * partitionSize]};
        checkNull(tempImag.get());
        memset(tempImag.get(), 0, 2 * partitionSize * sizeof(FLOAT_TYPE));
        
        /* Calculate transform */
        fft_priv(partition, partitionImag, tempReal.get(), tempImag.get(), 2 * partitionSize);
        
        /* Allocate an input buffer */
        mInputReal.emplace_back(2 * partitionSize);
        mInputImag.emplace_back(2 * partitionSize);

        std::fill(mInputReal[i].begin(), mInputReal[i].end(), 0);
        std::fill(mInputImag[i].begin(), mInputImag[i].end(), 0);
    }
    
    mOutputReal.reset(2 * partitionSize);
    checkNull(mOutputReal.data());
    std::fill(mOutputReal.begin(), mOutputReal.end(), 0);
    
    mOutputImag.reset(2 * partitionSize);
    checkNull(mOutputImag.data());
    std::fill(mOutputImag.begin(), mOutputImag.end(), 0);
    
    mBuffersReal[0] = std::make_shared<SampleBuffer>(2 * partitionSize);
    mBuffersReal[1] = std::make_shared<SampleBuffer>(2 * partitionSize);
    mBuffersReal[2] = std::make_shared<SampleBuffer>(2 * partitionSize);

    mBuffersImag[0] = std::make_shared<SampleBuffer>(2 * partitionSize);
    mBuffersImag[1] = std::make_shared<SampleBuffer>(2 * partitionSize);
    mBuffersImag[2] = std::make_shared<SampleBuffer>(2 * partitionSize);

    for (int i = 0; i < 3; ++i)
    {
        std::fill(mBuffersReal[i]->begin(), mBuffersReal[i]->end(), 0);
        std::fill(mBuffersImag[i]->begin(), mBuffersImag[i]->end(), 0);
    }
    
    mPreviousTail.reset(partitionSize);
    checkNull(mPreviousTail.data());
    std::fill(mPreviousTail.begin(), mPreviousTail.end(), 0);
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::processInput(FLOAT_TYPE *input)
{
    int partitionSize = 4 * mNumSamplesBaseTimePeriod;
    mCurrentPhase = trueMod((mCurrentPhase + 1), 4);
    int Q = mCurrentPhase * mNumSamplesBaseTimePeriod;
    
    switch (mCurrentPhase)
    {
    case kPhase0:
        {
            promoteBuffers();

            /* Buffer 'C' */
            std::fill(mBuffersReal[2]->begin(), mBuffersReal[2]->end(), 0);
            std::fill(mBuffersImag[2]->begin(), mBuffersImag[2]->end(), 0);
            
            FLOAT_TYPE *cr = mBuffersReal[2]->data();
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->data();

            forwardDecomposition(cr, ci, 2 * partitionSize, 0);
            
            /* Buffer 'B' */
            
            FLOAT_TYPE *br = mBuffersReal[1]->data();
            FLOAT_TYPE *bi = mBuffersImag[1]->data();
            
            fft(br, bi, partitionSize); /* X(2k) */
            
            FLOAT_TYPE *rex0 = mInputReal[mCurrentInputIndex].data();
            FLOAT_TYPE *imx0 = mInputImag[mCurrentInputIndex].data();
            
            memcpy(rex0, br, partitionSize * sizeof(FLOAT_TYPE));
            memcpy(imx0, bi, partitionSize * sizeof(FLOAT_TYPE));
            
            performConvolutions(0, 0);  /* Perform first half of convolutions for vector Y(2k) */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->data();
            FLOAT_TYPE *ai = mBuffersImag[0]->data();
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 0);
            prepareOutput();
            break;
        }
        case kPhase1:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->data();
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->data();
            forwardDecomposition(cr, ci, 2 * partitionSize, 1);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->data();
            FLOAT_TYPE *bi = mBuffersImag[1]->data();
            performConvolutions(0, 1);
            ifft(br, bi, partitionSize);    /* Y(2k) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->data();
            FLOAT_TYPE *ai = mBuffersImag[0]->data();
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 1);
            prepareOutput();
            break;
        }
        case kPhase2:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->data();
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->data();
            forwardDecomposition(cr, ci, 2 * partitionSize, 2);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->data();
            FLOAT_TYPE *bi = mBuffersImag[1]->data();
            FLOAT_TYPE *rex0 = mInputReal[mCurrentInputIndex].data();
            FLOAT_TYPE *imx0 = mInputImag[mCurrentInputIndex].data();

            fft(br + partitionSize, bi + partitionSize, partitionSize);
            memcpy(rex0 + partitionSize, br + partitionSize, partitionSize * sizeof(FLOAT_TYPE));
            memcpy(imx0 + partitionSize, bi + partitionSize, partitionSize * sizeof(FLOAT_TYPE));
            performConvolutions(1, 0);
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->data();
            FLOAT_TYPE *ai = mBuffersImag[0]->data();
            
            inverseDecomposition(ar, ai, 2 * partitionSize, 2);
            prepareOutput();
            break;
        }
        case kPhase3:
        {
            /* Buffer 'C' */
            FLOAT_TYPE *cr = mBuffersReal[2]->data();
            memcpy(cr + Q, input, mNumSamplesBaseTimePeriod * sizeof(FLOAT_TYPE));
            
            FLOAT_TYPE *ci = mBuffersImag[2]->data();
            forwardDecomposition(cr, ci, 2 * partitionSize, 3);

            /* Buffer 'B' */
            FLOAT_TYPE *br = mBuffersReal[1]->data();
            FLOAT_TYPE *bi = mBuffersImag[1]->data();
            performConvolutions(1, 1);
            ifft(br + partitionSize, bi + partitionSize, partitionSize);    /* Y(2k+1) sub-ifft */
            
            /* Buffer 'A' */
            FLOAT_TYPE *ar = mBuffersReal[0]->data();
            FLOAT_TYPE *ai = mBuffersImag[0]->data();

            inverseDecomposition(ar, ai, 2 * partitionSize, 3);
            
            prepareOutput();
            break;
        }
    }
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::prepareOutput()
{
    FLOAT_TYPE *out = mOutputReal.data();
    FLOAT_TYPE *ar = mBuffersReal[0]->data();
    FLOAT_TYPE *tail = mPreviousTail.data();
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
    auto temp = std::move(mBuffersReal[0]);
    mBuffersReal[0] = std::move(mBuffersReal[1]);
    mBuffersReal[1] = std::move(mBuffersReal[2]);
    mBuffersReal[2] = std::move(temp);
    
    temp = std::move(mBuffersImag[0]);
    mBuffersImag[0] = std::move(mBuffersImag[1]);
    mBuffersImag[1] = std::move(mBuffersImag[2]);
    mBuffersImag[2] = std::move(temp);
    
    mCurrentInputIndex = trueMod((mCurrentInputIndex + 1), (int)mNumPartitions);
}

template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, size_t length, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    size_t N = length;
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    
    size_t N8 = N >> 3;
    size_t N2 = N >> 1;
    size_t Q = whichQuarter * N8;
    
    for (size_t i = 0; i < N8; ++i)
    {
        size_t j = i + Q;
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
void TimeDistributedFFTConvolver<FLOAT_TYPE>::forwardDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, size_t length)
{
    for (int i = 0; i < 4; ++i)
    {
        forwardDecomposition(rex, imx, length, i);
    }
}


template <typename FLOAT_TYPE>
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecomposition(FLOAT_TYPE *rex, FLOAT_TYPE *imx, size_t N, int whichQuarter)
{
    if (whichQuarter > 3)
    {
        throw std::invalid_argument("Parameter \"whichQuarter\" must be between 0 and 3");
    }
    
    FLOAT_TYPE *re = rex;
    FLOAT_TYPE *im = imx;
    
    size_t N8 = N >> 3;
    size_t N2 = N >> 1;
    size_t Q = whichQuarter * N8;
        
    for (size_t i = 0; i < N8; ++i)
    {
        size_t j = i + Q;
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
void TimeDistributedFFTConvolver<FLOAT_TYPE>::inverseDecompositionComplete(FLOAT_TYPE *rex, FLOAT_TYPE *imx, size_t length)
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
    
    FLOAT_TYPE *rey = mBuffersReal[1]->data() + startIndex;
    FLOAT_TYPE *imy = mBuffersImag[1]->data() + startIndex;
    FLOAT_TYPE *reh = nullptr;
    FLOAT_TYPE *imh = nullptr;
    
    memset(rey, 0, N * sizeof(FLOAT_TYPE));
    memset(imy, 0, N * sizeof(FLOAT_TYPE));
    
    for (int i = 0; i < mNumPartitions; ++i)
    {
        int k = trueMod((mCurrentInputIndex - i), (int)mNumPartitions);
       
        rex = mInputReal[k].data() + startIndex;
        imx = mInputImag[k].data() + startIndex;
        reh = mImpulsePartitionsReal[i].data() + startIndex;
        imh = mImpulsePartitionsImag[i].data() + startIndex;

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
void TimeDistributedFFTConvolver<FLOAT_TYPE>::fft_priv(FLOAT_TYPE *rex, FLOAT_TYPE *imx, FLOAT_TYPE *trx, FLOAT_TYPE *tix, size_t N)
{
    forwardDecompositionComplete(rex, imx, N);
    size_t N2 = N >> 1;
    
    fft(rex, imx, N2);
    fft(rex+N2, imx+N2, N2);
}

