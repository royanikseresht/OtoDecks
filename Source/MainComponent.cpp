#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (1000, 800);
    
    static ModernLookAndFeel modernLook;
    juce::LookAndFeel::setDefaultLookAndFeel(&modernLook);

    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (0, 2);
    }

    addAndMakeVisible(deckGUI1);
    addAndMakeVisible(deckGUI2);
    addAndMakeVisible(playlistComponent);
    addAndMakeVisible(recordButton);
    recordButton.onClick = [this]()
    {
        if (recordButton.getToggleState())
        {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Select where to save your recording...",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.wav"
            );

            chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
            {
                auto chosenFile = fc.getResult();
                if (chosenFile != juce::File{})
                {
                    currentRecordingFile = chosenFile;
                    recorder.startRecording(currentRecordingFile, deviceSampleRate, deviceNumChannels);
                }
                else
                {
                    recordButton.setToggleState(false, juce::dontSendNotification);
                }
            });
        }
        else
        {
            recorder.stopRecording();
        }
    };

    formatManager.registerBasicFormats();

    startTimer(30);
}

MainComponent::~MainComponent()
{
    if (recorder.isRecording())
        recorder.stopRecording();

    stopTimer();
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    deviceSampleRate = sampleRate;
    deviceNumChannels = 2;

    player1.prepareToPlay(samplesPerBlockExpected, sampleRate);
    player2.prepareToPlay(samplesPerBlockExpected, sampleRate);

    mixerSource.prepareToPlay(samplesPerBlockExpected, sampleRate);

    mixerSource.addInputSource(&player1, false);
    mixerSource.addInputSource(&player2, false);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{   
    mixerSource.getNextAudioBlock(bufferToFill);

    // If recording, write the buffer to disk
    if (recorder.isRecording())
    {
        recorder.write(bufferToFill);
    }
}

void MainComponent::releaseResources()
{
    player1.releaseResources();
    player2.releaseResources();
    mixerSource.releaseResources();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    // Record button in the top center
    recordButton.setBounds((getWidth() - 30) / 2 - 6, 130, 30, 15);

    deckGUI1.setBounds(0, 0, getWidth() / 2, getHeight() * 0.66 );
    deckGUI2.setBounds(getWidth() / 2, 0, getWidth() / 2, getHeight() * 0.66);

    playlistComponent.setBounds(0, getHeight() * 0.66, getWidth(), getHeight() * 0.32);
}

bool DeckGUI::isInterestedInFileDrag(const juce::StringArray& files)
{
    DBG("DeckGUI::isInterestedInFileDrag");
    return true;
}

void DeckGUI::filesDropped(const juce::StringArray& files, int x, int y)
{
    std::cout << "DeckGUI::isInterestedInFileDrag" << std::endl;
    if (files.size() == 1)
    {
        player->loadURL( juce::URL{ juce::File{files[0]} } );
        waveformDisplay.loadURL( juce::URL{ juce::File{files[0]} } );

        // Update the labels
        juce::String newTrack = playlist->convertTrackPathToTitle(juce::File{ files[0] }.getFullPathName().toStdString());

        trackListComponent.currentTrack = newTrack;
        trackListComponent.nextTrack = "No Track";
        trackListComponent.previousTrack = "No Track";
        trackListComponent.labelUpdate();

        // Clean the playlist from the deck
        trackListComponent.trackTitles.clear();
        trackListComponent.trackPaths.clear();
    }
}

void MainComponent::timerCallback()
{
    if (player1.isPlaying())
    {
        double length1 = player1.getLengthInSeconds();
        if (length1 > 0)
        {
            double currentPos1 = player1.getCurrentPosition();
            double relativePos1 = 0.0;

            if (player1.isReversed())
                relativePos1 = 1.0 - (currentPos1 / length1);
            else
                relativePos1 = currentPos1 / length1;

            relativePos1 = juce::jlimit(0.0, 1.0, relativePos1);
            deckGUI1.waveformDisplay.setPositionRelative(relativePos1);
        }
    }

    if (player2.isPlaying())
    {
        double length2 = player2.getLengthInSeconds();
        if (length2 > 0)
        {
            double currentPos2 = player2.getCurrentPosition();
            double relativePos2 = 0.0;

            if (player2.isReversed())
                relativePos2 = 1.0 - (currentPos2 / length2);
            else
                relativePos2 = currentPos2 / length2;

            relativePos2 = juce::jlimit(0.0, 1.0, relativePos2);
            deckGUI2.waveformDisplay.setPositionRelative(relativePos2);
        }
    }
}
