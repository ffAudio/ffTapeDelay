/*
  ==============================================================================

    A simple editor for the delay plugin

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TapeDelayProcessor.h"


//==============================================================================
/**
*/
class TapeDelayAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    TapeDelayAudioProcessorEditor (TapeDelayAudioProcessor&);
    ~TapeDelayAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TapeDelayAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayAudioProcessorEditor)

    Slider mGainSlider      { Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow };
    Slider mTimeSlider      { Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow };
    Slider mFeedbackSlider  { Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow };

    AudioProcessorValueTreeState::SliderAttachment mGainAttachment      { processor.getValueTreeState(), TapeDelayAudioProcessor::paramGain,     mGainSlider };
    AudioProcessorValueTreeState::SliderAttachment mTimeAttachment      { processor.getValueTreeState(), TapeDelayAudioProcessor::paramTime,     mTimeSlider };
    AudioProcessorValueTreeState::SliderAttachment mFeedbackAttachment  { processor.getValueTreeState(), TapeDelayAudioProcessor::paramFeedback, mFeedbackSlider };
};
