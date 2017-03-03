/*
  ==============================================================================

    A simple editor for the delay plugin

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "TapeDelayProcessor.h"
#include "TapeDelayEditor.h"


//==============================================================================
FftapeDelayAudioProcessorEditor::FftapeDelayAudioProcessorEditor (FftapeDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    mGainSlider     = new Slider (Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow);
    mTimeSlider     = new Slider (Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow);
    mFeedbackSlider = new Slider (Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxBelow);

    addAndMakeVisible (mGainSlider);
    addAndMakeVisible (mTimeSlider);
    addAndMakeVisible (mFeedbackSlider);

    mGainAttachment     = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), FftapeDelayAudioProcessor::paramGain, *mGainSlider);
    mTimeAttachment     = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), FftapeDelayAudioProcessor::paramTime, *mTimeSlider);
    mFeedbackAttachment = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), FftapeDelayAudioProcessor::paramFeedback, *mFeedbackSlider);

    setSize (400, 250);
}

FftapeDelayAudioProcessorEditor::~FftapeDelayAudioProcessorEditor()
{
    mFeedbackAttachment = nullptr;
    mTimeAttachment     = nullptr;
    mGainAttachment     = nullptr;

    mFeedbackSlider = nullptr;
    mTimeSlider     = nullptr;
    mGainSlider     = nullptr;
}

//==============================================================================
void FftapeDelayAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::darkgoldenrod);

    g.setColour (Colours::silver);
    g.setFont   (24.0f);
    Rectangle<int> box (getX(), getBottom() - 40, getWidth() / 3, 40);
    g.drawFittedText (TRANS ("Gain"), box, Justification::centred, 1);
    box.setX (box.getRight());
    g.drawFittedText (TRANS ("Time"), box, Justification::centred, 1);
    box.setX (box.getRight());
    g.drawFittedText (TRANS ("Feedback"), box, Justification::centred, 1);
}

void FftapeDelayAudioProcessorEditor::resized()
{
    Rectangle<int> box (getLocalBounds());
    box.setWidth (getWidth() / 3);
    box.setHeight (getHeight() - 40);
    mGainSlider->setBounds (box);
    box.setX (box.getRight());
    mTimeSlider->setBounds (box);
    box.setX (box.getRight());
    mFeedbackSlider->setBounds (box);
}
