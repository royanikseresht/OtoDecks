/*
  ==============================================================================

    DJAudioPlayer.cpp

  ==============================================================================
*/

#include "DJAudioPlayer.h"
#include <juce_core/juce_core.h>

DJAudioPlayer::DJAudioPlayer(juce::AudioFormatManager& _formatManager)
    : formatManager(_formatManager),
      forwardResampler(&forwardTransport, false),
      reverseResampler(&reverseTransport, false),
      isDraggingPosSlider(false),
      remixReady(false)
{
}

DJAudioPlayer::~DJAudioPlayer()
{
}

//==============================================================================
void DJAudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;

    forwardTransport.prepareToPlay(samplesPerBlockExpected, sampleRate);
    reverseTransport.prepareToPlay(samplesPerBlockExpected, sampleRate);

    forwardResampler.prepareToPlay(samplesPerBlockExpected, sampleRate);
    reverseResampler.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void DJAudioPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (isReversedFlag)
        reverseResampler.getNextAudioBlock(bufferToFill);
    else
        forwardResampler.getNextAudioBlock(bufferToFill);
}

void DJAudioPlayer::releaseResources()
{
    forwardTransport.releaseResources();
    reverseTransport.releaseResources();

    forwardResampler.releaseResources();
    reverseResampler.releaseResources();
}

void DJAudioPlayer::loadURL(juce::URL audioURL)
{
    // Stop playback first
    forwardTransport.stop();
    reverseTransport.stop();

    // Disconnect sources from transport before releasing them
    forwardTransport.setSource(nullptr);
    reverseTransport.setSource(nullptr);

    // Reset your unique_ptrs to release old sources and free memory
    forwardSource.reset();
    reverseMemorySource.reset();

    // Clear buffers (optional but clean)
    audioBuffer.clear();
    reversedBuffer.clear();

    // Now load the new audio
    juce::URL::InputStreamOptions options((juce::URL::ParameterHandling)0);
    auto* reader = formatManager.createReaderFor(audioURL.createInputStream(options));

    if (reader != nullptr)
    {
        audioBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&audioBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

        // Setup forward transport source
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        forwardTransport.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        forwardSource.reset(newSource.release());

        // Prepare reversed buffer and reverseMemorySource as before
        reversedBuffer.setSize(audioBuffer.getNumChannels(), audioBuffer.getNumSamples());

        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
        {
            auto* writePtr = reversedBuffer.getWritePointer(channel);
            auto* readPtr = audioBuffer.getReadPointer(channel);
            int numSamples = audioBuffer.getNumSamples();

            for (int i = 0; i < numSamples; ++i)
                writePtr[i] = readPtr[numSamples - 1 - i];
        }

        reverseMemorySource.reset(new OtoDecksAudio::MemoryAudioSource(reversedBuffer, false));
        reverseTransport.setSource(reverseMemorySource.get(), 0, nullptr, reader->sampleRate);

        isReversedFlag = false;
        forwardTransport.start();
    }
    else
    {
        DBG("Something went wrong loading the file");
    }
}


void DJAudioPlayer::setPositionRelative(double pos)
{
    pos = juce::jlimit(0.0, 1.0, pos);
    double length = getLengthInSeconds();
    double posSecs = length * pos;

    if (isReversedFlag && reverseMemorySource)
    {
        // In reverse mode, slider=0 is end of file, slider=1 is start
        reverseTransport.setPosition(juce::jmax(0.0, length - posSecs));
    }
    else
    {
        forwardTransport.setPosition(posSecs);
    }
}

void DJAudioPlayer::setGain(double gain)
{
    forwardTransport.setGain(gain);
    reverseTransport.setGain(gain);
}

void DJAudioPlayer::setSpeed(double ratio)
{
    forwardResampler.setResamplingRatio(ratio);
    reverseResampler.setResamplingRatio(ratio);
}

double DJAudioPlayer::getPositionRelative()
{
    double length = getLengthInSeconds();
    if (length <= 0.0) return 0.0;

    if (isReversedFlag && reverseMemorySource)
    {
        // Reverse transport position in seconds
        double posSecs = reverseTransport.getCurrentPosition();
        if (std::isnan(posSecs)) return 0.0;

        // Convert so slider still goes left → right for forward timeline
        return 1.0 - (posSecs / length);
    }
    else
    {
        double posSecs = forwardTransport.getCurrentPosition();
        if (std::isnan(posSecs)) return 0.0;

        return posSecs / length;
    }
}


void DJAudioPlayer::start()
{
    if (isReversedFlag)
    {
        // Convert actual reverse position to forward position before switching
        double posReverse = reverseTransport.getCurrentPosition();
        reverseTransport.stop();

        double forwardPos = getLengthInSeconds() - posReverse;
        forwardTransport.setPosition(forwardPos);
        forwardTransport.start();

        isReversedFlag = false;
    }
    else
    {
        if (!forwardTransport.isPlaying())
            forwardTransport.start();
    }
}


void DJAudioPlayer::startForward()
{
    if (isReversedFlag)
    {
        // Switch from reverse → forward playback
        double posReverse = reverseTransport.getCurrentPosition();
        reverseTransport.stop();

        double forwardPos = getLengthInSeconds() - posReverse;
        forwardTransport.setPosition(forwardPos);
        forwardTransport.start();

        isReversedFlag = false;
    }
    else
    {
        if (!forwardTransport.isPlaying())
            forwardTransport.start();
    }
}

void DJAudioPlayer::startReverse()
{
    if (!isReversedFlag)
    {
        // Switch from forward → reverse playback
        double posForward = forwardTransport.getCurrentPosition();
        forwardTransport.stop();

        reverseTransport.setPosition(getLengthInSeconds() - posForward);
        reverseTransport.start();

        isReversedFlag = true;
    }
    else
    {
        // Already in reverse mode, just start/resume if not playing
        if (!reverseTransport.isPlaying())
            reverseTransport.start();
    }
}

void DJAudioPlayer::stop()
{
    if (isReversedFlag)
        reverseTransport.stop();
    else
        forwardTransport.stop();
}



void DJAudioPlayer::setPosition(double posInSecs)
{
    if (isReversedFlag && reverseMemorySource)
    {
        int posInSamples = static_cast<int>(posInSecs * currentSampleRate);
        int reversePos = reversedBuffer.getNumSamples() - posInSamples - 1;
        reverseMemorySource->setPosition(reversePos);
        reverseTransport.setPosition(posInSecs);
    }
    else
    {
        forwardTransport.setPosition(posInSecs);
    }
}

double DJAudioPlayer::sendTimer()
{
    if (isReversedFlag && reverseMemorySource)
        return reverseMemorySource->getCurrentPosition();
    else
        return forwardTransport.getCurrentPosition();
}

bool DJAudioPlayer::isTrackFinished()
{
    if (isReversedFlag)
        return reverseTransport.hasStreamFinished();
    else
        return forwardTransport.hasStreamFinished();
}

double DJAudioPlayer::getCurrentPosition() const
{
    if (isReversedFlag)
        return reverseTransport.getCurrentPosition();
    else
        return forwardTransport.getCurrentPosition();
}

double DJAudioPlayer::getLengthInSeconds() const
{
    return forwardTransport.getLengthInSeconds(); // Both should be equal
}

bool DJAudioPlayer::isPlaying() const
{
    return isReversedFlag ? reverseTransport.isPlaying() : forwardTransport.isPlaying();
}

