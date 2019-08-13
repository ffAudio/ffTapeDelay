/*
  ==============================================================================

    A simple delay example with time and feedback knobs

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "TapeDelayProcessor.h"
#include "TapeDelayEditor.h"


String TapeDelayAudioProcessor::paramGain     ("gain");
String TapeDelayAudioProcessor::paramTime     ("time");
String TapeDelayAudioProcessor::paramFeedback ("feedback");


//==============================================================================
TapeDelayAudioProcessor::TapeDelayAudioProcessor()
  : mState (*this, &mUndoManager, "FFTapeDelay",
          {
              std::make_unique<AudioParameterFloat>(paramGain,     TRANS ("Input Gain"),    NormalisableRange<float>(0.0,    2.0, 0.1), mGain.get()),
              std::make_unique<AudioParameterFloat>(paramTime,     TRANS ("Delay TIme"),    NormalisableRange<float>(0.0, 2000.0, 1.0), mTime.get()),
              std::make_unique<AudioParameterFloat>(paramFeedback, TRANS ("Feedback Gain"), NormalisableRange<float>(0.0,    2.0, 0.1), mFeedback.get())
          })
{
    mState.addParameterListener (paramGain, this);
    mState.addParameterListener (paramTime, this);
    mState.addParameterListener (paramFeedback, this);
}

TapeDelayAudioProcessor::~TapeDelayAudioProcessor()
{
}

//==============================================================================
void TapeDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    mSampleRate = sampleRate;

    const int totalNumInputChannels  = getTotalNumInputChannels();

    // sample buffer for 2 seconds + 2 buffers safety
    mDelayBuffer.setSize (totalNumInputChannels, 2.0 * (samplesPerBlock + sampleRate), false, true);


}

void TapeDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void TapeDelayAudioProcessor::parameterChanged (const String &parameterID, float newValue)
{
    if (parameterID == paramGain) {
        mGain = newValue;
    }
    else if (parameterID == paramTime) {
        mTime = newValue;
    }
    else if (parameterID == paramFeedback) {
        mFeedback = newValue;
    }
}

bool TapeDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // we only support stereo and mono
    if (layouts.getMainInputChannels() == 0 || layouts.getMainInputChannels() > 2)
        return false;

    if (layouts.getMainOutputChannels() == 0 || layouts.getMainOutputChannels() > 2)
        return false;

    // we don't allow the narrowing the number of channels
    if (layouts.getMainInputChannels() > layouts.getMainOutputChannels())
        return false;

    return true;
}

void TapeDelayAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    if (Bus* inputBus = getBus (true, 0))
    {
        const float gain = mGain.get();
        const float time = mTime.get();
        const float feedback = mFeedback.get();

        for (int i=0; i < inputBus->getNumberOfChannels(); ++i) {
            const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            fillDelayBuffer (buffer, inputChannelNum, i, mWritePos, mLastInputGain, gain);
        }
        mLastInputGain = gain;

        const int64 readPos = static_cast<int64>((mDelayBuffer.getNumSamples() + mWritePos - (mSampleRate * time / 1000.0))) % mDelayBuffer.getNumSamples();

        if (Bus* outputBus = getBus (false, 0)) {
            for (int i=0; i<outputBus->getNumberOfChannels(); ++i) {
                const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer (i);

                fetchFromDelayBuffer (buffer, i % mDelayBuffer.getNumChannels(), outputChannelNum, readPos);
            }
        }

        // feedback
        for (int i=0; i<inputBus->getNumberOfChannels(); ++i) {
            const int outputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            feedbackDelayBuffer (buffer, outputChannelNum, i, mWritePos, mLastFeedbackGain, feedback);
        }
        mLastFeedbackGain = feedback;

        mWritePos += buffer.getNumSamples();
        mWritePos %= mDelayBuffer.getNumSamples();
    }
}

void TapeDelayAudioProcessor::fillDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut,
                                               const int64 writePos, float startGain, float endGain)
{
    if (mDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
        mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - writePos;
        const float midGain = mLastInputGain +  ((endGain - startGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
        mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), midPos, mLastInputGain, midGain);
        mDelayBuffer.copyFromWithRamp (channelOut, 0,        buffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
    }
}

void TapeDelayAudioProcessor::fetchFromDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 readPos)
{
    if (mDelayBuffer.getNumSamples() > readPos + buffer.getNumSamples()) {
        buffer.copyFrom (channelOut, 0, mDelayBuffer.getReadPointer (channelIn) + readPos, buffer.getNumSamples());
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - readPos;
        buffer.copyFrom (channelOut, 0,      mDelayBuffer.getReadPointer (channelIn) + readPos, midPos);
        buffer.copyFrom (channelOut, midPos, mDelayBuffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos);
    }
}

void TapeDelayAudioProcessor::feedbackDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut,
                                                   const int64 writePos, float startGain, float endGain)
{
    if (mDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
        mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - writePos;
        const float midGain = startGain +  ((endGain - startGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
        mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), midPos, startGain, midGain);
        mDelayBuffer.addFromWithRamp (channelOut, 0,        buffer.getWritePointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
    }
}

AudioProcessorValueTreeState& TapeDelayAudioProcessor::getValueTreeState()
{
    return mState;
}

//==============================================================================
bool TapeDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* TapeDelayAudioProcessor::createEditor()
{
    return new TapeDelayAudioProcessorEditor (*this);
}

//==============================================================================
void TapeDelayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    MemoryOutputStream stream(destData, false);
    mState.state.writeToStream (stream);
}

void TapeDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        mState.state = tree;
    }
}

//==============================================================================
const String TapeDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TapeDelayAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TapeDelayAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

double TapeDelayAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int TapeDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int TapeDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TapeDelayAudioProcessor::setCurrentProgram (int index)
{
}

const String TapeDelayAudioProcessor::getProgramName (int index)
{
    return String();
}

void TapeDelayAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeDelayAudioProcessor();
}
