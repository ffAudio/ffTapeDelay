/*
  ==============================================================================

    A simple delay example with time and feedback knobs

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#ifndef TAPEDELAYPROCESSOR_H_INCLUDED
#define TAPEDELAYPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
/**
*/
class FftapeDelayAudioProcessor  :  public AudioProcessor,
                                    public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    FftapeDelayAudioProcessor();
    ~FftapeDelayAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void parameterChanged (const String &parameterID, float newValue) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    void fillDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos, float startGain, float endGain);

    void fetchFromDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 readPos);

    void feedbackDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos, float startGain, float endGain);

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FftapeDelayAudioProcessor)

    Atomic<float>   mGain     {   1.0 };
    Atomic<float>   mTime     { 200.0 };
    Atomic<float>   mFeedback {   0.6 };

    UndoManager                  mUndoManager;
    AudioProcessorValueTreeState mState;

    AudioSampleBuffer            mDelayBuffer;

    float mLastInputGain    = 0.0f;
    float mLastFeedbackGain = 0.0f;

    int64 mWritePos         = 0;
    double mSampleRate      = 0;
};


#endif  // TAPEDELAYPROCESSOR_H_INCLUDED
