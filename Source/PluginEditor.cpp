#include "PluginEditor.h"
#include "Colours.h"
#include "Knob.h"
#include "Look.h"
#include "Widgets.h"

using namespace SirenColours;
using namespace siren;

namespace
{
    constexpr int W = 960, H = 490, margin = 14, gap = 12, headerH = 44;
    const char* bundleName[3] = { "A", "B", "C" };
    const char* bundleRange[3] = { "LOW", "MID", "HIGH" };

    // XY pad: one LFO per bundle, the dot decides where it goes (corners = cutoff, reso, level, pitch)
    class XYPad : public juce::Component
    {
    public:
        XYPad (juce::AudioProcessorValueTreeState& a, const juce::String& idX, const juce::String& idY, juce::Colour c, std::function<float()> act)
            : px (*a.getParameter (idX)), py (*a.getParameter (idY)), colour (c), activity (std::move (act)),
              ax (px, [this] (float) { repaint(); }), ay (py, [this] (float) { repaint(); })
        { ax.sendInitialUpdate(); ay.sendInitialUpdate(); }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (inset); g.fillRoundedRectangle (r, 6.0f);
            g.setColour (colour.withAlpha (0.06f)); g.fillRoundedRectangle (r.reduced (1.0f), 5.0f);
            g.setColour (cardBorder.brighter (0.1f)); g.drawRoundedRectangle (r, 6.0f, 1.0f);
            g.setColour (text.withAlpha (0.07f));
            g.drawVerticalLine (juce::roundToInt (r.getCentreX()), r.getY() + 3.0f, r.getBottom() - 3.0f);
            g.drawHorizontalLine (juce::roundToInt (r.getCentreY()), r.getX() + 3.0f, r.getRight() - 3.0f);
            g.setColour (textDim); g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
            auto in = r.reduced (4.0f, 3.0f);
            g.drawText ("CUT", in, juce::Justification::topLeft); g.drawText ("RESO", in, juce::Justification::topRight);
            g.drawText ("LEVEL", in, juce::Justification::bottomLeft); g.drawText ("PITCH", in, juce::Justification::bottomRight);
            const auto pad = r.reduced (10.0f);
            const float cx = pad.getX() + px.getValue() * pad.getWidth(), cy = pad.getBottom() - py.getValue() * pad.getHeight();
            const float act = std::abs (activity ? activity() : 0.0f);
            g.setColour (colour.withAlpha (0.18f)); g.fillEllipse (cx - 11.0f - 5.0f * act, cy - 11.0f - 5.0f * act, 22.0f + 10.0f * act, 22.0f + 10.0f * act);
            g.setColour (colour); g.fillEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour (text); g.drawEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f, 1.2f);
        }
        void mouseDown (const juce::MouseEvent& e) override { px.beginChangeGesture(); py.beginChangeGesture(); set (e); }
        void mouseDrag (const juce::MouseEvent& e) override { set (e); }
        void mouseUp (const juce::MouseEvent&) override { px.endChangeGesture(); py.endChangeGesture(); }
        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            px.beginChangeGesture(); py.beginChangeGesture();
            px.setValueNotifyingHost (px.getDefaultValue()); py.setValueNotifyingHost (py.getDefaultValue());
            px.endChangeGesture(); py.endChangeGesture();
        }
    private:
        void set (const juce::MouseEvent& e)
        {
            const auto pad = getLocalBounds().toFloat().reduced (10.5f);
            px.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (e.position.x - pad.getX()) / pad.getWidth()));
            py.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (pad.getBottom() - e.position.y) / pad.getHeight()));
        }
        juce::RangedAudioParameter& px; juce::RangedAudioParameter& py; juce::Colour colour; std::function<float()> activity;
        juce::ParameterAttachment ax, ay;
    };
}

//==============================================================================
class SirenEditor::Panel : public juce::Component, private juce::Timer
{
public:
    explicit Panel (SirenProcessor& p) : proc (p), swellBar ("SWELL", ok), grBar ("LIMITER", clip, true)
    {
        setLookAndFeel (&look);
        auto& a = p.apvts;
        // Drone (envelope = yellow, like the amp envelope in Quad Shooter)
        kRoot.setup (*this, a, "root", "Root", accent); kAttack.setup (*this, a, "attack", "Attack", env); kRelease.setup (*this, a, "release", "Release", env);
        kStagger.setup (*this, a, "stagger", "Stagger", env); kGlide.setup (*this, a, "glide", "Glide", env);
        // Timbre
        kChord.setup (*this, a, "x", "Chord", accent); kSpread.setup (*this, a, "y", "Spread", accent); kTimbre.setup (*this, a, "z", "Timbre", accent);
        kIndex.setup (*this, a, "index", "Index", accent); kAnimate.setup (*this, a, "animate", "Animate", accent);
        // Reverb + out
        kMix.setup (*this, a, "rvMix", "Mix", reverbCol, true); kDecay.setup (*this, a, "rvDecay", "Decay", reverbCol, true);
        kTone.setup (*this, a, "rvTone", "Tone", reverbCol, true);
        kTone.slider.getProperties().set ("bipolar", true);
        kWidth.setup (*this, a, "width", "Width", accent, true); kMove.setup (*this, a, "move", "Move", accent, true);
        kLimit.setup (*this, a, "limit", "Limiter", accent2, true); kLevel.setup (*this, a, "level", "Level", accent2, true);
        for (int b = 0; b < kNumBundles; ++b)
        {
            const juce::String s (b); const auto col = bundle (b);
            kCut[(size_t) b].setup (*this, a, "cut" + s, "Cutoff", col, true); kRes[(size_t) b].setup (*this, a, "res" + s, "Reso", col, true);
            kLvl[(size_t) b].setup (*this, a, "lvl" + s, "Level", col, true); kDet[(size_t) b].setup (*this, a, "bdet" + s, "Detune", col, true);
            type[(size_t) b] = std::make_unique<SegmentSwitch> (*a.getParameter ("type" + s), juce::StringArray { "LP", "BP", "HP" }, col);
            addAndMakeVisible (*type[(size_t) b]);
            for (auto* sl : { &rate[(size_t) b], &depth[(size_t) b] })
            {
                sl->setSliderStyle (juce::Slider::LinearBar); sl->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 100, 15);
                sl->getProperties().set ("accent", col.withAlpha (0.55f).toString()); sl->setColour (juce::Slider::textBoxTextColourId, text);
                sl->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); sl->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
                addAndMakeVisible (*sl);
            }
            rateAtt[(size_t) b] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "lr" + s, rate[(size_t) b]);
            depthAtt[(size_t) b] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "ld" + s, depth[(size_t) b]);
            auto& c = shape[(size_t) b]; c.addItemList ({ "Sine", "Triangle", "Ramp", "Random" }, 1); c.getProperties().set ("accent", col.toString()); addAndMakeVisible (c);
            shapeAtt[(size_t) b] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "ls" + s, c);
            pad[(size_t) b] = std::make_unique<XYPad> (a, "lx" + s, "ly" + s, col, [this, b] { return proc.lfoOut[(size_t) b].load(); });
            addAndMakeVisible (*pad[(size_t) b]);
            if (b < 2)
            {
                auto& f = fm[(size_t) b]; f.setSliderStyle (juce::Slider::LinearBar); f.setTextBoxStyle (juce::Slider::TextBoxLeft, true, 70, 14);
                f.getProperties().set ("accent", col.withAlpha (0.55f).toString()); f.setColour (juce::Slider::textBoxTextColourId, text);
                f.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); f.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
                addAndMakeVisible (f); fmAtt[(size_t) b] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, b == 0 ? "fmA" : "fmB", f);
            }
        }
        addAndMakeVisible (swellBar); addAndMakeVisible (grBar);
        drone.getProperties().set ("style", "labelled"); drone.getProperties().set ("accent", accent.toString());
        drone.setClickingTogglesState (true); addAndMakeVisible (drone);
        droneAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, "gate", drone);
        drone.onStateChange = [this] { drone.setButtonText (drone.getToggleState() ? "DRONE RUNNING" : "START DRONE"); };
        drone.onStateChange();
        startTimerHz (20);
    }
    ~Panel() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) getHeight(), false)); g.fillAll();

        // logo: dot with two sound rings
        const float cx = (float) margin + 11.0f, cy = (float) (margin + headerH / 2);
        g.setColour (accent.withAlpha (0.35f)); g.drawEllipse (cx - 10.0f, cy - 10.0f, 20.0f, 20.0f, 1.6f);
        g.setColour (accent.withAlpha (0.6f)); g.drawEllipse (cx - 6.5f, cy - 6.5f, 13.0f, 13.0f, 1.4f);
        g.setColour (accent); g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
        g.setColour (text); g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
        g.drawText ("SIREN", margin + 34, margin + 4, 160, 22, juce::Justification::centredLeft);
        g.setColour (textDim); g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String::fromUTF8 ("by Dani Koenig  \xc2\xb7  v") + JucePlugin_VersionString, margin + 34, margin + 24, 200, 14, juce::Justification::centredLeft);

        auto card = [&] (juce::Rectangle<int> r, const juce::String& title, juce::Colour c)
        {
            g.setColour (SirenColours::card); g.fillRoundedRectangle (r.toFloat(), 10.0f);
            g.setColour (cardBorder); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
            g.setColour (c); g.fillEllipse ((float) r.getX() + 13.0f, (float) r.getY() + 11.0f, 7.0f, 7.0f);
            g.setColour (text.withAlpha (0.85f)); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (title, r.getX() + 26, r.getY() + 8, 200, 14, juce::Justification::centredLeft);
        };
        card (droneCard, "DRONE", env); card (timbreCard, "TIMBRE", accent); card (spaceCard, "REVERB", reverbCol);
        auto sub = [&] (int x, const juce::String& t, juce::Colour c)
        {
            g.setColour (cardBorder); g.drawVerticalLine (x, (float) spaceCard.getY() + 28.0f, (float) spaceCard.getBottom() - 12.0f);
            g.setColour (c); g.fillEllipse ((float) x + 8.0f, (float) spaceCard.getY() + 11.0f, 7.0f, 7.0f);
            g.setColour (text.withAlpha (0.85f)); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (t, x + 20, spaceCard.getY() + 8, 60, 14, juce::Justification::centredLeft);
        };
        sub (stereoDivider, "STEREO", accent); sub (outDivider, "OUT", accent2);

        // chord name
        const int ci = juce::jlimit (0, (int) chords().size() - 1, juce::roundToInt (proc.apvts.getRawParameterValue ("x")->load() * (float) (chords().size() - 1)));
        g.setColour (accent); g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (juce::String (chords()[(size_t) ci].name).toUpperCase(), timbreCard.getRight() - 170, timbreCard.getY() + 8, 158, 14, juce::Justification::centredRight);

        for (int b = 0; b < kNumBundles; ++b)
        {
            const auto r = bundleCard[b]; const auto col = bundle (b);
            g.setColour (SirenColours::card); g.fillRoundedRectangle (r.toFloat(), 10.0f);
            g.setColour (cardBorder); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
            // slot-style badge
            const auto badge = juce::Rectangle<float> ((float) r.getX() + 12.0f, (float) r.getY() + 7.0f, 22.0f, 22.0f);
            g.setColour (col); g.fillRoundedRectangle (badge, 5.0f);
            g.setColour (bgBottom); g.setFont (juce::FontOptions (13.0f, juce::Font::bold)); g.drawText (bundleName[b], badge.toNearestInt(), juce::Justification::centred);
            g.setColour (text.withAlpha (0.85f)); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (juce::String ("BUNDLE ") + bundleRange[b], r.getX() + 42, r.getY() + 11, 120, 14, juce::Justification::centredLeft);
            g.setColour (cardBorder); g.drawHorizontalLine (r.getY() + lfoY - 6, (float) r.getX() + 12.0f, (float) r.getRight() - 12.0f);
            g.setColour (textDim); g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText (b < 2 ? (b == 0 ? "FM FROM B" : "FM FROM C") : "FM SOURCE", r.getX() + 14, r.getY() + fmY, 100, 14, juce::Justification::centredLeft);
            g.drawText ("LFO", r.getX() + 14, r.getY() + lfoY - 1, 40, 12, juce::Justification::centredLeft);
            g.drawText ("RATE", r.getX() + 14, r.getY() + lfoY + 14, 40, 12, juce::Justification::centredLeft);
            g.drawText ("SHAPE", r.getX() + 14, r.getY() + lfoY + 44, 40, 12, juce::Justification::centredLeft);
            g.drawText ("DEPTH", r.getX() + 14, r.getY() + lfoY + 74, 40, 12, juce::Justification::centredLeft);
            g.drawText ("LFO GOES TO", r.getRight() - 12 - padSize, r.getY() + lfoY - 1, padSize, 12, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (margin);
        auto header = area.removeFromTop (headerH);
        auto meters = header.removeFromRight (310);
        swellBar.setBounds (meters.removeFromLeft (150).withSizeKeepingCentre (150, 18)); meters.removeFromLeft (10);
        grBar.setBounds (meters.withSizeKeepingCentre (meters.getWidth(), 18));
        drone.setBounds (margin + 250, header.getY() + (headerH - 28) / 2, 170, 28);

        // all three columns use the same widths in both rows
        const int colW = (area.getWidth() - 2 * gap) / 3;
        auto col = [&] (juce::Rectangle<int> row, int i) { return juce::Rectangle<int> (row.getX() + i * (colW + gap), row.getY(), i == 2 ? row.getWidth() - 2 * (colW + gap) : colW, row.getHeight()); };

        area.removeFromTop (gap);
        auto row1 = area.removeFromTop (132);
        droneCard = col (row1, 0); timbreCard = col (row1, 1); spaceCard = col (row1, 2);
        Knob::layoutRow (droneCard.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6), { &kRoot, &kAttack, &kRelease, &kStagger, &kGlide });
        Knob::layoutRow (timbreCard.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6), { &kChord, &kSpread, &kTimbre, &kIndex, &kAnimate });
        auto sp = spaceCard.reduced (6, 0).withTrimmedTop (26).withTrimmedBottom (6);
        const int unit = sp.getWidth() / 7;
        auto spReverb = sp.removeFromLeft (unit * 3); stereoDivider = spReverb.getRight() + 1;
        auto spStereo = sp.removeFromLeft (unit * 2); outDivider = spStereo.getRight() + 1;
        Knob::layoutRow (spReverb, { &kMix, &kDecay, &kTone }, 15);
        Knob::layoutRow (spStereo.withTrimmedLeft (3), { &kWidth, &kMove }, 15);
        Knob::layoutRow (sp.withTrimmedLeft (3), { &kLimit, &kLevel }, 15);

        area.removeFromTop (gap);
        auto row2 = area.removeFromTop (262);
        for (int b = 0; b < kNumBundles; ++b)
        {
            bundleCard[b] = col (row2, b);
            const auto r = bundleCard[b]; auto c = r.reduced (12, 0);
            type[(size_t) b]->setBounds (c.getRight() - 96, r.getY() + 8, 96, 20);
            Knob::layoutRow (c.withTrimmedTop (32).withHeight (88), { &kCut[(size_t) b], &kRes[(size_t) b], &kLvl[(size_t) b], &kDet[(size_t) b] }, 15);
            if (b < 2) fm[(size_t) b].setBounds (c.getRight() - 150, r.getY() + fmY, 150, 14);
            const int ly = r.getY() + lfoY;
            const int lw = c.getWidth() - padSize - 12;
            rate[(size_t) b].setBounds (c.getX() + 40, ly + 12, lw - 40, 15);
            shape[(size_t) b].setBounds (c.getX() + 40, ly + 40, lw - 40, 20);
            depth[(size_t) b].setBounds (c.getX() + 40, ly + 72, lw - 40, 15);
            pad[(size_t) b]->setBounds (c.getRight() - padSize, ly + 12, padSize, padSize - 12);
        }
    }

private:
    void timerCallback() override
    {
        swellBar.set (proc.envOut.load());
        const float gr = juce::jlimit (0.0f, 1.0f, -proc.grDb.load() / 12.0f);
        grBar.set (gr);
        const float old = (float) kLimit.slider.getProperties().getWithDefault ("grLight", 0.0);
        if (std::abs (gr - old) > 0.005f) { kLimit.slider.getProperties().set ("grLight", gr); kLimit.slider.repaint(); }
        repaint();
    }

    static constexpr int fmY = 124, lfoY = 150, padSize = 112;
    SirenProcessor& proc;
    SirenLookAndFeel look;
    Knob kRoot, kAttack, kRelease, kStagger, kGlide, kChord, kSpread, kTimbre, kIndex, kAnimate, kMix, kDecay, kTone, kWidth, kMove, kLimit, kLevel;
    std::array<Knob, kNumBundles> kCut, kRes, kLvl, kDet;
    std::array<std::unique_ptr<SegmentSwitch>, kNumBundles> type;
    std::array<juce::Slider, kNumBundles> rate, depth; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kNumBundles> rateAtt, depthAtt;
    std::array<juce::ComboBox, kNumBundles> shape; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, kNumBundles> shapeAtt;
    std::array<std::unique_ptr<XYPad>, kNumBundles> pad;
    std::array<juce::Slider, 2> fm; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 2> fmAtt;
    juce::ToggleButton drone { "START DRONE" }; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> droneAtt;
    LevelBar swellBar, grBar;
    juce::Rectangle<int> droneCard, timbreCard, spaceCard; std::array<juce::Rectangle<int>, kNumBundles> bundleCard;
    int outDivider = 0, stereoDivider = 0;
};

//==============================================================================
SirenEditor::SirenEditor (SirenProcessor& p) : AudioProcessorEditor (&p), panel (std::make_unique<Panel> (p))
{
    addAndMakeVisible (*panel);
    setResizable (true, true);
    constexpr int openW = 1180, openH = (int) (openW * (double) H / (double) W + 0.5);   // same opening width as Quad Shooter
    setResizeLimits ((int) (openW * 0.7), (int) (openH * 0.7), openW * 2, openH * 2);
    getConstrainer()->setFixedAspectRatio ((double) W / (double) H);
    setSize (openW, openH);   // always opens at this size, like the other plugins
}
void SirenEditor::paint (juce::Graphics& g) { g.fillAll (SirenColours::bgBottom); }
void SirenEditor::resized()
{
    panel->setBounds (0, 0, W, H);
    panel->setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) W));
}
