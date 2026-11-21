#pragma once
#include <juce_graphics/juce_graphics.h>

// Same palette as One Shooter / Quad Shooter (orange accent)
namespace SirenColours
{
    const juce::Colour bgTop         { 0xff17181c };
    const juce::Colour bgBottom      { 0xff0f1013 };
    const juce::Colour card          { 0xff1d1f24 };
    const juce::Colour cardBorder    { 0xff2b2e35 };
    const juce::Colour inset         { 0xff121317 };
    const juce::Colour accent        { 0xffff7a2f };   // Orange
    const juce::Colour accent2       { 0xffffb347 };   // Amber
    const juce::Colour clip          { 0xffff4d5e };   // Limiter = red
    const juce::Colour warn          { 0xffffd54f };
    const juce::Colour ok            { 0xff69f0ae };
    const juce::Colour rec           { 0xffff5a5f };
    const juce::Colour env           { 0xffffd54f };   // Envelope = yellow
    const juce::Colour reverbCol     { 0xff9fa8da };   // Reverb = lavender
    const juce::Colour text          { 0xffeceef2 };
    const juce::Colour textDim       { 0xff7d828c };
    const juce::Colour knobTrack     { 0xff2c2f36 };
    const juce::Colour knobBody      { 0xff26282e };

    // Bundle colours A, B, C (same as slots A, B, C in Quad Shooter)
    inline juce::Colour bundle (int i)
    {
        switch (i)
        {
            case 0:  return juce::Colour (0xffff7a2f);   // Orange
            case 1:  return juce::Colour (0xff4fc3f7);   // Blue
            default: return juce::Colour (0xff69f0ae);   // Green
        }
    }
}
