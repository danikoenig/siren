#pragma once
#include "PluginProcessor.h"

class SirenEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SirenEditor(SirenProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    struct Knob { juce::Slider s; juce::Label l; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a; };
    void addKnob(Knob& k, const char* id, const char* text, bool big);
    SirenProcessor& proc;
    Knob x, y, z, pitch, index, animate, xAtt, yAtt, zAtt, lfoSpeed, voct2, pmod, idxLfo, level;
    juce::ToggleButton nearest { "Nearest" }, lock { "Lock" }, midiReset { "Note resets phase" };
    juce::TextButton reset { "Reset" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aN, aL, aM;
    std::array<std::array<float, 120>, 2> hist{}; int hp = 0;
    juce::Rectangle<int> scope1, scope2;
};
