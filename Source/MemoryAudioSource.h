#pragma once
#include <JuceHeader.h>

namespace OtoDecksAudio
{
    class MemoryAudioSource : public juce::PositionableAudioSource
    {
    public:
        MemoryAudioSource(const juce::AudioBuffer<float>& bufferToUse, bool shouldLoop)
            : buffer(bufferToUse), looping(shouldLoop)
        {
            position = 0;
        }

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
        {
            this->sampleRate = sampleRate;
        }

        void releaseResources() override {}

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
        {
            auto numChannels = buffer.getNumChannels();
            auto numSamples = bufferToFill.numSamples;
            auto* outputBuffer = bufferToFill.buffer;

            for (int channel = 0; channel < outputBuffer->getNumChannels(); ++channel)
            {
                float* writePtr = outputBuffer->getWritePointer(channel, bufferToFill.startSample);
                for (int i = 0; i < numSamples; ++i)
                {
                    if (position >= buffer.getNumSamples())
                    {
                        if (looping)
                            position = 0;
                        else
                        {
                            writePtr[i] = 0.0f;
                            continue;
                        }
                    }

                    writePtr[i] = buffer.getSample(channel % numChannels, position);
                    ++position;
                }
            }
        }

        void setPosition(int newPosition) 
        {
            position = juce::jlimit(0, buffer.getNumSamples() - 1, newPosition);
        }

        int getPosition() const { return position; }

        // Implement pure virtual methods of PositionableAudioSource:
        void setNextReadPosition(int64 newPosition) override
        {
            position = juce::jlimit(0, buffer.getNumSamples() - 1, (int)newPosition);
        }

        int64 getNextReadPosition() const override
        {
            return position;
        }

        int64 getTotalLength() const override
        {
            return buffer.getNumSamples();
        }

        bool isLooping() const override
        {
            return looping;
        }

        double getCurrentPosition() const
        {
            return position / sampleRate;
        }

    private:
        const juce::AudioBuffer<float>& buffer;
        int position = 0;
        bool looping = false;
        double sampleRate = 44100.0;
    };
}