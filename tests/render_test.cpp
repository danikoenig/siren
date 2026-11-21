#include "../Source/SirenEngine.h"
#include <cstdio>
#include <vector>
#include <cstring>
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
int main()
{
    const int sr = 48000, N = sr * 8; Engine e; e.prepare(sr);
    std::vector<float> L(N), R(N); Params p; p.animate = 0.3f; p.yAtt = 0.3f; p.zAtt = 0.3f;
    for (int b = 0; b < N; b += 4800)
    {   // sweep X over 8 s
        p.x = b / float(N);
        e.process(p, nullptr, &L[b], &R[b], 4800);
    }
    float peak = 0, rms = 0; bool bad = false;
    for (int i = 0; i < N; ++i) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) bad = true; peak = std::max(peak, std::abs(L[i])); rms += L[i] * L[i]; }
    printf("peak %.3f rms %.4f nan/inf %d lfo1 %.2f lfo2 %.2f\n", peak, std::sqrt(rms / N), bad, e.getLfo1(), e.getLfo2());
    wav("siren_test.wav", L, R, sr);
    // heavy aliasing / extreme settings stay finite
    Params q; q.voct1 = 5; q.voct2 = 5; q.pitch = 1; q.y = 1; q.index = 1; q.idxLfo = 1; q.nearest = true;
    for (int k = 0; k < 100; ++k) e.process(q, nullptr, &L[0], &R[0], 4800);
    for (int i = 0; i < 4800; ++i) if (!std::isfinite(L[i])) bad = true;
    printf("extreme finite: %s\n", bad ? "NO" : "yes");
    return bad;
}
