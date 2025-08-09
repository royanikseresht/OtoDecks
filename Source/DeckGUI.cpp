/*
  ==============================================================================

    DeckGUI.cpp

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DeckGUI.h"
#include "MainComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
DeckGUI::DeckGUI(DJAudioPlayer* _player, juce::AudioFormatManager& _formatManagerToUse, juce::AudioThumbnailCache& cacheToUse, PlaylistComponent* _playlist)
                : player(_player), waveformDisplay(_formatManagerToUse, cacheToUse), playlist(_playlist)
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playSelectedButton);
    addAndMakeVisible(loadPlaylistButton);
    addAndMakeVisible(volSlider);
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(posSlider);
    addAndMakeVisible(muteButton);
    addAndMakeVisible(twiceSpeedButton);
    addAndMakeVisible(trackListComponent);
    addAndMakeVisible(waveformDisplay);

    addAndMakeVisible(remixButton);
    addAndMakeVisible(genreSelector);
    genreSelector.setLookAndFeel(&modernLNF);

    remixButton.addListener(this);

    genreSelector.addItem("Piano", 1);
    genreSelector.addItem("Guitar", 2);
    genreSelector.addItem("Violin", 3);
    genreSelector.addItem("Drum", 4);
    genreSelector.setVisible(false);  // Initially hidden
    genreSelector.onChange = [this]() {
        juce::String genre = genreSelector.getText();
        juce::File genreDir = juce::File::getCurrentWorkingDirectory().getChildFile("loops").getChildFile(genre.toLowerCase());

        if (genreDir.exists() && genreDir.isDirectory()) {
            juce::Array<juce::File> wavFiles = genreDir.findChildFiles(juce::File::findFiles, false, "*.wav");

            if (!wavFiles.isEmpty()) {
                selectedRemixFile = wavFiles.getReference(juce::Random::getSystemRandom().nextInt(wavFiles.size()));
                remixButton.setButtonText("GENERATE REMIX");
                remixReady = true;
            }
        }

        genreSelector.setVisible(false);
    };


    // Listeners
    playButton.addListener(this);
    stopButton.addListener(this);
    loadButton.addListener(this);
    volSlider.addListener(this);
    speedSlider.addListener(this);
    posSlider.addListener(this);
    playSelectedButton.addListener(this);
    loadPlaylistButton.addListener(this);
    muteButton.addListener(this);
    twiceSpeedButton.addListener(this);

    // Slider Ranges
    posSlider.setRange(0.0, 1.0);
    speedSlider.setRange(0.05, 2, 0.05);
    volSlider.setRange(0, 1, 0.01);

    startTimer(500);

    // Default Values for Sliders
    volSlider.setValue(1);
    speedSlider.setValue(1);

    // Make mute and 2x buttons toggleable and set their 'on' colours
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd5d5da));
    twiceSpeedButton.setClickingTogglesState(true);
    twiceSpeedButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd5d5da));

}

DeckGUI::~DeckGUI()
{
    stopTimer();
}

void DeckGUI::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colour(0xffd5d5da));
    g.fillRect(getLocalBounds());
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);

}

void DeckGUI::resized()
{

    // Variable Declaration for better grids
    double rowH = getHeight() / 20;
    double columnW = getWidth() / 16;

    // Button Bounds
    playButton.setBounds(columnW * 4, rowH, columnW * 4, rowH * 2);
    stopButton.setBounds(columnW * 8, rowH, columnW * 4, rowH * 2);
    playSelectedButton.setBounds(columnW * 4, rowH * 3, columnW * 8, rowH * 2);
    muteButton.setBounds(0, rowH * 4, columnW * 4, rowH);
    twiceSpeedButton.setBounds(columnW * 12, rowH * 4, columnW * 4, rowH);
    
        double buttonY = rowH * 19;
    double buttonH = rowH * 1.2;
    double buttonW = getWidth() / 3;

    remixButton.setBounds(0, buttonY, buttonW, buttonH);
    genreSelector.setBounds(0, buttonY, buttonW, buttonH);
    
    loadButton.setBounds(buttonW, buttonY, buttonW, buttonH);
    loadPlaylistButton.setBounds(buttonW * 2, buttonY, buttonW, buttonH);


    // Sliders
    volSlider.setBounds(0, 0, columnW * 4 , rowH * 4);
    volSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    volSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, columnW * 4, rowH * 0.75);

    speedSlider.setBounds(columnW * 12, 0, columnW * 4, rowH * 4);
    speedSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    speedSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, columnW * 4, rowH * 0.75);

    posSlider.setBounds(columnW * 0.1, rowH * 15, columnW * 15.9, rowH);
    posSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, columnW * 4, rowH * 0.75);

    trackListComponent.setBounds(0, rowH * 5, getWidth(), rowH * 10);

    waveformDisplay.setBounds(columnW * 0.1, rowH * 16, columnW * 15.8, rowH * 3);


}

void DeckGUI::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        player->start();

    }

    if (button == &stopButton)
    {
        player->stop();
    }

    // Loads and plays a track
    if (button == &loadButton)
    {
        auto* chooser = new juce::FileChooser(
            "Select a file to load...",
            juce::File{},
            "*.mp3;*.wav"
        );

        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                juce::File selectedFile = fc.getResult();

                delete chooser; // Clean up

                if (selectedFile.existsAsFile())
                {
                    juce::URL fileURL{ selectedFile };

                    player->loadURL(fileURL);
                    waveformDisplay.loadURL(fileURL);

                    // Update the Labels
                    trackListComponent.currentTrack = selectedFile.getFileName();
                    trackListComponent.nextTrack = "No Track";
                    trackListComponent.previousTrack = "No Track";
                    trackListComponent.labelUpdate();

                    // Clean the playlist from the deck
                    trackListComponent.trackTitles.clear();
                    trackListComponent.trackPaths.clear();
                }
            });
    }


    // Plays the prepared song from the playlist
    if (button == &playSelectedButton)
    {
        if (playlist->selectedTrack != "Nothing Selected")
        {
            // Plays the selected track
            player->loadURL(juce::URL{ juce::File(playlist->selectedTrackPath) });
            waveformDisplay.loadURL(juce::URL{ juce::File(playlist->selectedTrackPath) });

            // Update the Labels
            trackListComponent.currentTrack = playlist->selectedTrack;
            trackListComponent.nextTrack = "No Track";
            trackListComponent.previousTrack = "No Track";
            trackListComponent.labelUpdate();

            // Clean the playlist from the deck
            trackListComponent.trackTitles.clear();
            trackListComponent.trackPaths.clear();
        }

    }

    // Loads the playlist to the deck
    if (button == &loadPlaylistButton)
    {
        // In case there is no track, don't do anything
        if (playlist->trackTitles.size() > 0) 
        {
            trackListComponent.loadPlaylist(playlist->trackTitles, playlist->trackPaths);
        }

    }

    // Mutes the volume if on
    if (button == &muteButton)
    {
        if (muteButton.getToggleState())
        {
            player->setGain(0);
        }
        else
        {
            player->setGain(volSlider.getValue());
        }
    }

    // Set the speed 2x when 2x button is 'on'
    if (button == &twiceSpeedButton)
    {
        if (twiceSpeedButton.getToggleState())
        {
            player->setSpeed(2);
        }
        else
        {
            player->setSpeed(speedSlider.getValue());
        }
    }

    if (button == &remixButton)
    {
        if (!remixReady)
        {
            genreSelector.setVisible(true);
        }
        else if (selectedRemixFile.existsAsFile())
        {
            player->loadURL(juce::URL{ selectedRemixFile });
            waveformDisplay.loadURL(juce::URL{ selectedRemixFile });

            trackListComponent.currentTrack = selectedRemixFile.getFileName();
            trackListComponent.labelUpdate();

            remixReady = false;
            remixButton.setButtonText("CHOOSE GENRE");
        }
    }


}

// Slider Functions
void DeckGUI::sliderValueChanged(juce::Slider* slider)
{
    // Changes the volume if mute is 'off'
    if (slider == &volSlider && !muteButton.getToggleState())
    {
        player->setGain(slider->getValue());
    }

    // Changes the speed if 2x is 'off'
    if (slider == &speedSlider && !twiceSpeedButton.getToggleState())
    {
        player->setSpeed(slider->getValue());
    }

    if (slider == &posSlider)
    {
        player->setPositionRelative(slider->getValue());
    }
}

void DeckGUI::timerCallback()
{
    waveformDisplay.setPositionRelative(player->getPositionRelative());
    posSlider.setValue(player->getPositionRelative());
    trackListComponent.updateTimer(player->sendTimer());
}

