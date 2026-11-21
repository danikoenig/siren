#include "PluginEditor.h"
using namespace juce;
static const Colour bg(0xff121214), panel(0xff1b1b1f), accent(0xffe8452c), dim(0xff8a8a92);

SirenEditor::SirenEditor(SirenProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    addKnob(x, "x", "X", true); addKnob(y, "y", "Y", true); addKnob(z, "z", "Z", true);
    addKnob(pitch, "pitch", "Pitch", true); addKnob(index, "index", "Index", true); addKnob(animate, "animate", "Animate", true);
    addKnob(xAtt, "xAtt", "X att", false); addKnob(yAtt, "yAtt", "Y att (LFO1)", false); addKnob(zAtt, "zAtt", "Z att (LFO2)", false);
    addKnob(lfoSpeed, "lfoSpeed", "LFO speed", false); addKnob(voct2, "voct2", "V/Oct 2", false); addKnob(pmod, "pmod", "PMod", false);
    addKnob(idxLfo, "idxLfo", "Idx<-LFO1", false); addKnob(level, "level", "Level", false);
    for (auto* b : { (Button*) &nearest, (Button*) &lock, (Button*) &midiReset, (Button*) &reset }) { addAndMakeVisible(b); b->setColour(ToggleButton::tickColourId, accent); }
    aN = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "nearest", nearest);
    aL = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "pitchLock", lock);
    aM = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "midiReset", midiReset);
    lock.onClick = [this] { pitch.s.setEnabled(!lock.getToggleState()); };
    pitch.s.setEnabled(!lock.getToggleState());
    // Reset is momentary: high while held (like the gate input on the module)
    auto* rp = p.apvts.getParameter("reset");
    reset.onStateChange = [this, rp] { rp->setValueNotifyingHost(reset.isDown() ? 1.0f : 0.0f); };
    setResizable(true, true); setResizeLimits(540, 315, 1440, 840); getConstrainer()->setFixedAspectRatio(720.0 / 420.0);
    setSize(720, 420); startTimerHz(30);
}

void SirenEditor::addKnob(Knob& k, const char* id, const char* text, bool big)
{
    k.s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag); k.s.setTextBoxStyle(Slider::TextBoxBelow, false, big ? 64 : 52, 14);
    k.s.setColour(Slider::rotarySliderFillColourId, accent); k.s.setColour(Slider::thumbColourId, Colours::white);
    k.s.setColour(Slider::textBoxTextColourId, dim); k.s.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.s.setDoubleClickReturnValue(true, proc.apvts.getParameter(id)->convertFrom0to1(proc.apvts.getParameter(id)->getDefaultValue()));
    k.l.setText(text, dontSendNotification); k.l.setJustificationType(Justification::centred); k.l.setColour(Label::textColourId, Colours::white);
    k.l.setFont(FontOptions(big ? 15.0f : 11.0f, Font::bold));
    addAndMakeVisible(k.s); addAndMakeVisible(k.l);
    k.a = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, k.s);
}

void SirenEditor::timerCallback()
{
    hist[0][(size_t) hp] = proc.lfo1.load(); hist[1][(size_t) hp] = proc.lfo2.load(); hp = (hp + 1) % 120; repaint(scope1.getUnion(scope2));
}

void SirenEditor::paint(Graphics& g)
{
    g.fillAll(bg);
    const float s = getWidth() / 720.0f; g.addTransform(AffineTransform::scale(s));
    g.setColour(panel); g.fillRoundedRectangle(10, 10, 700, 400, 10);
    g.setColour(Colours::white); g.setFont(FontOptions(26.0f, Font::bold)); g.drawText("SIREN", 24, 14, 200, 30, Justification::left);
    g.setColour(dim); g.setFont(FontOptions(11.0f)); g.drawText("multi-oscillator drone generator  |  21 osc  23 LFO  just intonation", 24, 42, 420, 14, Justification::left);
    auto drawScope = [&](Rectangle<int> r, int ch, const char* name) {
        g.setColour(bg); g.fillRoundedRectangle(r.toFloat(), 4); g.setColour(dim); g.setFont(FontOptions(10.0f)); g.drawText(name, r.withHeight(12).translated(4, 1), Justification::left);
        Path path; for (int i = 0; i < 120; ++i) { const float v = hist[(size_t) ch][(size_t) ((hp + i) % 120)];
            const float px = r.getX() + 4 + (r.getWidth() - 8) * i / 119.0f, py = r.getBottom() - 4 - (r.getHeight() - 18) * v; if (i == 0) path.startNewSubPath(px, py); else path.lineTo(px, py); }
        g.setColour(accent); g.strokePath(path, PathStrokeType(1.5f)); };
    drawScope(scope1.transformedBy(AffineTransform::scale(1.0f / s)), 0, "LFO1 out");
    drawScope(scope2.transformedBy(AffineTransform::scale(1.0f / s)), 1, "LFO2 out");
}

void SirenEditor::resized()
{
    const float s = getWidth() / 720.0f;
    auto place = [&](Knob& k, float cx, float cy, float d, float labelH) {
        k.s.setBounds(Rectangle<float>((cx - d / 2) * s, 0, d * s, (d + 18) * s).withY((cy - d / 2) * s).toNearestInt());
        k.l.setBounds(Rectangle<float>((cx - d / 2 - 10) * s, (cy - d / 2 - labelH - 2) * s, (d + 20) * s, labelH * s).toNearestInt());
    };
    place(x, 120, 140, 96, 18); place(y, 270, 140, 96, 18); place(z, 420, 140, 96, 18);
    place(pitch, 120, 290, 96, 18); place(index, 270, 290, 96, 18); place(animate, 420, 290, 96, 18);
    place(xAtt, 120, 215, 46, 14); place(yAtt, 270, 215, 46, 14); place(zAtt, 420, 215, 46, 14);
    place(lfoSpeed, 530, 120, 56, 14); place(voct2, 610, 120, 56, 14);
    place(idxLfo, 530, 215, 56, 14); place(level, 610, 215, 56, 14);
    place(pmod, 530, 305, 56, 14);
    auto r = [&](float xx, float yy, float w, float h) { return Rectangle<float>(xx * s, yy * s, w * s, h * s).toNearestInt(); };
    nearest.setBounds(r(580, 285, 110, 22)); lock.setBounds(r(580, 310, 110, 22)); reset.setBounds(r(580, 337, 110, 24)); midiReset.setBounds(r(480, 372, 200, 22));
    scope1 = r(24, 340, 120, 56); scope2 = r(156, 340, 120, 56);
}
