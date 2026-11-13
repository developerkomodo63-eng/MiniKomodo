#pragma once
#include "Common/KomodoFx.h"

// Piezas de DSP de KeysKomodo: tabla de seno, ensemble, phaser, tremolo (suitcase) y rotary (Leslie).
// Todas reciben el audio mono en L (R se ignora de entrada) y escriben estereo en L/R.

struct SinTab
{
    float t[2050];
    SinTab() { for (int i = 0; i < 2050; ++i) t[i] = (float) std::sin (6.283185307179586 * i / 2048.0); }
};
inline const float* sinTable() { static const SinTab tab; return tab.t; }

inline float wrap1 (float x) noexcept { const float f = x - (float) (int) x; return f < 0.f ? f + 1.f : f; }

inline float sinLU (const float* t, float x) noexcept        // seno con x en ciclos (cualquier valor real)
{
    const float u = wrap1 (x) * 2048.f;
    const int i = (int) u;
    const float fr = u - (float) i;
    return t[i] + (t[i + 1] - t[i]) * fr;
}

// ---------------------------------------------------------------- ensemble de string machine (3 taps con LFO lento + rapido)
struct Ensemble
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    float sr = 44100.f, ph1 = 0.f, ph2 = 0.f;

    void prepare (double s)
    {
        sr = (float) s;
        int sz = 1;
        while (sz < (int) (s * 0.03) + 8) sz <<= 1;
        buf.assign ((size_t) sz, 0.f); mask = sz - 1; w = 0; ph1 = ph2 = 0.f;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.f); }

    void process (float* L, float* R, int n, float depth, float rateMul) noexcept
    {
        const float* T = sinTable();
        const float inc1 = 0.55f * rateMul / sr, inc2 = 6.3f * rateMul / sr;
        for (int i = 0; i < n; ++i)
        {
            const float x = L[i];
            buf[(size_t) w] = x;
            ph1 = wrap1 (ph1 + inc1);  ph2 = wrap1 (ph2 + inc2);
            float y[3];
            for (int t = 0; t < 3; ++t)
            {
                const float off = (float) t * (1.f / 3.f);
                const float m = 0.0035f * sinLU (T, ph1 + off) + 0.0007f * sinLU (T, ph2 + off);
                const float d = juce::jmax (2.f, (0.006f + depth * m) * sr);
                float p = (float) w - d + (float) (mask + 1);
                const int i0 = (int) p;
                const float fr = p - (float) i0;
                y[t] = buf[(size_t) (i0 & mask)] * (1.f - fr) + buf[(size_t) ((i0 + 1) & mask)] * fr;
            }
            L[i] = (x + y[0] + 0.5f * y[1]) * 0.4f;
            R[i] = (x + y[2] + 0.5f * y[1]) * 0.4f;
            w = (w + 1) & mask;
        }
    }
};

// ---------------------------------------------------------------- phaser de 4 etapas, estereo (LFO desfasado entre canales)
struct Phaser
{
    float zl[4] {}, zr[4] {}, fbL = 0.f, fbR = 0.f, ph = 0.f, sr = 44100.f, aL = 0.5f, aR = 0.5f;

    void prepare (double s) { sr = (float) s; clear(); }
    void clear() { for (int k = 0; k < 4; ++k) zl[k] = zr[k] = 0.f; fbL = fbR = 0.f; ph = 0.f; }

    static float coef (float lfoPh, float oct, float sr) noexcept
    {
        const float tri = 4.f * std::abs (wrap1 (lfoPh) - 0.5f) - 1.f;          // -1..1
        const float f = 250.f * std::exp2 ((0.5f + 0.5f * tri) * oct);
        const float x = 3.14159265f * juce::jmin (f, 0.2f * sr) / sr;
        return (1.f - x) / (1.f + x);
    }

    void process (float* L, float* R, int n, float rate, float depth) noexcept
    {
        const float inc = rate / sr, oct = 1.f + depth * 3.f;
        for (int i = 0; i < n; ++i)
        {
            if ((i & 15) == 0) { ph = wrap1 (ph + inc * 16.f); aL = coef (ph, oct, sr); aR = coef (ph + 0.25f, oct, sr); }
            const float x = L[i];
            float inL = x + fbL * 0.55f, inR = x + fbR * 0.55f;
            for (int k = 0; k < 4; ++k)
            {
                const float yl = aL * inL + zl[k];  zl[k] = inL - aL * yl;  inL = yl;
                const float yr = aR * inR + zr[k];  zr[k] = inR - aR * yr;  inR = yr;
            }
            fbL = inL; fbR = inR;
            L[i] = 0.5f * (x + inL);
            R[i] = 0.5f * (x + inR);
        }
    }
};

// ---------------------------------------------------------------- tremolo estereo tipo "suitcase" (L y R en oposicion de fase)
struct Tremolo
{
    float ph = 0.f, sr = 44100.f;
    void prepare (double s) { sr = (float) s; ph = 0.f; }
    void process (float* L, float* R, int n, float rate, float depth) noexcept
    {
        const float* T = sinTable();
        const float inc = rate / sr, d = depth * 0.85f;
        for (int i = 0; i < n; ++i)
        {
            ph = wrap1 (ph + inc);
            const float s = sinLU (T, ph), x = L[i];
            L[i] = x * (1.f + d * s);
            R[i] = x * (1.f - d * s);
        }
    }
};

// ---------------------------------------------------------------- rotary (Leslie): graves = tambor lento, agudos = bocina rapida, con doppler
struct Rotary
{
    std::vector<float> bh, bd;
    int mask = 0, w = 0;
    float sr = 44100.f, lp = 0.f, ph = 0.f, pd = 0.f, rh = 0.7f, rd = 0.6f;

    void prepare (double s)
    {
        sr = (float) s;
        int sz = 1;
        while (sz < (int) (s * 0.004) + 8) sz <<= 1;
        bh.assign ((size_t) sz, 0.f); bd.assign ((size_t) sz, 0.f); mask = sz - 1; w = 0;
        lp = 0.f; ph = pd = 0.f; rh = 0.7f; rd = 0.6f;
    }
    void clear() { std::fill (bh.begin(), bh.end(), 0.f); std::fill (bd.begin(), bd.end(), 0.f); lp = 0.f; }

    void process (float* L, float* R, int n, bool fast, float depth) noexcept
    {
        const float* T = sinTable();
        const float tH = fast ? 6.6f : 0.7f, tD = fast ? 5.8f : 0.6f;                    // el rotor acelera y frena de a poco
        const float cH = 1.f - std::exp (-1.f / (sr * 0.9f)), cD = 1.f - std::exp (-1.f / (sr * 2.5f));
        const float lpc = 1.f - std::exp (-6.2831853f * 800.f / sr);
        const float dmH = 0.0007f * sr * depth, dmD = 0.0011f * sr * depth;
        const float aH = 0.55f * depth, aD = 0.3f * depth;

        auto rdl = [this] (const std::vector<float>& b, float d)
        {
            float p = (float) w - d + (float) (mask + 1);
            const int i0 = (int) p;
            const float fr = p - (float) i0;
            return b[(size_t) (i0 & mask)] * (1.f - fr) + b[(size_t) ((i0 + 1) & mask)] * fr;
        };

        for (int i = 0; i < n; ++i)
        {
            const float x = L[i];
            lp += lpc * (x - lp);
            bd[(size_t) w] = lp;  bh[(size_t) w] = x - lp;
            rh += (tH - rh) * cH;  rd += (tD - rd) * cD;
            ph = wrap1 (ph + rh / sr);  pd = wrap1 (pd + rd / sr);

            const float sHL = sinLU (T, ph),        sHR = sinLU (T, ph + 0.25f);
            const float sDL = sinLU (T, pd),        sDR = sinLU (T, pd + 0.25f);
            const float hl = rdl (bh, 2.f + dmH * (0.5f + 0.5f * sHL)) * (1.f + aH * sHL);
            const float hr = rdl (bh, 2.f + dmH * (0.5f + 0.5f * sHR)) * (1.f + aH * sHR);
            const float dl = rdl (bd, 2.f + dmD * (0.5f + 0.5f * sDL)) * (1.f + aD * sDL);
            const float dr = rdl (bd, 2.f + dmD * (0.5f + 0.5f * sDR)) * (1.f + aD * sDR);
            L[i] = (hl + dl) * 0.95f;
            R[i] = (hr + dr) * 0.95f;
            w = (w + 1) & mask;
        }
    }
};
