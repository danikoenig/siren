#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SirenEngine.h"

class SirenProcessor : public juce::AudioProcessor
{
public:
    SirenProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Siren"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::String matrixId(int lfo, int col) { return "m" + juce::String(lfo) + "_" + juce::String(col); }
    static float matrixDefault(int l, int c);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>, siren::kNumLfo> lfoOut {};
    std::atomic<float> envOut { 0 }, grDb { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    siren::Engine engine;
    std::vector<int> held; int lastNote = -1; bool sustain = false; float bend = 0;
};
