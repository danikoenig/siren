#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

static String abc(int i) { return String::charToString("ABC"[i]); }
static String noteName(int n) { static const char* nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return String(nm[n % 12]) + String(n / 12 - 2); }

AudioProcessorValueTreeState::ParameterLayout SirenProcessor::layout()
{
    using namespace siren;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [&](const String& id, const String& name, float lo, float hi, float def, float skew = 1.0f, const char* unit = "") {
        const String u(unit);
        p.push_back(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, NormalisableRange<float>(lo, hi, 0.0f, skew), def,
            AudioParameterFloatAttributes().withLabel(unit).withStringFromValueFunction([u, id](float v, int) {
                if (id == "limit" || id == "level") return String(juce::roundToInt(v)) + u;
                const float a = std::abs(v); return String(v, a >= 100 ? 0 : a >= 10 ? 1 : 2) + u; })));
    };
    auto flag = [&](const char* id, const char* name, bool def) { p.push_back(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def)); };
    auto choice = [&](const String& id, const String& name, StringArray items, int def) { p.push_back(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, items, def)); };

    flag("gate", "Drone", false);
    p.push_back(std::make_unique<AudioParameterInt>(ParameterID { "root", 1 }, "Root", 24, 84, 36, AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return noteName(v); })));
    knob("attack", "Attack", 0.05f, 120.0f, 12.0f, 0.35f, " s"); knob("release", "Release", 0.05f, 180.0f, 6.0f, 0.35f, " s");
    knob("stagger", "Stagger", 0, 1, 0.6f); knob("glide", "Glide", 0.01f, 10.0f, 0.5f, 0.4f, " s");
    knob("x", "Chord", 0, 1, 0.0f); knob("y", "Spread", 0, 1, 0.4f); knob("z", "Timbre", 0, 1, 0.4f); knob("index", "Index", 0, 1, 0.0f); knob("animate", "Animate", 0, 1, 0.49f);
    for (int b = 0; b < kNumBundles; ++b)
    {
        const String s(b), n = abc(b);
        choice("type" + s, "Filter " + n + " Type", { "Lowpass", "Bandpass", "Highpass" }, 0);
        knob("cut" + s, "Cutoff " + n, 40, 18000, b == 0 ? 900.0f : b == 1 ? 2200.0f : 5000.0f, 0.3f, " Hz");
        knob("res" + s, "Resonance " + n, 0, 1, 0.2f); knob("lvl" + s, "Level " + n, -24, 6, 0, 1.0f, " dB");
        knob("bdet" + s, "Detune " + n, 0, 50, b == 1 ? 7.64f : 4.0f, 0.6f, " ct");
        knob("lr" + s, "LFO Rate " + n, 0.005f, 2.0f, 0.03f * (1.0f + 0.6f * (float) b), 0.3f, " Hz");
        choice("ls" + s, "LFO Shape " + n, { "Sine", "Tri", "Ramp", "Rand" }, 0);
        knob("ld" + s, "LFO Depth " + n, 0, 1, 0.5f);
        knob("lx" + s, "LFO Target X " + n, 0, 1, 0.0f); knob("ly" + s, "LFO Target Y " + n, 0, 1, 1.0f);
    }
    knob("fmA", "FM B to A", 0, 1, 0); knob("fmB", "FM C to B", 0, 1, 0);
    knob("rvMix", "Reverb Mix", 0, 1, 0.3f); knob("rvDecay", "Reverb Decay", 0.5f, 60, 8, 0.5f, " s"); knob("rvPre", "Pre-Delay", 0, 250, 20, 0.6f, " ms"); knob("rvTone", "Reverb Tone", -1, 1, 0);
    knob("level", "Level", -48, 6, -12, 1.0f, " dB"); knob("limit", "Limiter", -24, 0, 0, 1.0f, " dB"); knob("width", "Width", 0, 1, 0.35f); knob("move", "Move", 0, 1, 0.25f);
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
    for (int b = 0; b < kNumBundles; ++b)
    {
        const String s(b);
        q.bundle[(size_t) b].type = (int) get("type" + s); q.bundle[(size_t) b].cutoff = get("cut" + s); q.bundle[(size_t) b].res = get("res" + s);
        q.bundle[(size_t) b].levelDb = get("lvl" + s); q.bundle[(size_t) b].detuneCents = get("bdet" + s);
        q.bundle[(size_t) b].lfo = { get("lr" + s), (int) get("ls" + s), get("ld" + s), get("lx" + s), get("ly" + s) };
    }
    q.fmIntoA = get("fmA"); q.fmIntoB = get("fmB");
    q.levelDb = get("level"); q.rvMix = get("rvMix"); q.rvDecay = get("rvDecay"); q.rvPreMs = get("rvPre"); q.rvTone = get("rvTone"); q.limitDb = get("limit"); q.width = get("width"); q.move = get("move");

    buf.clear();
    engine.process(q, buf.getWritePointer(0), buf.getWritePointer(1), buf.getNumSamples());
    for (int l = 0; l < kNumLfo; ++l) lfoOut[(size_t) l] = engine.getLfo(l);
    envOut = engine.getEnvelope(); grDb = engine.getReductionDb();
}

AudioProcessorEditor* SirenProcessor::createEditor() { return new SirenEditor(*this); }
void SirenProcessor::getStateInformation(MemoryBlock& d) { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
void SirenProcessor::setStateInformation(const void* d, int s) { if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(ValueTree::fromXml(*xml)); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SirenProcessor(); }
