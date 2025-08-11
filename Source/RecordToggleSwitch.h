#pragma once
#include <JuceHeader.h>

class RecordToggleSwitch : public juce::ToggleButton
{
public:
    RecordToggleSwitch()
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        float cornerSize = bounds.getHeight() / 2.0f;

        // Background color
        g.setColour(getToggleState() ? juce::Colours::red : juce::Colours::darkgrey);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Knob
        float knobDiameter = bounds.getHeight() - 4;
        float knobX = getToggleState() ? bounds.getWidth() - knobDiameter - 2 : 2;
        g.setColour(juce::Colours::white);
        g.fillEllipse(knobX, 2, knobDiameter, knobDiameter);
    }
};
