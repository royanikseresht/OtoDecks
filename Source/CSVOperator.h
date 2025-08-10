#pragma once

#include "JuceHeader.h"
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

struct TrackInfo
{
    std::string path;
    float bpm = 0.0f;
    std::string key;
    bool favorite = false;
    std::string note;

    TrackInfo() = default;
    TrackInfo(const std::string& p) : path(p) {}
};

class CSVOperator
{
public:
    CSVOperator();

    // Returns all tracks info, including favorites and notes
    static std::vector<TrackInfo> loadAllTracks();

    // Saves entire track list back to CSV (overwrites)
    static void saveAllTracks(const std::vector<TrackInfo>& tracks);

    // Adds a new track with default metadata
    static void addNewTrack(const juce::String& path);

    // Removes track at given index
    static void removeTrack(int rowNumber);

private:
    // Reads CSV line-by-line and parses TrackInfo objects
    static std::vector<TrackInfo> readTracksCSV();

    // Writes vector of TrackInfo to CSV file
    static void writeTracksCSV(const std::vector<TrackInfo>& tracks);

    static juce::File getCSVFile();
};
