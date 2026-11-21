#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Colours.h"

// Drehregler mit Beschriftung, an einen Parameter gebunden
struct Knob
{
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void setup (juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                const juce::String& name, juce::Colour c, bool small = false)
    {
        using namespace SirenColours;
        slider.setName (name);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, small ? 15 : 18);
        slider.getProperties().set ("accent", c.toString());
        slider.setMouseDragSensitivity (180);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        parent.addAndMakeVisible (slider);

        label.setText (name.toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (small ? 9.5f : 10.5f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, textDim);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        parent.addAndMakeVisible (label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
        auto* param = apvts.getParameter (paramID);
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    // Auf einen anderen Parameter umhaengen (Edit-Zeile beim Slot-Wechsel)
    void rebind (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, juce::Colour c)
    {
        attachment.reset();
        slider.getProperties().set ("accent", c.toString());
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
        auto* param = apvts.getParameter (paramID);
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.repaint();
    }

    void setBounds (juce::Rectangle<int> r, int labelH = 16)
    {
        label.setBounds (r.removeFromTop (labelH));
        slider.setBounds (r);
    }

    static void layoutRow (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs, int labelH = 16)
    {
        const int w = area.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
            k->setBounds (area.removeFromLeft (w), labelH);
    }
};
