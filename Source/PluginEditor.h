#pragma once
#include "PluginProcessor.h"

// 8 LFO rows x 24 columns (21 oscillator pitches + 3 filter cutoffs).
// Drag a cell up/down to set depth, double-click = 0, wheel = fine.
class MatrixComponent : public juce::Component
{
public:
    explicit MatrixComponent (juce::AudioProcessorValueTreeState& a) : apvts (a) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hl = hc = -1; repaint(); }
    juce::Rectangle<float> cellRect (int l, int c) const;
private:
    bool hit (juce::Point<int> p, int& l, int& c) const;
    juce::RangedAudioParameter* param (int l, int c) const { return apvts.getParameter (SirenProcessor::matrixId (l, c)); }
    juce::AudioProcessorValueTreeState& apvts;
    int dl = -1, dc = -1, hl = -1, hc = -1; float startValue = 0;
};

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
