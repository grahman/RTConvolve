/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
RtconvolveAudioProcessorEditor::RtconvolveAudioProcessorEditor (RtconvolveAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , processor (p)
    , mButtonChooseIR("Choose Impulse Response")
{

    mButtonChooseIR.changeWidthToFitText();
    setSize (400, 300);
    
    Rectangle<int> boundingBox = getBounds();
    mButtonChooseIR.setBounds(boundingBox);

    addAndMakeVisible(&mButtonChooseIR);
    
    mButtonChooseIR.addListener(this);

}

RtconvolveAudioProcessorEditor::~RtconvolveAudioProcessorEditor()
{
}

void RtconvolveAudioProcessorEditor::buttonClicked(juce::Button* b)
{
    mFileChooser = std::make_unique<FileChooser>("Select Impulse Response");
    mFileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this](const FileChooser& chooser)
        {
            auto results = chooser.getResults();
            if (results.isEmpty()) return;

            File ir = results[0];
            AudioFormatManager manager;
            manager.registerBasicFormats();

            auto formatReader = std::unique_ptr<AudioFormatReader>{manager.createReaderFor(ir)};
            AudioSampleBuffer sampleBuffer(formatReader->numChannels, (int)formatReader->lengthInSamples);
            formatReader->read(&sampleBuffer, 0, (int)formatReader->lengthInSamples, 0, 1, 1);

            processor.setImpulseResponse(sampleBuffer, ir.getFullPathName());
        });
}

//==============================================================================
void RtconvolveAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::white);

    g.setColour (Colours::black);
    g.setFont (15.0f);

}

void RtconvolveAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
