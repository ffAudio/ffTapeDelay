/*
  ==============================================================================

    A simple editor for the delay plugin

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"


//==============================================================================
/**
*/
class FftapeDelayAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    FftapeDelayAudioProcessorEditor (FftapeDelayAudioProcessor&);
    ~FftapeDelayAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FftapeDelayAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FftapeDelayAudioProcessorEditor)

    ScopedPointer<Slider> mGainSlider;
    ScopedPointer<Slider> mTimeSlider;
    ScopedPointer<Slider> mFeedbackSlider;

    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> mGainAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> mTimeAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> mFeedbackAttachment;
};


#endif  // PLUGINEDITOR_H_INCLUDED
