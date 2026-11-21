#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

float SirenProcessor::matrixDefault(int l, int c)
{
    if (c < siren::kNumOsc) return c % 8 == l ? 0.7f : ((c * 3 + 1) % 8 == l ? -0.5f : 0.0f);
    const int f = c - siren::kNumOsc;
    return l == (f * 2 + 1) % 8 ? 0.8f : (l == (f * 2 + 4) % 8 ? -0.4f : 0.0f);
}

static String abc(int i) { return String::charToString("ABC"[i]); }
static String noteName(int n) { static const char* nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return String(nm[n % 12]) + String(n / 12 - 2); }

AudioProcessorValueTreeState::ParameterLayout SirenProcessor::layout()
{
    using namespace siren;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [&](const String& id, const String& name, float lo, float hi, float def, float skew = 1.0f, const char* unit = "") {
        const String u(unit);
        p.push_back(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, NormalisableRange<float>(lo, hi, 0.0f, skew), def,
            AudioParameterFloatAttributes().withLabel(unit).withStringFromValueFunction([u](float v, int) {
                const float a = std::abs(v); return String(v, a >= 100 ? 0 : a >= 10 ? 1 : 2) + u; })));
    };
    auto flag = [&](const char* id, const char* name, bool def) { p.push_back(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def)); };
    auto choice = [&](const String& id, const String& name, StringArray items, int def) { p.push_back(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, items, def)); };

    flag("gate", "Drone", false);
    p.push_back(std::make_unique<AudioParameterInt>(ParameterID { "root", 1 }, "Root", 24, 84, 45, AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return noteName(v); })));
    knob("attack", "Attack", 0.05f, 120.0f, 12.0f, 0.35f, " s"); knob("release", "Release", 0.05f, 180.0f, 20.0f, 0.35f, " s");
    knob("stagger", "Stagger", 0, 1, 0.6f); knob("glide", "Glide", 0.01f, 10.0f, 0.5f, 0.4f, " s");
    knob("x", "Chord", 0, 1, 0.25f); knob("y", "Spread", 0, 1, 0.5f); knob("z", "Timbre", 0, 1, 0.4f); knob("index", "Index", 0, 1, 0.0f); knob("animate", "Animate", 0, 1, 0.3f);
    knob("pitchDepth", "Pitch LFO", 0, 100, 25, 1.0f, " ct"); knob("filterDepth", "Filter LFO", 0, 4, 2, 1.0f, " oct");
    for (int b = 0; b < kNumBundles; ++b)
    {
        const String s(b);
        choice("type" + s, "Filter " + abc(b) + " Type", { "Lowpass", "Bandpass", "Highpass" }, 0);
        knob("cut" + s, "Cutoff " + abc(b), 40, 18000, b == 0 ? 900.0f : b == 1 ? 2200.0f : 5000.0f, 0.3f, " Hz");
        knob("res" + s, "Resonance " + abc(b), 0, 1, 0.2f); knob("lvl" + s, "Level " + abc(b), -24, 6, 0, 1.0f, " dB");
    }
    for (int i = 0; i < kNumOsc; ++i) knob("det" + String(i), "Detune " + String(i + 1), -50, 50, 0, 1.0f, " ct");
    for (int l = 0; l < kNumLfo; ++l)
    {
        knob("rate" + String(l), "LFO " + String(l + 1) + " Rate", 0.005f, 2.0f, 0.02f * std::pow(1.5f, (float) l), 0.3f, " Hz");
        choice("shape" + String(l), "LFO " + String(l + 1) + " Shape", { "Sine", "Triangle", "Ramp", "Random" }, l % 2);
        for (int c = 0; c < kMatrixCols; ++c) knob(matrixId(l, c), "LFO " + String(l + 1) + " > " + (c < kNumOsc ? "Osc " + String(c + 1) : "Filter " + abc(c - kNumOsc)), -1, 1, matrixDefault(l, c));
    }
    knob("rvMix", "Reverb Mix", 0, 1, 0.35f); knob("rvDecay", "Reverb Decay", 0.5f, 30, 8, 0.5f, " s"); knob("rvShimmer", "Shimmer", 0, 1, 0.4f); knob("rvTone", "Reverb Tone", 0, 1, 0.6f);
    knob("level", "Level", -48, 6, -12, 1.0f, " dB"); knob("ceiling", "Limiter Ceiling", -12, 0, -1, 1.0f, " dB");
    return { p.begin(), p.end() };
}

SirenProcessor::SirenProcessor()
    : AudioProcessor(BusesProperties().withOutput("Out", AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SIREN", layout()) {}

bool SirenProcessor::isBusesLayoutSupported(const BusesLayout& l) const { return l.getMainOutputChannelSet() == AudioChannelSet::stereo(); }

void SirenProcessor::prepareToPlay(double sr, int) { engine.prepare(sr); setLatencySamples(engine.latencySamples()); }

void SirenProcessor::processBlock(AudioBuffer<float>& buf, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    using namespace siren;
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn()) { held.erase(std::remove(held.begin(), held.end(), msg.getNoteNumber()), held.end()); held.push_back(msg.getNoteNumber()); lastNote = msg.getNoteNumber(); }
        else if (msg.isNoteOff()) { held.erase(std::remove(held.begin(), held.end(), msg.getNoteNumber()), held.end()); if (!held.empty()) lastNote = held.back(); }
        else if (msg.isPitchWheel()) bend = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
        else if (msg.isController() && msg.getControllerNumber() == 64) sustain = msg.getControllerValue() >= 64;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { held.clear(); sustain = false; }
    }
    Params q;
    q.x = get("x"); q.y = get("y"); q.z = get("z"); q.index = get("index"); q.animate = get("animate");
    q.attack = get("attack"); q.release = get("release"); q.stagger = get("stagger"); q.glide = get("glide");
    const bool midiOn = !held.empty() || (sustain && lastNote >= 0);
    q.gate = midiOn || get("gate") > 0.5f;
    q.rootNote = (midiOn ? (float) lastNote : get("root")) + bend;
    for (int i = 0; i < kNumOsc; ++i) q.detune[(size_t) i] = get("det" + String(i));
    for (int l = 0; l < kNumLfo; ++l)
    {
        q.lfoRate[(size_t) l] = get("rate" + String(l)); q.lfoShape[(size_t) l] = (int) get("shape" + String(l));
        for (int c = 0; c < kMatrixCols; ++c) q.matrix[l][c] = get(matrixId(l, c));
    }
    q.pitchDepthCents = get("pitchDepth"); q.filterDepthOct = get("filterDepth");
    for (int b = 0; b < kNumBundles; ++b) { const String s(b); q.bundle[(size_t) b] = { (int) get("type" + s), get("cut" + s), get("res" + s), get("lvl" + s) }; }
    q.levelDb = get("level"); q.rvMix = get("rvMix"); q.rvDecay = get("rvDecay"); q.rvShimmer = get("rvShimmer"); q.rvTone = get("rvTone"); q.ceilingDb = get("ceiling");

    buf.clear();
    engine.process(q, buf.getWritePointer(0), buf.getWritePointer(1), buf.getNumSamples());
    for (int l = 0; l < kNumLfo; ++l) lfoOut[(size_t) l] = engine.getLfo(l);
    envOut = engine.getEnvelope(); grDb = engine.getReductionDb();
}

AudioProcessorEditor* SirenProcessor::createEditor() { return new SirenEditor(*this); }
void SirenProcessor::getStateInformation(MemoryBlock& d) { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
void SirenProcessor::setStateInformation(const void* d, int s) { if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(ValueTree::fromXml(*xml)); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SirenProcessor(); }
