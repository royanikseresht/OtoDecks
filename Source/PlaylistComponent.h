/*
  ==============================================================================

    PlaylistComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>
#include <utility> // for std::pair
#include "CSVOperator.h"
#include "TrackListComponent.h"

//==============================================================================
class PlaylistComponent  : public juce::Component,
                            public juce::TableListBoxModel,
                            public juce::Button::Listener
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override;

    // JUCE component overrides
    void paint (juce::Graphics&) override;
    void resized() override;

    // Table model overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnID, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    // Button listener
    void buttonClicked(juce::Button* button) override;

    // Refresh the playlist
    void refreshPlaylist();

    // Converts a track path to track title
    juce::String convertTrackPathToTitle(std::string path);

    juce::String selectedTrackPath;
    juce::String selectedTrack { "Nothing Selected" };
    std::vector<juce::String> trackTitles;
    std::vector<std::string> trackPaths;

private:
    // === Playlist data ===
    std::vector<juce::String> trackNotes; // Notes per track
    std::vector<bool> isFavorite;         // Favorite status
    std::vector<std::pair<float, juce::String>> trackMetadata; // (BPM, Key)


    // === UI components ===
    juce::TableListBox tableComponent;

    juce::Label prepareLabel1; // Shows selected track name
    juce::Label prepareLabel2; // "PREPARED TRACK" label

    // Buttons
    juce::TextButton addTrackButton { "ADD NEW TRACK" };
    juce::TextButton removeTrackButton { "REMOVE TRACK" };
    juce::TextButton searchButton { "Find" };
    juce::TextButton suggestMixButton; // "Suggest Mix" button

    // Search box
    juce::TextEditor searchBox;

    // Audio format manager
    juce::AudioFormatManager formatManager;

    // Thread pool for async BPM analysis
    juce::ThreadPool threadPool { 2 };

    // === Internal methods ===
    void populateTrackTitles();
    void searchTracks(juce::String input);
    void analyzeTrackBPMs();
    int suggestNextTrack(int currentIndex);

    // Search helpers
    int rowCounter = -1;
    juce::String lastSearch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaylistComponent)
};
