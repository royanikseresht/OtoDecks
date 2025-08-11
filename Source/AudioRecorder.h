// AudioRecorder.h 
#pragma once
#include <JuceHeader.h>

class AudioRecorder
{
public:
    AudioRecorder() : sampleRate(44100.0), numChannels(2) {}

    void startRecording(const juce::File& fileToUse, double sr, int channels)
    {
        stopRecording();

        sampleRate = sr;
        numChannels = channels;

        // Ensure parent folder exists
        fileToUse.getParentDirectory().createDirectory();

        juce::WavAudioFormat wavFormat;
        outputStream = fileToUse.createOutputStream();

        if (outputStream != nullptr)
        {
            writer.reset(wavFormat.createWriterFor(outputStream.get(), sampleRate, (unsigned int)numChannels, 16, {}, 0));
            // writer now owns the stream, release our pointer
            outputStream.release();
            recording = (writer != nullptr);
        }
    }

    void stopRecording()
    {
        // flush & close writer
        writer.reset();
        recording = false;
    }

    bool isRecording() const noexcept { return recording; }

    // Call from audio thread (fast). Writes current buffer to disk.
    void write(const juce::AudioSourceChannelInfo& bufferToWrite)
    {
        if (!writer) return;

        // create a temporary AudioBuffer<float> if needed:
        // writer->writeFromAudioSampleBuffer requires AudioBuffer<float>
        writer->writeFromAudioSampleBuffer(*bufferToWrite.buffer, bufferToWrite.startSample, bufferToWrite.numSamples);
    }

private:
    std::unique_ptr<juce::FileOutputStream> outputStream;

    std::unique_ptr<juce::AudioFormatWriter> writer;
    double sampleRate;
    int numChannels;
    std::atomic<bool> recording{ false };
};
