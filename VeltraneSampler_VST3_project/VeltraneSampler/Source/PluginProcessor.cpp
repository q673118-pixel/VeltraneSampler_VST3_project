#include "PluginProcessor.h"
#include "PluginEditor.h"

VeltraneProcessor::VeltraneProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    formatManager.registerBasicFormats();

    auto* pitch = apvts.getRawParameterValue ("pitch");
    auto* atk   = apvts.getRawParameterValue ("attack");
    auto* rel   = apvts.getRawParameterValue ("release");
    auto* shot  = apvts.getRawParameterValue ("oneshot");

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new SampleVoice (pitch, atk, rel, shot));
}

juce::AudioProcessorValueTreeState::ParameterLayout VeltraneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Gain",
                 NormalisableRange<float> (-36.0f, 6.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                 NormalisableRange<float> (0.0f, 500.0f, 0.1f, 0.4f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",
                 NormalisableRange<float> (5.0f, 3000.0f, 1.0f, 0.4f), 150.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff",
                 NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pitch", 1 }, "Pitch",
                 NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "oneshot", 1 }, "One-shot", false));

    return { p.begin(), p.end() };
}

void VeltraneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.reset();
    lastGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("gain")->load());
}

bool VeltraneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VeltraneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // on-screen keyboard / preview events
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // one-shot mode: every note plays the sample at its original pitch, note-offs are ignored
    if (apvts.getRawParameterValue ("oneshot")->load() > 0.5f)
    {
        juce::MidiBuffer remapped;
        const int root = rootNote.load();
        for (const auto meta : midi)
        {
            auto m = meta.getMessage();
            if (m.isNoteOn())
            {
                m.setNoteNumber (root);
                remapped.addEvent (m, meta.samplePosition);
            }
            else if (! m.isNoteOff())
                remapped.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (remapped);
    }

    synth.renderNextBlock (buffer, midi, 0, numSamples);

    // low-pass filter
    const float cutoff = apvts.getRawParameterValue ("cutoff")->load();
    if (cutoff < 19900.0f)
    {
        filter.setCutoffFrequency (cutoff);
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
    }

    // output gain (smoothed)
    const float g = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("gain")->load());
    buffer.applyGainRamp (0, numSamples, lastGain, g);
    lastGain = g;
}

int VeltraneProcessor::parseRootNote (const juce::String& name)
{
    // file names end with the root note, e.g. Kick_808_339_A1 or Synth_Bell_412_As4
    const auto tok = name.fromLastOccurrenceOf ("_", false, false);
    if (tok.length() < 2) return 60;

    int semis = 0;
    switch (juce::CharacterFunctions::toUpperCase (tok[0]))
    {
        case 'C': semis = 0;  break;
        case 'D': semis = 2;  break;
        case 'E': semis = 4;  break;
        case 'F': semis = 5;  break;
        case 'G': semis = 7;  break;
        case 'A': semis = 9;  break;
        case 'B': semis = 11; break;
        default:  return 60;
    }

    int idx = 1;
    if (tok[idx] == 's' || tok[idx] == '#') { ++semis; ++idx; }

    const auto oct = tok.substring (idx);
    if (oct.isEmpty() || ! oct.containsOnly ("-0123456789")) return 60;

    return juce::jlimit (0, 127, (oct.getIntValue() + 1) * 12 + semis);
}

bool VeltraneProcessor::loadSample (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->lengthInSamples > 44100 * 60)
        return false;

    const int channels = (int) juce::jmin ((unsigned int) 2, reader->numChannels);
    juce::AudioBuffer<float> buf (channels, (int) reader->lengthInSamples);
    reader->read (&buf, 0, (int) reader->lengthInSamples, 0, true, channels > 1);

    const auto name = file.getFileNameWithoutExtension();
    const int root = parseRootNote (name);

    synth.clearSounds();
    synth.addSound (new SampleSound (name, std::move (buf), reader->sampleRate, root));

    rootNote = root;
    loadedName = name;
    apvts.state.setProperty ("sampleFile", file.getFullPathName(), nullptr);
    return true;
}

void VeltraneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VeltraneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            const juce::File f (apvts.state.getProperty ("sampleFile").toString());
            if (f.existsAsFile())
                loadSample (f);
        }
}

juce::AudioProcessorEditor* VeltraneProcessor::createEditor()
{
    return new VeltraneEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VeltraneProcessor();
}
