#include "CSVOperator.h"

CSVOperator::CSVOperator()
{
    if (!getCSVFile().exists())
        getCSVFile().create();  // Ensure file exists
}

juce::File CSVOperator::getCSVFile()
{
    return juce::File::getCurrentWorkingDirectory().getChildFile("Tracks.csv");
}

std::vector<TrackInfo> CSVOperator::loadAllTracks()
{
    return readTracksCSV();
}

void CSVOperator::saveAllTracks(const std::vector<TrackInfo>& tracks)
{
    writeTracksCSV(tracks);
}

std::vector<TrackInfo> CSVOperator::readTracksCSV()
{
    std::vector<TrackInfo> tracks;

    juce::File tracksFile = getCSVFile();

    if (!tracksFile.exists())
        return tracks;

    juce::FileInputStream csvFile(tracksFile);

    if (!csvFile.openedOk())
        return tracks;

    while (!csvFile.isExhausted())
    {
        auto line = csvFile.readNextLine().trim();

        if (line.isEmpty())
            continue;

        // CSV split on commas (basic, no escaping)
        auto parts = juce::StringArray::fromTokens(line, ",", "");

        TrackInfo info;
        info.path = parts[0].toStdString();

        if (parts.size() > 1)
            info.bpm = parts[1].getFloatValue();

        if (parts.size() > 2)
            info.key = parts[2].toStdString();

        if (parts.size() > 3)
            info.favorite = (parts[3] == "1");

        if (parts.size() > 4)
            info.note = parts[4].toStdString();

        tracks.push_back(std::move(info));
    }
    return tracks;
}

void CSVOperator::writeTracksCSV(const std::vector<TrackInfo>& tracks)
{
    juce::File tracksFile = getCSVFile();

    std::ofstream outFile(tracksFile.getFullPathName().toStdString(), std::ios::trunc);
    if (!outFile.is_open())
    {
        DBG("CSVOperator: Failed to open CSV for writing");
        return;
    }
    for (const auto& track : tracks)
    {
        // Escape commas in notes or paths if needed (simple version, wrap with quotes if comma found)
        auto escapeCSV = [](const std::string& str) -> std::string
        {
            if (str.find(',') != std::string::npos)
                return "\"" + str + "\"";
            return str;
        };
        outFile << escapeCSV(track.path) << ",";
        outFile << track.bpm << ",";
        outFile << escapeCSV(track.key) << ",";
        outFile << (track.favorite ? "1" : "0") << ",";
        outFile << escapeCSV(track.note) << std::endl;
    }
}

void CSVOperator::addNewTrack(const juce::String& path)
{
    auto tracks = readTracksCSV();
    TrackInfo newTrack(path.toStdString());
    tracks.push_back(std::move(newTrack));
    writeTracksCSV(tracks);
}

void CSVOperator::removeTrack(int rowNumber)
{
    auto tracks = readTracksCSV();
    if (rowNumber >= 0 && rowNumber < (int)tracks.size())
    {
        tracks.erase(tracks.begin() + rowNumber);
        writeTracksCSV(tracks);
    }
}
