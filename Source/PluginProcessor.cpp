/*
  ==============================================================================

    A simple delay example with time and feedback knobs

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


String FftapeDelayAudioProcessor::paramGain     ("gain");
String FftapeDelayAudioProcessor::paramTime     ("time");
String FftapeDelayAudioProcessor::paramFeedback ("feedback");


//==============================================================================
FftapeDelayAudioProcessor::FftapeDelayAudioProcessor() :
    mInputGain(1.0),
    mLastInputGain (0.0),
    mTime (100.0),
    mFeedbackGain (0.6),
    mLastFeedbackGain (0.0),
    mWritePos (0),
    mSampleRate (0)
{
    mUndoManager = new UndoManager();
    mState       = new AudioProcessorValueTreeState (*this, mUndoManager);

    mState->createAndAddParameter(paramGain,     "Gain",     TRANS ("Input Gain"),    NormalisableRange<float> (0.0,    2.0, 0.1), mInputGain,     nullptr, nullptr);
    mState->createAndAddParameter(paramTime,     "Time",     TRANS ("Delay time"),    NormalisableRange<float> (0.0, 2000.0, 1.0), mTime,          nullptr, nullptr);
    mState->createAndAddParameter(paramFeedback, "Feedback", TRANS ("Feedback Gain"), NormalisableRange<float> (0.0,    2.0, 0.1), mFeedbackGain,  nullptr, nullptr);

    mState->state = ValueTree ("FFTapeDelay");

    mState->addParameterListener (paramGain, this);
    mState->addParameterListener (paramFeedback, this);
    mState->addParameterListener (paramTime, this);
}

FftapeDelayAudioProcessor::~FftapeDelayAudioProcessor()
{
    mState->removeParameterListener (paramGain, this);
    mState->removeParameterListener (paramFeedback, this);
    mState->removeParameterListener (paramTime, this);

    mState = nullptr;
    mUndoManager = nullptr;
}

//==============================================================================
const String FftapeDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FftapeDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FftapeDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double FftapeDelayAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int FftapeDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FftapeDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FftapeDelayAudioProcessor::setCurrentProgram (int index)
{
}

const String FftapeDelayAudioProcessor::getProgramName (int index)
{
    return String();
}

void FftapeDelayAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void FftapeDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    mSampleRate = sampleRate;

    const int totalNumInputChannels  = getTotalNumInputChannels();

    // sample buffer for 2 seconds + 2 buffers safety
    mDelayBuffer.setSize (totalNumInputChannels, 2.0 * (samplesPerBlock + sampleRate));


}

void FftapeDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}


bool FftapeDelayAudioProcessor::setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet)
{
    // accept only one input and one output bus
    if (bus > 0) {
        return false;
    }

    // accept input bus == output bus
    return AudioProcessor::setPreferredBusArrangement (isInput, bus, preferredSet);
}


void FftapeDelayAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    AudioChannelSet inputBus = busArrangement.inputBuses.getUnchecked (0).channels;

    for (int i=0; i<inputBus.size(); ++i) {
        const int inputChannelNum = busArrangement.getChannelIndexInProcessBlockBuffer (true, 0, i);
        fillDelayBuffer (buffer, inputChannelNum, i, mWritePos);
    }
    mLastInputGain = mInputGain;

    const int64 readPos = static_cast<int64>((mDelayBuffer.getNumSamples() + mWritePos - (mSampleRate * mTime / 1000.0))) % mDelayBuffer.getNumSamples();

    AudioChannelSet outputBus = busArrangement.outputBuses.getUnchecked (0).channels;
    for (int i=0; i<outputBus.size(); ++i) {
        const int outputChannelNum = busArrangement.getChannelIndexInProcessBlockBuffer (false, 0, i);

        fetchFromDelayBuffer (buffer, i % mDelayBuffer.getNumChannels(), outputChannelNum, readPos);
    }

    for (int i=0; i<inputBus.size(); ++i) {
        const int outputChannelNum = busArrangement.getChannelIndexInProcessBlockBuffer (false, 0, i);
        feedbackDelayBuffer (buffer, outputChannelNum, i, mWritePos);
    }
    mLastFeedbackGain = mFeedbackGain;

    mWritePos += buffer.getNumSamples();
    mWritePos %= mDelayBuffer.getNumSamples();
}

void FftapeDelayAudioProcessor::fillDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos)
{
    if (mDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
        mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), buffer.getNumSamples(), mLastInputGain, mInputGain);
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - writePos;
        const float midGain = mLastInputGain +  ((mInputGain - mLastInputGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
        mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), midPos, mLastInputGain, midGain);
        mDelayBuffer.copyFromWithRamp (channelOut, 0,        buffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos, midGain, mInputGain);
    }
}

void FftapeDelayAudioProcessor::fetchFromDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 readPos)
{
    if (mDelayBuffer.getNumSamples() > readPos + buffer.getNumSamples()) {
        buffer.copyFromWithRamp (channelOut, 0, mDelayBuffer.getReadPointer (channelIn) + readPos, buffer.getNumSamples(), mLastFeedbackGain, mFeedbackGain);
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - readPos;
        buffer.copyFrom (channelOut, 0,      mDelayBuffer.getReadPointer (channelIn) + readPos, midPos);
        buffer.copyFrom (channelOut, midPos, mDelayBuffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos);
    }
}

void FftapeDelayAudioProcessor::feedbackDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos)
{
    if (mDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
        mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), buffer.getNumSamples(), mLastFeedbackGain, mFeedbackGain);
    }
    else {
        const int64 midPos  = mDelayBuffer.getNumSamples() - writePos;
        const float midGain = mLastFeedbackGain +  ((mFeedbackGain - mLastFeedbackGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
        mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), midPos, mLastFeedbackGain, midGain);
        mDelayBuffer.addFromWithRamp (channelOut, 0,        buffer.getWritePointer (channelIn), buffer.getNumSamples() - midPos, midGain, mFeedbackGain);
    }
}

AudioProcessorValueTreeState& FftapeDelayAudioProcessor::getValueTreeState()
{
    return *mState;
}

void FftapeDelayAudioProcessor::parameterChanged (const String &parameterID, float newValue)
{
    if (parameterID == paramGain) {
        mInputGain = newValue;
    }
    else if (parameterID == paramTime) {
        mTime = newValue;
    }
    else if (parameterID == paramFeedback) {
        mFeedbackGain = newValue;
    }
}


//==============================================================================
bool FftapeDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* FftapeDelayAudioProcessor::createEditor()
{
    return new FftapeDelayAudioProcessorEditor (*this);
}

//==============================================================================
void FftapeDelayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    MemoryOutputStream stream(destData, false);
    mState->state.writeToStream (stream);
}

void FftapeDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        mState->state = tree;
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FftapeDelayAudioProcessor();
}
