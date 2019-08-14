/*
  ==============================================================================

    A simple delay example with time and feedback knobs

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
/**
*/
class TapeDelayAudioProcessor  :  public AudioProcessor,
                                    public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    TapeDelayAudioProcessor();
    ~TapeDelayAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void parameterChanged (const String &parameterID, float newValue) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    void writeToDelayBuffer (AudioSampleBuffer& buffer,
                             const int channelIn, const int channelOut,
                             const int writePos,
                             float startGain, float endGain,
                             bool replacing);

    void readFromDelayBuffer (AudioSampleBuffer& buffer,
                              const int channelIn, const int channelOut,
                              const int readPos,
                              float startGain, float endGain,
                              bool replacing);

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState& getValueTreeState();

    static String paramGain;
    static String paramTime;
    static String paramFeedback;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayAudioProcessor)

    Atomic<float>   mGain     {   0.0f };
    Atomic<float>   mTime     { 200.0f };
    Atomic<float>   mFeedback {  -6.0f };

    UndoManager                  mUndoManager;
    AudioProcessorValueTreeState mState;

    AudioSampleBuffer            mDelayBuffer;

    float mLastInputGain    = 0.0f;
    float mLastFeedbackGain = 0.0f;

    int    mWritePos        = 0;
    int    mExpectedReadPos = -1;
    double mSampleRate      = 0;
};
