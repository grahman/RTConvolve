//
//  RefCountedAudioBuffer.h
//  RTConvolve
//
//  Created by Graham Barab on 2/9/17.
//
//

#ifndef RefCountedAudioBuffer_h
#define RefCountedAudioBuffer_h

#include "../JuceLibraryCode/JuceHeader.h"

template <typename FLOAT_TYPE>
class RefCountedAudioBuffer: public juce::AudioBuffer<FLOAT_TYPE>, public juce::ReferenceCountedObject
{
public:
    RefCountedAudioBuffer(int numChannels, int size)
    : juce::AudioBuffer<FLOAT_TYPE>(numChannels, size)
    {
    }
};

#endif /* RefCountedAudioBuffer_h */
