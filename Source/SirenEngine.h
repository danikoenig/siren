#pragma once
// Siren engine: 21 oscillators (Just Intonation stack) + 23 LFOs. JUCE-free.
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace siren
{
constexpr int kNumOsc = 21;
constexpr int kNumLfo = 23;          // 21 for the oscillators + LFO1 + LFO2
constexpr int kControlBlock = 32;
constexpr double kTwoPi = 6.283185307179586;

// ---------------------------------------------------------------- wavetables
struct Wavetables
{
    static constexpr int numTables = 8, numFrames = 16, size = 2048;
    static constexpr int vcaTable = 5;      // Z acts as volume here
    std::vector<float> data;

    Wavetables()
    {
        data.assign((size_t) numTables * numFrames * (size + 1), 0.0f);
        for (int t = 0; t < numTables; ++t)
            for (int f = 0; f < numFrames; ++f)
            {
                float* w = &data[((size_t) t * numFrames + f) * (size + 1)];
                const double z = f / double(numFrames - 1);
                double peak = 1e-9;
                for (int i = 0; i < size; ++i)
                {
                    w[i] = (float) sample(t, z, i / double(size));
                    peak = std::max(peak, (double) std::abs(w[i]));
                }
                if (t != vcaTable)
                    for (int i = 0; i < size; ++i) w[i] = (float) (w[i] / peak);
                w[size] = w[0];
            }
    }

    static double sample(int t, double z, double ph)
    {
        const double p = ph * kTwoPi;
        switch (t)
        {
            case 0: { double s = std::sin(p); for (int h = 2; h <= 40; ++h) s += z * std::sin(h * p) / h; return s; }          // sine -> saw
            case 1: { double s = std::sin(p); for (int h = 3; h <= 41; h += 2) s += z * std::sin(h * p) / h; return s; }      // sine -> square
            case 2: { const double w = 0.5 - 0.45 * z; return (ph < w ? 1.0 : -1.0) * (0.3 + 0.7 * z) + (1.0 - z) * (2.0 * std::abs(2.0 * ph - 1.0) - 1.0); } // tri -> pulse
            case 3: return std::sin(p + 4.0 * z * std::sin(2.0 * p));                                                         // FM
            case 4: return std::sin((0.5 + 6.0 * z) * std::sin(p));                                                           // wavefold
            case 5: return z * std::sin(p);                                                                                   // pseudo-VCA
            case 6: { const double c = 1.0 + z * 14.0; double s = 0; for (int h = 1; h <= 40; ++h) { const double d = (h - c) / 2.0; s += std::exp(-d * d) * std::sin(h * p) / std::sqrt((double) h); } return s; } // formant
            default: { const double lv = std::pow(2.0, 7.0 - 6.0 * z); return std::round(std::sin(p) * lv) / lv; }          // crushed sine
        }
    }

    inline float lookup(int t, float frame, double phase) const
    {
        const int f0 = std::min((int) frame, numFrames - 1), f1 = std::min(f0 + 1, numFrames - 1);
        const float ff = frame - (float) f0;
        const double idx = phase * size; const int i0 = (int) idx; const float fr = (float) (idx - i0);
        const float* a = &data[((size_t) t * numFrames + f0) * (size + 1)];
        const float* b = &data[((size_t) t * numFrames + f1) * (size + 1)];
        const float va = a[i0] + fr * (a[i0 + 1] - a[i0]);
        const float vb = b[i0] + fr * (b[i0 + 1] - b[i0]);
        return va + ff * (vb - va);
    }
};

// ------------------------------------------------------------------- chords
// Just Intonation note sets, ordered from "octaves" to "harmonic cluster".
inline const std::vector<std::vector<double>>& chords()
{
    static const std::vector<std::vector<double>> c = {
        { 1.0 },
        { 1.0, 3.0 / 2 },
        { 1.0, 4.0 / 3, 3.0 / 2 },
        { 1.0, 5.0 / 4, 3.0 / 2 },
        { 1.0, 6.0 / 5, 3.0 / 2 },
        { 1.0, 5.0 / 4, 3.0 / 2, 15.0 / 8 },
        { 1.0, 6.0 / 5, 3.0 / 2, 9.0 / 5 },
        { 1.0, 9.0 / 8, 5.0 / 4, 3.0 / 2, 27.0 / 16 },
        { 1.0, 9.0 / 8, 5.0 / 4, 45.0 / 32, 3.0 / 2, 15.0 / 8 },
        { 1.0, 9.0 / 8, 5.0 / 4, 11.0 / 8, 3.0 / 2, 13.0 / 8, 7.0 / 4, 15.0 / 8 } };
    return c;
}

// log2 frequency ratio of oscillator i in a given chord for octave spread y (0..1)
inline double stackLog2(int chord, int i, double y)
{
    const auto& notes = chords()[(size_t) chord];
    const int n = (int) notes.size();
    const int layer = i / n, k = i % n;
    const int maxLayer = std::max(1, (kNumOsc - 1) / n);
    const double span = y * 5.0;
    const double oct = std::floor(layer / double(maxLayer) * span - span * 0.5 + 0.5);
    return std::log2(notes[(size_t) k]) + oct;
}

inline double hash01(int i, double seed) { const double s = std::sin(i * 78.233 + seed) * 43758.5453; return s - std::floor(s); }

// ------------------------------------------------------------------- params
struct Params
{
    float x = 0.5f, y = 0.5f, z = 0.5f;           // 0..1 knobs
    float pitch = 0.5f, index = 0.5f, animate = 0.5f;
    float xAtt = 0, yAtt = 0, zAtt = 0;           // -1..1
    float xCv = 0, yCv = 0, zCv = 0;              // external CV, -1..1 (= -5V..5V). yCv/zCv <0 unused: LFOs are normalled
    float voct1 = 0, voct2 = 0;                   // volts
    float pmodDepth = 0, idxLfo = 0, lfoSpeedV = 0; // idxLfo: LFO1 -> index
    float indexCv = 0, animateCv = 0;
    float levelDb = -12;
    bool nearest = false;
};

// ------------------------------------------------------------------- engine
class Engine
{
public:
    void prepare(double sampleRate) { sr = sampleRate; hpState = 0; resetPhases(); for (auto& v : incSm) v = 0.0; first = true; }

    void resetPhases()
    {
        for (int i = 0; i < kNumOsc; ++i) { phase[i] = hash01(i, 1.0); }
        for (int i = 0; i < kNumLfo; ++i) lfoPhase[i] = hash01(i, 7.0);
    }

    float getLfo1() const { return lfoVal[kNumOsc]; }
    float getLfo2() const { return lfoVal[kNumOsc + 1]; }

    // pmod may be nullptr. Renders n samples into L/R (overwriting).
    void process(const Params& p, const float* pmod, float* L, float* R, int n)
    {
        int pos = 0;
        while (pos < n)
        {
            const int len = std::min(kControlBlock, n - pos);
            control(p, len);
            for (int s = 0; s < len; ++s)
            {
                float pm = 0.0f;
                if (pmod) { const float in = pmod[pos + s]; hpState += 0.0014f * (in - hpState); pm = (in - hpState) * p.pmodDepth * 0.5f; }
                float l = 0, r = 0;
                for (int i = 0; i < kNumOsc; ++i)
                {
                    incSm[i] += smoothCoef * (incTarget[i] - incSm[i]);
                    phase[i] += incSm[i]; phase[i] -= std::floor(phase[i]);
                    double ph = phase[i] + pm; ph -= std::floor(ph);
                    const float v = wt.lookup(tableIdx, frame, ph);
                    gainL[i] += gainCoef * (tgtL[i] - gainL[i]);
                    gainR[i] += gainCoef * (tgtR[i] - gainR[i]);
                    l += v * gainL[i]; r += v * gainR[i];
                }
                L[pos + s] = std::tanh(l * outGain);
                R[pos + s] = std::tanh(r * outGain);
            }
            pos += len;
        }
    }

private:
    void control(const Params& p, int len)
    {
        const double dt = len / sr;
        const double speed = (1.0 + p.animate * 6.0) * std::pow(2.0, p.lfoSpeedV);
        // --- LFOs (waveform follows the selected table -> LFO->Index "glitch" works)
        const int lfoFrame = (tableIdx == Wavetables::vcaTable) ? Wavetables::numFrames - 1 : Wavetables::numFrames / 2;
        for (int i = 0; i < kNumLfo; ++i)
        {
            double rate;
            if (i == kNumOsc) rate = 0.21; else if (i == kNumOsc + 1) rate = 0.137;
            else rate = 0.05 + 0.0173 * (i + 1) * (1.0 + 0.1 * hash01(i, 3.0));
            lfoPhase[i] += rate * speed * dt; lfoPhase[i] -= std::floor(lfoPhase[i]);
            lfoVal[i] = 0.5f * (wt.lookup(tableIdx, (float) lfoFrame, lfoPhase[i]) + 1.0f); // unipolar 0..1 (0-5V)
        }
        const float lfo1 = lfoVal[kNumOsc], lfo2 = lfoVal[kNumOsc + 1];

        // --- timbre controls (normalled: Y <- LFO1, Z <- LFO2)
        auto cl = [](float v) { return std::min(1.0f, std::max(0.0f, v)); };
        float xv = cl(p.x + p.xAtt * std::clamp(p.xCv, -1.0f, 1.0f));
        float yv = cl(p.y + p.yAtt * std::clamp(p.yCv + lfo1, -1.0f, 1.0f));
        float zv = cl(p.z + p.zAtt * std::clamp(p.zCv + lfo2, -1.0f, 1.0f));
        float iv = cl(p.index + p.indexCv + p.idxLfo * lfo1);
        const float av = cl(p.animate + p.animateCv);

        const int newTable = std::min(Wavetables::numTables - 1, (int) (iv * (Wavetables::numTables - 1) + 0.5f));
        tableIdx = newTable;

        const int nChords = (int) chords().size();
        double xpos = xv * (nChords - 1);
        float zf = zv * (Wavetables::numFrames - 1);
        if (p.nearest) { xpos = std::round(xpos); yv = std::round(yv * 10.0f) / 10.0f; zf = std::round(zf); }
        frame = zf;
        const int c0 = std::min((int) xpos, nChords - 2), c1 = c0 + 1; const double cf = std::min(1.0, xpos - c0);

        // --- base pitch: C3 at 12 o'clock, 5 octaves across knob, + both v/oct inputs
        const double volts = (p.pitch - 0.5) * 5.0 + p.voct1 + p.voct2;
        const double baseHz = 130.8127827 * std::pow(2.0, volts);

        const float width = 0.25f + 0.75f * av;
        for (int i = 0; i < kNumOsc; ++i)
        {
            const double l2 = (1.0 - cf) * stackLog2(c0, i, yv) + cf * stackLog2(c1, i, yv);
            const double cents = av * 20.0 * (hash01(i, 5.0) * 2.0 - 1.0) * (0.5 + 0.5 * lfoVal[i]);
            const double hz = baseHz * std::pow(2.0, l2 + cents / 1200.0);
            incTarget[i] = std::min(8.0, hz / sr);
            if (first || p.nearest) incSm[i] = incTarget[i];

            const float depth = 0.5f + 0.4f * av;
            const float a = (1.0f - depth * lfoVal[i]) / std::sqrt((float) kNumOsc);
            const float pan = width * (float) (hash01(i, 9.0) * 2.0 - 1.0);
            const float ang = (pan * 0.5f + 0.5f) * 1.5707963f;
            tgtL[i] = a * std::cos(ang) * 1.414f; tgtR[i] = a * std::sin(ang) * 1.414f;
            if (first) { gainL[i] = tgtL[i]; gainR[i] = tgtR[i]; }
        }
        first = false;
        outGain = std::pow(10.0f, p.levelDb / 20.0f) * 1.5f;
        smoothCoef = p.nearest ? 1.0 : 1.0 - std::exp(-1.0 / (0.02 * sr));
        gainCoef = (float) (1.0 - std::exp(-1.0 / (0.01 * sr)));
    }

    Wavetables wt;
    double sr = 48000.0;
    std::array<double, kNumOsc> phase{}, incTarget{}, incSm{};
    std::array<double, kNumLfo> lfoPhase{};
    std::array<float, kNumLfo> lfoVal{};
    std::array<float, kNumOsc> tgtL{}, tgtR{}, gainL{}, gainR{};
    int tableIdx = 0; float frame = 0, outGain = 1, hpState = 0; double smoothCoef = 0.01; float gainCoef = 0.01f; bool first = true;
};
} // namespace siren
