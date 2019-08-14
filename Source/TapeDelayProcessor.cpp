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

    mExpectedReadPos = -1;
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

        // write original to delay
        for (int i=0; i < inputBus->getNumberOfChannels(); ++i)
        {
            const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            writeToDelayBuffer (buffer, inputChannelNum, i, mWritePos, 1.0f, 1.0f, true);
        }

        // adapt dry gain
        buffer.applyGainRamp (0, buffer.getNumSamples(), mLastInputGain, gain);
        mLastInputGain = gain;

        // read delayed signal
        auto readPos = static_cast<int>(mWritePos - (mSampleRate * time / 1000.0));
        if (readPos < 0)
            readPos += mDelayBuffer.getNumSamples();

        if (Bus* outputBus = getBus (false, 0))
        {
            // if has run before
            if (mExpectedReadPos >= 0)
            {
                // fade out if readPos is off
                auto endGain = (readPos == mExpectedReadPos) ? 1.0f : 0.0f;
                for (int i=0; i<outputBus->getNumberOfChannels(); ++i)
                {
                    const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer (i);
                    readFromDelayBuffer (buffer, i % mDelayBuffer.getNumChannels(), outputChannelNum, mExpectedReadPos, 1.0, endGain, false);
                }
            }

            // fade in at new position
            if (readPos != mExpectedReadPos)
            {
                for (int i=0; i<outputBus->getNumberOfChannels(); ++i)
                {
                    const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer (i);
                    readFromDelayBuffer (buffer, i % mDelayBuffer.getNumChannels(), outputChannelNum, readPos, 0.0, 1.0, false);
                }
            }
        }

        // add feedback to delay
        for (int i=0; i<inputBus->getNumberOfChannels(); ++i)
        {
            const int outputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            writeToDelayBuffer (buffer, outputChannelNum, i, mWritePos, mLastFeedbackGain, feedback, false);
        }
        mLastFeedbackGain = feedback;

        // advance positions
        mWritePos += buffer.getNumSamples();
        if (mWritePos >= mDelayBuffer.getNumSamples())
            mWritePos -= mDelayBuffer.getNumSamples();

        mExpectedReadPos = readPos + buffer.getNumSamples();
        if (mExpectedReadPos >= mDelayBuffer.getNumSamples())
            mExpectedReadPos -= mDelayBuffer.getNumSamples();
    }
}

void TapeDelayAudioProcessor::writeToDelayBuffer (AudioSampleBuffer& buffer,
                                                  const int channelIn, const int channelOut,
                                                  const int writePos, float startGain, float endGain, bool replacing)
{
    if (writePos + buffer.getNumSamples() <= mDelayBuffer.getNumSamples())
    {
        if (replacing)
            mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), buffer.getNumSamples(), startGain, endGain);
        else
            mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos  = mDelayBuffer.getNumSamples() - writePos;
        const auto midGain = jmap (float (midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            mDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn),         midPos, mLastInputGain, midGain);
            mDelayBuffer.copyFromWithRamp (channelOut, 0,        buffer.getReadPointer (channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            mDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn),         midPos, mLastInputGain, midGain);
            mDelayBuffer.addFromWithRamp (channelOut, 0,        buffer.getReadPointer (channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
    }
}

void TapeDelayAudioProcessor::readFromDelayBuffer (AudioSampleBuffer& buffer,
                                                   const int channelIn, const int channelOut,
                                                   const int readPos,
                                                   float startGain, float endGain,
                                                   bool replacing)
{
    if (readPos + buffer.getNumSamples() <= mDelayBuffer.getNumSamples())
    {
        if (replacing)
            buffer.copyFromWithRamp (channelOut, 0, mDelayBuffer.getReadPointer (channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
        else
            buffer.addFromWithRamp (channelOut, 0, mDelayBuffer.getReadPointer (channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos  = mDelayBuffer.getNumSamples() - readPos;
        const auto midGain = jmap (float (midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            buffer.copyFromWithRamp (channelOut, 0,      mDelayBuffer.getReadPointer (channelIn, readPos), midPos, startGain, midGain);
            buffer.copyFromWithRamp (channelOut, midPos, mDelayBuffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            buffer.addFromWithRamp (channelOut, 0,      mDelayBuffer.getReadPointer (channelIn, readPos), midPos, startGain, midGain);
            buffer.addFromWithRamp (channelOut, midPos, mDelayBuffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
        }
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
