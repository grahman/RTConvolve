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

//==============================================================================
RtconvolveAudioProcessor::RtconvolveAudioProcessor()
 : mSampleRate(0.0)
 , mBufferSize(0)
 , mImpulseResponseFilePath("")
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

void RtconvolveAudioProcessor::setImpulseResponse(const AudioSampleBuffer& impulseResponseBuffer, const juce::String pathToImpulse)
{
    juce::ScopedLock lock(mLoadingLock);
    mImpulseResponseFilePath = pathToImpulse;
    AudioSampleBuffer impulseResponse(impulseResponseBuffer);
    
    if (impulseResponseBuffer.getNumChannels() == 2)
    {
        float *impulseResponseLeft = impulseResponse.getWritePointer(0);
        float *impulseResponseRight = impulseResponse.getWritePointer(1);
        
        normalizeStereoImpulseResponse(impulseResponseLeft, impulseResponseRight, impulseResponse.getNumSamples());
        mConvolutionManager[0].setImpulseResponse(impulseResponseLeft, impulseResponse.getNumSamples());
        mConvolutionManager[1].setImpulseResponse(impulseResponseRight, impulseResponse.getNumSamples());
    }
    else
    {
        float *ir = impulseResponse.getWritePointer(0);
        
        normalizeMonoImpulseResponse(ir, impulseResponse.getNumSamples());
        mConvolutionManager[0].setImpulseResponse(ir, impulseResponse.getNumSamples());
        mConvolutionManager[1].setImpulseResponse(ir, impulseResponse.getNumSamples());
    }
}

//==============================================================================
void RtconvolveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mConvolutionManager[0].setBufferSize(samplesPerBlock);
    mConvolutionManager[1].setBufferSize(samplesPerBlock);
}

void RtconvolveAudioProcessor::releaseResources()
{
}

bool RtconvolveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainInputChannelSet();
    return main == AudioChannelSet::mono() || main == AudioChannelSet::stereo();
}

void RtconvolveAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    juce::ScopedTryLock tryLock(mLoadingLock);
    
    if (tryLock.isLocked())
    {
        for (int channel = 0; channel < 1; ++channel)
        {
            float* channelData = buffer.getWritePointer (channel);
            mConvolutionManager[channel].processInput(channelData);
            const float* y = mConvolutionManager[channel].getOutputBuffer();
            memcpy(channelData, y, buffer.getNumSamples() * sizeof(float));
            
            if (buffer.getNumChannels() == 2)
            {
                float *channelDataR = buffer.getWritePointer(1);
                const float* y = mConvolutionManager[0].getOutputBuffer();
                memcpy(channelDataR, y, buffer.getNumSamples() * sizeof(float));
            }
        }
    }
    else
    {
        buffer.clear();
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
    XmlElement xml("STATEINFO");
    xml.setAttribute("impulseResponseFilePath", mImpulseResponseFilePath);
    copyXmlToBinary(xml, destData);
}

void RtconvolveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    String impulseResponseFilePath = xml->getStringAttribute("impulseResponseFilePath", "");
    juce::File ir(impulseResponseFilePath);
    AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> formatReader{manager.createReaderFor(ir)};
    if (formatReader)
    {
        AudioSampleBuffer sampleBuffer(formatReader->numChannels, (int)formatReader->lengthInSamples);
        formatReader->read(&sampleBuffer, 0, (int)formatReader->lengthInSamples, 0, 1, 1);
        setImpulseResponse(sampleBuffer, impulseResponseFilePath);
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RtconvolveAudioProcessor();
}
