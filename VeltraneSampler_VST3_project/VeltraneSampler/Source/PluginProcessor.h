#pragma once
#include <JuceHeader.h>

// ---------------------------------------------------------------- sound
struct SampleSound : public juce::SynthesiserSound
{
    SampleSound (const juce::String& n, juce::AudioBuffer<float>&& b, double sr, int root)
        : name (n), data (std::move (b)), sourceRate (sr), rootNote (root) {}

    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }

    juce::String name;
    juce::AudioBuffer<float> data;
    double sourceRate;
    int rootNote;
};

// ---------------------------------------------------------------- voice
class SampleVoice : public juce::SynthesiserVoice
{
public:
    SampleVoice (std::atomic<float>* pitchSemis,
                 std::atomic<float>* attackMs,
                 std::atomic<float>* releaseMs,
                 std::atomic<float>* oneShot)
        : pitch (pitchSemis), atk (attackMs), rel (releaseMs), shot (oneShot) {}

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SampleSound*> (s) != nullptr;
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound* s, int) override
    {
        sound = dynamic_cast<SampleSound*> (s);
        if (sound == nullptr) return;

        pos   = 0.0;
        level = velocity;
        const double semis = (double) (midiNote - sound->rootNote) + (double) pitch->load();
        ratio = std::pow (2.0, semis / 12.0) * sound->sourceRate / getSampleRate();

        adsr.setSampleRate (getSampleRate());
        adsr.setParameters ({ atk->load() * 0.001f, 0.0f, 1.0f, rel->load() * 0.001f });
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (shot->load() > 0.5f && allowTailOff)
            return;                      // one-shot: let the sample play out
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            adsr.reset();
            sound = nullptr;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (sound == nullptr) return;

        const auto& d = sound->data;
        const int len = d.getNumSamples();
        const int nch = d.getNumChannels();
        const float* s0 = d.getReadPointer (0);
        const float* s1 = d.getReadPointer (nch > 1 ? 1 : 0);
        float* l = out.getWritePointer (0, start);
        float* r = out.getNumChannels() > 1 ? out.getWritePointer (1, start) : nullptr;

        auto at = [len] (const float* p, int i) { return p[juce::jlimit (0, len - 1, i)]; };

        for (int i = 0; i < num; ++i)
        {
            const int ip = (int) pos;
            if (ip >= len - 1 || ! adsr.isActive())
            {
                adsr.reset();
                sound = nullptr;
                clearCurrentNote();
                break;
            }

            const float fr = (float) (pos - ip);
            const float a = hermite (fr, at (s0, ip - 1), at (s0, ip), at (s0, ip + 1), at (s0, ip + 2));
            const float b = hermite (fr, at (s1, ip - 1), at (s1, ip), at (s1, ip + 1), at (s1, ip + 2));
            const float e = adsr.getNextSample() * level;

            l[i] += a * e;
            if (r != nullptr) r[i] += b * e;
            pos += ratio;
        }
    }

private:
    static float hermite (float x, float y0, float y1, float y2, float y3)
    {
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * x + c2) * x + c1) * x + y1;
    }

    std::atomic<float>* pitch;
    std::atomic<float>* atk;
    std::atomic<float>* rel;
    std::atomic<float>* shot;

    SampleSound* sound = nullptr;
    juce::ADSR adsr;
    double pos = 0.0, ratio = 1.0;
    float level = 1.0f;
};

// ---------------------------------------------------------------- processor
class VeltraneProcessor : public juce::AudioProcessor
{
public:
    VeltraneProcessor();
    ~VeltraneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool loadSample (const juce::File& file);
    juce::String getLoadedName() const { return loadedName; }
    int getRootNote() const { return rootNote.load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static int parseRootNote (const juce::String& fileName);

    juce::AudioFormatManager formatManager;
    juce::Synthesiser synth;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::String loadedName;
    std::atomic<int> rootNote { 60 };
    float lastGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VeltraneProcessor)
};
