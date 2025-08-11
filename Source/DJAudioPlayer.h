/*
  ==============================================================================

    DJAudioPlayer.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "MemoryAudioSource.h"

class DJAudioPlayer : public juce::AudioSource {
    public:

        DJAudioPlayer(juce::AudioFormatManager& _formatManager);
        ~DJAudioPlayer();

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
        void releaseResources() override;

        void loadURL(juce::URL audioURL);
        void setGain(double gain);
        void setSpeed(double ratio);
       
        void setPosition(double posInSecs);
        void setPositionRelative(double pos);

        void start();
        void startForward();
        void startReverse();
        void stop();

        // Get the relative position of the playhead
        double getPositionRelative();

        double sendTimer();

        // Sends if the track is finished or not
        bool isTrackFinished();

        double getCurrentPosition() const;
        double getLengthInSeconds() const;
        bool isPlaying() const;

        bool isReversed() const { return isReversedFlag; } 
        double getSampleRate() const { return currentSampleRate; }

    private:
        juce::AudioFormatManager& formatManager;
        juce::AudioBuffer<float> audioBuffer;
        juce::AudioBuffer<float> reversedBuffer;
        std::unique_ptr<juce::AudioFormatReaderSource> forwardSource;
        std::unique_ptr<OtoDecksAudio::MemoryAudioSource> reverseMemorySource;
        juce::AudioTransportSource forwardTransport;
        juce::AudioTransportSource reverseTransport;

        // For speed control
        juce::ResamplingAudioSource forwardResampler { &forwardTransport, false };
        juce::ResamplingAudioSource reverseResampler { &reverseTransport, false };

        double currentSampleRate = 44100.0;
        bool isReversedFlag = false;

        bool isDraggingPosSlider = false;
        bool remixReady = false;
};
