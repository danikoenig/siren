#include "PluginEditor.h"
using namespace juce;
using namespace siren;
static const Colour bg(0xff121214), panel(0xff1b1b1f), accent(0xffe8452c), cool(0xff3d8bfd), dim(0xff8a8a92);
static constexpr int kW = 1000, kH = 710;

// ------------------------------------------------------------------ matrix
Rectangle<float> MatrixComponent::cellRect(int l, int c) const
{
    const float cw = (getWidth() - 10.0f) / (kMatrixCols), ch = getHeight() / (float) kNumLfo;
    return { (c < gapAfter ? c * cw : c * cw + 10.0f), l * ch, cw - 1.5f, ch - 1.5f };
}
bool MatrixComponent::hit(Point<int> p, int& l, int& c) const
{
    for (l = 0; l < kNumLfo; ++l) for (c = 0; c < kMatrixCols; ++c) if (cellRect(l, c).contains(p.toFloat())) return true;
    return false;
}
void MatrixComponent::paint(Graphics& g)
{
    for (int l = 0; l < kNumLfo; ++l) for (int c = 0; c < kMatrixCols; ++c)
    {
        const auto r = cellRect(l, c); const float v = apvts.getRawParameterValue(SirenProcessor::matrixId(l, c))->load();
        g.setColour(bg); g.fillRoundedRectangle(r, 3.0f);
        if (std::abs(v) > 0.01f)
        {
            g.setColour((v > 0 ? accent : cool).withAlpha(0.25f + 0.75f * std::abs(v)));
            const float h = r.getHeight() * std::abs(v); g.fillRoundedRectangle(r.withHeight(h).withBottomY(v > 0 ? r.getBottom() : r.getBottom()), 3.0f);
        }
    }
}
void MatrixComponent::mouseDown(const MouseEvent& e)
{
    if (hit(e.getPosition(), dl, dc)) { startValue = param(dl, dc)->convertFrom0to1(param(dl, dc)->getValue()); param(dl, dc)->beginChangeGesture(); }
    else dl = dc = -1;
}
void MatrixComponent::mouseDrag(const MouseEvent& e)
{
    if (dl < 0) return;
    const float v = jlimit(-1.0f, 1.0f, startValue - e.getDistanceFromDragStartY() / 50.0f);
    param(dl, dc)->setValueNotifyingHost(param(dl, dc)->convertTo0to1(v)); repaint();
}
void MatrixComponent::mouseUp(const MouseEvent&) { if (dl >= 0) param(dl, dc)->endChangeGesture(); dl = dc = -1; }
void MatrixComponent::mouseDoubleClick(const MouseEvent& e)
{
    int l, c; if (!hit(e.getPosition(), l, c)) return;
    param(l, c)->beginChangeGesture(); param(l, c)->setValueNotifyingHost(param(l, c)->convertTo0to1(0.0f)); param(l, c)->endChangeGesture(); repaint();
}
void MatrixComponent::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& w)
{
    int l, c; if (!hit(e.getPosition(), l, c)) return;
    const float v = jlimit(-1.0f, 1.0f, param(l, c)->convertFrom0to1(param(l, c)->getValue()) + w.deltaY * 0.5f);
    param(l, c)->setValueNotifyingHost(param(l, c)->convertTo0to1(v)); repaint();
}

// ----------------------------------------------------------------- content
class SirenEditor::Content : public Component
{
public:
    explicit Content(SirenProcessor& p) : proc(p), matrix(p.apvts)
    {
        auto& a = p.apvts;
        addKnob("root", "Root", 0, 54, 0, 0, true); addKnob("attack", "Attack", 0, 54, 0, 0, true); addKnob("release", "Release", 0, 54, 0, 0, true);
        addKnob("stagger", "Stagger", 0, 54, 0, 0, true); addKnob("glide", "Glide", 0, 54, 0, 0, true);
        addKnob("x", "Chord", 0, 0, 0, 0, true); addKnob("y", "Spread", 0, 0, 0, 0, true); addKnob("z", "Timbre", 0, 0, 0, 0, true);
        addKnob("index", "Index", 0, 0, 0, 0, true); addKnob("animate", "Animate", 0, 0, 0, 0, true);
        addKnob("rvMix", "Mix", 0, 0, 0, 0, true); addKnob("rvDecay", "Decay", 0, 0, 0, 0, true); addKnob("rvShimmer", "Shimmer", 0, 0, 0, 0, true); addKnob("rvTone", "Tone", 0, 0, 0, 0, true);
        addKnob("level", "Level", 0, 0, 0, 0, true); addKnob("ceiling", "Ceiling", 0, 0, 0, 0, true);
        addKnob("pitchDepth", "Pitch LFO", 0, 0, 0, 0, false); addKnob("filterDepth", "Filter LFO", 0, 0, 0, 0, false);
        for (int b = 0; b < kNumBundles; ++b)
        {
            const String s(b);
            addKnob("cut" + s, "Cutoff", 0, 0, 0, 0, true); addKnob("res" + s, "Reso", 0, 0, 0, 0, true); addKnob("lvl" + s, "Level", 0, 0, 0, 0, true);
            type[(size_t) b].addItemList({ "Lowpass", "Bandpass", "Highpass" }, 1); addAndMakeVisible(type[(size_t) b]);
            typeAtt[(size_t) b] = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(a, "type" + s, type[(size_t) b]);
            for (int k = 0; k < kOscPerBundle; ++k)
            {
                auto& sl = det[(size_t) (b * kOscPerBundle + k)]; sl.setSliderStyle(Slider::LinearVertical); sl.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
                sl.setColour(Slider::trackColourId, accent); sl.setColour(Slider::thumbColourId, Colours::white); sl.setDoubleClickReturnValue(true, 0.0);
                addAndMakeVisible(sl); detAtt[(size_t) (b * kOscPerBundle + k)] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(a, "det" + String(b * kOscPerBundle + k), sl);
            }
        }
        for (int l = 0; l < kNumLfo; ++l)
        {
            auto& r = rate[(size_t) l]; r.setSliderStyle(Slider::LinearHorizontal); r.setTextBoxStyle(Slider::TextBoxRight, false, 50, 14);
            r.setColour(Slider::trackColourId, accent); r.setColour(Slider::textBoxTextColourId, dim); r.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack); addAndMakeVisible(r);
            rateAtt[(size_t) l] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(a, "rate" + String(l), r);
            shape[(size_t) l].addItemList({ "Sine", "Tri", "Ramp", "Rand" }, 1); addAndMakeVisible(shape[(size_t) l]);
            shapeAtt[(size_t) l] = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(a, "shape" + String(l), shape[(size_t) l]);
        }
        addAndMakeVisible(matrix);
        drone.setClickingTogglesState(true); drone.setColour(TextButton::buttonOnColourId, accent); drone.setColour(TextButton::buttonColourId, bg); addAndMakeVisible(drone);
        droneAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(a, "gate", drone);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(bg);
        auto sect = [&](Rectangle<int> r, const String& t) { g.setColour(panel); g.fillRoundedRectangle(r.toFloat(), 8); g.setColour(dim); g.setFont(FontOptions(11.0f, Font::bold)); g.drawText(t.toUpperCase(), r.getX() + 10, r.getY() + 4, 560, 14, Justification::left); };
        g.setColour(Colours::white); g.setFont(FontOptions(26.0f, Font::bold)); g.drawText("SIREN", 14, 6, 140, 32, Justification::left);
        g.setColour(dim); g.setFont(FontOptions(11.0f)); g.drawText("drone generator  |  21 oscillators  |  just intonation", 110, 16, 420, 16, Justification::left);
        sect({ 10, 42, 330, 150 }, "Drone"); sect({ 350, 42, 330, 150 }, "Timbre"); sect({ 690, 42, 300, 150 }, "Space and output");
        for (int b = 0; b < kNumBundles; ++b) sect({ 10 + b * 330, 200, 320, 250 }, String("Bundle ") + "ABC"[b] + (b == 0 ? " (low)" : b == 1 ? " (mid)" : " (high)"));
        sect({ 10, 458, 980, 244 }, "LFO matrix  -  drag cells (red = +, blue = -), double-click = 0");
        // swell meter + limiter GR
        g.setColour(bg); g.fillRoundedRectangle(20.0f, 183.0f, 310.0f, 5.0f, 2); g.setColour(accent); g.fillRoundedRectangle(20.0f, 183.0f, 310.0f * proc.envOut.load(), 5.0f, 2);
        g.setColour(bg); g.fillRoundedRectangle(700.0f, 183.0f, 280.0f, 5.0f, 2); g.setColour(cool);
        const float gr = jlimit(0.0f, 1.0f, -proc.grDb.load() / 12.0f); g.fillRoundedRectangle(980.0f - 280.0f * gr, 183.0f, 280.0f * gr, 5.0f, 2);
        g.setColour(dim); g.setFont(FontOptions(9.0f)); g.drawText("limiter gain reduction", 700, 170, 140, 12, Justification::left); g.drawText("swell (envelope)", 182, 160, 140, 12, Justification::left);
        for (int l = 0; l < kNumLfo; ++l)       // LFO activity next to each row
        {
            const auto r = matrix.getBounds().toFloat(); const float ch = r.getHeight() / kNumLfo, v = proc.lfoOut[(size_t) l].load() * 0.5f + 0.5f;
            g.setColour(bg); g.fillRoundedRectangle(20.0f, r.getY() + l * ch + 4, 38.0f, ch - 9, 2); g.setColour(accent); g.fillRect(20.0f + 38.0f * v - 1.5f, r.getY() + l * ch + 4, 3.0f, ch - 9);
            g.setColour(dim); g.setFont(FontOptions(10.0f)); g.drawText("LFO " + String(l + 1), 62, (int) (r.getY() + l * ch), 40, (int) ch, Justification::left);
        }
        g.setColour(dim); g.setFont(FontOptions(9.0f));
        for (int b = 0; b < kNumBundles; ++b) { g.drawText("DETUNE +-50 ct  (oscillators " + String(b * 7 + 1) + "-" + String(b * 7 + 7) + ")", 22 + b * 330, 305, 300, 12, Justification::left); for (int k = 0; k < kOscPerBundle; ++k) g.drawText(String(b * 7 + k + 1), 10 + b * 330 + 14 + k * 43 - 8, 436, 44, 11, Justification::centred); }
        const auto mr = matrix.getBounds().toFloat();
        for (int c = 0; c < kMatrixCols; ++c) g.drawText(c < kNumOsc ? String(c + 1) : String("F") + "ABC"[c - kNumOsc], matrix.getX() + (int) matrix.cellRect(0, c).getX() - 2, (int) mr.getY() - 12, (int) matrix.cellRect(0, c).getWidth() + 4, 11, Justification::centred);
    }

    void resized() override
    {
        auto P = [&](const char* id, float cx, float cy, float d) { auto& k = knobs.at(id); const float w = std::max(d, 62.0f); k->s.setBounds((int) (cx - w / 2), (int) (cy - d / 2), (int) w, (int) d + 16); k->l.setBounds((int) (cx - d / 2 - 8), (int) (cy - d / 2 - 14), (int) d + 16, 13); };
        const char* d1[] = { "root", "attack", "release", "stagger", "glide" }; for (int i = 0; i < 5; ++i) P(d1[i], 45 + i * 62.0f, 108, 46);
        drone.setBounds(20, 152, 150, 22); 
        const char* d2[] = { "x", "y", "z", "index", "animate" }; for (int i = 0; i < 5; ++i) P(d2[i], 385 + i * 62.0f, 108, 46);
        const char* d3[] = { "rvMix", "rvDecay", "rvShimmer", "rvTone", "level", "ceiling" }; for (int i = 0; i < 6; ++i) P(d3[i], 712 + i * 50.0f, 108, 36);
        for (int b = 0; b < kNumBundles; ++b)
        {
            const String s(b); const int x0 = 10 + b * 330;
            type[(size_t) b].setBounds(x0 + 12, 222, 80, 22);
            P(("cut" + s).toRawUTF8(), x0 + 125, 262, 38); P(("res" + s).toRawUTF8(), x0 + 195, 262, 38); P(("lvl" + s).toRawUTF8(), x0 + 265, 262, 38);
            for (int k = 0; k < kOscPerBundle; ++k) det[(size_t) (b * kOscPerBundle + k)].setBounds(x0 + 14 + k * 43, 320, 28, 114);
        }
        P("pitchDepth", 0, 0, 0); P("filterDepth", 0, 0, 0);
        knobs.at("pitchDepth")->s.setBounds(640, 462, 150, 14); knobs.at("filterDepth")->s.setBounds(830, 462, 150, 14);
        knobs.at("pitchDepth")->l.setBounds(590, 460, 50, 14); knobs.at("filterDepth")->l.setBounds(780, 460, 50, 14);
        for (int l = 0; l < kNumLfo; ++l) { const int y = 492 + l * 25; rate[(size_t) l].setBounds(104, y, 128, 22); shape[(size_t) l].setBounds(234, y + 1, 64, 20); }
        matrix.setBounds(302, 492, 680, 200);
    }

private:
    struct K { Slider s; Label l; std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> a; };
    void addKnob(const String& id, const String& text, int, int, int, int, bool rotary)
    {
        auto k = std::make_unique<K>();
        if (rotary) { k->s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag); k->s.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 14); }
        else { k->s.setSliderStyle(Slider::LinearHorizontal); k->s.setTextBoxStyle(Slider::NoTextBox, false, 0, 0); }
        k->s.setColour(Slider::rotarySliderFillColourId, accent); k->s.setColour(Slider::trackColourId, accent); k->s.setColour(Slider::thumbColourId, Colours::white);
        k->s.setColour(Slider::textBoxTextColourId, dim); k->s.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        auto* pr = proc.apvts.getParameter(id); k->s.setDoubleClickReturnValue(true, pr->convertFrom0to1(pr->getDefaultValue()));
        k->l.setText(text, dontSendNotification); k->l.setJustificationType(rotary ? Justification::centred : Justification::right); k->l.setColour(Label::textColourId, rotary ? Colours::white : dim);
        k->l.setFont(FontOptions(rotary ? 11.0f : 10.0f, Font::bold));
        addAndMakeVisible(k->s); addAndMakeVisible(k->l);
        k->a = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, k->s);
        knobs[id.toStdString()] = std::move(k);
    }
    SirenProcessor& proc;
    std::map<std::string, std::unique_ptr<K>> knobs;
    std::array<ComboBox, kNumBundles> type; std::array<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>, kNumBundles> typeAtt;
    std::array<Slider, kNumOsc> det; std::array<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>, kNumOsc> detAtt;
    std::array<Slider, kNumLfo> rate; std::array<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>, kNumLfo> rateAtt;
    std::array<ComboBox, kNumLfo> shape; std::array<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>, kNumLfo> shapeAtt;
    TextButton drone { "START DRONE" }; std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> droneAtt;
public:
    MatrixComponent matrix;
};

SirenEditor::SirenEditor(SirenProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    content = std::make_unique<Content>(p); addAndMakeVisible(*content);
    setResizable(true, true); setResizeLimits(700, 497, 1500, 1065); getConstrainer()->setFixedAspectRatio((double) kW / kH);
    setSize(kW, kH); startTimerHz(20);
}
void SirenEditor::resized() { content->setBounds(0, 0, kW, kH); content->setTransform(AffineTransform::scale(getWidth() / (float) kW)); }
void SirenEditor::timerCallback() { content->repaint(); }
