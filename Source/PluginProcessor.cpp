#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

AudioProcessorValueTreeState::ParameterLayout SirenProcessor::layout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [&](const char* id, const char* name, float lo, float hi, float def, const char* unit = "") {
        p.push_back(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, NormalisableRange<float>(lo, hi), def, AudioParameterFloatAttributes().withLabel(unit)));
    };
    auto flag = [&](const char* id, const char* name, bool def) { p.push_back(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def)); };
    knob("x", "X", 0, 1, 0.5f);  knob("y", "Y", 0, 1, 0.5f);  knob("z", "Z", 0, 1, 0.5f);
    knob("pitch", "Pitch", 0, 1, 0.5f); knob("index", "Index", 0, 1, 0.5f); knob("animate", "Animate", 0, 1, 0.5f);
    knob("xAtt", "X Attenuverter", -1, 1, 0); knob("yAtt", "Y Attenuverter", -1, 1, 0); knob("zAtt", "Z Attenuverter", -1, 1, 0);
    knob("voct2", "V/Oct 2", -5, 5, 0, " V"); knob("pmod", "PMod", 0, 1, 0);
    knob("idxLfo", "Index from LFO1", 0, 1, 0); knob("lfoSpeed", "LFO Speed", -5, 5, 0, " V");
    knob("indexCv", "Index CV", -1, 1, 0); knob("animateCv", "Animate CV", -1, 1, 0);
    knob("level", "Level", -48, 6, -12, " dB");
    flag("nearest", "Nearest", false); flag("reset", "Reset", false); flag("pitchLock", "Pitch Lock", false);
    flag("midiPitch", "MIDI to V/Oct", true); flag("midiReset", "Note Resets Phase", false);
    return { p.begin(), p.end() };
}

SirenProcessor::SirenProcessor()
    : AudioProcessor(BusesProperties().withOutput("Out", AudioChannelSet::stereo(), true)
                                      .withInput("PMod (sidechain)", AudioChannelSet::mono(), false)),
      apvts(*this, nullptr, "SIREN", layout()) {}

bool SirenProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    const auto in = l.getMainInputChannelSet();
    return in.isDisabled() || in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
}

void SirenProcessor::prepareToPlay(double sr, int) { engine.prepare(sr); }

void SirenProcessor::processBlock(AudioBuffer<float>& buf, MidiBuffer& midi)
{
    ScopedNoDenormals nd;
    const int n = buf.getNumSamples();
    const bool midiPitch = get("midiPitch") > 0.5f, midiReset = get("midiReset") > 0.5f;
    bool triggered = false;
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn()) { heldNote = msg.getNoteNumber(); noteVolts = (heldNote - 48) / 12.0f; triggered = true; }
        else if (msg.isNoteOff() && msg.getNoteNumber() == heldNote) heldNote = -1;   // pitch stays (drone)
        else if (msg.isPitchWheel()) bendVolts = (msg.getPitchWheelValue() - 8192) / 8192.0f * (2.0f / 12.0f);
        else if (msg.isControllerOfType(1)) modWheel = msg.getControllerValue() / 127.0f;
    }
    const bool rst = get("reset") > 0.5f;
    if ((rst && !lastReset) || (triggered && midiReset)) engine.resetPhases();
    lastReset = rst;

    siren::Params q;
    q.x = get("x"); q.y = get("y"); q.z = get("z"); q.pitch = get("pitch"); q.index = get("index"); q.animate = get("animate");
    q.xAtt = get("xAtt"); q.yAtt = get("yAtt"); q.zAtt = get("zAtt");
    q.xCv = modWheel;                                  // mod wheel acts as X CV
    q.voct1 = midiPitch ? noteVolts + bendVolts : 0.0f; q.voct2 = get("voct2");
    q.pmodDepth = get("pmod"); q.idxLfo = get("idxLfo"); q.lfoSpeedV = get("lfoSpeed");
    q.indexCv = get("indexCv"); q.animateCv = get("animateCv"); q.levelDb = get("level"); q.nearest = get("nearest") > 0.5f;

    const float* pm = nullptr;
    auto in = getBusBuffer(buf, true, 0);
    if (getBus(true, 0) != nullptr && getBus(true, 0)->isEnabled() && in.getNumChannels() > 0) pm = in.getReadPointer(0);
    std::vector<float> pmCopy;
    if (pm) pmCopy.assign(pm, pm + n);                  // input aliases the output buffer, copy before rendering

    buf.clear();
    engine.process(q, pm ? pmCopy.data() : nullptr, buf.getWritePointer(0), buf.getWritePointer(1), n);
    lfo1 = engine.getLfo1(); lfo2 = engine.getLfo2();
}

AudioProcessorEditor* SirenProcessor::createEditor() { return new SirenEditor(*this); }
void SirenProcessor::getStateInformation(MemoryBlock& d) { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d); }
void SirenProcessor::setStateInformation(const void* d, int s) { if (auto xml = getXmlFromBinary(d, s)) apvts.replaceState(ValueTree::fromXml(*xml)); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SirenProcessor(); }
