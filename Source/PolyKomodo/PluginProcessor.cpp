#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// valores por defecto, en el mismo orden que el enum
const float kDefault[NP] = {
    1.f, .8f, 0.f, .5f, 0.f, .3f, 0.f, .3f, 0.f,          // RNG SAW PUL PW PWM SUB NOI LR LP
    2000.f, .2f, .4f, .5f, 0.f, 0.f, .6f,                  // CUT RES ENV KEY LFF GL VOL
    .01f, .35f, .3f, .25f, .01f, .3f, .8f, .25f,           // FA FD FS FR AA AD AS AR
    0.f, 0.f, 0.f,                                         // HPF MODE CHO
    0.f, 3.f, 0.f, .6f, .15f, .35f                         // ARP ARR ARO ARG DRF VEL
#if KOMODO_FULL
    , 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, .3f, 0.f, .3f, 0.f,    // FTY FDR O2L O2D O2R UNI UND SPR L2R L2S
    0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,   // matriz 4 x (fuente, destino, cantidad)
    0.f, 3.f, 0.f, .35f, 0.f, .5f, .5f                     // DRV DLT DLM DLF RVM RVS RVD
#endif
    };

// texto de los parametros: porcentaje, Hz/kHz o cents (en vez de 7 decimales)
juce::AudioParameterFloatAttributes attrs (int kind)
{
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([kind] (float v, int)
        {
            if (kind == 1) return v >= 1000.f ? juce::String (v / 1000.f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
            if (kind == 2) return juce::String (juce::roundToInt (v)) + " ct";
            return juce::String (juce::roundToInt (v * 100.f)) + "%";
        })
        .withValueFromStringFunction ([kind] (const juce::String& text)
        {
            const auto t = text.trim();
            const float x = t.getFloatValue();
            if (kind == 0) return x / 100.f;
            return (kind == 1 && t.containsIgnoreCase ("k")) ? x * 1000.f : x;
        });
}

float T (float x) { return 0.001f + x * x * x * 8.f; }     // 0..1 -> segundos

#if KOMODO_FULL
inline float lfoShape (int shape, float ph, float sh) noexcept
{
    switch (shape)
    {
        case 0:  return 4.f * std::abs (ph - 0.5f) - 1.f;      // triangulo
        case 1:  return 1.f - 2.f * ph;                        // sierra descendente
        case 2:  return ph < 0.5f ? 1.f : -1.f;                // cuadrada
        default: return sh;                                    // sample & hold
    }
}
#endif

EnvP makeEnv (float a, float d, float s, float r, float tickRate)
{
    return makeEnvSec (T (a), T (d), s, T (r), tickRate);
}

struct Ov { int id; float v; };
struct Preset { const char* name; std::vector<Ov> ov; };

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", {} },
        // ---- Bass
        { "Bass Sub",       { {RNG,0}, {SAW,.5f}, {SUB,.9f}, {CUT,450}, {RES,.1f}, {ENV,.3f}, {KEY,.3f}, {FD,.2f}, {FS,.1f}, {FR,.08f}, {AA,0}, {AD,.25f}, {AS,.85f}, {AR,.08f}, {MODE,1}, {VOL,0.818f} } },
        { "Bass Pluck",     { {RNG,1}, {SAW,.9f}, {PUL,.3f}, {SUB,.5f}, {CUT,320}, {RES,.3f}, {ENV,.75f}, {KEY,.4f}, {FD,.18f}, {FS,0}, {FR,.1f}, {AD,.2f}, {AS,.2f}, {AR,.1f}, {MODE,1}, {VOL,1.000f} } },
        { "Bass Resonant",  { {RNG,0}, {SAW,1}, {SUB,.5f}, {CUT,280}, {RES,.65f}, {ENV,.8f}, {KEY,.3f}, {FD,.25f}, {FS,.1f}, {FR,.1f}, {AS,.9f}, {AR,.08f}, {GL,.1f}, {MODE,1}, {VOL,0.778f} } },
        { "Bass Square",    { {RNG,0}, {SAW,0}, {PUL,1}, {PW,.5f}, {SUB,.5f}, {CUT,700}, {RES,.2f}, {ENV,.3f}, {FD,.3f}, {FS,.4f}, {AR,.08f}, {MODE,1}, {VOL,0.793f} } },
        { "Bass Wide",      { {RNG,0}, {SAW,.7f}, {SUB,.6f}, {CUT,650}, {RES,.2f}, {ENV,.3f}, {KEY,.4f}, {FD,.3f}, {FS,.4f}, {AR,.1f}, {CHO,1}, {VOL,0.647f} } },
        // ---- Lead
        { "Lead Saw",       { {RNG,1}, {SAW,1}, {SUB,0}, {CUT,3500}, {RES,.3f}, {ENV,.3f}, {KEY,.8f}, {FD,.4f}, {FS,.6f}, {AS,.9f}, {AR,.15f}, {GL,.1f}, {LP,.05f}, {LR,.45f}, {MODE,1}, {VOL,0.968f} } },
        { "Lead PWM",       { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.5f}, {PWM,.6f}, {LR,.3f}, {SUB,.2f}, {CUT,4500}, {RES,.2f}, {ENV,.2f}, {KEY,.8f}, {AS,.9f}, {AR,.15f}, {CHO,1}, {MODE,1}, {VOL,0.837f} } },
        { "Lead Bright",    { {RNG,1}, {SAW,.8f}, {PUL,.6f}, {PW,.45f}, {SUB,0}, {CUT,6000}, {RES,.45f}, {ENV,.5f}, {KEY,.8f}, {FD,.3f}, {FS,.5f}, {AS,.9f}, {AR,.15f}, {GL,.05f}, {MODE,1}, {VOL,1.000f} } },
        { "Lead Soft",      { {RNG,1}, {SAW,.5f}, {PUL,.5f}, {PW,.35f}, {SUB,.1f}, {CUT,2500}, {RES,.15f}, {ENV,.25f}, {KEY,.7f}, {AA,.05f}, {AS,.9f}, {AR,.2f}, {LP,.1f}, {LR,.5f}, {GL,.15f}, {CHO,2}, {MODE,1}, {VOL,1.000f} } },
        { "Lead Octave",    { {RNG,1}, {SAW,1}, {SUB,.8f}, {CUT,3000}, {RES,.25f}, {ENV,.3f}, {KEY,.7f}, {AS,.9f}, {AR,.12f}, {GL,.05f}, {MODE,1}, {VOL,0.730f} } },
        // ---- Pad
        { "Pad Warm",       { {RNG,1}, {SAW,.7f}, {PUL,.5f}, {PW,.5f}, {PWM,.5f}, {LR,.15f}, {SUB,.2f}, {CUT,1400}, {RES,.15f}, {ENV,.2f}, {FA,.35f}, {FD,.5f}, {FS,.7f}, {FR,.55f}, {AA,.45f}, {AD,.5f}, {AS,.85f}, {AR,.6f}, {CHO,2}, {VOL,0.926f} } },
        { "Pad Strings",    { {RNG,1}, {SAW,1}, {SUB,0}, {CUT,2200}, {RES,.1f}, {ENV,.15f}, {FA,.4f}, {FD,.5f}, {FS,.8f}, {FR,.5f}, {AA,.4f}, {AD,.5f}, {AS,.9f}, {AR,.5f}, {HPF,1}, {CHO,2}, {VOL,0.596f} } },
        { "Pad Glass",      { {RNG,2}, {SAW,0}, {PUL,1}, {PW,.4f}, {PWM,.8f}, {LR,.2f}, {SUB,0}, {CUT,5000}, {RES,.3f}, {ENV,.1f}, {FA,.3f}, {FS,.8f}, {AA,.3f}, {AS,.9f}, {AR,.6f}, {HPF,1}, {CHO,1}, {VOL,0.538f} } },
        { "Pad Dark",       { {RNG,1}, {SAW,.6f}, {PUL,.6f}, {SUB,.6f}, {CUT,600}, {RES,.25f}, {ENV,.5f}, {FA,.6f}, {FD,.6f}, {FS,.5f}, {FR,.6f}, {AA,.5f}, {AS,.9f}, {AR,.65f}, {CHO,2}, {VOL,0.680f} } },
        { "Pad Air",        { {RNG,1}, {SAW,.5f}, {PUL,.5f}, {PW,.3f}, {PWM,.3f}, {NOI,.15f}, {CUT,1800}, {RES,.35f}, {ENV,.3f}, {FA,.5f}, {FD,.6f}, {FS,.6f}, {FR,.6f}, {AA,.5f}, {AS,.9f}, {AR,.7f}, {LR,.2f}, {CHO,1}, {VOL,0.926f} } },
        // ---- Brass
        { "Brass Classic",  { {RNG,1}, {SAW,1}, {SUB,.3f}, {CUT,700}, {RES,.15f}, {ENV,.75f}, {KEY,.5f}, {FA,.06f}, {FD,.35f}, {FS,.6f}, {FR,.12f}, {AA,.03f}, {AD,.3f}, {AS,.9f}, {AR,.1f}, {VOL,0.566f} } },
        { "Brass Section",  { {SAW,.9f}, {PUL,.4f}, {PW,.5f}, {SUB,.2f}, {CUT,800}, {RES,.15f}, {ENV,.7f}, {FA,.1f}, {FD,.35f}, {FS,.6f}, {FR,.15f}, {AA,.05f}, {AS,.9f}, {AR,.12f}, {CHO,1}, {VOL,0.812f} } },
        { "Brass Stab",     { {SAW,1}, {SUB,.2f}, {CUT,600}, {RES,.2f}, {ENV,.85f}, {FA,0}, {FD,.18f}, {FS,.3f}, {FR,.1f}, {AA,0}, {AD,.25f}, {AS,.6f}, {AR,.08f}, {VOL,0.634f} } },
        { "Brass Soft",     { {SAW,.8f}, {SUB,.2f}, {CUT,500}, {RES,.1f}, {ENV,.6f}, {FA,.15f}, {FD,.4f}, {FS,.6f}, {FR,.2f}, {AA,.1f}, {AS,.9f}, {AR,.15f}, {CHO,2}, {VOL,0.692f} } },
        { "Arp Pluck",      { {RNG,1}, {SAW,.8f}, {PUL,.4f}, {PW,.5f}, {CUT,1200}, {RES,.45f}, {ENV,.85f}, {KEY,.5f}, {FD,.15f}, {FS,0}, {FR,.1f}, {AA,0}, {AD,.15f}, {AS,0}, {AR,.1f}, {CHO,1}, {ARP,1}, {ARR,3}, {ARO,1}, {ARG,.5f} } },
        { "Arp Glass",      { {RNG,2}, {SAW,0}, {PUL,.8f}, {PW,.3f}, {PWM,.5f}, {LR,.5f}, {HPF,1}, {CUT,7000}, {RES,.2f}, {ENV,.3f}, {KEY,.9f}, {FD,.4f}, {FS,0}, {AA,0}, {AD,.4f}, {AS,.1f}, {AR,.4f}, {CHO,2}, {ARP,3}, {ARR,4}, {ARO,2}, {ARG,.6f} } },
        { "Brass Synth",    { {SAW,.8f}, {PUL,.6f}, {PW,.45f}, {PWM,.2f}, {SUB,.2f}, {CUT,1000}, {RES,.3f}, {ENV,.7f}, {FA,.1f}, {FD,.4f}, {FS,.5f}, {FR,.15f}, {AA,.04f}, {AS,.9f}, {AR,.12f}, {HPF,1}, {CHO,1}, {VOL,0.872f} } },
        // ---- Keys
        { "Keys EP",        { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.35f}, {SUB,.2f}, {CUT,1800}, {RES,.1f}, {ENV,.45f}, {KEY,.7f}, {FD,.35f}, {FS,.15f}, {FR,.3f}, {AA,0}, {AD,.4f}, {AS,.35f}, {AR,.3f}, {CHO,1}, {VOL,0.574f} } },
        { "Keys Piano",     { {RNG,1}, {SAW,.6f}, {PUL,.4f}, {PW,.45f}, {SUB,.2f}, {CUT,2200}, {RES,.1f}, {ENV,.6f}, {KEY,.8f}, {FD,.3f}, {FS,.1f}, {FR,.25f}, {AA,0}, {AD,.45f}, {AS,.15f}, {AR,.25f}, {VOL,0.840f} } },
        { "Keys Clav",      { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.2f}, {SUB,0}, {HPF,2}, {CUT,2500}, {RES,.25f}, {ENV,.6f}, {KEY,.8f}, {FD,.12f}, {FS,0}, {FR,.08f}, {AA,0}, {AD,.15f}, {AS,0}, {AR,.08f}, {VOL,0.694f} } },
        { "Keys Organ",     { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.5f}, {SUB,1}, {CUT,6000}, {RES,0}, {ENV,0}, {KEY,.5f}, {FS,1}, {AA,0}, {AD,0}, {AS,1}, {AR,.05f}, {CHO,3}, {VOL,0.378f} } },
        { "Keys Bell",      { {RNG,2}, {SAW,0}, {PUL,.8f}, {PW,.3f}, {PWM,.5f}, {LR,.5f}, {SUB,0}, {HPF,1}, {CUT,7000}, {RES,.2f}, {ENV,.3f}, {KEY,.9f}, {FD,.4f}, {FS,0}, {AA,0}, {AD,.5f}, {AS,.1f}, {AR,.5f}, {CHO,1}, {VOL,0.614f} } },
        { "Keys Rhodes",    { {RNG,1}, {SAW,.2f}, {PUL,.8f}, {PW,.4f}, {SUB,.3f}, {CUT,1400}, {RES,.15f}, {ENV,.4f}, {KEY,.7f}, {FD,.45f}, {FS,.2f}, {FR,.35f}, {AA,0}, {AD,.5f}, {AS,.4f}, {AR,.35f}, {LR,.25f}, {PWM,.15f}, {CHO,2}, {VOL,0.630f} } },
        // ---- Pluck
        { "Pluck Classic",  { {RNG,1}, {SAW,1}, {SUB,.2f}, {CUT,700}, {RES,.3f}, {ENV,.8f}, {KEY,.6f}, {FD,.2f}, {FS,0}, {FR,.15f}, {AA,0}, {AD,.22f}, {AS,0}, {AR,.15f}, {VOL,0.853f} } },
        { "Pluck Pulse",    { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.3f}, {CUT,900}, {RES,.35f}, {ENV,.75f}, {KEY,.6f}, {FD,.18f}, {FS,0}, {FR,.12f}, {AA,0}, {AD,.2f}, {AS,0}, {AR,.12f}, {CHO,1}, {VOL,0.666f} } },
        { "Pluck Harp",     { {RNG,1}, {SAW,.5f}, {PUL,.5f}, {PW,.4f}, {HPF,1}, {CUT,1500}, {RES,.15f}, {ENV,.6f}, {KEY,.8f}, {FD,.35f}, {FS,0}, {FR,.3f}, {AA,0}, {AD,.5f}, {AS,0}, {AR,.4f}, {CHO,2}, {VOL,0.930f} } },
        { "Pluck Marimba",  { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.5f}, {SUB,.3f}, {CUT,600}, {RES,.2f}, {ENV,.7f}, {KEY,.9f}, {FD,.15f}, {FS,0}, {FR,.12f}, {AA,0}, {AD,.18f}, {AS,0}, {AR,.12f}, {VOL,0.509f} } },
        { "Pluck Arp",      { {RNG,1}, {SAW,.8f}, {PUL,.4f}, {PW,.5f}, {CUT,1200}, {RES,.45f}, {ENV,.85f}, {KEY,.5f}, {FD,.15f}, {FS,0}, {FR,.1f}, {AA,0}, {AD,.15f}, {AS,0}, {AR,.1f}, {CHO,1}, {VOL,1.000f} } },
        // ---- Extra
        { "Bass Acid",      { {RNG,0}, {SAW,1}, {SUB,0}, {CUT,250}, {RES,.8f}, {ENV,.85f}, {KEY,.2f}, {FD,.22f}, {FS,0}, {FR,.08f}, {AA,0}, {AD,.25f}, {AS,.6f}, {AR,.08f}, {GL,.1f}, {MODE,1}, {VOL,0.985f} } },
        { "Lead Prophet",   { {RNG,1}, {SAW,.8f}, {PUL,.5f}, {PW,.45f}, {SUB,0}, {CUT,3000}, {RES,.3f}, {ENV,.4f}, {KEY,.8f}, {FD,.4f}, {FS,.6f}, {AS,.9f}, {AR,.15f}, {GL,.12f}, {LP,.06f}, {LR,.5f}, {MODE,1}, {VOL,1.000f} } },
        { "Poly Stack",     { {RNG,1}, {SAW,.7f}, {PUL,.7f}, {PW,.5f}, {PWM,.35f}, {LR,.2f}, {SUB,.3f}, {CUT,1800}, {RES,.15f}, {ENV,.25f}, {FA,.02f}, {FD,.4f}, {FS,.6f}, {FR,.3f}, {AA,.02f}, {AD,.4f}, {AS,.8f}, {AR,.3f}, {CHO,2}, {VOL,0.757f} } },
#if KOMODO_FULL
        // ---- Solo version completa (matriz: fuentes 1 LFO1 2 LFO2 3 EnvF 4 EnvV 5 Vel 6 Key 7 Wheel 8 Rand; destinos 1 Pitch 2 Corte 3 Reso 4 Vol 5 Pan 6 PW 7 Detune)
        // UNI: 0=1 1=2 2=3 3=5 4=7 capas;  O2R: 0=-1oct 1=0 2=+5ta 3=+1oct
        { "Super Saw",      { {RNG,1}, {SAW,1}, {CUT,5000}, {RES,.15f}, {ENV,.2f}, {KEY,.7f}, {FD,.4f}, {FS,.7f}, {AS,.9f}, {AR,.25f}, {UNI,4}, {UND,.55f}, {SPR,1}, {CHO,1}, {RVM,.15f}, {VOL,0.567f} } },
        { "Trance Pluck",   { {RNG,1}, {SAW,1}, {CUT,1100}, {RES,.3f}, {ENV,.8f}, {KEY,.6f}, {FD,.18f}, {FS,0}, {FR,.12f}, {AD,.2f}, {AS,0}, {AR,.15f}, {UNI,2}, {UND,.4f}, {SPR,.8f}, {DLM,.3f}, {DLT,2}, {DLF,.4f}, {RVM,.2f}, {VOL,0.628f} } },
        { "Huge Pad",       { {RNG,1}, {SAW,.8f}, {O2L,.7f}, {O2R,3}, {O2D,8}, {SUB,.2f}, {CUT,1500}, {RES,.15f}, {ENV,.2f}, {FA,.4f}, {FD,.5f}, {FS,.7f}, {FR,.55f}, {AA,.5f}, {AD,.5f}, {AS,.9f}, {AR,.65f}, {UNI,3}, {UND,.4f}, {SPR,1}, {CHO,2}, {M1S,2}, {M1D,2}, {M1A,.15f}, {L2R,.15f}, {RVM,.4f}, {RVS,.85f}, {VOL,0.556f} } },
        { "Wobble Bass",    { {RNG,0}, {SAW,1}, {SUB,.6f}, {CUT,300}, {RES,.45f}, {ENV,.1f}, {KEY,.3f}, {FD,.3f}, {FS,.6f}, {AS,.9f}, {AR,.08f}, {UNI,1}, {UND,.2f}, {L2R,.55f}, {M1S,2}, {M1D,2}, {M1A,.6f}, {FDR,.3f}, {DRV,.2f}, {MODE,1}, {VOL,0.530f} } },
        { "Dub Chord",      { {RNG,1}, {SAW,.6f}, {PUL,.5f}, {PW,.4f}, {HPF,1}, {CUT,1800}, {RES,.25f}, {ENV,.5f}, {KEY,.6f}, {FD,.25f}, {FS,.1f}, {AD,.3f}, {AS,.15f}, {AR,.25f}, {UNI,1}, {UND,.25f}, {SPR,.6f}, {DLM,.5f}, {DLT,2}, {DLF,.6f}, {RVM,.3f}, {VOL,0.788f} } },
#endif
        { "Sweep Pad",      { {RNG,1}, {SAW,.8f}, {PUL,.4f}, {PW,.5f}, {SUB,.2f}, {CUT,500}, {RES,.45f}, {ENV,.7f}, {KEY,.4f}, {FA,.7f}, {FD,.7f}, {FS,.3f}, {FR,.6f}, {AA,.5f}, {AS,.9f}, {AR,.7f}, {LFF,.15f}, {LR,.12f}, {CHO,2}, {VOL,0.984f} } },
    };
    return p;
}
} // namespace

// ---------------------------------------------------------------- parametros
juce::AudioProcessorValueTreeState::ParameterLayout PolyKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using R = juce::NormalisableRange<float>;
    auto f = [&] (int i, const juce::String& name, R r)
        { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { paramIds[i], 1 }, name, r, kDefault[i],
                                                                      attrs (i == CUT ? 1 : 0
                                                                           #if KOMODO_FULL
                                                                             + (i == O2D ? 2 : 0)
                                                                           #endif
                                                                             ))); };
    auto c = [&] (int i, const juce::String& name, juce::StringArray s)
        { p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { paramIds[i], 1 }, name, s, (int) kDefault[i])); };
    const R u (0.f, 1.f);

    c (RNG, "Rango", { "16'", "8'", "4'" });
    f (SAW, "Sierra", u);      f (PUL, "Pulso", u);        f (PW, "Ancho", R (0.1f, 0.9f));
    f (PWM, "PWM (LFO)", u);   f (SUB, "Sub", u);          f (NOI, "Ruido", u);
    f (LR, "LFO vel.", u);     f (LP, "LFO tono", u);
    f (CUT, "Corte", R (20.f, 18000.f, 0.f, 0.3f));
    f (RES, "Resonancia", u);  f (ENV, "Cant. env", R (-1.f, 1.f));
    f (KEY, "Keytrack", u);    f (LFF, "LFO filtro", u);   f (GL, "Glide", u);   f (VOL, "Master", u);
    f (FA, "Filtro A", u);     f (FD, "Filtro D", u);      f (FS, "Filtro S", u); f (FR, "Filtro R", u);
    f (AA, "Vol A", u);        f (AD, "Vol D", u);         f (AS, "Vol S", u);    f (AR, "Vol R", u);
    c (HPF, "HPF", { "Off", "100 Hz", "300 Hz", "800 Hz" });
    c (MODE, "Modo", { "Poly", "Mono" });
    c (CHO, "Chorus", { "Off", "I", "II", "I+II" });
    c (ARP, "Arp", { "Off", "Up", "Down", "Up/Down", "Random", "Played" });
    c (ARR, "Arp ritmo", { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" });
    c (ARO, "Arp oct.", { "1 oct", "2 oct", "3 oct" });
    f (ARG, "Arp gate", R (0.1f, 0.95f));  f (DRF, "Drift", u);   f (VEL, "Velocity", u);
   #if KOMODO_FULL
    const R bi (-1.f, 1.f);
    c (FTY, "Filtro tipo", { "LP 24", "LP 12", "BP", "HP" });
    f (FDR, "Filtro drive", u);  f (O2L, "Osc2 nivel", u);  f (O2D, "Osc2 fino", R (-100.f, 100.f));
    c (O2R, "Osc2 semitonos", { "-1 oct", "0", "+5th", "+1 oct" });
    c (UNI, "Unison", { "1", "2", "3", "5", "7" });
    f (UND, "Unison detune", u);  f (SPR, "Stereo", u);  f (L2R, "LFO2 vel.", u);
    c (L2S, "LFO2 onda", { "Triangulo", "Sierra", "Cuadrada", "S&H" });
    const juce::StringArray src { "Off", "LFO 1", "LFO 2", "Env filtro", "Env vol", "Velocity", "Key", "Mod wheel", "Random" },
        dst { "Off", "Pitch", "Corte", "Reso", "Volumen", "Pan", "PW", "Detune" };
    for (int i = 0; i < 4; ++i)
    {
        const auto n = "Mod " + juce::String (i + 1);
        c (M1S + 3 * i, n + " fuente", src);  c (M1D + 3 * i, n + " destino", dst);  f (M1A + 3 * i, n + " cant.", bi);
    }
    f (DRV, "Drive", u);
    c (DLT, "Delay tiempo", { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2" });
    f (DLM, "Delay mix", u);  f (DLF, "Delay fdbk", u);
    f (RVM, "Reverb mix", u); f (RVS, "Reverb size", u);  f (RVD, "Reverb damp", u);
   #endif
    return { p.begin(), p.end() };
}

PolyKomodoAudioProcessor::PolyKomodoAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", layout())
{
    for (int i = 0; i < NP; ++i) pp[i] = apvts.getRawParameterValue (paramIds[i]);
}

bool PolyKomodoAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
   #if KOMODO_FULL
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
   #else
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
   #endif
}

void PolyKomodoAudioProcessor::prepareToPlay (double s, int)
{
    sr = s; invSr = 1.f / (float) s;
    chorus.prepare (s);
   #if KOMODO_FULL
    delay.prepare (s);  reverb.setSampleRate (s);  reverb.reset();  reverbWasOn = false;
    lfo2Ph = 0.f;
   #endif
    for (auto& v : voices) { v = Voice(); }
    nHeld = 0; sustain = false; lfoPh = 0.f; bend = 0.f; arp = Arp(); lastArp = 0;
    lastMode = mode = (int) pp[MODE]->load();
}

// ---------------------------------------------------------------- programas (presets)
int PolyKomodoAudioProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String PolyKomodoAudioProcessor::getProgramName (int i)
{
    const auto& p = presets();
    return (i >= 0 && i < (int) p.size()) ? juce::String (p[(size_t) i].name) : juce::String();
}

void PolyKomodoAudioProcessor::setCurrentProgram (int i)
{
    const auto& all = presets();
    if (i < 0 || i >= (int) all.size() || i == curProg) return;
    curProg = i;
    float val[NP];
    for (int k = 0; k < NP; ++k) val[k] = kDefault[k];
    for (auto& o : all[(size_t) i].ov) val[o.id] = o.v;
    for (int k = 0; k < NP; ++k)
        if (auto* par = apvts.getParameter (paramIds[k]))
            par->setValueNotifyingHost (par->convertTo0to1 (val[k]));
}

void PolyKomodoAudioProcessor::handleAsyncUpdate()
{
    const int p = pendingProgram.exchange (-1);
    if (p >= 0) setCurrentProgram (juce::jmin (p, getNumPrograms() - 1));
}

void PolyKomodoAudioProcessor::doPanic()
{
    for (auto& v : voices) v = Voice();
    nHeld = 0; sustain = false; arp = Arp(); lastArp = 0;
   #if KOMODO_FULL
    chorus.clear(); delay.clear(); reverb.reset();
   #endif
}

// ---------------------------------------------------------------- MIDI / voces
Voice* PolyKomodoAudioProcessor::allocate (int n) { return allocateVoice (voices, n); }

void PolyKomodoAudioProcessor::noteOn (int n, float vel)
{
    const float hz = (float) juce::MidiMessage::getMidiNoteInHertz (n);

    if (mode == 1)                                                        // mono legato, prioridad a la ultima nota
    {
        auto& v = voices[0];
        const bool wasHeld = nHeld > 0;
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { held[i] = held[--nHeld]; break; }
        if (nHeld < 32) held[nHeld++] = n;
        v.tgtF = hz; v.note = n; v.vel = vel; v.drift = xorNoise (rngState); v.released = false; v.age = ++ageCounter;
       #if KOMODO_FULL
        v.rnd = xorNoise (rngState);
       #endif
        if (v.free()) v.curF = hz;                                        // sin glide desde el silencio
        if (! wasHeld) { v.amp.on(); v.flt.on(); }
        return;
    }

    auto* v = allocate (n);
    v->note = n; v->vel = vel; v->drift = xorNoise (rngState); v->tgtF = v->curF = hz;
    v->released = v->sustained = false; v->age = ++ageCounter;
   #if KOMODO_FULL
    v->rnd = xorNoise (rngState);
    for (int j = 0; j < kMaxLay; ++j) { v->ph[j] = std::abs (xorNoise (rngState)); v->ph2[j] = std::abs (xorNoise (rngState)); }   // fases al azar: unison sin "fase pegada"
   #endif
    v->amp.on(); v->flt.on();
}

void PolyKomodoAudioProcessor::noteOff (int n)
{
    if (mode == 1)
    {
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { held[i] = held[--nHeld]; break; }
        // el swap rompe el orden: reconstruimos "ultima nota" como la de mayor edad de llegada no disponible -> usamos la ultima del array
        auto& v = voices[0];
        if (nHeld == 0) { v.amp.off(); v.flt.off(); v.released = true; }
        else { v.note = held[nHeld - 1]; v.tgtF = (float) juce::MidiMessage::getMidiNoteInHertz (v.note); }
        return;
    }
    for (auto& v : voices)
        if (v.note == n && ! v.released && ! v.free())
        {
            if (sustain) v.sustained = true;
            else { v.amp.off(); v.flt.off(); v.released = true; }
        }
}

void PolyKomodoAudioProcessor::releaseAll()
{
    for (auto& v : voices) { v.amp.off(); v.flt.off(); v.released = true; v.sustained = false; }
    nHeld = 0;
}

void PolyKomodoAudioProcessor::handle (const juce::MidiMessage& m)
{
    if (m.isProgramChange())     { pendingProgram.store (m.getProgramChangeNumber()); triggerAsyncUpdate(); }
    else if (m.isNoteOn())       noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())      noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())   bend = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), val = m.getControllerValue();
        if (cc == 1) modWheel = (float) val / 127.f;
        else if (cc == 64)
        {
            sustain = val >= 64;
            if (! sustain)
                for (auto& v : voices)
                    if (v.sustained) { v.amp.off(); v.flt.off(); v.released = true; v.sustained = false; }
        }
        else if (cc == 123 || cc == 120) releaseAll();
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) releaseAll();
}

// ---------------------------------------------------------------- DSP
void PolyKomodoAudioProcessor::readParams()
{
    auto g = [this] (int i) { return pp[i]->load(); };
    mode = (int) g (MODE);  cho = (int) g (CHO);
    lsaw = g (SAW); lpul = g (PUL); lsub = g (SUB); noi = g (NOI) * 0.8f;
    pwBase = g (PW); pwmAmt = g (PWM);
    driftCents = g (DRF) * 12.f;  velS = g (VEL);

    double bpm = 120.0;                                              // arpegiador sincronizado al tempo del host
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    arp.mode = (int) g (ARP);  arp.octaves = 1 + (int) g (ARO);
    arp.setTiming (bpm, (int) g (ARR), g (ARG), sr);
    cut = g (CUT); res = g (RES); envAmt = g (ENV); keyAmt = g (KEY); lffAmt = g (LFF);
    rangeMul = std::exp2 (g (RNG) - 1.f);
   #if KOMODO_FULL
    fty = (int) g (FTY);  fdr = g (FDR);  lo2 = g (O2L);
    static const float o2s[4] { -12.f, 0.f, 7.f, 12.f };
    o2Mul = std::exp2 ((o2s[juce::jlimit (0, 3, (int) g (O2R))] + g (O2D) * 0.01f) * (1.f / 12.f));
    static const int lays[5] { 1, 2, 3, 5, 7 };
    nLay = lays[juce::jlimit (0, 4, (int) g (UNI))];
    und = g (UND);  spr = g (SPR);  dual = nLay > 1 && spr > 0.001f;
    const float norm = 1.f / std::sqrt ((float) nLay);
    for (int j = 0; j < nLay; ++j)
    {
        lyOff[j] = nLay > 1 ? ((float) j / (float) (nLay - 1)) * 2.f - 1.f : 0.f;
        if (dual) { panGains (lyOff[j] * spr, lyGL[j], lyGR[j]); lyGL[j] *= norm; lyGR[j] *= norm; }
        else      lyGL[j] = lyGR[j] = norm;
    }
    l2s = (int) g (L2S);  lfo2Inc = (0.1f + g (L2R) * g (L2R) * 18.f) * invSr;
    for (int i = 0; i < 4; ++i) { msrc[i] = (int) g (M1S + 3 * i); mdst[i] = (int) g (M1D + 3 * i); mamt[i] = g (M1A + 3 * i); }
    drv = g (DRV);  dlm = g (DLM);  dlf = g (DLF) * 0.85f;  rvm = g (RVM);
    static const double beats[6] { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
    dlTarget = (float) juce::jmin (beats[juce::jlimit (0, 5, (int) g (DLT))] * 60.0 / juce::jmax (20.0, bpm) * sr, sr * 2.15);
   #endif
    vibCents = g (LP) * 100.f + modWheel * 60.f;
    lfoInc = 0.1f * std::exp2 (g (LR) * 7.64f) * invSr;

    const float tc = g (GL) * g (GL) * 0.5f;
    glideC = (mode == 1 && tc > 0.002f) ? 1.f - std::exp (-(float) kChunk / (tc * (float) sr)) : 1.f;

    static const float hpHz[4] { 0.f, 100.f, 300.f, 800.f };
    const int h = (int) g (HPF);
    hpOn = h > 0;
    hpa = hpOn ? 1.f - std::exp (-6.2831853f * hpHz[h] * invSr) : 0.f;

    const float tick = (float) sr / (float) kChunk;
    ampP = makeEnv (g (AA), g (AD), g (AS), g (AR), tick);
    fltP = makeEnv (g (FA), g (FD), g (FS), g (FR), tick);
    gain = g (VOL) * g (VOL) * 2.f;
}

#if KOMODO_FULL
// Version completa: unison de hasta 7 capas, osc2, filtro multimodo (doble canal si hay stereo), matriz de modulacion.
void PolyKomodoAudioProcessor::renderVoice (Voice& v, float* oL, float* oR, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float ae = v.amp.next (ampP);
    const float fe = v.flt.next (fltP);
    const float vals[ModSrc::Count] { 0.f, lfo, lfo2, fe, ae, v.vel, (float) (v.note - 60) * (1.f / 36.f), modWheel, v.rnd };
    ModOut mo;
    evalMatrix (msrc, mdst, mamt, vals, mo);

    const float f   = v.curF * rangeMul * std::exp2 ((bend * 200.f + lfo * vibCents + v.drift * driftCents + mo.pitch * 100.f) * (1.f / 1200.f));
    const float pw  = juce::jlimit (0.05f, 0.95f, pwBase + lfo * pwmAmt * 0.4f + mo.d1 * 0.4f);
    const float dc  = 2.f * pw - 1.f;
    const float undC = und * 50.f * juce::jmax (0.f, 1.f + mo.d2);

    float dt[kMaxLay], dt2[kMaxLay];
    for (int j = 0; j < nLay; ++j)
    {
        const float m = std::exp2 (lyOff[j] * undC * (1.f / 1200.f));
        dt[j]  = juce::jmin (f * m * invSr, 0.45f);
        dt2[j] = juce::jmin (f * m * o2Mul * invSr, 0.45f);
    }
    const float sdt = juce::jmin (f * invSr, 0.45f) * 0.5f;

    const float a0 = v.gAmp, a1 = ae * (1.f - velS * (1.f - v.vel)) * juce::jmax (0.f, 1.f + mo.amp);
    const float oct = envAmt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f) + velS * (v.vel - 0.5f) * 1.6f + mo.cutoff;
    const float fc = juce::jlimit (20.f, 0.25f * (float) sr, cut * std::exp2 (oct));
    const float x  = 6.2831853f * fc * invSr;
    const float g1 = x / (1.f + x);
    const float k  = juce::jlimit (0.f, 1.f, res + mo.res) * 3.8f;
    const float inScale = 0.35f * (1.f + k * 0.25f) * (1.f + fdr * 3.f);
    const float outScale = 1.f / (1.f + fdr * 0.9f);
    float gl1, gr1;
    panGains (mo.pan, gl1, gr1);

    const float inv = 1.f / (float) n;
    float g = v.g, a = a0, gl = v.gl, gr = v.gr;
    const float dg = (g1 - g) * inv, da = (a1 - a0) * inv, dgl = (gl1 - gl) * inv, dgr = (gr1 - gr) * inv;
    float ph[kMaxLay], ph2[kMaxLay], st[4], stR[4];
    for (int j = 0; j < kMaxLay; ++j) { ph[j] = v.ph[j]; ph2[j] = v.ph2[j]; }
    for (int j = 0; j < 4; ++j) { st[j] = v.st[j]; stR[j] = v.stR[j]; }
    float sph = v.sph, hpL = v.hpL, hpR = v.hpR;
    const bool useSaw = lsaw > 0.001f, usePul = lpul > 0.001f, useSub = lsub > 0.001f, useNoi = noi > 0.001f, useO2 = lo2 > 0.001f;

    for (int i = 0; i < n; ++i)
    {
        float accL = 0.f, accR = 0.f;
        for (int j = 0; j < nLay; ++j)
        {
            float& p = ph[j];
            p += dt[j]; if (p >= 1.f) p -= 1.f;
            float w = 0.f;
            if (useSaw) w += lsaw * (2.f * p - 1.f - blep (p, dt[j]));
            if (usePul)
            {
                float t2 = p + 1.f - pw; if (t2 >= 1.f) t2 -= 1.f;
                w += lpul * ((p < pw ? 1.f : -1.f) + blep (p, dt[j]) - blep (t2, dt[j]) - dc);
            }
            if (useO2)
            {
                float& q = ph2[j];
                q += dt2[j]; if (q >= 1.f) q -= 1.f;
                w += lo2 * (2.f * q - 1.f - blep (q, dt2[j]));
            }
            accL += w * lyGL[j];
            accR += w * lyGR[j];
        }
        float extra = 0.f;
        if (useSub)
        {
            sph += sdt; if (sph >= 1.f) sph -= 1.f;
            float t2 = sph + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
            extra += lsub * ((sph < 0.5f ? 1.f : -1.f) + blep (sph, sdt) - blep (t2, sdt));
        }
        if (useNoi) extra += noi * xorNoise (rngState);
        accL = (accL + extra) * inScale;
        accR = (accR + extra) * inScale;

        if (hpOn) { hpL += hpa * (accL - hpL); accL -= hpL; if (dual) { hpR += hpa * (accR - hpR); accR -= hpR; } }

        g += dg;  a += da;  gl += dgl;  gr += dgr;
        const float yL = ladderTick (accL, st, g, k, fty) * outScale * a;
        const float yR = dual ? ladderTick (accR, stR, g, k, fty) * outScale * a : yL;
        oL[i] += yL * gl;
        oR[i] += yR * gr;
    }
    for (int j = 0; j < kMaxLay; ++j) { v.ph[j] = ph[j]; v.ph2[j] = ph2[j]; }
    for (int j = 0; j < 4; ++j) { v.st[j] = st[j]; v.stR[j] = stR[j]; }
    v.sph = sph; v.hpL = hpL; v.hpR = hpR;
    v.g = g1; v.gAmp = a1; v.gl = gl1; v.gr = gr1;
    if (v.amp.st == 0)
    {
        v.note = -1; v.released = false; v.sustained = false; v.hpL = v.hpR = 0.f;
        for (int j = 0; j < 4; ++j) v.st[j] = v.stR[j] = 0.f;
    }
}
#else
// Suma una voz en out[0..n). Los parametros de control se actualizan 1 vez por tick (16 muestras)
// y se interpolan linealmente dentro del tick: barato y sin "zipper noise".
void PolyKomodoAudioProcessor::renderVoice (Voice& v, float* out, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float f   = v.curF * rangeMul * std::exp2 ((bend * 200.f + lfo * vibCents + v.drift * driftCents) * (1.f / 1200.f));
    const float dt  = juce::jmin (f * invSr, 0.45f);
    const float sdt = dt * 0.5f;
    const float pw  = juce::jlimit (0.05f, 0.95f, pwBase + lfo * pwmAmt * 0.4f);
    const float dc  = 2.f * pw - 1.f;

    const float a0 = v.gAmp, a1 = v.amp.next (ampP) * (1.f - velS * (1.f - v.vel));
    const float fe = v.flt.next (fltP);
    const float oct = envAmt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f) + velS * (v.vel - 0.5f) * 1.6f;
    const float fc = juce::jlimit (20.f, 0.25f * (float) sr, cut * std::exp2 (oct));
    const float x  = 6.2831853f * fc * invSr;
    const float g1 = x / (1.f + x);
    const float k  = res * 3.8f;
    const float inScale = 0.35f * (1.f + k * 0.25f);

    float g  = v.g;
    const float dg = (g1 - g) / (float) n, da = (a1 - a0) / (float) n;
    float a = a0;
    float ph = v.ph, sph = v.sph, s1 = v.s1, s2 = v.s2, s3 = v.s3, s4 = v.s4, hp = v.hpL;
    const bool useSaw = lsaw > 0.001f, usePul = lpul > 0.001f, useSub = lsub > 0.001f, useNoi = noi > 0.001f;

    for (int i = 0; i < n; ++i)
    {
        ph += dt;   if (ph >= 1.f)  ph -= 1.f;
        sph += sdt; if (sph >= 1.f) sph -= 1.f;

        float s = 0.f;
        if (useSaw) s += lsaw * (2.f * ph - 1.f - blep (ph, dt));
        if (usePul)
        {
            float t2 = ph + 1.f - pw; if (t2 >= 1.f) t2 -= 1.f;
            s += lpul * ((ph < pw ? 1.f : -1.f) + blep (ph, dt) - blep (t2, dt) - dc);
        }
        if (useSub)
        {
            float t2 = sph + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
            s += lsub * ((sph < 0.5f ? 1.f : -1.f) + blep (sph, sdt) - blep (t2, sdt));
        }
        if (useNoi)
        {
            s += noi * xorNoise (rngState);
        }
        s *= inScale;

        if (hpOn) { hp += hpa * (s - hp); s -= hp; }

        g += dg;
        const float u = clip3 (s - k * s4);                 // realimentacion acotada: nunca explota
        s1 += g * (u - s1);
        s2 += g * (s1 - s2);
        s3 += g * (s2 - s3);
        s4 += g * (s3 - s4);

        a += da;
        out[i] += s4 * a;
    }
    v.ph = ph; v.sph = sph; v.s1 = s1; v.s2 = s2; v.s3 = s3; v.s4 = s4; v.hpL = hp;
    v.g = g1; v.gAmp = a1;
    if (v.amp.st == 0) { v.note = -1; v.released = false; v.sustained = false; v.s1 = v.s2 = v.s3 = v.s4 = v.hpL = 0.f; }
}
#endif

#if KOMODO_FULL
void PolyKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int total = b.getNumSamples();
    if (b.getNumChannels() < 2) { b.clear(); return; }
    if (panicFlag.exchange (false)) doPanic();
    keyboardState.processNextMidiBuffer (midi, 0, total, true);

    readParams();
    if (mode != lastMode) { releaseAll(); lastMode = mode; }
    auto arpSend = [this] (bool on, int n) { if (on) noteOn (n, arp.vel); else noteOff (n); };
    if (arp.mode == 0 && lastArp != 0) arp.stop (arpSend);
    lastArp = arp.mode;

    float* L = b.getWritePointer (0);
    float* R = b.getWritePointer (1);

    auto it = midi.begin();
    int pos = 0;
    while (pos < total)
    {
        while (it != midi.end() && (*it).samplePosition <= pos)
        {
            const auto m = (*it).getMessage(); ++it;
            if (arp.mode != 0 && m.isNoteOn())       { arp.vel = m.getFloatVelocity(); arp.noteOn (m.getNoteNumber()); }
            else if (arp.mode != 0 && m.isNoteOff()) arp.noteOff (m.getNoteNumber(), arpSend);
            else                                     handle (m);
        }
        if (arp.mode != 0) arp.fire (arpSend);
        int next = juce::jmin (total, pos + kChunk);
        if (it != midi.end()) next = juce::jmin (next, (*it).samplePosition);
        if (arp.mode != 0) next = juce::jmin (next, pos + arp.maxChunk());
        const int n = next - pos;

        lfoPh += lfoInc * (float) n; if (lfoPh >= 1.f) lfoPh -= 1.f;
        lfo = 4.f * std::abs (lfoPh - 0.5f) - 1.f;                       // LFO 1 triangular
        lfo2Ph += lfo2Inc * (float) n;
        if (lfo2Ph >= 1.f) { lfo2Ph -= 1.f; sh2Val = xorNoise (sh2State); }
        lfo2 = lfoShape (l2s, lfo2Ph, sh2Val);

        std::fill (L + pos, L + next, 0.f);
        std::fill (R + pos, R + next, 0.f);
        const int nv = mode == 1 ? 1 : kVoices;
        for (int i = 0; i < nv; ++i)
            if (! voices[i].free()) renderVoice (voices[i], L + pos, R + pos, n);
        for (int i = pos; i < next; ++i) { L[i] *= gain; R[i] *= gain; }

        if (arp.mode != 0) arp.advance (n);
        pos = next;
    }

    // efectos en bloque: drive -> chorus -> delay -> reverb
    if (drv > 0.001f)
    {
        const float gIn = 1.f + drv * 9.f, gOut = 1.f / (1.f + drv * 1.6f);
        for (int i = 0; i < total; ++i) { L[i] = clip3 (L[i] * gIn) * gOut; R[i] = clip3 (R[i] * gIn) * gOut; }
    }
    chorus.process (L, R, total, cho);
    delay.process (L, R, total, dlTarget, dlf, dlm);
    if (rvm > 0.002f)
    {
        if (! reverbWasOn) { reverb.reset(); reverbWasOn = true; }
        juce::Reverb::Parameters rp;
        rp.roomSize = pp[RVS]->load();  rp.damping = pp[RVD]->load();
        rp.wetLevel = rvm * 0.3f;  rp.dryLevel = 0.5f;  rp.width = 1.f;  rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        reverb.processStereo (L, R, total);
    }
    else reverbWasOn = false;

    for (int i = 0; i < total; ++i)
    {
        L[i] = clip3 (L[i]);  R[i] = clip3 (R[i]);
        scope.push (0.5f * (L[i] + R[i]));
    }
}
#else
void PolyKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int total = b.getNumSamples();
    if (panicFlag.exchange (false)) doPanic();
    keyboardState.processNextMidiBuffer (midi, 0, total, true);

    readParams();
    if (mode != lastMode) { releaseAll(); lastMode = mode; }
    auto arpSend = [this] (bool on, int n) { if (on) noteOn (n, arp.vel); else noteOff (n); };
    if (arp.mode == 0 && lastArp != 0) arp.stop (arpSend);
    lastArp = arp.mode;

    float* L = b.getWritePointer (0);
    float* R = b.getNumChannels() > 1 ? b.getWritePointer (1) : nullptr;
    float mono[kChunk], cl[kChunk], cr[kChunk];

    auto it = midi.begin();
    int pos = 0;
    while (pos < total)
    {
        while (it != midi.end() && (*it).samplePosition <= pos)
        {
            const auto m = (*it).getMessage(); ++it;
            if (arp.mode != 0 && m.isNoteOn())       { arp.vel = m.getFloatVelocity(); arp.noteOn (m.getNoteNumber()); }
            else if (arp.mode != 0 && m.isNoteOff()) arp.noteOff (m.getNoteNumber(), arpSend);
            else                                     handle (m);
        }
        if (arp.mode != 0) arp.fire (arpSend);
        int next = juce::jmin (total, pos + kChunk);
        if (it != midi.end()) next = juce::jmin (next, (*it).samplePosition);
        if (arp.mode != 0) next = juce::jmin (next, pos + arp.maxChunk());
        const int n = next - pos;

        lfoPh += lfoInc * (float) n; if (lfoPh >= 1.f) lfoPh -= 1.f;
        lfo = 4.f * std::abs (lfoPh - 0.5f) - 1.f;                       // LFO triangular, global (como el Juno)

        std::fill (mono, mono + n, 0.f);
        const int nv = mode == 1 ? 1 : kVoices;
        for (int i = 0; i < nv; ++i)
            if (! voices[i].free()) renderVoice (voices[i], mono, n);

        for (int i = 0; i < n; ++i) mono[i] = mono[i] * gain;
        chorus.process (mono, cl, cr, n, cho);

        for (int i = 0; i < n; ++i)
        {
            const float l = clip3 (cl[i]), r = clip3 (cr[i]);
            if (R) { L[pos + i] = l; R[pos + i] = r; }
            else   L[pos + i] = 0.5f * (l + r);
        }
        if (arp.mode != 0) arp.advance (n);
        pos = next;
    }
}
#endif

juce::AudioProcessorEditor* PolyKomodoAudioProcessor::createEditor() { return new ScaledEditor<PolyUi> (*this); }

void PolyKomodoAudioProcessor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    st.setProperty ("prog", curProg, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, d);
}

void PolyKomodoAudioProcessor::setStateInformation (const void* d, int sz)
{
    if (auto xml = getXmlFromBinary (d, sz))
        if (xml->hasTagName (apvts.state.getType()))
        {
            curProg = juce::jlimit (0, getNumPrograms() - 1, xml->getIntAttribute ("prog", 0));
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PolyKomodoAudioProcessor(); }
