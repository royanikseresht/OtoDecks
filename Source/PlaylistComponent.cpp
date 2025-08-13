#include <JuceHeader.h>
#include "PlaylistComponent.h"
#include "CSVOperator.h" 
#include <iostream> 

//==============================================================================
static const char* heartFilledSVG = R"(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
  <path fill="#E53935" d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3
    c1.74 0 3.41 0.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5
    c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
</svg>
)";

static const char* heartOutlineSVG = R"(
<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
  <path fill="#555555" d="M16.5,3c-1.74,0-3.41,0.81-4.5,2.09C10.91,3.81,9.24,3,7.5,3
    C4.42,3,2,5.42,2,8.5c0,3.78,3.4,6.86,8.55,11.54L12,21.35l1.45-1.32
    C18.6,15.36,22,12.28,22,8.5C22,5.42,19.58,3,16.5,3z M12.1,18.55l-0.1,0.1l-0.1-0.1
    C7.14,14.24,4,11.39,4,8.5C4,6.5,5.5,5,7.5,5c1.54,0,3.04,1,3.57,2.36h1.87
    C13.46,6,14.96,5,16.5,5C18.5,5,20,6.5,20,8.5C20,11.39,16.86,14.24,12.1,18.55z"/>
</svg>
)";
//==============================================================================

class BPMAnalysisJob : public juce::ThreadPoolJob
{
public:
    BPMAnalysisJob(std::vector<std::string> paths,
                   std::function<void(std::vector<std::pair<float, juce::String>>)> cb)
        : ThreadPoolJob("BPMAnalysisJob"), trackPaths(std::move(paths)), callback(std::move(cb)) {}

    JobStatus runJob() override
    {
        std::vector<std::pair<float, juce::String>> results;

        for (const auto& path : trackPaths)
        {
            juce::String escapedPath = juce::String(path).replace(" ", "\\ ");
            juce::String cmd = "python scripts/analyze_track.py " + escapedPath;

            juce::ChildProcess cp;
            if (cp.start(cmd))
            {
                juce::String output = cp.readAllProcessOutput().trim();
                auto parts = juce::StringArray::fromTokens(output, ",", "");

                if (parts.size() >= 1)
                {
                    float bpm = parts[0].getFloatValue();
                    juce::String key = (parts.size() > 1) ? parts[1] : "Unknown";
                    results.emplace_back(bpm > 0.0f ? bpm : 0.0f, key);
                }
                else
                {
                    results.emplace_back(0.0f, "Unknown");
                }
            }
            else
            {
                results.emplace_back(0.0f, "Unknown");
            }
        }

        juce::MessageManager::callAsync([cb = callback, results = std::move(results)] {
            cb(results);
        });

        return jobHasFinished;
    }

private:
    std::vector<std::string> trackPaths;
    std::function<void(std::vector<std::pair<float, juce::String>>)> callback;
};

PlaylistComponent::PlaylistComponent()
{
    tracks = CSVOperator::loadAllTracks();  // load all track info from CSV
    populateTrackTitles();
    analyzeTrackBPMs();

    tableComponent.getHeader().addColumn("Title", 1, 200);
    tableComponent.getHeader().addColumn("Length", 2, 100);
    tableComponent.getHeader().addColumn("", 3, 150);
    tableComponent.getHeader().addColumn("BPM", 4, 60);
    tableComponent.getHeader().addColumn("Notes", 5, 200);
    tableComponent.getHeader().addColumn("Favorites", 6, 60);

    tableComponent.setModel(this);

    addAndMakeVisible(tableComponent);
    addAndMakeVisible(prepareLabel1);
    addAndMakeVisible(prepareLabel2);
    addAndMakeVisible(addTrackButton);
    addAndMakeVisible(removeTrackButton);
    addAndMakeVisible(searchBox);
    addAndMakeVisible(searchButton);
    addAndMakeVisible(suggestMixButton);

    addTrackButton.addListener(this);
    removeTrackButton.addListener(this);
    searchButton.addListener(this);
    suggestMixButton.addListener(this);
    suggestMixButton.setButtonText("Suggest Mix");

    allTracksToggle.setButtonText("All Tracks");
    favoritesToggle.setButtonText("Favorites");

    addAndMakeVisible(allTracksToggle);
    addAndMakeVisible(favoritesToggle);

    allTracksToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    favoritesToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    allTracksToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
    favoritesToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::black);

    allTracksToggle.setToggleState(true, juce::dontSendNotification); // default
    favoritesToggle.setToggleState(false, juce::dontSendNotification);

    currentFilter = FilterMode::All;
    rebuildFilteredList();
    tableComponent.updateContent();

    allTracksToggle.onClick = [this]() {
        if (allTracksToggle.getToggleState())
        {
            favoritesToggle.setToggleState(false, juce::dontSendNotification);
            currentFilter = FilterMode::All;
            rebuildFilteredList();
            tableComponent.updateContent();
        }
    };

    favoritesToggle.onClick = [this]() {
        if (favoritesToggle.getToggleState())
        {
            allTracksToggle.setToggleState(false, juce::dontSendNotification);
            currentFilter = FilterMode::Favorites;
            rebuildFilteredList();
            tableComponent.updateContent();
        }
    };

    formatManager.registerBasicFormats();
}

PlaylistComponent::~PlaylistComponent() {}

void PlaylistComponent::populateTrackTitles()
{
    trackTitles.clear();
    trackNotes.clear();
    isFavorite.clear();
    trackPaths.clear();

    for (const auto& track : tracks)
    {
        trackTitles.push_back(convertTrackPathToTitle(track.path).toStdString());
        trackNotes.push_back(track.note);
        isFavorite.push_back(track.favorite);
        trackPaths.push_back(track.path);
    }
}

juce::String PlaylistComponent::convertTrackPathToTitle(const std::string& path)
{
    juce::File file(path);
    return file.getFileNameWithoutExtension();
}

void PlaylistComponent::refreshPlaylist()
{
    tracks = CSVOperator::loadAllTracks();
    populateTrackTitles();
    analyzeTrackBPMs();

    rebuildFilteredList();

    tableComponent.updateContent();
    tableComponent.repaint();
}

void PlaylistComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::black);
    juce::Rectangle<float> prepareBorder(getWidth() * 0.295f, getHeight() * 0.65f, getWidth() * 0.41f, getHeight() * 0.35f);
    g.fillRect(prepareBorder);

    g.setColour(juce::Colour(0xffd5d5da));
    juce::Rectangle<float> prepareArea(getWidth() * 0.3f, getHeight() * 0.68f, getWidth() * 0.4f, getHeight() * 0.31f);
    g.fillRect(prepareArea);

    prepareLabel1.setColour(juce::Label::textColourId, juce::Colours::black);
    prepareLabel1.setFont(juce::Font(20.0f));
    prepareLabel1.setText(selectedTrack, juce::dontSendNotification);
    prepareLabel1.setJustificationType(juce::Justification::centred);
    prepareLabel1.setBounds(getWidth() * 0.25f, getHeight() * 0.80f, getWidth() * 0.5f, getHeight() * 0.1f);

    prepareLabel2.setColour(juce::Label::textColourId, juce::Colours::black);
    prepareLabel2.setFont(juce::Font(14.0f));
    prepareLabel2.setText("PREPARED TRACK", juce::dontSendNotification);
    prepareLabel2.setJustificationType(juce::Justification::centred);
    prepareLabel2.setBounds(getWidth() * 0.3f, getHeight() * 0.72f, getWidth() * 0.4f, getHeight() * 0.1f);

    addTrackButton.setBounds(getWidth() * 0.72f, getHeight() * 0.68f, getWidth() * 0.25f, getHeight() * 0.08f);
    removeTrackButton.setBounds(getWidth() * 0.72f, getHeight() * 0.80f, getWidth() * 0.25f, getHeight() * 0.08f);
    suggestMixButton.setBounds(getWidth() * 0.72f, getHeight() * 0.92f, getWidth() * 0.25f, getHeight() * 0.08f);

    searchBox.setBounds(getWidth() * 0.02f, getHeight() * 0.70f, getWidth() * 0.25f, getHeight() * 0.1f);
    searchButton.setBounds(getWidth() * 0.02f, getHeight() * 0.82f, getWidth() * 0.12f, getHeight() * 0.1f);
}

void PlaylistComponent::resized()
{
    int toggleWidth = 120;
    int toggleHeight = 25;
    int toggleY = 5;
    int toggleSpacing = 10;
    int togglesAreaHeight = toggleHeight + 10; // total height for toggles row

    // Set toggles first
    allTracksToggle.setBounds(10, toggleY, toggleWidth, toggleHeight);
    favoritesToggle.setBounds(10 + toggleWidth + toggleSpacing, toggleY, toggleWidth, toggleHeight);

    // Table below the toggles
    tableComponent.setBounds(0, togglesAreaHeight, getWidth(), getHeight() * 0.67f - togglesAreaHeight);

    // Column widths
    tableComponent.getHeader().setColumnWidth(1, getWidth() / 4);
    tableComponent.getHeader().setColumnWidth(2, getWidth() / 16);
    tableComponent.getHeader().setColumnWidth(3, getWidth() / 8);
    tableComponent.getHeader().setColumnWidth(4, getWidth() / 16);
    tableComponent.getHeader().setColumnWidth(5, getWidth() / 2.35);
    tableComponent.getHeader().setColumnWidth(6, getWidth() / 16);
}

int PlaylistComponent::getNumRows()
{
    return (int)filteredTrackIndices.size();
}

void PlaylistComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    g.fillAll(rowIsSelected ? juce::Colour(0xffd5d5da) : juce::Colours::grey);
}

void PlaylistComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool)
{
    if (rowNumber >= (int)filteredTrackIndices.size())
        return;

    if (columnId == 1)
    {
        int trackIndex = filteredTrackIndices[rowNumber];
        juce::String trackName = convertTrackPathToTitle(tracks[trackIndex].path);


        g.drawText(trackName, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
    else if (columnId == 2)
    {
        juce::File filepath{ juce::String(trackPaths[rowNumber]) };
        int lengthInSeconds = 0;

        if (auto* reader = formatManager.createReaderFor(filepath))
        {
            lengthInSeconds = (int)(reader->lengthInSamples / reader->sampleRate);
            delete reader;

            juce::String formattedTime = TrackListComponent::convertSecondsToTimer(lengthInSeconds);
            g.drawText(formattedTime, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        else
        {
            g.drawText("Unknown", 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
    }
    else if (columnId == 4)
    {
        if (rowNumber >= 0 && rowNumber < (int)filteredTrackIndices.size())
        {
            int originalIndex = filteredTrackIndices[rowNumber];

            if (originalIndex >= 0 && originalIndex < (int)trackMetadata.size())
            {
                float bpm = trackMetadata[originalIndex].first;
                if (bpm > 0.0f)
                {
                    g.setColour(juce::Colours::black);
                    g.setFont(juce::Font(14.0f, juce::Font::plain));
                    juce::String bpmText = juce::String(bpm, 1);
                    g.drawText(bpmText, 2, 0, width - 4, height,
                            juce::Justification::centredLeft, true);
                }
                else
                {
                    g.setColour(juce::Colours::lightgrey);
                    g.setFont(juce::Font(14.0f, juce::Font::italic));
                    g.drawText("N/A", 2, 0, width - 4, height,
                            juce::Justification::centredLeft, true);
                }
            }
            else
            {
                g.setColour(juce::Colours::lightgrey);
                g.setFont(juce::Font(14.0f, juce::Font::italic));
                g.drawText("Loading...", 2, 0, width - 4, height,
                        juce::Justification::centredLeft, true);
            }
        }
    }
}

juce::Component* PlaylistComponent::refreshComponentForCell(int rowNumber, int columnID, bool, juce::Component* existingComponentToUpdate)
{
    if (rowNumber >= (int)filteredTrackIndices.size())
        return nullptr;

    if (columnID == 3 && existingComponentToUpdate == nullptr)
    {
        auto* btn = new juce::TextButton("Prepare To Play");
        btn->setComponentID(std::to_string(rowNumber));
        btn->addListener(this);
        return btn;
    }
    else if (columnID == 5) // Notes column
    {
        auto* noteLabel = dynamic_cast<juce::Label*>(existingComponentToUpdate);
        if (noteLabel == nullptr)
            noteLabel = new juce::Label();

        int originalIndex = filteredTrackIndices[rowNumber];  // Map filtered row to original track index

        noteLabel->setEditable(true);
        noteLabel->setText(trackNotes[originalIndex], juce::dontSendNotification);

        noteLabel->onTextChange = [this, noteLabel, originalIndex]()
        {
            if (originalIndex < (int)trackNotes.size())
            {
                trackNotes[originalIndex] = noteLabel->getText();

                // Update CSV and internal struct immediately
                if (originalIndex < (int)tracks.size())
                {
                    tracks[originalIndex].note = trackNotes[originalIndex].toStdString();
                    CSVOperator::saveAllTracks(tracks);
                }
            }
        };

        return noteLabel;
    }
    else if (columnID == 6) // Favorite column with heart button
    {
        auto* heartButton = dynamic_cast<juce::DrawableButton*>(existingComponentToUpdate);
        if (heartButton == nullptr)
        {
            heartButton = new juce::DrawableButton("heartButton", juce::DrawableButton::ImageFitted);
            heartButton->setClickingTogglesState(true);
            heartButton->addListener(this);

            heartButton->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            heartButton->setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
        }

        static std::unique_ptr<juce::XmlElement> filledHeartXml = juce::parseXML(juce::String(heartFilledSVG));
        static std::unique_ptr<juce::Drawable> filledHeart = filledHeartXml ? juce::Drawable::createFromSVG(*filledHeartXml) : nullptr;

        static std::unique_ptr<juce::XmlElement> outlineHeartXml = juce::parseXML(juce::String(heartOutlineSVG));
        static std::unique_ptr<juce::Drawable> outlineHeart = outlineHeartXml ? juce::Drawable::createFromSVG(*outlineHeartXml) : nullptr;

        int trackIndex = filteredTrackIndices[rowNumber];
        bool fav = isFavorite[trackIndex];

        heartButton->setToggleState(fav, juce::dontSendNotification);

        heartButton->setImages(fav ? filledHeart.get() : outlineHeart.get());

        heartButton->onClick = [this, heartButton, rowNumber]()
        {
            bool newState = heartButton->getToggleState();
            isFavorite[rowNumber] = newState;

            if (newState)
                heartButton->setImages(filledHeart.get());
            else
                heartButton->setImages(outlineHeart.get());

            // Update CSV and internal tracks vector immediately
            if (rowNumber < (int)tracks.size())
            {
                tracks[rowNumber].favorite = newState;
                CSVOperator::saveAllTracks(tracks);
            }
        };
        return heartButton;
    }
    return existingComponentToUpdate;
}

void PlaylistComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    if (rowNumber < 0 || rowNumber >= (int)filteredTrackIndices.size())
        return;

    tableComponent.selectRow(rowNumber, false, true);

    if (e.mods.isPopupMenu()) // Right click menu
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Prepare to Play");
        menu.addSeparator();
        menu.addItem(2, "Remove Track");
        menu.addSeparator();
        menu.addItem(3, "Suggest Mix");

        menu.showMenuAsync(
            juce::PopupMenu::Options()
                .withTargetComponent(&tableComponent)
                .withTargetScreenArea({ e.getScreenPosition().x, e.getScreenPosition().y, 1, 1 }),
            [this, rowNumber](int result)
            {
                if (result <= 0) return;

                if (result == 1) // Prepare to Play
                {
                    if (rowNumber >= 0 && rowNumber < (int)trackPaths.size())
                    {
                        selectedTrackPath = trackPaths[rowNumber];
                        selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
                        prepareLabel1.setText("SELECTED TRACK : " + selectedTrack, juce::dontSendNotification);
                    }
                }
                else if (result == 2) // Remove Track
                {
                    if (rowNumber >= 0 && rowNumber < (int)trackPaths.size())
                    {
                        CSVOperator::removeTrack(rowNumber);
                        refreshPlaylist();
                    }
                }
                else if (result == 3) // Suggest Mix
                {
                    int suggestion = suggestNextTrack(rowNumber);
                    if (suggestion >= 0)
                    {
                        tableComponent.selectRow(suggestion, true, true);
                        selectedTrackPath = trackPaths[suggestion];
                        selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
                        prepareLabel1.setText("SUGGESTED MIX: " + selectedTrack, juce::dontSendNotification);
                    }
                }
            }
        );
    }
}

void PlaylistComponent::searchTracks(juce::String input)
{
    if (input != lastSearch)
        rowCounter = -1;

    lastSearch = input;

    for (int i = rowCounter + 1; i < (int)trackTitles.size(); ++i)
    {
        if (trackTitles[i].toLowerCase().contains(input.toLowerCase()))
        {
            tableComponent.selectRow(i, false, true);
            rowCounter = i;
            return;
        }
    }
    rowCounter = -1;
}

void PlaylistComponent::buttonClicked(juce::Button* button)
{
    if (button->getButtonText() == "Prepare To Play")
    {
        int id = std::stoi(button->getComponentID().toStdString());
        if (id >= 0 && id < (int)trackPaths.size())
        {
            selectedTrackPath = trackPaths[id];
            selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
            prepareLabel1.setText("SELECTED TRACK : " + selectedTrack, juce::dontSendNotification);
        }
    }
    if (button == &addTrackButton)
    {
        auto chooser = std::make_shared<juce::FileChooser>("Select a file...", juce::File{}, "*.mp3;*.wav");

        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) mutable
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    CSVOperator::addNewTrack(file.getFullPathName());
                    refreshPlaylist();
                }
            });
    }
    if (button == &removeTrackButton)
    {
        int selectedRow = tableComponent.getSelectedRow();
        if (selectedRow >= 0)
        {
            CSVOperator::removeTrack(selectedRow);
            refreshPlaylist();
        }
    }
    if (button == &searchButton)
    {
        searchTracks(searchBox.getText());
    }
    if (button == &suggestMixButton)
    {
        int currentIndex = tableComponent.getSelectedRow();
        int suggestion = suggestNextTrack(currentIndex);

        if (suggestion >= 0)
        {
            tableComponent.selectRow(suggestion, true, true);
            selectedTrackPath = trackPaths[suggestion];
            selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
            prepareLabel1.setText("SUGGESTED MIX: " + selectedTrack, juce::dontSendNotification);
        }
    }
}

void PlaylistComponent::analyzeTrackBPMs()
{
    // Use trackPaths to analyze BPMs
    auto job = new BPMAnalysisJob(trackPaths, [this](std::vector<std::pair<float, juce::String>> result) {
        trackMetadata = std::move(result);
        tableComponent.repaint();
    });
    threadPool.addJob(job, true);
}

int PlaylistComponent::suggestNextTrack(int currentIndex)
{
    if (currentIndex < 0 || currentIndex >= (int)trackMetadata.size())
        return -1;

    float currentBPM = trackMetadata[currentIndex].first;
    float minDiff = std::numeric_limits<float>::max();
    int bestIndex = -1;

    for (int i = 0; i < (int)trackMetadata.size(); ++i)
    {
        if (i == currentIndex || trackMetadata[i].first <= 0.0f)
            continue;

        float diff = std::abs(trackMetadata[i].first - currentBPM);

        if (diff < minDiff)
        {
            minDiff = diff;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void PlaylistComponent::rebuildFilteredList()
{
    filteredTrackIndices.clear();

    if (currentFilter == FilterMode::All)
    {
        // Show all tracks
        for (int i = 0; i < (int)trackTitles.size(); ++i)
            filteredTrackIndices.push_back(i);
    }
    else if (currentFilter == FilterMode::Favorites)
    {
        // Show only favorites
        for (int i = 0; i < (int)isFavorite.size(); ++i)
        {
            if (isFavorite[i])
                filteredTrackIndices.push_back(i);
        }
    }
}
