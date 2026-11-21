#include "PluginEditor.h"
#include "Colours.h"
#include "Knob.h"
#include "Look.h"
#include "Widgets.h"

using namespace SirenColours;
using namespace siren;

namespace
{
    constexpr int W = 960, H = 632, margin = 14, gap = 12, headerH = 44;
    const juce::Colour bundleColour[3] = { juce::Colour (0xff5ce1e6), juce::Colour (0xff4fa3ff), juce::Colour (0xffa78bfa) };
    const char* bundleName[3] = { "A", "B", "C" };
    const char* bundleRange[3] = { "LOW", "MID", "HIGH" };
    const juce::Colour posCol { 0xff5ce1e6 }, negCol { 0xffa78bfa };
}

//==============================================================================
// Matrix
juce::Rectangle<float> MatrixComponent::cellRect (int l, int c) const
{
    const float cw = ((float) getWidth() - 10.0f) / (float) kMatrixCols, ch = (float) getHeight() / (float) kNumLfo;
    return { c < kNumOsc ? (float) c * cw : (float) c * cw + 10.0f, (float) l * ch, cw - 2.0f, ch - 2.0f };
}
bool MatrixComponent::hit (juce::Point<int> p, int& l, int& c) const
{
    for (l = 0; l < kNumLfo; ++l) for (c = 0; c < kMatrixCols; ++c) if (cellRect (l, c).expanded (1.0f).contains (p.toFloat())) return true;
    return false;
}
void MatrixComponent::paint (juce::Graphics& g)
{
    for (int l = 0; l < kNumLfo; ++l)
        for (int c = 0; c < kMatrixCols; ++c)
        {
            const auto r = cellRect (l, c);
            const float v = apvts.getRawParameterValue (SirenProcessor::matrixId (l, c))->load();
            g.setColour (inset); g.fillRoundedRectangle (r, 3.0f);
            const bool hot = l == hl && c == hc;
            g.setColour (hot ? textDim : cardBorder.withAlpha (0.8f)); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            if (std::abs (v) > 0.01f)
            {
                const float half = r.getHeight() * 0.5f - 1.5f, h = half * std::abs (v);
                const auto col = v > 0 ? posCol : negCol;
                const auto bar = juce::Rectangle<float> (r.getX() + 2.5f, v > 0 ? r.getCentreY() - h : r.getCentreY(), r.getWidth() - 5.0f, h);
                g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (bar.expanded (0.0f, 1.0f), 2.0f);
                g.setColour (col); g.fillRoundedRectangle (bar, 2.0f);
            }
            g.setColour (text.withAlpha (0.18f)); g.fillRect (r.getX() + 2.0f, r.getCentreY() - 0.5f, r.getWidth() - 4.0f, 1.0f);
        }
}
void MatrixComponent::mouseMove (const juce::MouseEvent& e) { int l, c; if (! hit (e.getPosition(), l, c)) l = c = -1; if (l != hl || c != hc) { hl = l; hc = c; repaint(); } }
void MatrixComponent::mouseDown (const juce::MouseEvent& e)
{
    if (hit (e.getPosition(), dl, dc)) { startValue = param (dl, dc)->convertFrom0to1 (param (dl, dc)->getValue()); param (dl, dc)->beginChangeGesture(); }
    else dl = dc = -1;
}
void MatrixComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dl < 0) return;
    const float v = juce::jlimit (-1.0f, 1.0f, startValue - (float) e.getDistanceFromDragStartY() / 45.0f);
    param (dl, dc)->setValueNotifyingHost (param (dl, dc)->convertTo0to1 (v)); repaint();
}
void MatrixComponent::mouseUp (const juce::MouseEvent&) { if (dl >= 0) param (dl, dc)->endChangeGesture(); dl = dc = -1; }
void MatrixComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    int l, c; if (! hit (e.getPosition(), l, c)) return;
    param (l, c)->beginChangeGesture(); param (l, c)->setValueNotifyingHost (param (l, c)->convertTo0to1 (0.0f)); param (l, c)->endChangeGesture(); repaint();
}
void MatrixComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int l, c; if (! hit (e.getPosition(), l, c)) return;
    const float v = juce::jlimit (-1.0f, 1.0f, param (l, c)->convertFrom0to1 (param (l, c)->getValue()) + w.deltaY * 0.5f);
    param (l, c)->setValueNotifyingHost (param (l, c)->convertTo0to1 (v)); repaint();
}

//==============================================================================
class SirenEditor::Panel : public juce::Component, private juce::Timer
{
public:
    explicit Panel (SirenProcessor& p) : proc (p), matrix (p.apvts), swellBar ("SWELL", ok), grBar ("LIMITER", clip, true)
    {
        setLookAndFeel (&look);
        auto& a = p.apvts;
        // Drone
        kRoot.setup (*this, a, "root", "Root", accent); kAttack.setup (*this, a, "attack", "Attack", accent); kRelease.setup (*this, a, "release", "Release", accent);
        kStagger.setup (*this, a, "stagger", "Stagger", accent); kGlide.setup (*this, a, "glide", "Glide", accent);
        // Timbre
        kChord.setup (*this, a, "x", "Chord", accent2); kSpread.setup (*this, a, "y", "Spread", accent2); kTimbre.setup (*this, a, "z", "Timbre", accent2);
        kIndex.setup (*this, a, "index", "Index", accent2); kAnimate.setup (*this, a, "animate", "Animate", accent2);
        // Space + out
        const juce::Colour blue (0xff4fa3ff);
        kMix.setup (*this, a, "rvMix", "Mix", blue, true); kDecay.setup (*this, a, "rvDecay", "Decay", blue, true);
        kShimmer.setup (*this, a, "rvShimmer", "Shimmer", blue, true); kTone.setup (*this, a, "rvTone", "Tone", blue, true);
        kLevel.setup (*this, a, "level", "Level", accent, true); kCeiling.setup (*this, a, "ceiling", "Ceiling", accent, true);
        // Bundles
        for (int b = 0; b < kNumBundles; ++b)
        {
            const juce::String s (b);
            kCut[(size_t) b].setup (*this, a, "cut" + s, "Cutoff", bundleColour[b], true);
            kRes[(size_t) b].setup (*this, a, "res" + s, "Reso", bundleColour[b], true);
            kLvl[(size_t) b].setup (*this, a, "lvl" + s, "Level", bundleColour[b], true);
            kLvl[(size_t) b].slider.getProperties().set ("bipolar", false);
            type[(size_t) b] = std::make_unique<SegmentSwitch> (*a.getParameter ("type" + s), juce::StringArray { "LP", "BP", "HP" }, bundleColour[b]);
            addAndMakeVisible (*type[(size_t) b]);
            for (int k = 0; k < kOscPerBundle; ++k)
            {
                const int i = b * kOscPerBundle + k; auto& sl = det[(size_t) i];
                sl.setSliderStyle (juce::Slider::LinearVertical); sl.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                sl.getProperties().set ("accent", bundleColour[b].toString()); sl.getProperties().set ("detune", true);
                sl.setDoubleClickReturnValue (true, 0.0); addAndMakeVisible (sl);
                detAtt[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "det" + juce::String (i), sl);
            }
        }
        // LFOs
        for (int l = 0; l < kNumLfo; ++l)
        {
            auto& r = rate[(size_t) l]; r.setSliderStyle (juce::Slider::LinearBar); r.setTextBoxStyle (juce::Slider::TextBoxLeft, true, 90, 15);
            r.getProperties().set ("accent", accent.withAlpha (0.55f).toString()); r.setColour (juce::Slider::textBoxTextColourId, text);
            r.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); r.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack); addAndMakeVisible (r);
            rateAtt[(size_t) l] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "rate" + juce::String (l), r);
            auto& c = shape[(size_t) l]; c.addItemList ({ "Sine", "Tri", "Ramp", "Rand" }, 1); c.getProperties().set ("accent", accent.toString()); addAndMakeVisible (c);
            shapeAtt[(size_t) l] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "shape" + juce::String (l), c);
        }
        for (auto* d : { &pitchDepth, &filterDepth })
        { d->setSliderStyle (juce::Slider::LinearBar); d->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 90, 14); d->setColour (juce::Slider::textBoxTextColourId, text); d->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); d->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack); addAndMakeVisible (*d); }
        pitchDepth.getProperties().set ("accent", accent.withAlpha (0.55f).toString()); filterDepth.getProperties().set ("accent", accent.withAlpha (0.55f).toString());
        pdAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "pitchDepth", pitchDepth);
        fdAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "filterDepth", filterDepth);
        addAndMakeVisible (matrix); addAndMakeVisible (swellBar); addAndMakeVisible (grBar);
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
            g.setColour (c); g.fillRoundedRectangle ((float) r.getX() + 12.0f, (float) r.getY() + 10.0f, 3.0f, 10.0f, 1.5f);
            g.setColour (textDim); g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.drawText (title, r.getX() + 21, r.getY() + 8, 200, 14, juce::Justification::centredLeft);
        };
        card (droneCard, "DRONE", accent); card (timbreCard, "TIMBRE", accent2); card (spaceCard, "SHIMMER REVERB", juce::Colour (0xff4fa3ff));
        g.setColour (cardBorder); g.drawVerticalLine (outDivider, (float) spaceCard.getY() + 28.0f, (float) spaceCard.getBottom() - 12.0f);
        g.setColour (textDim); g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("OUT", outDivider + 10, spaceCard.getY() + 8, 60, 14, juce::Justification::centredLeft);

        // chord name
        const int ci = juce::jlimit (0, (int) chords().size() - 1, juce::roundToInt (proc.apvts.getRawParameterValue ("x")->load() * (float) (chords().size() - 1)));
        g.setColour (accent2); g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (juce::String (chords()[(size_t) ci].name).toUpperCase(), timbreCard.getRight() - 170, timbreCard.getY() + 8, 158, 14, juce::Justification::centredRight);

        for (int b = 0; b < kNumBundles; ++b)
        {
            const auto r = bundleCard[b]; card (r, juce::String ("BUNDLE ") + bundleName[b] + "  " + bundleRange[b], bundleColour[b]);
            g.setColour (textDim); g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText ("DETUNE  +-50 ct", r.getX() + 14, detuneY - 14, 150, 12, juce::Justification::centredLeft);
            for (int k = 0; k < kOscPerBundle; ++k)
                g.drawText (juce::String (b * kOscPerBundle + k + 1), detuneX0 + b * 0 + r.getX() - bundleCard[0].getX() + k * detuneStep - 4, detuneY + detuneH + 1, detuneStep, 11, juce::Justification::centredLeft);
        }

        card (matrixCard, "LFO MATRIX", accent);
        g.setColour (textDim); g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String::fromUTF8 ("drag cells  \xc2\xb7  double-click = 0"), matrixCard.getX() + 110, matrixCard.getY() + 8, 220, 14, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("PITCH DEPTH", pitchDepth.getX() - 82, pitchDepth.getY() - 1, 78, 14, juce::Justification::centredRight);
        g.drawText ("FILTER DEPTH", filterDepth.getX() - 82, filterDepth.getY() - 1, 78, 14, juce::Justification::centredRight);
        const auto mb = matrix.getBounds();
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        for (int c = 0; c < kMatrixCols; ++c)
        {
            const auto cr = matrix.cellRect (0, c);
            g.setColour (c < kNumOsc ? bundleColour[c / kOscPerBundle].withAlpha (0.9f) : text);
            g.drawText (c < kNumOsc ? juce::String (c + 1) : juce::String ("F") + "ABC"[c - kNumOsc], mb.getX() + (int) cr.getX() - 3, mb.getY() - 14, (int) cr.getWidth() + 6, 12, juce::Justification::centred);
        }
        for (int l = 0; l < kNumLfo; ++l)      // LFO activity dot
        {
            const float v = proc.lfoOut[(size_t) l].load() * 0.5f + 0.5f, ry = (float) mb.getY() + ((float) l + 0.5f) * (float) mb.getHeight() / kNumLfo;
            g.setColour (accent.withAlpha (0.15f + 0.85f * v)); g.fillEllipse ((float) matrixCard.getX() + 20.0f, ry - 4.0f, 8.0f, 8.0f);
            g.setColour (textDim); g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            g.drawText (juce::String (l + 1), matrixCard.getX() + 32, (int) ry - 7, 14, 14, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (margin);
        auto header = area.removeFromTop (headerH);
        auto meters = header.removeFromRight (310);
        swellBar.setBounds (meters.removeFromLeft (150).withSizeKeepingCentre (150, 18)); meters.removeFromLeft (10);
        grBar.setBounds (meters.withSizeKeepingCentre (meters.getWidth(), 18));
        drone.setBounds (header.withTrimmedLeft (250).withSizeKeepingCentre (170, 28).withX (margin + 250));

        area.removeFromTop (gap);
        auto row1 = area.removeFromTop (132);
        const int w1 = 290, w3 = row1.getWidth() - 2 * w1 - 2 * gap;
        droneCard = row1.removeFromLeft (w1); row1.removeFromLeft (gap); timbreCard = row1.removeFromLeft (w1); row1.removeFromLeft (gap); spaceCard = row1;
        (void) w3;
        Knob::layoutRow (droneCard.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6), { &kRoot, &kAttack, &kRelease, &kStagger, &kGlide });
        Knob::layoutRow (timbreCard.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6), { &kChord, &kSpread, &kTimbre, &kIndex, &kAnimate });
        auto sp = spaceCard.reduced (6, 0).withTrimmedTop (26).withTrimmedBottom (6);
        auto spReverb = sp.removeFromLeft (sp.getWidth() * 62 / 100); outDivider = spReverb.getRight() + 2;
        Knob::layoutRow (spReverb, { &kMix, &kDecay, &kShimmer, &kTone }, 15);
        Knob::layoutRow (sp.withTrimmedLeft (4), { &kLevel, &kCeiling }, 15);

        area.removeFromTop (gap);
        auto row2 = area.removeFromTop (206);
        const int bw = (row2.getWidth() - 2 * gap) / 3;
        for (int b = 0; b < kNumBundles; ++b)
        {
            bundleCard[b] = row2.removeFromLeft (bw); row2.removeFromLeft (gap);
            auto c = bundleCard[b].reduced (12, 0);
            type[(size_t) b]->setBounds (c.getRight() - 96, bundleCard[b].getY() + 6, 96, 20);
            auto kr = c.withTrimmedTop (30).withHeight (86);
            Knob::layoutRow (kr, { &kCut[(size_t) b], &kRes[(size_t) b], &kLvl[(size_t) b] }, 15);
            detuneY = bundleCard[b].getY() + 142; detuneH = 48; detuneStep = (c.getWidth()) / kOscPerBundle; detuneX0 = bundleCard[0].getX() + 12;
            for (int k = 0; k < kOscPerBundle; ++k)
                det[(size_t) (b * kOscPerBundle + k)].setBounds (c.getX() + k * detuneStep, detuneY, detuneStep - 6, detuneH);
        }

        area.removeFromTop (gap);
        matrixCard = area;
        const int gx = matrixCard.getX() + 16 + 176, gy = matrixCard.getY() + 40;
        matrix.setBounds (gx, gy, matrixCard.getRight() - 16 - gx, 17 * kNumLfo);
        for (int l = 0; l < kNumLfo; ++l)
        {
            const int y = gy + l * 17 + 1;
            rate[(size_t) l].setBounds (matrixCard.getX() + 48, y, 86, 15); shape[(size_t) l].setBounds (matrixCard.getX() + 140, y, 54, 15);
        }
        pitchDepth.setBounds (matrixCard.getRight() - 16 - 100 - 82 - 100 - 82 + 82, matrixCard.getY() + 7, 100, 14);
        filterDepth.setBounds (matrixCard.getRight() - 16 - 100, matrixCard.getY() + 7, 100, 14);
    }

private:
    void timerCallback() override
    {
        swellBar.set (proc.envOut.load());
        grBar.set (juce::jlimit (0.0f, 1.0f, -proc.grDb.load() / 12.0f));
        const float gr = juce::jlimit (0.0f, 1.0f, -proc.grDb.load() / 12.0f);
        const float old = (float) kCeiling.slider.getProperties().getWithDefault ("grLight", 0.0);
        if (std::abs (gr - old) > 0.005f) { kCeiling.slider.getProperties().set ("grLight", gr); kCeiling.slider.repaint(); }
        repaint();
    }

    SirenProcessor& proc;
    SirenLookAndFeel look;
    Knob kRoot, kAttack, kRelease, kStagger, kGlide, kChord, kSpread, kTimbre, kIndex, kAnimate, kMix, kDecay, kShimmer, kTone, kLevel, kCeiling;
    std::array<Knob, kNumBundles> kCut, kRes, kLvl;
    std::array<std::unique_ptr<SegmentSwitch>, kNumBundles> type;
    std::array<juce::Slider, kNumOsc> det; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kNumOsc> detAtt;
    std::array<juce::Slider, kNumLfo> rate; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kNumLfo> rateAtt;
    std::array<juce::ComboBox, kNumLfo> shape; std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, kNumLfo> shapeAtt;
    juce::Slider pitchDepth, filterDepth; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pdAtt, fdAtt;
    juce::ToggleButton drone { "START DRONE" }; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> droneAtt;
    MatrixComponent matrix; LevelBar swellBar, grBar;
    juce::Rectangle<int> droneCard, timbreCard, spaceCard, matrixCard; std::array<juce::Rectangle<int>, kNumBundles> bundleCard;
    int outDivider = 0, detuneY = 0, detuneH = 0, detuneStep = 0, detuneX0 = 0;
};

//==============================================================================
SirenEditor::SirenEditor (SirenProcessor& p) : AudioProcessorEditor (&p), panel (std::make_unique<Panel> (p))
{
    addAndMakeVisible (*panel);
    setResizable (true, true);
    setResizeLimits ((int) (W * 0.7), (int) (H * 0.7), W * 2, H * 2);
    getConstrainer()->setFixedAspectRatio ((double) W / (double) H);
    setSize (W, H);   // always opens at 100 %, like the other plugins
}
void SirenEditor::paint (juce::Graphics& g) { g.fillAll (SirenColours::bgBottom); }
void SirenEditor::resized()
{
    panel->setBounds (0, 0, W, H);
    panel->setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) W));
}
