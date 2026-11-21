#pragma once
#include <juce_graphics/juce_graphics.h>

namespace SirenColours
{
    const juce::Colour bgTop         { 0xff14161c };
    const juce::Colour bgBottom      { 0xff0c0d11 };
    const juce::Colour card          { 0xff1b1d24 };
    const juce::Colour cardBorder    { 0xff2a2d36 };
    const juce::Colour inset         { 0xff111218 };
    const juce::Colour accent        { 0xff5ce1e6 };   // Cyan
    const juce::Colour accent2       { 0xffa78bfa };   // Violett
    const juce::Colour clip          { 0xffff4d5e };   // Rot (Limiter)
    const juce::Colour warn          { 0xffffd54f };   // Gelb
    const juce::Colour ok            { 0xff69f0ae };   // Gruen
    const juce::Colour rec           { 0xffff5a5f };   // Record
    const juce::Colour text          { 0xffeceef2 };
    const juce::Colour textDim       { 0xff7d828c };
    const juce::Colour knobTrack     { 0xff2a2d35 };
    const juce::Colour knobBody      { 0xff24262d };

    // Algorithmus-Farben
    inline juce::Colour algo (int i)
    {
        switch (i)
        {
            case 0:  return juce::Colour (0xff5ce1e6);   // 2 Delays  Cyan
            case 1:  return juce::Colour (0xff4fa3ff);   // 4 Delays  Blau
            case 2:  return juce::Colour (0xffa78bfa);   // Reverb    Violett
            default: return juce::Colour (0xffffb347);   // Granular  Amber
        }
    }
}
