#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Piezas de DSP compartidas por MiniKomodo y PolyKomodo. Todo es barato a proposito:
// sin asignaciones en el hilo de audio, sin oversampling, sin lookahead (latencia 0).

constexpr int kChunk = 16;     // muestras por "tick" de control (envolventes, filtro, LFO)

// ---------------------------------------------------------------- envolvente (control rate)
struct EnvP { float atk = 1.f, dec = 0.1f, sus = 1.f, rel = 0.1f; };

struct Env
{
    int st = 0;            // 0 idle, 1 attack, 2 decay, 3 sustain, 4 release
    float lv = 0.f;

    void on() noexcept   { st = 1; }
    void off() noexcept  { if (st != 0) st = 4; }
    void kill() noexcept { st = 0; lv = 0.f; }

    float next (const EnvP& p) noexcept
    {
        switch (st)
        {
            case 1:  lv += p.atk; if (lv >= 1.f) { lv = 1.f; st = 2; } break;
            case 2:  lv += (p.sus - lv) * p.dec; if (lv - p.sus < 0.0005f) st = 3; break;
            case 3:  lv = p.sus; break;
            case 4:  lv -= lv * p.rel; if (lv < 0.0004f) { lv = 0.f; st = 0; } break;
            default: lv = 0.f; break;
        }
        return lv;
    }
};

// tiempos en segundos; tickRate = sampleRate / kChunk
inline EnvP makeEnvSec (float a, float d, float s, float r, float tickRate)
{
    EnvP e;
    e.atk = 1.f / (a * tickRate);
    e.dec = 1.f - std::exp (-4.6f / (d * tickRate));
    e.sus = s;
    e.rel = 1.f - std::exp (-4.6f / (r * tickRate));
    return e;
}

// ---------------------------------------------------------------- osciladores / saturacion
inline float blep (float t, float dt) noexcept             // PolyBLEP: reduce aliasing
{
    if (t < dt)        { t /= dt;            return t + t - t * t - 1.f; }
    if (t > 1.f - dt)  { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

inline float clip3 (float x) noexcept                      // saturacion suave y acotada, sin divisiones
{
    x = juce::jlimit (-1.5f, 1.5f, x);
    return x - 0.148148f * x * x * x;
}

inline float xorNoise (unsigned& s) noexcept               // ruido blanco -1..1, barato
{
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float) (int) s * (1.f / 2147483648.f);
}

// ---------------------------------------------------------------- asignacion de voces (polifonia)
// V necesita: note, released, age, amp (Env), free()
template <class V, int N>
V* allocateVoice (V (&voices)[N], int note) noexcept
{
    for (auto& v : voices) if (v.note == note && ! v.free()) return &v;     // misma nota: reutiliza
    for (auto& v : voices) if (v.free()) return &v;                         // voz libre
    V* best = nullptr;                                                      // robar: la que ya esta en release y mas baja
    for (auto& v : voices)
        if (v.released && (best == nullptr || v.amp.lv < best->amp.lv)) best = &v;
    if (best) return best;
    best = &voices[0];                                                      // si no, la mas antigua
    for (auto& v : voices) if (v.age < best->age) best = &v;
    return best;
}


// ---------------------------------------------------------------- arpegiador (solo genera notas MIDI: costo ~0)
struct Arp
{
    int mode = 0;                       // 0 off, 1 up, 2 down, 3 up/down, 4 random, 5 played
    int octaves = 1;
    double stepLen = 5000.0, gateLen = 3000.0, toNext = 0.0;
    int held[16] {}, nHeld = 0, seq[64] {}, seqLen = 0, idx = 0, dir = 1, playing = -1;
    bool gateOn = false;
    float vel = 1.f;
    unsigned rnd = 12345u;

    void setTiming (double bpm, int rateIdx, float gate, double sr) noexcept
    {
        static const double beats[6] { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
        stepLen = juce::jmax (8.0, sr * 60.0 / juce::jmax (20.0, bpm) * beats[juce::jlimit (0, 5, rateIdx)]);
        gateLen = juce::jlimit (1.0, stepLen - 1.0, stepLen * (double) gate);
    }

    void rebuild() noexcept
    {
        int tmp[16];
        for (int i = 0; i < nHeld; ++i) tmp[i] = held[i];
        if (mode != 5) std::sort (tmp, tmp + nHeld);
        seqLen = 0;
        for (int o = 0; o < octaves; ++o)
            for (int i = 0; i < nHeld; ++i)
            {
                const int n = tmp[i] + 12 * o;
                if (n <= 127 && seqLen < 64) seq[seqLen++] = n;
            }
        if (mode == 2) std::reverse (seq, seq + seqLen);
        if (idx >= seqLen) idx = 0;
    }

    void noteOn (int n) noexcept
    {
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) return;
        const bool first = nHeld == 0;
        if (nHeld < 16) held[nHeld++] = n;
        rebuild();
        if (first) { idx = 0; dir = 1; toNext = 0.0; }          // arranca de inmediato con la primera nota
    }

    template <class F> void noteOff (int n, F&& send) noexcept
    {
        for (int i = 0; i < nHeld; ++i)
            if (held[i] == n) { for (int k = i; k < nHeld - 1; ++k) held[k] = held[k + 1]; --nHeld; break; }
        rebuild();
        if (nHeld == 0 && gateOn) { send (false, playing); gateOn = false; playing = -1; }
    }

    template <class F> void stop (F&& send) noexcept
    {
        if (gateOn) send (false, playing);
        gateOn = false; playing = -1; nHeld = 0; seqLen = 0; idx = 0; dir = 1;
    }

    int take() noexcept
    {
        if (seqLen == 0) return -1;
        if (idx < 0 || idx >= seqLen) idx = 0;
        const int note = seq[idx];
        if (mode == 4) { rnd ^= rnd << 13; rnd ^= rnd >> 17; rnd ^= rnd << 5; idx = (int) ((rnd >> 8) % (unsigned) seqLen); }
        else if (mode == 3 && seqLen > 1)
        {
            idx += dir;
            if (idx >= seqLen)     { idx = seqLen - 2; dir = -1; }
            else if (idx < 0)      { idx = 1; dir = 1; }
        }
        else idx = (idx + 1) % seqLen;
        return note;
    }

    // dispara los eventos que ya tocan; send(bool on, int note)
    template <class F> void fire (F&& send) noexcept
    {
        while (toNext < 1.0)
        {
            if (! gateOn)
            {
                const int n = take();
                if (n < 0) { toNext += stepLen; continue; }
                send (true, n);
                playing = n; gateOn = true; toNext += gateLen;
            }
            else
            {
                send (false, playing);
                playing = -1; gateOn = false; toNext += stepLen - gateLen;
            }
        }
    }

    int maxChunk() const noexcept { return juce::jmax (1, (int) std::floor (toNext)); }
    void advance (int n) noexcept { toNext -= (double) n; }
};

// ---------------------------------------------------------------- chorus (linea de retardo corta)
struct Chorus
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    float ph = 0.f, sr = 44100.f;
    int lastMode = 0;

    void prepare (double s)
    {
        sr = (float) s;
        int sz = 1;
        while (sz < (int) (s * 0.03) + 4) sz <<= 1;
        buf.assign ((size_t) sz, 0.f);
        mask = sz - 1; w = 0; ph = 0.f; lastMode = 0;
    }

    // mode: 0 off, 1 = I, 2 = II, 3 = I+II (rapido y corto)
    void process (const float* in, float* L, float* R, int n, int mode) noexcept
    {
        if (mode == 0)
        {
            for (int i = 0; i < n; ++i) L[i] = R[i] = in[i];
            lastMode = 0;
            return;
        }
        if (lastMode == 0) std::fill (buf.begin(), buf.end(), 0.f);
        lastMode = mode;

        static const float rate[4]  { 0.f, 0.51f, 0.86f, 9.5f };
        static const float depth[4] { 0.f, 1.0f,  1.4f,  0.2f };
        static const float base[4]  { 0.f, 3.4f,  3.4f,  2.2f };
        const float inc = rate[mode] / sr, k = 0.001f * sr;

        for (int i = 0; i < n; ++i)
        {
            buf[(size_t) w] = in[i];
            ph += inc; if (ph >= 1.f) ph -= 1.f;
            const float tri = 4.f * std::abs (ph - 0.5f) - 1.f;
            const float dl = (base[mode] + depth[mode] * tri) * k;
            const float dr = (base[mode] - depth[mode] * tri) * k;

            auto rd = [this] (float d)
            {
                const float pos = (float) w - d;
                const int i0 = (int) std::floor (pos);
                const float fr = pos - (float) i0;
                return buf[(size_t) (i0 & mask)] * (1.f - fr) + buf[(size_t) ((i0 + 1) & mask)] * fr;
            };
            L[i] = (in[i] + rd (dl)) * 0.6f;
            R[i] = (in[i] + rd (dr)) * 0.6f;
            w = (w + 1) & mask;
        }
    }
};
