#pragma once
#include "PluginProcessor.h"

class SirenEditor : public juce::AudioProcessorEditor
{
public:
    explicit SirenEditor (SirenProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    class Panel;
    std::unique_ptr<Panel> panel;
};
