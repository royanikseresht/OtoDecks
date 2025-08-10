#include <JuceHeader.h>
#include "PlaylistComponent.h"
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

        // Run callback on the message thread
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
    trackPaths = CSVOperator::returnTrackPathsArray();
    populateTrackTitles();
    analyzeTrackBPMs();

    tableComponent.getHeader().addColumn("Title", 1, 200);
    tableComponent.getHeader().addColumn("Length", 2, 200);
    tableComponent.getHeader().addColumn("", 3, 400);
    tableComponent.getHeader().addColumn("BPM", 4, 100);
    tableComponent.getHeader().addColumn("Notes", 5, 400);     // 🆕 Notes column
    tableComponent.getHeader().addColumn("Favorites", 6, 60); 

    tableComponent.setModel(this);

    addAndMakeVisible(tableComponent);
    addAndMakeVisible(prepareLabel1);
    addAndMakeVisible(prepareLabel2);
    addAndMakeVisible(addTrackButton);
    addAndMakeVisible(removeTrackButton);
    addAndMakeVisible(searchBox);
    addAndMakeVisible(searchButton);

    addTrackButton.addListener(this);
    removeTrackButton.addListener(this);
    searchButton.addListener(this);

    addAndMakeVisible(suggestMixButton);
    suggestMixButton.setButtonText("Suggest Mix");
    suggestMixButton.addListener(this);

    formatManager.registerBasicFormats();
}

PlaylistComponent::~PlaylistComponent() {}

void PlaylistComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::black);
    juce::Rectangle<float> prepareBorder(getWidth() * 0.295, getHeight() * 0.65, getWidth() * 0.41, getHeight() * 0.35);
    g.fillRect(prepareBorder);

    g.setColour(juce::Colour(0xffd5d5da));
    juce::Rectangle<float> prepareArea(getWidth() * 0.3, getHeight() * 0.68, getWidth() * 0.4, getHeight() * 0.31);
    g.fillRect(prepareArea);

    // PREPARE TO PLAY TEXT
    prepareLabel1.setColour(juce::Label::textColourId, juce::Colours::white);
    prepareLabel1.setFont(juce::Font(20.0f));
    prepareLabel1.setText(selectedTrack, juce::dontSendNotification);
    prepareLabel1.setJustificationType(juce::Justification::centred);
    prepareLabel1.setBounds(getWidth() * 0.25, getHeight() * 0.80, getWidth() * 0.5, getHeight() * 0.1);

    prepareLabel2.setColour(juce::Label::textColourId, juce::Colours::white);
    prepareLabel2.setFont(juce::Font(14.0f));
    prepareLabel2.setText("PREPARED TRACK", juce::dontSendNotification);
    prepareLabel2.setJustificationType(juce::Justification::centred);
    prepareLabel2.setBounds(getWidth() * 0.3, getHeight() * 0.72, getWidth() * 0.4, getHeight() * 0.1);

    // UI LAYOUT
    addTrackButton.setBounds(getWidth() * 0.72, getHeight() * 0.68, getWidth() * 0.25, getHeight() * 0.08);
    removeTrackButton.setBounds(getWidth() * 0.72, getHeight() * 0.80, getWidth() * 0.25, getHeight() * 0.08);
    suggestMixButton.setBounds(getWidth() * 0.72, getHeight() * 0.92, getWidth() * 0.25, getHeight() * 0.08);

    searchBox.setBounds(getWidth() * 0.02, getHeight() * 0.70, getWidth() * 0.25, getHeight() * 0.1);
    searchButton.setBounds(getWidth() * 0.15, getHeight() * 0.82, getWidth() * 0.12, getHeight() * 0.1);


}

void PlaylistComponent::resized()
{
    tableComponent.setBounds(0, 0, getWidth(), getHeight() * 0.67);
    tableComponent.getHeader().setColumnWidth(1, getWidth() / 4);
    tableComponent.getHeader().setColumnWidth(2, getWidth() / 16);
    tableComponent.getHeader().setColumnWidth(3, getWidth() / 8);
    tableComponent.getHeader().setColumnWidth(4, getWidth() / 16);
    tableComponent.getHeader().setColumnWidth(5, getWidth() / 2.35);
    tableComponent.getHeader().setColumnWidth(6, getWidth() / 16);
    
}

int PlaylistComponent::getNumRows()
{
    return trackTitles.size();
}

void PlaylistComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    g.fillAll(rowIsSelected ? juce::Colour(0xffd5d5da) : juce::Colours::grey);
}

void PlaylistComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool)
{
    if (columnId == 1)
    {
        g.drawText(trackTitles[rowNumber], 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
    else if (columnId == 2)
    {
        juce::File filepath{ juce::String(trackPaths[rowNumber]) };
        int lengthInSeconds = 0;

        if (auto* reader = formatManager.createReaderFor(filepath))
        {
            lengthInSeconds = reader->lengthInSamples / reader->sampleRate;
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
        if (rowNumber < (int)trackMetadata.size())
        {
            float bpm = trackMetadata[rowNumber].first;
            if (bpm > 0.0f)
            {
                // Normal BPM styling
                g.setColour(juce::Colours::black);
                g.setFont(juce::Font(14.0f, juce::Font::plain));
                juce::String bpmText = juce::String(bpm, 1);
                g.drawText(bpmText, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
            }
            else
            {
                // BPM not found
                g.setColour(juce::Colours::lightgrey);
                g.setFont(juce::Font(14.0f, juce::Font::italic));
                g.drawText("N/A", 2, 0, width - 4, height, juce::Justification::centredLeft, true);
            }
        }
        else
        {
            // Still loading BPM
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(14.0f, juce::Font::italic));
            g.drawText("Loading...", 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
    }

}

juce::Component* PlaylistComponent::refreshComponentForCell(int rowNumber, int columnID, bool, juce::Component* existingComponentToUpdate)
{
    if (columnID == 3 && existingComponentToUpdate == nullptr)
    {
        auto* btn = new juce::TextButton("Prepare To Play");
        btn->setComponentID(std::to_string(rowNumber));
        btn->addListener(this);
        return btn;
    }

    else if (columnID == 5) // Notes
    {
        if (rowNumber >= trackNotes.size())
            return nullptr;

        auto* noteLabel = dynamic_cast<juce::Label*>(existingComponentToUpdate);
        if (noteLabel == nullptr)
            noteLabel = new juce::Label();

        noteLabel->setEditable(true);
        noteLabel->setText(trackNotes[rowNumber], juce::dontSendNotification);

        noteLabel->onTextChange = [this, noteLabel, rowNumber]()
        {
            if (rowNumber < trackNotes.size())
                trackNotes[rowNumber] = noteLabel->getText();
        };

        return noteLabel;
    }

    else if (columnID == 6) // Favorite (heart button)
    {
        auto* heartButton = dynamic_cast<juce::DrawableButton*>(existingComponentToUpdate);
        if (heartButton == nullptr)
        {
            heartButton = new juce::DrawableButton("heartButton", juce::DrawableButton::ImageFitted);
            heartButton->setClickingTogglesState(true);
            heartButton->addListener(this);

            heartButton->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            heartButton->setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
            
            // Set background colour for normal (off) state
            heartButton->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            // Set background colour for toggled (on) state
            heartButton->setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);


        }

        // Load SVG drawables once (cache static/shared if you want for perf)
        static std::unique_ptr<juce::XmlElement> filledHeartXml = juce::parseXML(juce::String(heartFilledSVG));
        static std::unique_ptr<juce::Drawable> filledHeart = filledHeartXml ? juce::Drawable::createFromSVG(*filledHeartXml) : nullptr;

        static std::unique_ptr<juce::XmlElement> outlineHeartXml = juce::parseXML(juce::String(heartOutlineSVG));
        static std::unique_ptr<juce::Drawable> outlineHeart = outlineHeartXml ? juce::Drawable::createFromSVG(*outlineHeartXml) : nullptr;


        if (isFavorite[rowNumber])
            heartButton->setImages(filledHeart.get());
        else
            heartButton->setImages(outlineHeart.get());

        heartButton->setToggleState(isFavorite[rowNumber], juce::dontSendNotification);

        // Capture rowNumber copy for lambda
        heartButton->onClick = [this, heartButton, rowNumber]()
        {
            bool newState = heartButton->getToggleState();
            isFavorite[rowNumber] = newState;

            // Update button image immediately
            if (newState)
                heartButton->setImages(filledHeart.get());
            else
                heartButton->setImages(outlineHeart.get());
        };

        return heartButton;
    }

    return existingComponentToUpdate;
}

void PlaylistComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    // Select the row immediately (Finder-style)
    tableComponent.selectRow(rowNumber, false, true);

    if (e.mods.isPopupMenu()) // Right click
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
                if (result <= 0) return; // cancelled

                if (result == 1) // Prepare to Play
                {
                    if (rowNumber >= 0 && rowNumber < trackPaths.size())
                    {
                        selectedTrackPath = trackPaths[rowNumber];
                        selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
                        prepareLabel1.setText("SELECTED TRACK : " + selectedTrack, juce::dontSendNotification);
                    }
                }
                else if (result == 2) // Remove Track
                {
                    if (rowNumber >= 0 && rowNumber < trackPaths.size())
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

void PlaylistComponent::populateTrackTitles()
{
    trackTitles.clear();
    for (const std::string& s : trackPaths)
    {
        trackTitles.push_back(convertTrackPathToTitle(s));
        trackNotes.push_back(" ... ");
        isFavorite.push_back(false); // Default: not favorited

    }
}

juce::String PlaylistComponent::convertTrackPathToTitle(std::string path)
{
    juce::File juceFilePath(path);
    return juceFilePath.getFileNameWithoutExtension();
}

void PlaylistComponent::refreshPlaylist()
{
    trackPaths = CSVOperator::returnTrackPathsArray();
    populateTrackTitles();
    analyzeTrackBPMs();
    tableComponent.updateContent();
    tableComponent.repaint();
}

void PlaylistComponent::searchTracks(juce::String input)
{
    if (input != lastSearch)
        rowCounter = -1;

    lastSearch = input;

    for (int i = rowCounter + 1; i < trackTitles.size(); ++i)
    {
        if (trackTitles[i].containsIgnoreCase(input))
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
        selectedTrackPath = trackPaths[id];
        selectedTrack = convertTrackPathToTitle(selectedTrackPath.toStdString());
        prepareLabel1.setText("SELECTED TRACK : " + selectedTrack, juce::dontSendNotification);

        DBG("TrackName: " << selectedTrack);
        DBG("TrackPath: " << selectedTrackPath);
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
            DBG("Suggested track index: " << suggestion);
        }
        else
        {
            DBG("No suggestion could be made.");
        }
    }
}


void PlaylistComponent::analyzeTrackBPMs()
{
    auto job = new BPMAnalysisJob(trackPaths, [this](std::vector<std::pair<float, juce::String>> result) {
        trackMetadata = std::move(result);
        tableComponent.repaint(); // Or updateContent() if needed
    });

    threadPool.addJob(job, true);
}

int PlaylistComponent::suggestNextTrack(int currentIndex)
{
    if (currentIndex < 0 || currentIndex >= trackMetadata.size())
        return -1;

    float currentBPM = trackMetadata[currentIndex].first;
    float minDiff = std::numeric_limits<float>::max();
    int bestIndex = -1;

    for (int i = 0; i < trackMetadata.size(); ++i)
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