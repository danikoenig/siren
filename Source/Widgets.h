#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Colours.h"

// Segment switch for choice parameters (filter type)
class SegmentSwitch : public juce::Component
{
public:
    SegmentSwitch (juce::RangedAudioParameter& p, juce::StringArray l, juce::Colour c)
        : attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); }), labels (l), colour (c)
    { attachment.sendInitialUpdate(); }

    void paint (juce::Graphics& g) override
    {
        using namespace SirenColours;
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (inset); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cardBorder.brighter (0.1f)); g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const int n = labels.size(); const float w = r.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.5f);
            const bool on = i == index;
            if (on) { g.setColour (colour); g.fillRoundedRectangle (seg, 4.0f); }
            g.setColour (on ? bgBottom : textDim); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawFittedText (labels[i], seg.toNearestInt(), juce::Justification::centred, 1, 0.6f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = labels.size();
        attachment.setValueAsCompleteGesture ((float) juce::jlimit (0, n - 1, (int) ((float) e.x / (float) getWidth() * (float) n)));
    }
private:
    juce::ParameterAttachment attachment; juce::StringArray labels; juce::Colour colour; int index = 0;
};

// Thin level bar with title (header meters)
class LevelBar : public juce::Component
{
public:
    LevelBar (juce::String t, juce::Colour c, bool fromRight = false) : title (t), colour (c), reverse (fromRight) {}
    void set (float v) { v = juce::jlimit (0.0f, 1.0f, v); if (std::abs (v - value) > 0.002f) { value = v; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        using namespace SirenColours;
        auto r = getLocalBounds().toFloat();
        g.setColour (textDim); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (title, r.removeFromLeft (44.0f), juce::Justification::centredLeft);
        auto bar = r.withSizeKeepingCentre (r.getWidth(), 8.0f);
        g.setColour (inset); g.fillRoundedRectangle (bar, 4.0f);
        if (value > 0.0f)
        {
            const float w = juce::jmax (8.0f, bar.getWidth() * value);
            g.setColour (colour);
            g.fillRoundedRectangle (reverse ? bar.withLeft (bar.getRight() - w) : bar.withWidth (w), 4.0f);
        }
    }
private:
    juce::String title; juce::Colour colour; bool reverse; float value = 0.0f;
};
