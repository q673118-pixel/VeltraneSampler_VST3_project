#include "PluginEditor.h"

namespace
{
    const juce::Colour bgTop    (0xff15161b);
    const juce::Colour bgBottom (0xff08080a);
    const juce::Colour gold     (0xffe6c88a);
    const juce::Colour dim      (0xff8d8a82);
}

VeltraneEditor::VeltraneEditor (VeltraneProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    laf.setColour (juce::ResizableWindow::backgroundColourId, bgBottom);
    laf.setColour (juce::Slider::rotarySliderFillColourId, gold);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff34343b));
    laf.setColour (juce::Slider::thumbColourId, gold);
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, dim);
    laf.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff23242b));
    laf.setColour (juce::TextButton::textColourOffId, gold);
    laf.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff101116));
    laf.setColour (juce::ListBox::outlineColourId, juce::Colour (0xff2a2b33));
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff23242b));
    laf.setColour (juce::ComboBox::textColourId, gold);
    laf.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff101116));
    laf.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    laf.setColour (juce::ToggleButton::textColourId, gold);
    laf.setColour (juce::ToggleButton::tickColourId, gold);
    setLookAndFeel (&laf);

    addAndMakeVisible (folderButton);
    folderButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Select the Veltrane Sound Pack folder", juce::File(), "");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.isDirectory())
                                  {
                                      proc.apvts.state.setProperty ("libraryPath", f.getFullPathName(), nullptr);
                                      refreshCategories();
                                  }
                              });
    };

    addAndMakeVisible (playButton);
    playButton.onClick = [this] { preview(); };

    addAndMakeVisible (categoryBox);
    categoryBox.onChange = [this]
    {
        const auto n = categoryBox.getText().toLowerCase();
        const bool drum = n.contains ("snare") || n.contains ("hat");
        if (auto* prm = proc.apvts.getParameter ("oneshot"))
            prm->setValueNotifyingHost (drum ? 1.0f : 0.0f);
        refreshList();
    };

    addAndMakeVisible (searchBox);
    searchBox.setTextToShowWhenEmpty ("search...", dim);
    searchBox.onTextChange = [this] { refreshList(); };

    addAndMakeVisible (list);
    list.setRowHeight (22);

    addAndMakeVisible (nameLabel);
    nameLabel.setFont (juce::Font (juce::FontOptions (20.0f)));
    nameLabel.setColour (juce::Label::textColourId, gold);
    nameLabel.setText (proc.getLoadedName().isEmpty() ? "No sample loaded" : proc.getLoadedName(),
                       juce::dontSendNotification);

    addKnob (gain,    "GAIN",    "gain",    " dB");
    addKnob (attack,  "ATTACK",  "attack",  " ms");
    addKnob (release, "RELEASE", "release", " ms");
    addKnob (cutoff,  "CUTOFF",  "cutoff",  " Hz");
    addKnob (pitch,   "PITCH",   "pitch",   " st");

    addAndMakeVisible (oneShotButton);
    oneShotAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "oneshot", oneShotButton);

    keyboard.setLowestVisibleKey (24);
    keyboard.setKeyWidth (22.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, gold.withAlpha (0.7f));
    addAndMakeVisible (keyboard);

    setSize (940, 540);
    refreshCategories();
}

VeltraneEditor::~VeltraneEditor()
{
    setLookAndFeel (nullptr);
}

void VeltraneEditor::addKnob (Knob& k, const juce::String& text, const juce::String& paramID, const juce::String& suffix)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    k.slider.setTextValueSuffix (suffix);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramID, k.slider);
}

void VeltraneEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (gold);
    g.setFont (22.0f);
    g.drawText ("V E L T R A N E   S A M P L E R", 20, 12, 500, 28, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff2a2b33));
    g.drawLine (20.0f, 46.0f, (float) getWidth() - 20.0f, 46.0f);
}

void VeltraneEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (34);

    auto kb = area.removeFromBottom (110);
    keyboard.setBounds (kb);
    area.removeFromBottom (12);

    auto left = area.removeFromLeft (320);
    auto row = left.removeFromTop (30);
    folderButton.setBounds (row.removeFromLeft (200));
    row.removeFromLeft (8);
    playButton.setBounds (row);
    left.removeFromTop (8);
    categoryBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (6);
    searchBox.setBounds (left.removeFromTop (26));
    left.removeFromTop (8);
    list.setBounds (left);

    area.removeFromLeft (24);
    nameLabel.setBounds (area.removeFromTop (34));
    area.removeFromTop (14);

    auto knobs = area.removeFromTop (190);
    const int w = knobs.getWidth() / 5;
    Knob* ks[] = { &gain, &attack, &release, &cutoff, &pitch };
    for (auto* k : ks)
    {
        auto c = knobs.removeFromLeft (w);
        k->label.setBounds (c.removeFromTop (20));
        k->slider.setBounds (c);
    }

    area.removeFromTop (10);
    oneShotButton.setBounds (area.removeFromTop (28).removeFromLeft (260));
}

void VeltraneEditor::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= visibleNames.size()) return;
    if (selected)
        g.fillAll (gold.withAlpha (0.22f));
    g.setColour (selected ? juce::Colours::white : juce::Colour (0xffc9c6bd));
    g.setFont (14.0f);
    g.drawText (visibleNames[row], 8, 0, w - 12, h, juce::Justification::centredLeft);
}

void VeltraneEditor::selectedRowsChanged (int row)
{
    if (row < 0 || row >= visibleFiles.size()) return;
    if (proc.loadSample (visibleFiles[row]))
    {
        nameLabel.setText (proc.getLoadedName(), juce::dontSendNotification);
        preview();
    }
}

void VeltraneEditor::preview()
{
    const int note = proc.getRootNote();
    proc.keyboardState.noteOn (1, note, 0.9f);

    juce::Component::SafePointer<VeltraneEditor> safe (this);
    juce::Timer::callAfterDelay (900, [safe, note]
    {
        if (safe != nullptr)
            safe->proc.keyboardState.noteOff (1, note, 0.0f);
    });
}

void VeltraneEditor::refreshCategories()
{
    categoryBox.clear (juce::dontSendNotification);
    categories.clear();

    const juce::File root (proc.apvts.state.getProperty ("libraryPath").toString());
    if (! root.isDirectory())
    {
        visibleFiles.clear();
        visibleNames.clear();
        list.updateContent();
        return;
    }

    auto dirs = root.findChildFiles (juce::File::findDirectories, false);
    std::sort (dirs.begin(), dirs.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    for (auto& d : dirs) categories.add (d);
    if (categories.isEmpty()) categories.add (root);

    int id = 1;
    for (auto& c : categories)
        categoryBox.addItem (c.getFileName(), id++);

    categoryBox.setSelectedId (1, juce::sendNotification);
}

void VeltraneEditor::refreshList()
{
    visibleFiles.clear();
    visibleNames.clear();

    const int idx = categoryBox.getSelectedId() - 1;
    if (idx >= 0 && idx < categories.size())
    {
        auto files = categories[idx].findChildFiles (juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac");
        std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
                   { return a.getFileName().compareNatural (b.getFileName()) < 0; });

        const auto q = searchBox.getText().trim();
        for (auto& f : files)
            if (q.isEmpty() || f.getFileNameWithoutExtension().containsIgnoreCase (q))
            {
                visibleFiles.add (f);
                visibleNames.add (f.getFileNameWithoutExtension());
            }
    }
    list.updateContent();
    list.repaint();
}
