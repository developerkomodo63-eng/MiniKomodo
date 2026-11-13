#pragma once
#include "KomodoDsp.h"

// Piezas de la version completa (no se usan en las Lite): FX estereo, matriz de modulacion, filtro multimodo, osciloscopio.

// ---------------------------------------------------------------- matriz de modulacion (4 ranuras, control rate)
struct ModSrc { enum { Off, Lfo1, Lfo2, FEnv, AEnv, Vel, Key, Wheel, Rand, Count }; };

struct ModOut { float pitch = 0.f, cutoff = 0.f, res = 0.f, amp = 0.f, pan = 0.f, d1 = 0.f, d2 = 0.f; };

// destinos: 1 pitch (semitonos), 2 corte (octavas), 3 resonancia, 4 volumen, 5 pan, 6 d1, 7 d2 (cada sinte decide que son d1/d2)
inline void evalMatrix (const int* src, const int* dst, const float* amt, const float* vals, ModOut& o) noexcept
{
    for (int i = 0; i < 4; ++i)
    {
        if (src[i] == 0 || dst[i] == 0) continue;
        const float v = vals[src[i]] * amt[i];
        switch (dst[i])
        {
            case 1:  o.pitch  += v * 12.f; break;
            case 2:  o.cutoff += v * 4.f;  break;
            case 3:  o.res    += v * 0.5f; break;
            case 4:  o.amp    += v;        break;
            case 5:  o.pan    += v;        break;
            case 6:  o.d1     += v;        break;
            default: o.d2     += v;        break;
        }
    }
}

// pan de potencia constante normalizado para que el centro valga 1.0 en cada canal
inline void panGains (float pan, float& gl, float& gr) noexcept
{
    const float a = (juce::jlimit (-1.f, 1.f, pan) + 1.f) * 0.78539816f;
    gl = std::cos (a) * 1.41421356f;
    gr = std::sin (a) * 1.41421356f;
}

// ---------------------------------------------------------------- filtro ladder de 4 polos con salidas multimodo
// type: 0 LP24, 1 LP12, 2 BP, 3 HP.  s[4] = estado. La realimentacion esta acotada (nunca explota).
inline float ladderTick (float x, float* s, float g, float k, int type) noexcept
{
    const float u = clip3 (x - k * s[3]);
    s[0] += g * (u - s[0]);
    s[1] += g * (s[0] - s[1]);
    s[2] += g * (s[1] - s[2]);
    s[3] += g * (s[2] - s[3]);
    switch (type)
    {
        case 1:  return s[1];
        case 2:  return (s[0] - s[1]) * 2.2f;
        case 3:  return x - 2.f * s[0] + s[1];
        default: return s[3];
    }
}

// ---------------------------------------------------------------- chorus estereo (cada canal con su modulacion, en espejo)
struct StereoChorus
{
    std::vector<float> bl, br;
    int mask = 0, w = 0, lastMode = 0;
    float ph = 0.f, sr = 44100.f;

    void prepare (double s)
    {
        sr = (float) s;
        int sz = 1;
        while (sz < (int) (s * 0.03) + 4) sz <<= 1;
        bl.assign ((size_t) sz, 0.f); br.assign ((size_t) sz, 0.f);
        mask = sz - 1; w = 0; ph = 0.f; lastMode = 0;
    }

    void clear() { std::fill (bl.begin(), bl.end(), 0.f); std::fill (br.begin(), br.end(), 0.f); lastMode = 0; }

    void process (float* L, float* R, int n, int mode) noexcept
    {
        if (mode == 0) { lastMode = 0; return; }
        if (lastMode == 0) { std::fill (bl.begin(), bl.end(), 0.f); std::fill (br.begin(), br.end(), 0.f); }
        lastMode = mode;

        static const float rate[4]  { 0.f, 0.51f, 0.86f, 9.5f };
        static const float depth[4] { 0.f, 1.0f,  1.4f,  0.2f };
        static const float base[4]  { 0.f, 3.4f,  3.4f,  2.2f };
        const float inc = rate[mode] / sr, k = 0.001f * sr;

        auto rd = [this] (const std::vector<float>& b, float d)
        {
            const float pos = (float) w - d;
            const int i0 = (int) std::floor (pos);
            const float fr = pos - (float) i0;
            return b[(size_t) (i0 & mask)] * (1.f - fr) + b[(size_t) ((i0 + 1) & mask)] * fr;
        };

        for (int i = 0; i < n; ++i)
        {
            bl[(size_t) w] = L[i]; br[(size_t) w] = R[i];
            ph += inc; if (ph >= 1.f) ph -= 1.f;
            const float tri = 4.f * std::abs (ph - 0.5f) - 1.f;
            const float dl = (base[mode] + depth[mode] * tri) * k;
            const float dr = (base[mode] - depth[mode] * tri) * k;
            L[i] = (L[i] + rd (bl, dl)) * 0.6f;
            R[i] = (R[i] + rd (br, dr)) * 0.6f;
            w = (w + 1) & mask;
        }
    }
};

// ---------------------------------------------------------------- delay ping-pong sincronizado (hasta 2 s)
struct StereoDelay
{
    std::vector<float> bl, br;
    int size = 0, w = 0;
    float cur = 0.f, lpL = 0.f, lpR = 0.f;
    bool wasOff = true;

    void prepare (double sr)
    {
        size = (int) (sr * 2.2) + 8;
        bl.assign ((size_t) size, 0.f); br.assign ((size_t) size, 0.f);
        w = 0; cur = 0.f; lpL = lpR = 0.f; wasOff = true;
    }

    void clear() { std::fill (bl.begin(), bl.end(), 0.f); std::fill (br.begin(), br.end(), 0.f); lpL = lpR = 0.f; wasOff = true; }

    void process (float* L, float* R, int n, float target, float fb, float mix) noexcept
    {
        if (mix < 0.002f) { wasOff = true; return; }
        if (wasOff) { clear(); cur = target; wasOff = false; }

        for (int i = 0; i < n; ++i)
        {
            cur += (target - cur) * 0.0008f;                         // el tiempo se desliza (sin clicks al cambiar tempo)
            float rp = (float) w - cur; if (rp < 0.f) rp += (float) size;
            const int i0 = (int) rp; const float fr = rp - (float) i0;
            int i1 = i0 + 1; if (i1 >= size) i1 -= size;
            const float dl = bl[(size_t) i0] * (1.f - fr) + bl[(size_t) i1] * fr;
            const float dr = br[(size_t) i0] * (1.f - fr) + br[(size_t) i1] * fr;
            lpL += 0.55f * (dl - lpL);  lpR += 0.55f * (dr - lpR);   // amortiguacion en la realimentacion
            bl[(size_t) w] = 0.5f * (L[i] + R[i]) + lpR * fb;        // cruzado = ping-pong
            br[(size_t) w] = lpL * fb;
            L[i] += dl * mix;  R[i] += dr * mix;
            if (++w >= size) w = 0;
        }
    }
};

// ---------------------------------------------------------------- osciloscopio: buffer circular sin bloqueos
struct ScopeBuffer
{
    static constexpr int N = 4096;
    float buf[N] {};
    std::atomic<int> pos { 0 };

    void push (float x) noexcept
    {
        const int p = pos.load (std::memory_order_relaxed);
        buf[p & (N - 1)] = x;
        pos.store (p + 1, std::memory_order_release);
    }

    void snapshot (float* dst, int count) const noexcept
    {
        const int end = pos.load (std::memory_order_acquire);
        for (int i = 0; i < count; ++i) dst[i] = buf[(end - count + i) & (N - 1)];
    }
};
