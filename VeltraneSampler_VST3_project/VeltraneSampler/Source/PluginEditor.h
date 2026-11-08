#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VeltraneEditor : public juce::AudioProcessorEditor,
                       private juce::ListBoxModel
{
public:
    explicit VeltraneEditor (VeltraneProcessor&);
    ~VeltraneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // ListBoxModel
    int getNumRows() override { return visibleNames.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void selectedRowsChanged (int row) override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    void addKnob (Knob& k, const juce::String& text, const juce::String& paramID, const juce::String& suffix);

    void refreshCategories();
    void refreshList();
    void preview();

    VeltraneProcessor& proc;
    juce::LookAndFeel_V4 laf;

    juce::TextButton folderButton { "Library folder..." };
    juce::TextButton playButton { "Play" };
    juce::ComboBox categoryBox;
    juce::TextEditor searchBox;
    juce::ListBox list { "samples", this };
    juce::Label nameLabel;
    juce::ToggleButton oneShotButton { "One-shot (fixed pitch)" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oneShotAttach;

    Knob gain, attack, release, cutoff, pitch;
    juce::MidiKeyboardComponent keyboard;

    juce::Array<juce::File> categories;
    juce::Array<juce::File> visibleFiles;
    juce::StringArray visibleNames;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeltraneEditor)
};
