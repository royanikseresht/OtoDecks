#include "LookAndFeel.h"

ModernLookAndFeel::ModernLookAndFeel()
{
    // GENERAL BACKGROUNDS
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xfff2f2f7)); // light grey
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xfff2f2f7)); // same as main bg

    // TEXT
    setColour(juce::Label::textColourId, juce::Colours::black);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);  // when pressed
    setColour(juce::TextButton::textColourOffId, juce::Colours::black); // normal

    // BUTTONS
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff2d55));      // pinkish red
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd81b60));    // darker pink-red

    // TABLES & LISTS
    setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colour(0xffe5e5ea)); // light greyish
    setColour(juce::TableHeaderComponent::textColourId, juce::Colours::black);
    setColour(juce::ListBox::backgroundColourId, juce::Colour(0xfff2f2f7));
    setColour(juce::ListBox::outlineColourId, juce::Colour(0xffd1d1d6));

    // TEXTBOXES
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xffffffff)); // pure white
    setColour(juce::TextEditor::textColourId, juce::Colours::black);
    setColour(juce::TextEditor::highlightColourId, juce::Colour(0xffff2d55)); // pink highlight

    // SLIDERS
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffff2d55));
    setColour(juce::Slider::trackColourId, juce::Colour(0xffe5e5ea));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xfff2f2f7));

    // COMBOBOX / DROPDOWN
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xffffffff));         
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffd1d1d6));            
    setColour(juce::ComboBox::textColourId, juce::Colours::black);                   
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffff2d55));               
    setColour(juce::PopupMenu::textColourId, juce::Colours::black);                   // <-- normal menu text
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffff2d55)); 
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);        

}