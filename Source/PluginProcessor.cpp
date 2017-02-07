/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "util/SincFilter.hpp"
#include "util/util.h"

static const int SFLTR_SIZE = 128;
//==============================================================================
RtconvolveAudioProcessor::RtconvolveAudioProcessor()
{
    
}

RtconvolveAudioProcessor::~RtconvolveAudioProcessor()
{
}

//==============================================================================
const String RtconvolveAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RtconvolveAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RtconvolveAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double RtconvolveAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RtconvolveAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RtconvolveAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RtconvolveAudioProcessor::setCurrentProgram (int index)
{
}

const String RtconvolveAudioProcessor::getProgramName (int index)
{
    return String();
}

void RtconvolveAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void RtconvolveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ScopedPointer<float> sincFilter = new float[SFLTR_SIZE * samplesPerBlock];
    checkNull(sincFilter);

    genSincFilter((float*)sincFilter, SFLTR_SIZE * samplesPerBlock, 0.01f);
    mUniformConvolver[0] = new UPConvolver<float>(sincFilter, SFLTR_SIZE * samplesPerBlock, samplesPerBlock, 200);
    mUniformConvolver[1] = new UPConvolver<float>(sincFilter, SFLTR_SIZE * samplesPerBlock, samplesPerBlock, 200);
}

void RtconvolveAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RtconvolveAudioProcessor::setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet)
{
    // Reject any bus arrangements that are not compatible with your plugin

    const int numChannels = preferredSet.size();

   #if JucePlugin_IsMidiEffect
    if (numChannels != 0)
        return false;
   #elif JucePlugin_IsSynth
    if (isInput || (numChannels != 1 && numChannels != 2))
        return false;
   #else
    if (numChannels != 1 && numChannels != 2)
        return false;

    if (! AudioProcessor::setPreferredBusArrangement (! isInput, bus, preferredSet))
        return false;
   #endif

    return AudioProcessor::setPreferredBusArrangement (isInput, bus, preferredSet);
}
#endif

void RtconvolveAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        mUniformConvolver[channel]->processInput(channelData);
        const float* y = mUniformConvolver[channel]->getOutputBuffer();
        
        memcpy(channelData, y, buffer.getNumSamples() * sizeof(float));
    }
}

//==============================================================================
bool RtconvolveAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* RtconvolveAudioProcessor::createEditor()
{
    return new RtconvolveAudioProcessorEditor (*this);
}

//==============================================================================
void RtconvolveAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RtconvolveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RtconvolveAudioProcessor();
}
