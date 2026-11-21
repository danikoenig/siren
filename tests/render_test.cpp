#include "../Source/SirenEngine.h"
#include <cstdio>
using namespace siren;
static void wav(const char* f, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* o = fopen(f, "wb"); uint32_t n = (uint32_t) L.size() * 4, v;
    fwrite("RIFF", 1, 4, o); v = 36 + n; fwrite(&v, 4, 1, o); fwrite("WAVEfmt ", 1, 8, o);
    v = 16; fwrite(&v, 4, 1, o); uint16_t s = 1; fwrite(&s, 2, 1, o); s = 2; fwrite(&s, 2, 1, o);
    v = sr; fwrite(&v, 4, 1, o); v = sr * 4; fwrite(&v, 4, 1, o); s = 4; fwrite(&s, 2, 1, o); s = 16; fwrite(&s, 2, 1, o);
    fwrite("data", 1, 4, o); fwrite(&n, 4, 1, o);
    for (size_t i = 0; i < L.size(); ++i) { int16_t a = (int16_t) (L[i] * 32000), b = (int16_t) (R[i] * 32000); fwrite(&a, 2, 1, o); fwrite(&b, 2, 1, o); }
    fclose(o);
}
static Params init()
{
    Params p; p.x = 0.25f; p.attack = 10; p.release = 8; p.gate = true; p.rootNote = 45; // A2
    for (int l = 0; l < kNumLfo; ++l) { p.lfoRate[(size_t) l] = 0.02f * std::pow(1.5f, (float) l); p.lfoShape[(size_t) l] = l % 2; }
    for (int c = 0; c < kNumOsc; ++c) { p.matrix[c % 8][c] = 0.7f; p.matrix[(c * 3 + 1) % 8][c] = -0.5f; }
    for (int f = 0; f < 3; ++f) p.matrix[(f * 2 + 1) % 8][kNumOsc + f] = 0.8f;
    return p;
}
int main()
{
    const int sr = 48000, secs = 40, N = sr * secs; Engine e; e.prepare(sr);
    std::vector<float> L(N), R(N); Params p = init();
    bool bad = false; float peak = 0; std::vector<float> rmsAt;
    for (int b = 0; b < N; b += 4800)
    {
        if (b >= sr * 20) p.gate = false;               // 20 s held, then release into reverb tail
        e.process(p, &L[b], &R[b], 4800);
        float r = 0; for (int i = b; i < b + 4800; ++i) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) bad = true; peak = std::max(peak, std::max(std::abs(L[i]), std::abs(R[i]))); r += L[i] * L[i]; }
        if ((b / 4800) % 50 == 0) rmsAt.push_back(std::sqrt(r / 4800));
    }
    printf("peak %.3f (ceiling -1 dB = 0.891)  nan %d\nrms every 5s:", peak, bad);
    for (float v : rmsAt) printf(" %.3f", v);
    printf("\n");
    wav("siren_drone.wav", L, R, sr);
    Params q = init(); q.levelDb = 12; q.rvShimmer = 1; q.rvDecay = 30; q.rvMix = 1; q.y = 1; q.index = 1; q.bundle[0].res = 1;
    for (int k = 0; k < 300; ++k) e.process(q, &L[0], &R[0], 4800);
    float pk = 0; for (int i = 0; i < 4800; ++i) { if (!std::isfinite(L[i])) bad = true; pk = std::max(pk, std::abs(L[i])); }
    printf("extreme: peak %.3f finite %s\n", pk, bad ? "NO" : "yes");
    return bad || peak > 0.9f;
}
