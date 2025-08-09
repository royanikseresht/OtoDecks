#include "CSVOperator.h"

CSVOperator::CSVOperator()
{
    readTracksCSV();  // Ensures file exists
}

std::vector<std::string> CSVOperator::returnTrackPathsArray()
{
    return readTracksCSV();
}

std::vector<std::string> CSVOperator::readTracksCSV()
{
    std::vector<std::string> tracks;

    // Use forward slashes for cross-platform compatibility
    juce::File tracksFile = juce::File::getCurrentWorkingDirectory().getChildFile("Tracks.csv");

    if (!tracksFile.exists())
    {
        tracksFile.create();  // Create empty file if missing
    }

    juce::FileInputStream csvFile(tracksFile);

    if (csvFile.openedOk())
    {
        while (!csvFile.isExhausted())
        {
            try
            {
                juce::String juceLine = csvFile.readNextLine();
                tracks.push_back(juceLine.toStdString());
            }
            catch (const std::exception& e)
            {
                DBG("CSVOperator::readTracksCSV error: " << e.what());
            }
        }
    }

    return tracks;
}

void CSVOperator::addNewTrack(juce::String path)
{
    juce::File tracksFile = juce::File::getCurrentWorkingDirectory().getChildFile("Tracks.csv");

    std::ofstream csvFile(tracksFile.getFullPathName().toStdString(), std::ios::app);
    if (csvFile.is_open())
    {
        csvFile << path.toStdString() << std::endl;
        csvFile.close();
    }
    else
    {
        DBG("Failed to open Tracks.csv for appending.");
    }
}

void CSVOperator::removeTrack(int rowNumber)
{
    juce::File originalFile = juce::File::getCurrentWorkingDirectory().getChildFile("Tracks.csv");
    juce::File tempFile = juce::File::getCurrentWorkingDirectory().getChildFile("Temp.csv");

    std::ifstream in(originalFile.getFullPathName().toStdString());
    std::ofstream out(tempFile.getFullPathName().toStdString());

    if (!in.is_open() || !out.is_open())
    {
        DBG("Failed to open CSV files for deletion.");
        return;
    }

    std::string line;
    int lineIndex = 0;

    while (std::getline(in, line))
    {
        if (lineIndex != rowNumber)
        {
            out << line << std::endl;
        }
        lineIndex++;
    }

    in.close();
    out.close();

    // Replace original file
    originalFile.deleteFile();
    tempFile.moveFileTo(originalFile);
}