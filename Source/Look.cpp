#include "Look.h"
#include "Colours.h"
#include <cmath>

using namespace SirenColours;


//==============================================================================
// LookAndFeel
SirenLookAndFeel::SirenLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, text);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, text);
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, card);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, cardBorder.brighter (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, text);
    setColour (juce::ComboBox::textColourId, text);
}

void SirenLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto col = juce::Colour::fromString (s.getProperties().getWithDefault ("accent", accent.toString()).toString());
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 6.0f;
    auto r = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto c = r.getCentre();
    const float radius = size * 0.5f;
    const float lineW = juce::jmax (3.0f, size * 0.075f);
    const float arcR = radius - lineW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool hover = s.isMouseOverOrDragging();

    // Track
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Limiter-Licht: Ring glueht rot mit der Gain Reduction
    const float gr = (float) s.getProperties().getWithDefault ("grLight", -1.0);
    if (gr > 0.01f)
    {
        juce::Path ring;
        ring.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (clip.withAlpha (0.18f * gr));
        g.strokePath (ring, juce::PathStrokeType (lineW * 3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (clip.withAlpha (0.25f + 0.6f * gr));
        g.strokePath (ring, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Wert (bipolar von der Mitte)
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        const auto vc = gr > 0.0f ? col.interpolatedWith (clip, juce::jmin (1.0f, gr * 1.5f)) : col;
        g.setColour (vc.withAlpha (0.25f));
        g.strokePath (val, juce::PathStrokeType (lineW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (vc);
        g.strokePath (val, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Markierung (z.B. FB = 1.0)
    if (s.getProperties().contains ("mark"))
    {
        const float mp = (float) s.getProperties()["mark"];
        const float ma = startAngle + mp * (endAngle - startAngle);
        const float r0 = radius + 1.0f, r1 = radius + 5.0f;
        g.setColour (text.withAlpha (0.8f));
        g.drawLine (c.x + std::sin (ma) * r0, c.y - std::cos (ma) * r0,
                    c.x + std::sin (ma) * r1, c.y - std::cos (ma) * r1, 1.6f);
    }

    // Body
    const float bodyR = arcR - lineW * 1.6f;
    auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (knobBody.brighter (hover ? 0.25f : 0.15f), body.getX(), body.getY(),
                                             knobBody.darker (0.3f), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    // Zeiger
    const float px = c.x + std::sin (angle) * bodyR * 0.72f;
    const float py = c.y - std::cos (angle) * bodyR * 0.72f;
    const float qx = c.x + std::sin (angle) * bodyR * 0.25f;
    const float qy = c.y - std::cos (angle) * bodyR * 0.25f;
    g.setColour (text);
    g.drawLine (qx, qy, px, py, 2.2f);
}

juce::Label* SirenLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::FontOptions (11.5f));
    l->setColour (juce::Label::textColourId, text.withAlpha (0.9f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::TextEditor::backgroundColourId, inset);
    l->setColour (juce::TextEditor::textColourId, text);
    l->setColour (juce::TextEditor::outlineColourId, accent);
    return l;
}

void SirenLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool primary = (bool) b.getProperties().getWithDefault ("primary", false);
    if (b.getProperties().contains ("fill"))
    {
        // Farbiger Schalter (Lead A..D, Mute): an = gefuellt, aus = Rahmen
        const auto col = juce::Colour::fromString (b.getProperties()["fill"].toString());
        const bool on = b.getToggleState();
        g.setColour (on ? col : inset);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? col : (highlighted ? col.withAlpha (0.8f) : col.withAlpha (0.35f)));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);
        if (down)
        {
            g.setColour (juce::Colours::white.withAlpha (0.15f));
            g.fillRoundedRectangle (r, 5.0f);
        }
        return;
    }
    if (primary)
    {
        g.setGradientFill (juce::ColourGradient (accent2, r.getX(), r.getY(), accent, r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.0f);
        if (highlighted || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.2f : 0.1f));
            g.fillRoundedRectangle (r, 6.0f);
        }
        b.setColour (juce::TextButton::textColourOffId, bgBottom);
        return;
    }
    g.setColour (down ? cardBorder.brighter (0.2f) : highlighted ? cardBorder : inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (cardBorder.brighter (0.15f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void SirenLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const auto col = juce::Colour::fromString (b.getProperties().getWithDefault ("accent", accent.toString()).toString());
    const auto style = b.getProperties().getWithDefault ("style", "power").toString();
    const bool on = b.getToggleState();

    if (style == "segment")
    {
        // SOFT | HARD
        g.setColour (inset);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto left = r.withWidth (r.getWidth() * 0.5f).reduced (2.0f);
        auto right = left.withX (r.getCentreX() + 2.0f);
        g.setColour (col.withAlpha (0.9f));
        g.fillRoundedRectangle (on ? right : left, 4.0f);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.setColour (on ? textDim : bgBottom);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawFittedText (b.getProperties().getWithDefault ("left", "SOFT").toString(), left.toNearestInt(), juce::Justification::centred, 1, 0.7f);
        g.setColour (on ? bgBottom : textDim);
        g.drawFittedText (b.getProperties().getWithDefault ("right", "HARD").toString(), right.toNearestInt(), juce::Justification::centred, 1, 0.7f);
        return;
    }

    if (style == "sidebar")
    {
        g.setColour (on ? col.withAlpha (0.18f) : inset);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);
        auto ic = juce::Rectangle<float> (18.0f, 14.0f).withCentre (r.getCentre());
        g.setColour (on ? text : textDim);
        g.drawRoundedRectangle (ic, 2.0f, 1.5f);
        g.fillRoundedRectangle (ic.withWidth (6.0f), 2.0f);
        return;
    }

    if (style == "keys")
    {
        // Mini-Keyboard wie in Bitwig
        g.setColour (on ? col.withAlpha (0.2f) : inset);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);

        auto kb = juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ r.getX() + 19.0f, r.getCentreY() });
        const float kw = kb.getWidth() / 5.0f;
        for (int i = 0; i < 5; ++i)
        {
            g.setColour (on ? text : textDim);
            g.fillRect (kb.getX() + (float) i * kw + 0.5f, kb.getY(), kw - 1.0f, kb.getHeight());
        }
        g.setColour (on ? bgBottom : inset);
        for (int i : { 1, 2, 4 })
            g.fillRect (kb.getX() + (float) i * kw - kw * 0.3f, kb.getY(), kw * 0.6f, kb.getHeight() * 0.6f);

        g.setColour (on ? text : textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (on ? "CHROMATIC" : "ONE KEY", r.withTrimmedLeft (36.0f), juce::Justification::centredLeft);
        return;
    }

    if (style == "labelled")
    {
        // Pill mit Punkt und eigenem Text (z.B. KEY SYNC)
        g.setColour (on ? col.withAlpha (0.18f) : inset);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
        auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ r.getX() + 13.0f, r.getCentreY() });
        g.setColour (on ? col : textDim.withAlpha (0.5f));
        g.fillEllipse (dot);
        g.setColour (on ? text : textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (22.0f).withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        return;
    }

    // Power-Pill
    g.setColour (on ? col.withAlpha (0.18f) : inset);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() });
    g.setColour (on ? col : textDim.withAlpha (0.5f));
    g.fillEllipse (dot);
    g.setColour (on ? text : textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (on ? "ON" : "OFF", r.withTrimmedLeft (22.0f), juce::Justification::centred);
}

void SirenLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                     int thumbStart, int thumbSize, bool over, bool down)
{
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize)
                          : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour (textDim.withAlpha (over || down ? 0.6f : 0.3f));
    g.fillRoundedRectangle (thumb.reduced (1).toFloat(), 3.0f);
}

void SirenLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    juce::Colour c = text;
    if (b.getProperties().contains ("fill"))
        c = b.getToggleState() ? bgBottom : juce::Colour::fromString (b.getProperties()["fill"].toString());
    else if ((bool) b.getProperties().getWithDefault ("primary", false))
        c = bgBottom;
    if (! b.isEnabled())
        c = c.withAlpha (0.35f);
    g.setColour (c);
    g.setFont (juce::FontOptions ((float) juce::jmin (13, b.getHeight() - 8), juce::Font::bold));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void SirenLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    const auto col = juce::Colour::fromString (box.getProperties().getWithDefault ("accent", accent.toString()).toString());
    g.setColour (inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver (true) ? col : cardBorder.brighter (0.15f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col);
    g.fillPath (arrow);
}

void SirenLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 0, box.getWidth() - 28, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, text);
}

void SirenLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style == juce::Slider::LinearVertical && (bool) s.getProperties().getWithDefault ("detune", false))
    {
        // use the full component height (JUCE shrinks the track area by the thumb radius)
        const auto col = juce::Colour::fromString (s.getProperties().getWithDefault ("accent", accent.toString()).toString());
        auto b = s.getLocalBounds().toFloat().reduced (0.0f, 4.0f);
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float py = b.getBottom() - (float) s.valueToProportionOfLength (s.getValue()) * b.getHeight();
        g.setColour (cardBorder.brighter (0.25f));
        g.fillRoundedRectangle (cx - 2.0f, b.getY(), 4.0f, b.getHeight(), 2.0f);
        g.setColour (col.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.85f));
        g.fillRoundedRectangle (cx - 2.0f, juce::jmin (cy, py), 4.0f, std::abs (py - cy) + 0.5f, 2.0f);
        g.setColour (text.withAlpha (0.45f));
        g.fillRect (cx - 7.0f, cy - 0.5f, 14.0f, 1.0f);
        g.setColour (text);
        g.fillRoundedRectangle (juce::Rectangle<float> (14.0f, 5.0f).withCentre ({ cx, py }), 2.0f);
        return;
    }
    if (style != juce::Slider::LinearBar)
        return LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);

    const auto col = juce::Colour::fromString (s.getProperties().getWithDefault ("accent", accent.toString()).toString());
    auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    g.setColour (knobTrack);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (col.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.85f));
    if ((bool) s.getProperties().getWithDefault ("bipolar", false))
    {
        // Von der Mitte aus fuellen, Mittelstrich als 0-Marke
        const float c = r.getCentreX();
        auto fill = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (c, pos), r.getY(), juce::jmax (c, pos), r.getBottom());
        if (fill.getWidth() > 0.5f)
            g.fillRoundedRectangle (fill, 2.0f);
        g.setColour (text.withAlpha (0.6f));
        g.fillRect (c - 0.5f, r.getY() - 2.0f, 1.0f, r.getHeight() + 4.0f);
        g.setColour (text);
        g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, r.getHeight() + 4.0f).withCentre ({ pos, r.getCentreY() }), 1.5f);
        return;
    }
    auto fill = r.withRight (juce::jmax (r.getX() + r.getHeight(), pos));
    g.fillRoundedRectangle (fill, r.getHeight() * 0.5f);
}
