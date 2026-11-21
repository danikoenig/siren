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
    Params p; p.x = 0.25f; p.attack = 10; p.release = 6; p.gate = true; p.rootNote = 45;   // A2
    for (int b = 0; b < kNumBundles; ++b)
    {
        auto& u = p.bundle[(size_t) b]; u.cutoff = b == 0 ? 900.0f : b == 1 ? 2200.0f : 5000.0f;
        u.lfo.rate = 0.03f * (1 + 0.6f * b); u.lfo.shape = b; u.lfo.depth = 0.5f; u.lfo.padX = 0.15f; u.lfo.padY = 0.85f;
    }
    return p;
}
int main()
{
    const int sr = 48000, secs = 45, N = sr * secs; Engine e; e.prepare(sr);
    std::vector<float> L(N), R(N); Params p = init();
    bool bad = false; float peak = 0; double eL = 0, eR = 0; int nBal = 0; std::vector<float> rmsAt;
    for (int b = 0; b < N; b += 4800)
    {
        if (b >= sr * 15) p.gate = false;               // 15 s held, then release (6 s) into the reverb tail
        e.process(p, &L[b], &R[b], 4800);
        float r = 0; for (int i = b; i < b + 4800; ++i) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) bad = true; peak = std::max(peak, std::max(std::abs(L[i]), std::abs(R[i]))); r += L[i] * L[i];
            if (b > sr * 10 && b < sr * 15) { eL += L[i] * L[i]; eR += R[i] * R[i]; ++nBal; } }
        if ((b / 4800) % 50 == 0) rmsAt.push_back(std::sqrt(r / 4800));
    }
    printf("peak %.3f (%.1f dBFS)  nan %d\nrms every 5s:", peak, 20 * std::log10(peak), bad);
    for (float v : rmsAt) printf(" %.3f", v);
    printf("\nL/R balance while held: %.2f dB   (L rms %.4f, R rms %.4f)\n", 10 * std::log10(eL / eR), std::sqrt(eL / nBal), std::sqrt(eR / nBal));
    float tail = 0; for (int i = N - sr; i < N; ++i) tail = std::max(tail, std::max(std::abs(L[i]), std::abs(R[i])));
    printf("last second after release: peak %.6f (%.0f dBFS)\n", tail, 20 * std::log10(std::max(tail, 1e-9f)));
    wav("siren_drone.wav", L, R, sr);
    { Engine e2; e2.prepare(sr); Engine e3; e3.prepare(sr); Params a = init(), b = init(); a.attack = b.attack = 0.1f; a.stagger = b.stagger = 0; b.fmIntoA = 0.6f; b.fmIntoB = 0.8f; std::vector<float> A(48000), B(48000), R2(48000); double d = 0, en = 0; bool ok = true;
      for (int k = 0; k < 120; ++k) { e2.process(a, &A[0], &R2[0], 1200); e3.process(b, &B[0], &R2[0], 1200); if (k > 100) for (int i = 0; i < 1200; ++i) { d += (A[i] - B[i]) * (A[i] - B[i]); en += A[i] * A[i]; if (!std::isfinite(B[i])) ok = false; } }
      printf("FM: finite %d, relative difference %.3f\n", ok, d / std::max(en, 1e-9)); }
    Params q = init(); q.levelDb = 12; q.rvDecay = 60; q.rvMix = 1; q.y = 1; q.index = 1; q.bundle[0].res = 1; q.rvTone = -1;
    for (int k = 0; k < 300; ++k) e.process(q, &L[0], &R[0], 4800);
    float pk = 0; for (int i = 0; i < 4800; ++i) { if (!std::isfinite(L[i])) bad = true; pk = std::max(pk, std::abs(L[i])); }
    printf("extreme: peak %.3f finite %s\n", pk, bad ? "NO" : "yes");
    return bad || peak > 0.9f;
}
