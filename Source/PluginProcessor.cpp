#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// valores por defecto, en el mismo orden que el enum
const float kDefault[NP] = {
    2.f, 2.f, 2.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f,          // R0 R1 R2 W0 W1 W2 MODE LSH CHO
    0.f, 0.f, 1.f, .7f, 0.f, 0.f,                          // D1 D2 L0 L1 L2 NZ
    1500.f, .25f, .5f, .5f, 0.f,                           // CUT RES AMT KEY LFF
    .01f, .35f, .2f, .01f, .3f, .8f, 1.f,                  // FA FD FS AA AD AS DEC
    0.f, .3f, 0.f, .6f,                                    // GL LR LA VOL
    0.f, 3.f, 0.f, .6f, .15f, .35f                         // ARP ARR ARO ARG DRF VEL
#if KOMODO_FULL
    , 0.f, 0.f, 0.f, 0.f, 0.f, .3f, 0.f,                   // FTY FDR SYN XMD SPR L2R L2S
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

float T (float x) { return 0.002f + x * x * 3.f; }

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
#endif          // 0..1 -> segundos (igual que la version web)

float wave (int w, float t, float dt)
{
    switch (w)
    {
        case 0: return 1.f - 4.f * std::abs (t - 0.5f);                  // triangulo
        case 1: return 2.f * t - 1.f - blep (t, dt);                      // sierra
        case 2: return -(2.f * t - 1.f - blep (t, dt));                   // sierra inversa
        default:
        {
            const float pw = (w == 3 ? 0.5f : w == 4 ? 0.25f : 0.1f);    // cuadrada / pulso ancho / angosto
            float t2 = t + 1.f - pw; if (t2 >= 1.f) t2 -= 1.f;
            return (t < pw ? 1.f : -1.f) + blep (t, dt) - blep (t2, dt);
        }
    }
}

struct Ov { int id; float v; };
struct Preset { const char* name; std::vector<Ov> ov; };

// Rangos: 0=32' 1=16' 2=8' 3=4' 4=2'.  Ondas: 0 tri, 1 sierra, 2 sierra inv, 3 cuadrada, 4 pulso ancho, 5 pulso angosto.
// MODE: 0 poly, 1 mono.  DEC=0 -> release corto (15 ms); DEC=1 -> release = tiempo de decay.
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", {} },
        // ---- Bass (clasicos tipo Minimoog)
        { "Moog Bass",       { {R0,1}, {W0,1}, {R1,1}, {W1,3}, {D1,4}, {R2,0}, {W2,0}, {L0,1}, {L1,.8f}, {L2,.6f}, {CUT,350}, {RES,.35f}, {AMT,.6f}, {KEY,.3f}, {FA,0}, {FD,.25f}, {FS,.2f}, {AA,0}, {AD,.3f}, {AS,.9f}, {DEC,0}, {MODE,1}, {VOL,0.870f} } },
        { "Fat Moog Bass",   { {R0,1}, {W0,1}, {R1,1}, {W1,1}, {D1,7}, {R2,0}, {W2,1}, {D2,-6}, {L0,1}, {L1,.9f}, {L2,.8f}, {CUT,280}, {RES,.3f}, {AMT,.7f}, {KEY,.3f}, {FD,.3f}, {FS,.15f}, {AD,.3f}, {AS,.9f}, {DEC,0}, {GL,.08f}, {MODE,1}, {VOL,0.746f} } },
        { "Sub Bass",        { {R0,0}, {W0,0}, {R1,1}, {W1,0}, {L0,1}, {L1,.5f}, {L2,0}, {CUT,220}, {RES,.1f}, {AMT,.2f}, {KEY,0}, {FD,.2f}, {FS,.8f}, {AD,.3f}, {AS,.9f}, {DEC,0}, {MODE,1}, {VOL,0.991f} } },
        { "Rubber Bass",     { {R0,1}, {W0,1}, {R1,1}, {W1,3}, {D1,5}, {L0,.9f}, {L1,.7f}, {L2,0}, {CUT,200}, {RES,.5f}, {AMT,.9f}, {KEY,.3f}, {FD,.15f}, {FS,0}, {AD,.2f}, {AS,.4f}, {DEC,0}, {MODE,1}, {VOL,1.000f} } },
        { "Funk Bass",       { {R0,1}, {W0,4}, {R1,1}, {W1,1}, {D1,4}, {L0,1}, {L1,.6f}, {L2,0}, {CUT,300}, {RES,.4f}, {AMT,.85f}, {KEY,.4f}, {FD,.12f}, {FS,.1f}, {AD,.18f}, {AS,.5f}, {DEC,0}, {MODE,1}, {VOL,1.000f} } },
        { "Taurus Bass",     { {R0,0}, {W0,1}, {R1,0}, {W1,3}, {L0,1}, {L1,.8f}, {L2,0}, {CUT,160}, {RES,.55f}, {AMT,.5f}, {KEY,.2f}, {FD,.4f}, {FS,.3f}, {AD,.4f}, {AS,.9f}, {DEC,0}, {MODE,1}, {VOL,1.000f} } },
        { "Acid Bass",       { {R0,1}, {W0,1}, {L0,1}, {L1,0}, {L2,0}, {CUT,250}, {RES,.8f}, {AMT,.85f}, {KEY,.2f}, {FD,.22f}, {FS,0}, {AD,.25f}, {AS,.6f}, {DEC,0}, {GL,.1f}, {MODE,1}, {VOL,1.000f} } },
        // ---- Lead (clasicos)
        { "Moog Lead",       { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,6}, {R2,1}, {W2,3}, {D2,-5}, {L0,1}, {L1,.8f}, {L2,.5f}, {CUT,2500}, {RES,.3f}, {AMT,.3f}, {KEY,.8f}, {FD,.35f}, {FS,.6f}, {AS,.9f}, {DEC,0}, {GL,.12f}, {LA,.05f}, {LR,.45f}, {MODE,1}, {VOL,0.790f} } },
        { "Prog Lead",       { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,8}, {R2,3}, {W2,1}, {D2,-10}, {L0,1}, {L1,.7f}, {L2,.3f}, {CUT,4000}, {RES,.25f}, {AMT,.25f}, {KEY,.8f}, {FD,.35f}, {FS,.6f}, {AS,.9f}, {DEC,0}, {GL,.15f}, {LA,.08f}, {LR,.5f}, {MODE,1}, {VOL,0.848f} } },
        { "Screamer Lead",   { {R0,2}, {W0,4}, {R1,2}, {W1,1}, {D1,12}, {L0,1}, {L1,.9f}, {L2,0}, {CUT,5000}, {RES,.5f}, {AMT,.5f}, {KEY,.8f}, {FD,.3f}, {FS,.5f}, {AS,.9f}, {DEC,0}, {LA,.1f}, {MODE,1}, {VOL,0.925f} } },
        { "Flute Lead",      { {R0,2}, {W0,0}, {R1,3}, {W1,0}, {D1,3}, {L0,1}, {L1,.4f}, {L2,0}, {NZ,.04f}, {CUT,2200}, {RES,.1f}, {AMT,.2f}, {KEY,.7f}, {AA,.12f}, {AS,.9f}, {DEC,0}, {LA,.15f}, {LR,.4f}, {GL,.1f}, {MODE,1}, {VOL,1.000f} } },
        { "Square Lead",     { {R0,2}, {W0,3}, {R1,2}, {W1,3}, {D1,6}, {L0,1}, {L1,.8f}, {L2,0}, {CUT,3500}, {RES,.2f}, {AMT,.3f}, {KEY,.8f}, {AS,.9f}, {DEC,0}, {CHO,1}, {MODE,1}, {VOL,0.817f} } },
        { "Sync-ish Lead",   { {R0,2}, {W0,5}, {R1,3}, {W1,1}, {D1,15}, {L0,1}, {L1,.8f}, {L2,0}, {CUT,6000}, {RES,.45f}, {AMT,.6f}, {KEY,.7f}, {FD,.25f}, {FS,.5f}, {AS,.9f}, {DEC,0}, {LA,.12f}, {LR,.55f}, {GL,.06f}, {MODE,1}, {VOL,0.953f} } },
        // ---- Brass / Pad / Strings
        { "Moog Brass",      { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,5}, {R2,2}, {W2,1}, {D2,-5}, {L0,1}, {L1,.8f}, {L2,.7f}, {CUT,600}, {RES,.15f}, {AMT,.75f}, {KEY,.5f}, {FA,.08f}, {FD,.35f}, {FS,.6f}, {AA,.05f}, {AD,.3f}, {AS,.9f}, {DEC,0}, {VOL,0.449f} } },
        { "Brass Stab",      { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,6}, {L0,1}, {L1,.8f}, {L2,0}, {CUT,500}, {RES,.2f}, {AMT,.85f}, {KEY,.5f}, {FA,0}, {FD,.2f}, {FS,.25f}, {AA,0}, {AD,.25f}, {AS,.6f}, {DEC,0}, {VOL,0.552f} } },
        { "Moog Pad",        { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,8}, {R2,2}, {W2,1}, {D2,-8}, {L0,.8f}, {L1,.8f}, {L2,.8f}, {CUT,900}, {RES,.15f}, {AMT,.3f}, {KEY,.5f}, {FA,.35f}, {FD,.5f}, {FS,.7f}, {AA,.45f}, {AD,.5f}, {AS,.9f}, {DEC,1}, {LA,.05f}, {LR,.2f}, {CHO,2}, {VOL,0.634f} } },
        { "Strings",         { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,7}, {R2,3}, {W2,1}, {D2,-7}, {L0,.8f}, {L1,.8f}, {L2,.5f}, {CUT,2500}, {RES,.1f}, {AMT,.15f}, {KEY,.6f}, {FA,.3f}, {FD,.5f}, {FS,.8f}, {AA,.35f}, {AD,.5f}, {AS,.9f}, {DEC,1}, {LA,.08f}, {LR,.3f}, {CHO,2}, {VOL,0.589f} } },
        { "Dark Pad",        { {R0,1}, {W0,1}, {R1,2}, {W1,3}, {D1,6}, {R2,2}, {W2,1}, {D2,-6}, {L0,.8f}, {L1,.7f}, {L2,.7f}, {CUT,500}, {RES,.25f}, {AMT,.5f}, {KEY,.3f}, {FA,.6f}, {FD,.6f}, {FS,.5f}, {AA,.5f}, {AD,.5f}, {AS,.9f}, {DEC,1}, {LFF,.1f}, {LR,.15f}, {CHO,2}, {VOL,0.668f} } },
        // ---- Keys / Pluck
        { "Square Organ",    { {R0,2}, {W0,3}, {R1,3}, {W1,3}, {R2,4}, {W2,3}, {L0,1}, {L1,.6f}, {L2,.4f}, {CUT,6000}, {RES,0}, {AMT,0}, {KEY,.5f}, {FS,1}, {AA,0}, {AD,0}, {AS,1}, {DEC,0}, {CHO,3}, {VOL,0.496f} } },
        { "Clav",            { {R0,2}, {W0,5}, {R1,2}, {W1,4}, {D1,6}, {L0,1}, {L1,.5f}, {L2,0}, {CUT,1500}, {RES,.3f}, {AMT,.6f}, {KEY,.8f}, {FD,.15f}, {FS,0}, {AD,.15f}, {AS,0}, {DEC,0}, {VOL,0.603f} } },
        { "Electric Piano",  { {R0,2}, {W0,0}, {R1,3}, {W1,3}, {D1,3}, {L0,1}, {L1,.3f}, {L2,0}, {CUT,1800}, {RES,.1f}, {AMT,.4f}, {KEY,.7f}, {FD,.35f}, {FS,.15f}, {AD,.4f}, {AS,.3f}, {DEC,1}, {CHO,1}, {VOL,0.869f} } },
        { "Moog Pluck",      { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,5}, {L0,1}, {L1,.7f}, {L2,0}, {CUT,800}, {RES,.3f}, {AMT,.8f}, {KEY,.6f}, {FD,.2f}, {FS,0}, {AD,.22f}, {AS,0}, {DEC,1}, {VOL,0.671f} } },
        { "Marimba",         { {R0,2}, {W0,0}, {R1,4}, {W1,0}, {L0,1}, {L1,.3f}, {L2,0}, {CUT,900}, {RES,.2f}, {AMT,.6f}, {KEY,.9f}, {FD,.15f}, {FS,0}, {AD,.18f}, {AS,0}, {DEC,1}, {VOL,0.885f} } },
        // ---- Arpegiador
        { "Arp Moog",        { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,5}, {L0,1}, {L1,.7f}, {L2,0}, {CUT,800}, {RES,.3f}, {AMT,.8f}, {KEY,.6f}, {FD,.2f}, {FS,0}, {AD,.22f}, {AS,0}, {DEC,1}, {ARP,1}, {ARR,3}, {ARO,1}, {ARG,.5f}, {CHO,1} } },
        { "Arp Sequence",    { {R0,1}, {W0,1}, {R1,1}, {W1,3}, {D1,6}, {L0,1}, {L1,.7f}, {L2,0}, {CUT,350}, {RES,.55f}, {AMT,.8f}, {KEY,.3f}, {FD,.18f}, {FS,0}, {AD,.2f}, {AS,.5f}, {DEC,0}, {ARP,3}, {ARR,3}, {ARO,1}, {ARG,.55f}, {MODE,1} } },
        // ---- FX
        { "Wind Sweep",      { {L0,0}, {L1,0}, {L2,0}, {NZ,1}, {CUT,800}, {RES,.6f}, {AMT,.4f}, {KEY,0}, {FA,.6f}, {FD,.6f}, {FS,.6f}, {AA,.5f}, {AD,.5f}, {AS,.9f}, {DEC,1}, {LFF,.5f}, {LR,.15f}, {CHO,1}, {VOL,1.000f} } },
#if KOMODO_FULL
        // ---- Solo version completa (matriz de modulacion: fuentes 1 LFO1 2 LFO2 3 EnvF 4 EnvV 5 Vel 6 Key 7 Wheel 8 Rand; destinos 1 Pitch 2 Corte 3 Reso 4 Vol 5 Pan 6 Osc2 7 Osc3)
        { "Sync Lead",       { {R0,2}, {W0,1}, {L0,0}, {R1,2}, {W1,1}, {L1,1}, {L2,0}, {SYN,1}, {M1S,3}, {M1D,6}, {M1A,.45f}, {CUT,5000}, {RES,.25f}, {AMT,.3f}, {KEY,.8f}, {FD,.4f}, {FS,.3f}, {AS,.9f}, {DEC,0}, {GL,.1f}, {LA,.05f}, {LR,.5f}, {MODE,1}, {DLM,.2f}, {DLT,1}, {VOL,1.000f} } },
        { "Wobble Bass",     { {R0,1}, {W0,1}, {R1,1}, {W1,3}, {D1,6}, {L0,1}, {L1,.8f}, {L2,0}, {CUT,260}, {RES,.5f}, {AMT,.3f}, {KEY,.3f}, {FD,.3f}, {FS,.5f}, {AS,.9f}, {DEC,0}, {L2R,.55f}, {L2S,0}, {M1S,2}, {M1D,2}, {M1A,.55f}, {FDR,.3f}, {DRV,.2f}, {MODE,1}, {VOL,0.621f} } },
        { "Super Pad",       { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,9}, {R2,2}, {W2,1}, {D2,-9}, {L0,.8f}, {L1,.8f}, {L2,.8f}, {CUT,1400}, {RES,.15f}, {AMT,.25f}, {KEY,.5f}, {FA,.4f}, {FD,.5f}, {FS,.7f}, {AA,.45f}, {AD,.5f}, {AS,.9f}, {DEC,1}, {SPR,1}, {CHO,2}, {M1S,2}, {M1D,2}, {M1A,.2f}, {L2R,.15f}, {RVM,.4f}, {RVS,.8f}, {VOL,0.666f} } },
        { "Space Arp",       { {R0,2}, {W0,1}, {R1,2}, {W1,1}, {D1,5}, {L0,1}, {L1,.7f}, {L2,0}, {CUT,900}, {RES,.35f}, {AMT,.8f}, {KEY,.6f}, {FD,.2f}, {FS,0}, {AD,.22f}, {AS,0}, {DEC,1}, {ARP,3}, {ARR,3}, {ARO,1}, {ARG,.5f}, {SPR,.8f}, {DLM,.4f}, {DLT,2}, {DLF,.5f}, {RVM,.3f}, {VOL,0.888f} } },
        { "X-Mod Bell",      { {R0,2}, {W0,0}, {L0,0}, {R1,3}, {W1,0}, {L1,1}, {L2,0}, {XMD,.55f}, {CUT,6000}, {RES,.1f}, {AMT,.2f}, {KEY,.8f}, {FD,.45f}, {FS,0}, {AD,.5f}, {AS,0}, {DEC,1}, {M1S,3}, {M1D,6}, {M1A,.15f}, {SPR,.5f}, {RVM,.3f}, {VOL,0.710f} } },
#endif
        { "S&H Bleeps",      { {R0,2}, {W0,1}, {R1,2}, {W1,3}, {D1,7}, {L0,1}, {L1,.6f}, {L2,0}, {CUT,1200}, {RES,.55f}, {AMT,.2f}, {KEY,.4f}, {FD,.2f}, {FS,.5f}, {AS,.9f}, {DEC,0}, {LSH,3}, {LFF,.8f}, {LR,.6f}, {MODE,1}, {VOL,0.950f} } },
    };
    return p;
}
} // namespace

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;

juce::AudioProcessorValueTreeState::ParameterLayout MiniKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using R = juce::NormalisableRange<float>;
    const R u (0.f, 1.f);
    auto f = [&] (int i, const juce::String& name, R r)
        { p.push_back (std::make_unique<APF> (juce::ParameterID { paramIds[i], 1 }, name, r, kDefault[i],
                                                    attrs (i == CUT ? 1 : (i == D1 || i == D2) ? 2 : 0))); };
    auto c = [&] (int i, const juce::String& name, juce::StringArray s)
        { p.push_back (std::make_unique<APC> (juce::ParameterID { paramIds[i], 1 }, name, s, (int) kDefault[i])); };

    const juce::StringArray rng { "32'", "16'", "8'", "4'", "2'" },
        wav { "Triangulo", "Sierra", "Sierra inv", "Cuadrada", "Pulso ancho", "Pulso angosto" };
    for (int i = 0; i < 3; ++i) c (R0 + i, "Osc " + juce::String (i + 1) + " rango", rng);
    for (int i = 0; i < 3; ++i) c (W0 + i, "Osc " + juce::String (i + 1) + " onda", wav);
    c (MODE, "Modo", { "Poly", "Mono" });
    c (LSH, "LFO onda", { "Triangulo", "Sierra", "Cuadrada", "S&H" });
    c (CHO, "Chorus", { "Off", "I", "II", "I+II" });

    f (D1, "Osc2 fino", R (-100.f, 100.f));   f (D2, "Osc3 fino", R (-100.f, 100.f));
    f (L0, "Nivel 1", u);   f (L1, "Nivel 2", u);   f (L2, "Nivel 3", u);   f (NZ, "Ruido", u);
    f (CUT, "Corte", R (40.f, 18000.f, 0.f, 0.3f));
    f (RES, "Resonancia", u);   f (AMT, "Cant. env", u);
    f (KEY, "Keytrack", u);     f (LFF, "LFO filtro", u);
    f (FA, "Filtro A", u);      f (FD, "Filtro D", u);   f (FS, "Filtro S", u);
    f (AA, "Vol A", u);         f (AD, "Vol D", u);      f (AS, "Vol S", u);
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { paramIds[DEC], 1 }, "Decay on", kDefault[DEC] > 0.5f));
    f (GL, "Glide", u);         f (LR, "LFO vel.", u);   f (LA, "LFO vibrato", u);   f (VOL, "Master", u);
    c (ARP, "Arp", { "Off", "Up", "Down", "Up/Down", "Random", "Played" });
    c (ARR, "Arp ritmo", { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" });
    c (ARO, "Arp oct.", { "1 oct", "2 oct", "3 oct" });
    f (ARG, "Arp gate", R (0.1f, 0.95f));  f (DRF, "Drift", u);   f (VEL, "Velocity", u);
   #if KOMODO_FULL
    const R bi (-1.f, 1.f);
    c (FTY, "Filtro tipo", { "LP 24", "LP 12", "BP", "HP" });
    f (FDR, "Filtro drive", u);
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { paramIds[SYN], 1 }, "Sync", false));
    f (XMD, "X-Mod", u);  f (SPR, "Stereo", u);  f (L2R, "LFO2 vel.", u);
    c (L2S, "LFO2 onda", { "Triangulo", "Sierra", "Cuadrada", "S&H" });
    const juce::StringArray src { "Off", "LFO 1", "LFO 2", "Env filtro", "Env vol", "Velocity", "Key", "Mod wheel", "Random" },
        dst { "Off", "Pitch", "Corte", "Reso", "Volumen", "Pan", "Osc2 pitch", "Osc3 pitch" };
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

MiniKomodoAudioProcessor::MiniKomodoAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", layout())
{
    for (int i = 0; i < NP; ++i) pp[i] = apvts.getRawParameterValue (paramIds[i]);
}

bool MiniKomodoAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
   #if KOMODO_FULL
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
   #else
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
   #endif
}

void MiniKomodoAudioProcessor::prepareToPlay (double s, int)
{
    sr = s; invSr = 1.f / (float) s;
    chorus.prepare (s);
   #if KOMODO_FULL
    delay.prepare (s);  reverb.setSampleRate (s);  reverb.reset();  reverbWasOn = false;
    lfo2Ph = 0.f;
   #endif
    for (auto& v : voices) v = Voice();
    nHeld = 0; sustain = false; lfoPh = 0.f; bend = 0.f; arp = Arp(); lastArp = 0;
    lastMode = mode = (int) pp[MODE]->load();
}

// ---------------------------------------------------------------- programas (presets)
int MiniKomodoAudioProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String MiniKomodoAudioProcessor::getProgramName (int i)
{
    const auto& p = presets();
    return (i >= 0 && i < (int) p.size()) ? juce::String (p[(size_t) i].name) : juce::String();
}

void MiniKomodoAudioProcessor::setCurrentProgram (int i)
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

void MiniKomodoAudioProcessor::handleAsyncUpdate()
{
    const int p = pendingProgram.exchange (-1);
    if (p >= 0) setCurrentProgram (juce::jmin (p, getNumPrograms() - 1));
}

void MiniKomodoAudioProcessor::doPanic()
{
    for (auto& v : voices) v = Voice();
    nHeld = 0; sustain = false; arp = Arp(); lastArp = 0;
   #if KOMODO_FULL
    chorus.clear(); delay.clear(); reverb.reset();
   #endif
}

// ---------------------------------------------------------------- MIDI / voces
void MiniKomodoAudioProcessor::noteOn (int n, float vel)
{
    const float hz = (float) juce::MidiMessage::getMidiNoteInHertz (n);

    if (mode == 1)                                                        // mono legato, prioridad a la ultima nota
    {
        auto& v = voices[0];
        const bool wasHeld = nHeld > 0;
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { std::memmove (held + i, held + i + 1, sizeof (int) * (size_t) (nHeld - i - 1)); --nHeld; break; }
        if (nHeld < 32) held[nHeld++] = n;
        v.tgtF = hz; v.note = n; v.vel = vel; v.drift = xorNoise (rngState);
       #if KOMODO_FULL
        v.rnd = xorNoise (rngState);
       #endif
        v.released = false; v.age = ++ageCounter;
        if (v.free()) v.curF = hz;                                        // sin glide desde el silencio
        if (! wasHeld) { v.amp.on(); v.flt.on(); }                        // legato: no retrigger
        return;
    }

    auto* v = allocateVoice (voices, n);
    v->note = n; v->vel = vel; v->drift = xorNoise (rngState); v->tgtF = v->curF = hz;
   #if KOMODO_FULL
    v->rnd = xorNoise (rngState);
   #endif
    v->released = v->sustained = false; v->age = ++ageCounter;
    v->amp.on(); v->flt.on();
}

void MiniKomodoAudioProcessor::noteOff (int n)
{
    if (mode == 1)
    {
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { std::memmove (held + i, held + i + 1, sizeof (int) * (size_t) (nHeld - i - 1)); --nHeld; break; }
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

void MiniKomodoAudioProcessor::releaseAll()
{
    for (auto& v : voices) { v.amp.off(); v.flt.off(); v.released = true; v.sustained = false; }
    nHeld = 0;
}

void MiniKomodoAudioProcessor::handle (const juce::MidiMessage& m)
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
void MiniKomodoAudioProcessor::readParams()
{
    auto g = [this] (int i) { return pp[i]->load(); };
    mode = (int) g (MODE);  cho = (int) g (CHO);  lsh = (int) g (LSH);

    const float det[3] { 0.f, g (D1), g (D2) };
    for (int k = 0; k < 3; ++k)
    {
        mul[k] = std::exp2 (g (R0 + k) - 2.f + det[k] * (1.f / 1200.f));
        lv[k]  = g (L0 + k);
        wf[k]  = (int) g (W0 + k);
    }
    noise = g (NZ) * 0.8f;
    cut = g (CUT); res = g (RES); amt = g (AMT); keyAmt = g (KEY); lffAmt = g (LFF);
    driftCents = g (DRF) * 12.f;  velS = g (VEL);

    double bpm = 120.0;                                              // arpegiador sincronizado al tempo del host
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    arp.mode = (int) g (ARP);  arp.octaves = 1 + (int) g (ARO);
    arp.setTiming (bpm, (int) g (ARR), g (ARG), sr);
   #if KOMODO_FULL
    fty = (int) g (FTY);  fdr = g (FDR);  syn = g (SYN) > 0.5f;  xmd = g (XMD);  spr = g (SPR);
    l2s = (int) g (L2S);  lfo2Inc = (0.1f + g (L2R) * g (L2R) * 18.f) * invSr;
    for (int i = 0; i < 4; ++i) { msrc[i] = (int) g (M1S + 3 * i); mdst[i] = (int) g (M1D + 3 * i); mamt[i] = g (M1A + 3 * i); }
    drv = g (DRV);  dlm = g (DLM);  dlf = g (DLF) * 0.85f;  rvm = g (RVM);
    static const double beats[6] { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
    dlTarget = (float) juce::jmin (beats[juce::jlimit (0, 5, (int) g (DLT))] * 60.0 / juce::jmax (20.0, bpm) * sr, sr * 2.15);
   #endif
    vibCents = g (LA) * 60.f + modWheel * 60.f;
    lfoInc = (0.5f + g (LR) * 9.f) * invSr;

    const float tc = g (GL) * g (GL) * 0.4f;
    glideC = (mode == 1 && tc > 0.002f) ? 1.f - std::exp (-(float) kChunk / (tc * (float) sr)) : 1.f;

    const float rel = g (DEC) > 0.5f ? T (g (AD)) : 0.015f;
    const float tick = (float) sr / (float) kChunk;
    ampP = makeEnvSec (T (g (AA)), T (g (AD)), g (AS), rel, tick);
    fltP = makeEnvSec (T (g (FA)), T (g (FD)), g (FS), rel, tick);
    gain = g (VOL) * g (VOL) * 1.6f;
}

#if KOMODO_FULL
// Version completa: suma una voz en estereo. Sync, X-mod, filtro multimodo, matriz de modulacion y pan por voz.
void MiniKomodoAudioProcessor::renderVoice (Voice& v, float* oL, float* oR, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float ae = v.amp.next (ampP);
    const float fe = v.flt.next (fltP);

    const float vals[ModSrc::Count] { 0.f, lfo, lfo2, fe, ae, v.vel, (float) (v.note - 60) * (1.f / 36.f), modWheel, v.rnd };
    ModOut mo;
    evalMatrix (msrc, mdst, mamt, vals, mo);

    const float base = v.curF * std::exp2 ((bend * 200.f + lfo * vibCents + v.drift * driftCents + mo.pitch * 100.f) * (1.f / 1200.f));
    const float m1 = std::exp2 (mo.d1 * 2.f), m2 = std::exp2 (mo.d2 * 2.f);
    const float dt0 = juce::jmin (base * mul[0] * invSr, 0.45f);
    const float dt1 = juce::jmin (base * mul[1] * m1 * invSr, 0.45f);
    const float dt2 = juce::jmin (base * mul[2] * m2 * invSr, 0.45f);

    const float a0 = v.gAmp, a1 = ae * (1.f - velS * (1.f - v.vel)) * juce::jmax (0.f, 1.f + mo.amp);
    const float oct = amt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f) + velS * (v.vel - 0.5f) * 1.6f + mo.cutoff;
    const float fc = juce::jlimit (20.f, 0.25f * (float) sr, cut * std::exp2 (oct));
    const float x  = 6.2831853f * fc * invSr;
    const float g1 = x / (1.f + x);
    const float k  = juce::jlimit (0.f, 1.f, res + mo.res) * 3.8f;
    const float inScale = 0.3f * (1.f + k * 0.25f) * (1.f + fdr * 3.f);
    const float outScale = 1.f / (1.f + fdr * 0.9f);
    float gl1, gr1;
    panGains (v.rnd * spr + mo.pan, gl1, gr1);

    const float inv = 1.f / (float) n;
    float g = v.g, a = a0, gl = v.gl, gr = v.gr;
    const float dg = (g1 - g) * inv, da = (a1 - a0) * inv, dgl = (gl1 - gl) * inv, dgr = (gr1 - gr) * inv;
    float p0 = v.ph[0], p1 = v.ph[1], p2 = v.ph[2];
    float st[4] { v.st[0], v.st[1], v.st[2], v.st[3] };
    const bool u0 = lv[0] > 0.001f, u1 = lv[1] > 0.001f, u2 = lv[2] > 0.001f, un = noise > 0.001f;
    const bool useX = xmd > 0.001f, useSync = syn && u1;
    const bool needO1 = u0 || (u1 && (useX || useSync));
    const float ratio = dt1 / juce::jmax (dt0, 1.0e-6f);

    for (int i = 0; i < n; ++i)
    {
        float s = 0.f, w0 = 0.f;
        bool wrap = false;
        if (needO1)
        {
            p0 += dt0; if (p0 >= 1.f) { p0 -= 1.f; wrap = true; }
            w0 = wave (wf[0], p0, dt0);
            if (u0) s += lv[0] * w0;
        }
        if (u1)
        {
            if (useSync && wrap) { p1 = p0 * ratio; p1 -= std::floor (p1); }          // hard sync: osc1 reinicia a osc2
            const float d = useX ? juce::jlimit (1.0e-5f, 0.45f, dt1 * (1.f + xmd * 2.f * w0)) : dt1;   // cross-mod: osc1 modula la frecuencia de osc2
            p1 += d; if (p1 >= 1.f) p1 -= 1.f;
            s += lv[1] * wave (wf[1], p1, d);
        }
        if (u2) { p2 += dt2; if (p2 >= 1.f) p2 -= 1.f; s += lv[2] * wave (wf[2], p2, dt2); }
        if (un) s += noise * xorNoise (rngState);
        s *= inScale;

        g += dg;  a += da;  gl += dgl;  gr += dgr;
        const float o = ladderTick (s, st, g, k, fty) * outScale * a;
        oL[i] += o * gl;
        oR[i] += o * gr;
    }
    v.ph[0] = p0; v.ph[1] = p1; v.ph[2] = p2;
    for (int j = 0; j < 4; ++j) v.st[j] = st[j];
    v.g = g1; v.gAmp = a1; v.gl = gl1; v.gr = gr1;
    if (v.amp.st == 0) { v.note = -1; v.released = false; v.sustained = false; for (auto& q : v.st) q = 0.f; }
}
#else
// Suma una voz en out[0..n). Control (frecuencia, filtro, envolventes) 1 vez por tick, rampa lineal dentro.
void MiniKomodoAudioProcessor::renderVoice (Voice& v, float* out, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float base = v.curF * std::exp2 ((bend * 200.f + lfo * vibCents + v.drift * driftCents) * (1.f / 1200.f));
    float dt[3];
    for (int k = 0; k < 3; ++k) dt[k] = juce::jmin (base * mul[k] * invSr, 0.45f);

    const float a0 = v.gAmp, a1 = v.amp.next (ampP) * (1.f - velS * (1.f - v.vel));
    const float fe = v.flt.next (fltP);
    const float oct = amt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f) + velS * (v.vel - 0.5f) * 1.6f;
    const float fc = juce::jlimit (20.f, 0.25f * (float) sr, cut * std::exp2 (oct));
    const float x  = 6.2831853f * fc * invSr;
    const float g1 = x / (1.f + x);
    const float k  = res * 3.8f;
    const float inScale = 0.3f * (1.f + k * 0.25f);

    float g = v.g, a = a0;
    const float dg = (g1 - g) / (float) n, da = (a1 - a0) / (float) n;
    float p0 = v.ph[0], p1 = v.ph[1], p2 = v.ph[2], s1 = v.s1, s2 = v.s2, s3 = v.s3, s4 = v.s4;
    const bool u0 = lv[0] > 0.001f, u1 = lv[1] > 0.001f, u2 = lv[2] > 0.001f, un = noise > 0.001f;

    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        if (u0) { p0 += dt[0]; if (p0 >= 1.f) p0 -= 1.f; s += lv[0] * wave (wf[0], p0, dt[0]); }
        if (u1) { p1 += dt[1]; if (p1 >= 1.f) p1 -= 1.f; s += lv[1] * wave (wf[1], p1, dt[1]); }
        if (u2) { p2 += dt[2]; if (p2 >= 1.f) p2 -= 1.f; s += lv[2] * wave (wf[2], p2, dt[2]); }
        if (un) s += noise * xorNoise (rngState);
        s *= inScale;

        g += dg;
        const float u = clip3 (s - k * s4);                 // realimentacion acotada: nunca explota
        s1 += g * (u - s1);
        s2 += g * (s1 - s2);
        s3 += g * (s2 - s3);
        s4 += g * (s3 - s4);

        a += da;
        out[i] += s4 * a;
    }
    v.ph[0] = p0; v.ph[1] = p1; v.ph[2] = p2; v.s1 = s1; v.s2 = s2; v.s3 = s3; v.s4 = s4;
    v.g = g1; v.gAmp = a1;
    if (v.amp.st == 0) { v.note = -1; v.released = false; v.sustained = false; v.s1 = v.s2 = v.s3 = v.s4 = 0.f; }
}
#endif

#if KOMODO_FULL
void MiniKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
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

        lfoPh += lfoInc * (float) n;
        if (lfoPh >= 1.f) { lfoPh -= 1.f; shVal = xorNoise (shState); }
        lfo = lfoShape (lsh, lfoPh, shVal);
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
void MiniKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
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

        lfoPh += lfoInc * (float) n;
        if (lfoPh >= 1.f) { lfoPh -= 1.f; shVal = xorNoise (shState); }
        switch (lsh)
        {
            case 0:  lfo = 4.f * std::abs (lfoPh - 0.5f) - 1.f; break;    // triangulo
            case 1:  lfo = 1.f - 2.f * lfoPh; break;                      // sierra descendente
            case 2:  lfo = lfoPh < 0.5f ? 1.f : -1.f; break;             // cuadrada
            default: lfo = shVal; break;                                  // sample & hold
        }

        std::fill (mono, mono + n, 0.f);
        const int nv = mode == 1 ? 1 : kVoices;
        for (int i = 0; i < nv; ++i)
            if (! voices[i].free()) renderVoice (voices[i], mono, n);

        for (int i = 0; i < n; ++i) mono[i] *= gain;
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

juce::AudioProcessorEditor* MiniKomodoAudioProcessor::createEditor() { return new ScaledEditor<MiniUi> (*this); }

void MiniKomodoAudioProcessor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    st.setProperty ("prog", curProg, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, d);
}
void MiniKomodoAudioProcessor::setStateInformation (const void* d, int sz)
{
    if (auto xml = getXmlFromBinary (d, sz))
        if (xml->hasTagName (apvts.state.getType()))
        {
            curProg = juce::jlimit (0, getNumPrograms() - 1, xml->getIntAttribute ("prog", 0));
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MiniKomodoAudioProcessor(); }
