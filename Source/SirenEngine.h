#pragma once
// Siren drone engine. JUCE-free so it can be tested offline.
// 21 oscillators (Just Intonation triad stacks) in 3 bundles of 7, 8 LFOs with a
// modulation matrix, one filter per bundle, shimmer reverb and a look-ahead limiter.
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace siren
{
constexpr int kNumOsc = 21, kNumBundles = 3, kOscPerBundle = 7, kNumLfo = 8;
constexpr int kMatrixCols = kNumOsc + kNumBundles;   // 21 pitch targets + 3 filter cutoffs
constexpr int kControlBlock = 32;
constexpr double kTwoPi = 6.283185307179586;

// ---------------------------------------------------------------- wavetables
struct Wavetables
{
    static constexpr int numTables = 8, numFrames = 16, size = 2048;
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
                for (int i = 0; i < size; ++i) { w[i] = (float) sample(t, z, i / double(size)); peak = std::max(peak, (double) std::abs(w[i])); }
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
            case 2: { const double w = 0.5 - 0.45 * z; return (ph < w ? 1.0 : -1.0) * (0.3 + 0.7 * z) + (1.0 - z) * (2.0 * std::abs(2.0 * ph - 1.0) - 1.0); }
            case 3: return std::sin(p + 4.0 * z * std::sin(2.0 * p));                                                         // FM
            case 4: return std::sin((0.5 + 6.0 * z) * std::sin(p));                                                           // wavefold
            case 5: return std::sin(p) + z * (0.5 * std::sin(2 * p) + 0.4 * std::sin(3 * p) + 0.3 * std::sin(4 * p) + 0.25 * std::sin(6 * p)); // organ
            case 6: { const double c = 1.0 + z * 14.0; double s = 0; for (int h = 1; h <= 40; ++h) { const double d = (h - c) / 2.0; s += std::exp(-d * d) * std::sin(h * p) / std::sqrt((double) h); } return s; }
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
// Just Intonation sets relative to the root, ordered so neighbours are related. X morphs through them.
struct Chord { const char* name; std::vector<double> notes; };
inline const std::vector<Chord>& chords()
{
    static const std::vector<Chord> c = {
        { "Octaves",        { 1.0 } },
        { "Power",          { 1.0, 3.0 / 2 } },
        { "Minor",          { 1.0, 6.0 / 5, 3.0 / 2 } },
        { "Minor 1st inv",  { 6.0 / 5, 3.0 / 2, 2.0 } },
        { "Minor 2nd inv",  { 3.0 / 2, 2.0, 12.0 / 5 } },
        { "Minor open",     { 1.0, 3.0 / 2, 12.0 / 5 } },
        { "Minor 7",        { 1.0, 6.0 / 5, 3.0 / 2, 9.0 / 5 } },
        { "Minor 9",        { 1.0, 6.0 / 5, 3.0 / 2, 9.0 / 5, 9.0 / 4 } },
        { "Sus2",           { 1.0, 9.0 / 8, 3.0 / 2 } },
        { "Dorian cluster", { 1.0, 9.0 / 8, 6.0 / 5, 3.0 / 2, 5.0 / 3 } } };
    return c;
}

// log2 frequency ratio of oscillator i in chord c for octave spread y (0..1); root is the lowest register.
inline double stackLog2(int chord, int i, double y)
{
    const auto& notes = chords()[(size_t) chord].notes;
    const int n = (int) notes.size();
    const int layer = i / n, k = i % n;
    const int maxLayer = std::max(1, (kNumOsc - 1) / n);
    const double oct = std::floor(layer / double(maxLayer) * (y * 5.0) + 0.5);
    return std::log2(notes[(size_t) k]) + oct;
}

inline double hash01(int i, double seed) { const double s = std::sin(i * 78.233 + seed) * 43758.5453; return s - std::floor(s); }

// ------------------------------------------------------------------- params
struct Params
{
    float x = 0.2f, y = 0.5f, z = 0.4f, index = 0.0f, animate = 0.3f;
    float attack = 12.0f, release = 20.0f, stagger = 0.6f, glide = 0.5f;   // seconds, stagger 0..1
    bool gate = false; float rootNote = 48.0f;                              // MIDI note number (float: pitch bend)
    std::array<float, kNumOsc> detune{};                                    // cents, manual
    std::array<float, kNumLfo> lfoRate{}; std::array<int, kNumLfo> lfoShape{};
    float matrix[kNumLfo][kMatrixCols] = {};                                // -1..1
    float pitchDepthCents = 25.0f, filterDepthOct = 2.0f;
    struct Bundle { int type = 0; float cutoff = 2000.0f, res = 0.2f, levelDb = 0.0f; };
    std::array<Bundle, kNumBundles> bundle{};
    float levelDb = -12.0f;
    float rvMix = 0.35f, rvDecay = 8.0f, rvShimmer = 0.4f, rvTone = 0.6f;
    float ceilingDb = -1.0f;
};

// ----------------------------------------------------------------- filters
struct SVF
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    void set(double fc, double sr, float res)
    {
        fc = std::min(std::max(fc, 20.0), std::min(18000.0, sr * 0.45));
        const double g = std::tan(3.14159265358979 * fc / sr), Q = 0.5 + (double) res * res * 11.5;
        k = (float) (1.0 / Q); a1 = (float) (1.0 / (1.0 + g * (g + k))); a2 = (float) (g * a1); a3 = (float) (g * a2);
    }
    inline float process(float v0, int type)
    {
        const float v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        return type == 0 ? v2 : type == 1 ? v1 * k : v0 - k * v1 - v2;
    }
};

// ------------------------------------------------------------ shimmer reverb
// octave-up pitch shifter: two cross-faded delay taps
struct PitchShift
{
    std::vector<float> buf; int w = 0, W = 1; double phase = 0; float lp = 0;
    void prepare(double sr) { W = (int) (0.06 * sr); buf.assign((size_t) W * 2 + 8, 0.0f); w = 0; phase = 0; lp = 0; }
    float process(float in)
    {
        const int sz = (int) buf.size();
        buf[(size_t) w] = in;
        phase += 1.0 / W; phase -= std::floor(phase);
        float out = 0;
        for (int t = 0; t < 2; ++t)
        {
            double ph = phase + 0.5 * t; ph -= std::floor(ph);
            const double d = 1.0 + W * (1.0 - ph); const float win = (float) std::pow(std::sin(3.14159265358979 * ph), 2.0);
            double rp = w - d; while (rp < 0) rp += sz;
            const int i0 = (int) rp; const float fr = (float) (rp - i0);
            out += win * (buf[(size_t) (i0 % sz)] * (1.0f - fr) + buf[(size_t) ((i0 + 1) % sz)] * fr);
        }
        if (++w >= sz) w = 0;
        lp += 0.35f * (out - lp);
        return lp;
    }
};

class ShimmerReverb
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        static const double ms[8] = { 29.7, 37.1, 41.1, 43.7, 53.9, 59.3, 67.7, 71.9 };
        for (int i = 0; i < 8; ++i) { len[i] = (float) (ms[i] * 0.001 * sr); line[i].assign((size_t) len[i] + 256, 0.0f); w[i] = 0; lp[i] = 0; }
        static const int ap[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i) { apLen[i] = std::max(8, (int) (ap[i] * sr / 29500.0)); apBuf[i].assign((size_t) apLen[i], 0.0f); apPos[i] = 0; }
        shFF.prepare(sr); shFB.prepare(sr); wetPrev = 0;
        lfoPh = 0;
    }

    void process(float inL, float inR, float& outL, float& outR, float mix, float decaySec, float shimmer, float tone)
    {
        float x = 0.5f * (inL + inR);
        for (int i = 0; i < 4; ++i)                                // input diffusion
        {
            float& s = apBuf[i][(size_t) apPos[i]]; const float y = s - 0.6f * x; s = x + 0.6f * y; x = y;
            if (++apPos[i] >= apLen[i]) apPos[i] = 0;
        }
        const float shF = shFF.process(x), shB = shFB.process(wetPrev);   // feed-forward octave + regenerating feedback octaves
        // energy-normalised injection: steady-state wet level ~ input level regardless of decay time
        const float gAvg = std::pow(10.0f, -3.0f * 0.05f / std::max(0.2f, decaySec));
        const float inj = (x + shimmer * (0.9f * shF + 1.2f * shB)) * 0.35355339f * std::sqrt(std::max(1e-4f, 1.0f - gAvg * gAvg));

        lfoPh += 0.35 / sr; lfoPh -= std::floor(lfoPh);
        const float damp = 0.08f + 0.9f * tone * tone;   // lowpass coef (tone = brightness)
        float y[8];
        for (int i = 0; i < 8; ++i)
        {
            const float mod = 6.0f * (float) std::sin(kTwoPi * (lfoPh * (0.7 + 0.13 * i) + i * 0.17));
            float rp = (float) w[i] - (len[i] + mod); const int sz = (int) line[i].size();
            while (rp < 0) rp += (float) sz;
            const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % sz;
            const float v = line[i][(size_t) (i0 % sz)] * (1.0f - fr) + line[i][(size_t) i1] * fr;
            lp[i] += damp * (v - lp[i]);
            y[i] = lp[i];
        }
        // Hadamard feedback matrix (8x8, scaled)
        float h[8]; for (int i = 0; i < 8; ++i) h[i] = y[i];
        for (int s = 1; s < 8; s <<= 1)
            for (int i = 0; i < 8; i += s * 2) for (int j = i; j < i + s; ++j) { const float a = h[j], b = h[j + s]; h[j] = a + b; h[j + s] = a - b; }
        const float norm = 0.35355339f;
        for (int i = 0; i < 8; ++i)
        {
            const float g = std::pow(10.0f, -3.0f * (len[i] / (float) sr) / std::max(0.2f, decaySec));
            const int sz = (int) line[i].size();
            line[i][(size_t) w[i]] = inj * ((i & 1) ? -1.0f : 1.0f) + h[i] * norm * g;
            if (++w[i] >= sz) w[i] = 0;
        }
        const float wl = 0.7f * (y[0] + y[2] + y[4] + y[6]), wr = 0.7f * (y[1] + y[3] + y[5] + y[7]);
        wetPrev = std::tanh(0.5f * (wl + wr));
        outL = inL * (1.0f - mix) + wl * mix;
        outR = inR * (1.0f - mix) + wr * mix;
    }

private:
    double sr = 48000, lfoPh = 0;
    std::array<std::vector<float>, 8> line; std::array<float, 8> len{}, lp{}; std::array<int, 8> w{};
    std::array<std::vector<float>, 4> apBuf; std::array<int, 4> apLen{}, apPos{};
    PitchShift shFF, shFB; float wetPrev = 0;
};

// ------------------------------------------------------------------ limiter
class Limiter
{
public:
    void prepare(double sr)
    {
        N = std::max(8, (int) (0.003 * sr));
        dl.assign((size_t) N, 0.0f); dr.assign((size_t) N, 0.0f); tg.assign((size_t) N, 1.0f); mg.assign((size_t) N, 1.0f);
        pos = 0; sum = (double) N; env = 1.0f; relCoef = (float) (1.0 - std::exp(-1.0 / (0.25 * sr)));
    }
    int latency() const { return N; }
    float lastGain() const { return env; }

    void process(float& l, float& r, float ceilingDb)
    {
        const float ceil = std::pow(10.0f, ceilingDb / 20.0f);
        const float peak = std::max(std::abs(l), std::abs(r));
        tg[(size_t) pos] = peak > ceil ? ceil / peak : 1.0f;
        float mn = 1.0f; for (int i = 0; i < N; ++i) mn = std::min(mn, tg[(size_t) i]);   // sliding minimum
        sum += mn - mg[(size_t) pos]; mg[(size_t) pos] = mn;                               // moving average -> smooth attack
        const float avg = (float) (sum / N);
        env = std::min(avg, env + (1.0f - env) * relCoef);
        const float ol = dl[(size_t) pos] * env, orr = dr[(size_t) pos] * env;
        dl[(size_t) pos] = l; dr[(size_t) pos] = r;
        if (++pos >= N) pos = 0;
        l = std::max(-ceil, std::min(ceil, ol)); r = std::max(-ceil, std::min(ceil, orr));
    }
private:
    int N = 144, pos = 0; double sum = 0; float env = 1, relCoef = 0.001f;
    std::vector<float> dl, dr, tg, mg;
};

// ------------------------------------------------------------------- engine
class Engine
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate; reverb.prepare(sr); limiter.prepare(sr);
        for (int i = 0; i < kNumOsc; ++i) { phase[i] = hash01(i, 1.0); incSm[i] = 0; gainL[i] = gainR[i] = 0; }
        for (int l = 0; l < kNumLfo; ++l) { lfoPhase[l] = hash01(l, 7.0); lfoPrev[l] = 0; lfoTgt[l] = (float) (hash01(l, 2.0) * 2 - 1); lfoVal[l] = 0; }
        for (auto& f : filt) for (auto& s : f) s = SVF();
        env = 0; first = true; lfoVal.fill(0);
    }
    int latencySamples() const { return limiter.latency(); }
    float getLfo(int i) const { return lfoVal[(size_t) i]; }
    float getEnvelope() const { return env; }
    float getReductionDb() const { return 20.0f * std::log10(std::max(1e-4f, limiter.lastGain())); }

    void process(const Params& p, float* L, float* R, int n)
    {
        int pos = 0;
        while (pos < n)
        {
            const int len = std::min(kControlBlock, n - pos);
            control(p, len);
            for (int s = 0; s < len; ++s)
            {
                float bl[kNumBundles] = {}, br[kNumBundles] = {};
                for (int i = 0; i < kNumOsc; ++i)
                {
                    incSm[i] += smoothCoef * (incTarget[i] - incSm[i]);
                    phase[i] += incSm[i]; phase[i] -= std::floor(phase[i]);
                    const float v = wt.lookup(tableIdx, frame, phase[i]);
                    gainL[i] += gainCoef * (tgtL[i] - gainL[i]); gainR[i] += gainCoef * (tgtR[i] - gainR[i]);
                    bl[i / kOscPerBundle] += v * gainL[i]; br[i / kOscPerBundle] += v * gainR[i];
                }
                float l = 0, r = 0;
                for (int b = 0; b < kNumBundles; ++b)
                {
                    l += filt[b][0].process(bl[b], p.bundle[(size_t) b].type) * bundleGain[b];
                    r += filt[b][1].process(br[b], p.bundle[(size_t) b].type) * bundleGain[b];
                }
                l *= outGain; r *= outGain;
                reverb.process(l, r, l, r, p.rvMix, p.rvDecay, p.rvShimmer, p.rvTone);
                limiter.process(l, r, p.ceilingDb);
                L[pos + s] = l; R[pos + s] = r;
            }
            pos += len;
        }
    }

private:
    float lfoShape(int l, int shape, double ph)
    {
        switch (shape)
        {
            case 0: return (float) std::sin(kTwoPi * ph);
            case 1: return (float) (1.0 - 4.0 * std::abs(ph - 0.5));
            case 2: return (float) (2.0 * ph - 1.0);
            default: { const float t = (float) ph, s = t * t * (3 - 2 * t); return lfoPrev[l] + (lfoTgt[l] - lfoPrev[l]) * s; }
        }
    }

    void control(const Params& p, int len)
    {
        const double dt = len / sr;
        // drone envelope: very slow swell and release
        if (p.gate) env += (float) (dt / std::max(0.05f, p.attack)); else env -= (float) (dt / std::max(0.05f, p.release));
        env = std::min(1.0f, std::max(0.0f, env));
        const float motion = 0.25f + 0.75f * env;                 // modulation grows with the swell

        const double speed = 1.0 + p.animate * 3.0;
        for (int l = 0; l < kNumLfo; ++l)
        {
            const double old = lfoPhase[l];
            lfoPhase[l] += p.lfoRate[(size_t) l] * speed * dt; lfoPhase[l] -= std::floor(lfoPhase[l]);
            if (lfoPhase[l] < old) { lfoPrev[l] = lfoTgt[l]; lfoTgt[l] = (float) (hash01((int) (++rnd), 4.0 + l) * 2 - 1); }
            lfoVal[(size_t) l] = lfoShape(l, p.lfoShape[(size_t) l], lfoPhase[l]);
        }

        auto cl = [](float v) { return std::min(1.0f, std::max(0.0f, v)); };
        const int nChords = (int) chords().size();
        const double xpos = cl(p.x) * (nChords - 1);
        const int c0 = std::min((int) xpos, nChords - 2), c1 = c0 + 1; const double cf = std::min(1.0, xpos - c0);
        tableIdx = std::min(Wavetables::numTables - 1, (int) (cl(p.index) * (Wavetables::numTables - 1) + 0.5f));
        frame = cl(p.z) * (Wavetables::numFrames - 1);
        const float yv = cl(p.y), av = cl(p.animate);

        const double rootHz = 440.0 * std::pow(2.0, (p.rootNote - 69.0) / 12.0);
        const float width = 0.25f + 0.75f * av;
        for (int i = 0; i < kNumOsc; ++i)
        {
            float mod = 0; for (int l = 0; l < kNumLfo; ++l) mod += p.matrix[l][i] * lfoVal[(size_t) l];
            const double l2 = (1.0 - cf) * stackLog2(c0, i, yv) + cf * stackLog2(c1, i, yv);
            const double cents = p.detune[(size_t) i] + av * 15.0 * (hash01(i, 5.0) * 2.0 - 1.0) + p.pitchDepthCents * motion * mod;
            incTarget[i] = std::min(0.45, rootHz * std::pow(2.0, l2 + cents / 1200.0) / sr);
            if (first) incSm[i] = incTarget[i];

            // staggered swell: the root triad enters first, upper layers follow
            const float raw = std::min(1.0f, std::max(0.0f, env * (1.0f + p.stagger) - p.stagger * (float) i / (kNumOsc - 1)));
            const float eg = raw * raw * (3.0f - 2.0f * raw);
            const float a = eg * 1.414f / std::sqrt((float) kNumOsc);
            const float pan = width * (float) (hash01(i, 9.0) * 2.0 - 1.0), ang = (pan * 0.5f + 0.5f) * 1.5707963f;
            tgtL[i] = a * std::cos(ang); tgtR[i] = a * std::sin(ang);
            if (first) { gainL[i] = tgtL[i]; gainR[i] = tgtR[i]; }
        }
        first = false;
        for (int b = 0; b < kNumBundles; ++b)
        {
            float m = 0; for (int l = 0; l < kNumLfo; ++l) m += p.matrix[l][kNumOsc + b] * lfoVal[(size_t) l];
            m = std::min(1.5f, std::max(-1.5f, m));
            const double fc = p.bundle[(size_t) b].cutoff * std::pow(2.0, p.filterDepthOct * motion * m);
            for (int c = 0; c < 2; ++c) filt[b][(size_t) c].set(fc, sr, p.bundle[(size_t) b].res);
            bundleGain[b] = std::pow(10.0f, p.bundle[(size_t) b].levelDb / 20.0f);
        }
        outGain = std::pow(10.0f, p.levelDb / 20.0f);
        smoothCoef = 1.0 - std::exp(-1.0 / (std::max(0.01f, p.glide) * sr));
        gainCoef = (float) (1.0 - std::exp(-1.0 / (0.02 * sr)));
    }

    Wavetables wt; ShimmerReverb reverb; Limiter limiter;
    double sr = 48000.0;
    std::array<double, kNumOsc> phase{}, incTarget{}, incSm{};
    std::array<double, kNumLfo> lfoPhase{}; std::array<float, kNumLfo> lfoVal{}, lfoPrev{}, lfoTgt{};
    std::array<float, kNumOsc> tgtL{}, tgtR{}, gainL{}, gainR{};
    std::array<std::array<SVF, 2>, kNumBundles> filt{}; std::array<float, kNumBundles> bundleGain{};
    int tableIdx = 0; float frame = 0, outGain = 1, env = 0; double smoothCoef = 0.001; float gainCoef = 0.01f; bool first = true; uint32_t rnd = 1;
};
} // namespace siren
