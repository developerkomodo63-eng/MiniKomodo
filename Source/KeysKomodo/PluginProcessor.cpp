#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// valores por defecto, en el mismo orden que el enum
const float kDefault[NP] = {
    0.f, 1.f, .02f, .55f, 0.f, .3f, .6f, 0.f, .4f,                    // ENG RNG ATK DEC SUS REL VEL VIB VBR
    0.f, .32f, .6f, .45f, .3f, .2f,                                   // FMR FMI FMV FMD TNE BRK
    1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, .2f,           // DB0..DB8 PRC CLK
    0.f, .3f, 3000.f, .1f, .2f, .35f, .5f,                            // WAV ENS CUT RES FEA FDC KEY
    0.f, .15f,                                                        // VOW BRT
    0.f, 0.f, 0.f, .4f, .5f, 3.f, 0.f, .35f, 0.f, .5f, .5f, .6f };   // DRV TONE MOD MDR MDD DLT DLM DLF RVM RVS RVD VOL

juce::AudioParameterFloatAttributes attrs (int kind)
{
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([kind] (float v, int)
        {
            if (kind == 1) return v >= 1000.f ? juce::String (v / 1000.f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
            return juce::String (juce::roundToInt (v * 100.f)) + "%";
        })
        .withValueFromStringFunction ([kind] (const juce::String& text)
        {
            const auto t = text.trim();
            const float x = t.getFloatValue();
            if (kind == 0) return x / 100.f;
            return t.containsIgnoreCase ("k") ? x * 1000.f : x;
        });
}

float T (float x) { return 0.001f + x * x * x * 8.f; }          // 0..1 -> segundos

inline float anaWave (int w, float t, float dt) noexcept
{
    const float saw = 2.f * t - 1.f - blep (t, dt);
    if (w == 0) return saw;
    float t2 = t + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
    const float sq = (t < 0.5f ? 1.f : -1.f) + blep (t, dt) - blep (t2, dt);
    return w == 1 ? sq : 0.6f * saw + 0.6f * sq;
}

struct Ov { int id; float v; };
struct Preset { const char* name; std::vector<Ov> ov; };

// "888000000" -> posiciones de drawbar (0..8) a DB0..DB8, mas el resto de parametros
std::vector<Ov> bars (const char* s, std::vector<Ov> rest)
{
    std::vector<Ov> v;
    for (int i = 0; i < 9; ++i) v.push_back ({ DB0 + i, (float) (s[i] - '0') / 8.f });
    for (auto& o : rest) v.push_back (o);
    v.push_back ({ ENG, 1 });
    return v;
}

// ENG: 0 piano electrico (FM), 1 organo, 2 analogico, 3 coro.  FMR: 0:1 1:2 2:3 3:3.5 4:4 5:7 6:14 7:0.5
// MOD: 0 off, 1 chorus, 2 ensemble, 3 phaser, 4 tremolo, 5 rotary lento, 6 rotary rapido.  PRC: 0 off, 1 2da rapida, 2 3ra rapida, 3 2da lenta, 4 3ra lenta
// DLT: 0 1/16 1 1/8 2 1/8. 3 1/4 4 1/4. 5 1/2.  WAV: 0 saw, 1 pulso, 2 saw+pulso.  VOW: 0 a, .25 e, .5 i, .75 o, 1 u
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", {} },
        // ---- Pianos electricos (FM)
        { "EP Suitcase Rhodes",  { {FMI,.32f}, {FMV,.6f}, {FMD,.45f}, {TNE,.35f}, {BRK,.25f}, {DEC,.6f}, {REL,.25f}, {VEL,.7f}, {MOD,4}, {MDR,.45f}, {MDD,.5f}, {DRV,.15f}, {RVM,.15f}, {VOL,0.456f} } },
        { "EP Stage Rhodes Bark",{ {FMI,.42f}, {FMV,.8f}, {FMD,.4f}, {TNE,.45f}, {BRK,.7f}, {DEC,.6f}, {REL,.22f}, {VEL,.8f}, {MOD,1}, {MDD,.3f}, {DRV,.25f}, {RVM,.12f}, {VOL,0.501f} } },
        { "EP Mark I Soft",      { {FMI,.22f}, {FMV,.4f}, {FMD,.5f}, {TNE,.2f}, {BRK,.1f}, {DEC,.65f}, {REL,.3f}, {MOD,1}, {MDD,.2f}, {RVM,.2f}, {VOL,0.495f} } },
        { "EP Wurly",            { {FMR,1}, {FMI,.3f}, {FMV,.6f}, {FMD,.35f}, {TNE,.1f}, {BRK,.55f}, {DEC,.5f}, {REL,.2f}, {MOD,4}, {MDR,.35f}, {MDD,.35f}, {DRV,.2f}, {VOL,0.477f} } },
        { "EP DX Classic",       { {FMI,.4f}, {FMV,.7f}, {FMD,.35f}, {TNE,.6f}, {BRK,0}, {DEC,.65f}, {REL,.3f}, {MOD,1}, {MDD,.6f}, {RVM,.2f}, {VOL,0.482f} } },
        { "EP Dream Phase",      { {FMI,.3f}, {FMV,.5f}, {FMD,.5f}, {TNE,.3f}, {DEC,.7f}, {REL,.4f}, {MOD,3}, {MDR,.25f}, {MDD,.6f}, {DLM,.25f}, {RVM,.3f}, {VOL,0.445f} } },
        // ---- Mallets / Bells (FM)
        { "Bell Vibraphone",     { {FMR,4}, {FMI,.15f}, {FMV,.3f}, {FMD,.3f}, {TNE,0}, {BRK,0}, {DEC,.6f}, {REL,.5f}, {MOD,4}, {MDR,.5f}, {MDD,.45f}, {RVM,.25f}, {VOL,0.446f} } },
        { "Bell Marimba",        { {FMR,3}, {FMI,.12f}, {FMV,.3f}, {FMD,.12f}, {TNE,0}, {BRK,0}, {DEC,.35f}, {REL,.15f}, {RVM,.15f}, {VOL,0.472f} } },
        { "Bell Glass",          { {RNG,2}, {FMR,6}, {FMI,.25f}, {FMV,.4f}, {FMD,.5f}, {TNE,.5f}, {BRK,0}, {DEC,.75f}, {REL,.6f}, {DLM,.2f}, {RVM,.35f}, {VOL,0.445f} } },
        { "Bell Music Box",      { {RNG,2}, {FMR,3}, {FMI,.2f}, {FMV,.3f}, {FMD,.2f}, {TNE,.1f}, {BRK,0}, {DEC,.45f}, {REL,.4f}, {RVM,.3f}, {VOL,0.455f} } },
        // ---- Organos (drawbars)
        { "ORG B3 Full",         bars ("888888888", { {SUS,1}, {DEC,.5f}, {ATK,0}, {REL,.06f}, {CLK,.25f}, {MOD,5}, {MDD,.7f}, {DRV,.25f}, {RVM,.12f}, {VOL,0.461f} }) },
        { "ORG Jazz 888",        bars ("888000000", { {SUS,1}, {DEC,.5f}, {ATK,0}, {REL,.06f}, {CLK,.3f}, {PRC,2}, {MOD,5}, {MDD,.7f}, {DRV,.15f}, {RVM,.12f}, {VOL,0.479f} }) },
        { "ORG Rock Fast",       bars ("888000568", { {SUS,1}, {DEC,.5f}, {ATK,0}, {REL,.05f}, {CLK,.3f}, {PRC,1}, {MOD,6}, {MDD,.8f}, {DRV,.4f}, {VOL,0.445f} }) },
        { "ORG Gospel",          bars ("884400000", { {SUS,1}, {DEC,.5f}, {ATK,0}, {REL,.07f}, {CLK,.25f}, {MOD,5}, {MDD,.8f}, {DRV,.2f}, {RVM,.25f}, {VOL,0.485f} }) },
        { "ORG Perc Lead",       bars ("808000000", { {SUS,1}, {DEC,.5f}, {ATK,0}, {REL,.05f}, {CLK,.2f}, {PRC,2}, {MOD,5}, {MDD,.6f}, {VOL,0.450f} }) },
        { "ORG Church",          bars ("407850004", { {SUS,1}, {DEC,.5f}, {ATK,.18f}, {REL,.45f}, {CLK,0}, {MOD,0}, {RVM,.5f}, {RVS,.9f}, {VOL,0.451f} }) },
        { "ORG Soft Flute",      bars ("006500000", { {SUS,1}, {DEC,.5f}, {ATK,.1f}, {REL,.25f}, {CLK,.05f}, {RVM,.2f}, {VOL,0.510f} }) },
        // ---- Strings / Pads / Brass / Keys (analogico)
        { "STR String Machine",  { {ENG,2}, {WAV,0}, {ENS,.6f}, {CUT,3500}, {RES,.05f}, {FEA,.1f}, {KEY,.5f}, {ATK,.3f}, {SUS,1}, {REL,.5f}, {MOD,2}, {MDD,.8f}, {RVM,.2f}, {VOL,0.482f} } },
        { "STR Solina",          { {ENG,2}, {WAV,0}, {ENS,.4f}, {CUT,2500}, {RES,.05f}, {FEA,0}, {KEY,.5f}, {ATK,.25f}, {SUS,1}, {REL,.6f}, {MOD,2}, {MDD,1}, {RVM,.3f}, {VOL,0.486f} } },
        { "STR Slow Strings",    { {ENG,2}, {WAV,0}, {ENS,.5f}, {CUT,2000}, {RES,.05f}, {FEA,.15f}, {FDC,.6f}, {KEY,.5f}, {ATK,.6f}, {SUS,1}, {REL,.7f}, {MOD,2}, {MDD,.7f}, {RVM,.35f}, {VOL,0.727f} } },
        { "PAD Warm",            { {ENG,2}, {WAV,2}, {ENS,.5f}, {CUT,1200}, {RES,.1f}, {FEA,.2f}, {FDC,.6f}, {KEY,.4f}, {ATK,.5f}, {SUS,1}, {REL,.7f}, {MOD,1}, {MDD,.5f}, {RVM,.35f}, {VOL,0.622f} } },
        { "PAD Glass Phase",     { {ENG,2}, {WAV,1}, {ENS,.3f}, {CUT,3000}, {RES,.15f}, {FEA,.1f}, {KEY,.6f}, {ATK,.4f}, {SUS,1}, {REL,.6f}, {MOD,3}, {MDR,.2f}, {MDD,.6f}, {RVM,.4f}, {VOL,0.445f} } },
        { "BRS Poly Brass",      { {ENG,2}, {WAV,0}, {ENS,.2f}, {CUT,900}, {RES,.15f}, {FEA,.7f}, {FDC,.45f}, {KEY,.5f}, {ATK,.08f}, {SUS,1}, {REL,.2f}, {MOD,1}, {MDD,.3f}, {VOL,0.469f} } },
        { "BRS Synth Stab",      { {ENG,2}, {WAV,0}, {ENS,.25f}, {CUT,700}, {RES,.2f}, {FEA,.8f}, {FDC,.22f}, {KEY,.5f}, {ATK,0}, {DEC,.4f}, {SUS,.3f}, {REL,.12f}, {MOD,1}, {MDD,.3f}, {DLM,.15f}, {VOL,0.506f} } },
        { "KEY Clav D6",         { {ENG,2}, {WAV,1}, {ENS,0}, {CUT,2500}, {RES,.25f}, {FEA,.6f}, {FDC,.15f}, {KEY,.8f}, {ATK,0}, {DEC,.3f}, {SUS,0}, {REL,.1f}, {VOL,0.461f} } },
        { "KEY Clav Wah",        { {ENG,2}, {WAV,1}, {ENS,0}, {CUT,1800}, {RES,.35f}, {FEA,.5f}, {FDC,.15f}, {KEY,.8f}, {ATK,0}, {DEC,.3f}, {SUS,0}, {REL,.1f}, {MOD,3}, {MDR,.5f}, {MDD,.7f}, {VOL,0.445f} } },
        { "KEY Harpsichord",     { {ENG,2}, {WAV,2}, {ENS,.05f}, {CUT,6000}, {RES,.1f}, {FEA,.3f}, {FDC,.1f}, {KEY,.8f}, {ATK,0}, {DEC,.35f}, {SUS,0}, {REL,.08f}, {RVM,.2f}, {VOL,0.547f} } },
        { "KEY Farfisa",         { {ENG,2}, {WAV,1}, {ENS,.15f}, {CUT,5000}, {RES,.1f}, {FEA,0}, {KEY,.6f}, {ATK,.005f}, {SUS,1}, {REL,.12f}, {MOD,1}, {MDD,.4f}, {VOL,0.445f} } },
        { "KEY Pizzicato",       { {ENG,2}, {WAV,0}, {ENS,.3f}, {CUT,1500}, {RES,.1f}, {FEA,.5f}, {FDC,.1f}, {KEY,.6f}, {ATK,0}, {DEC,.22f}, {SUS,0}, {REL,.15f}, {RVM,.2f}, {VOL,0.604f} } },
        // ---- Coros
        { "VOX Choir Aahs",      { {ENG,3}, {VOW,0}, {BRT,.15f}, {ATK,.35f}, {SUS,1}, {REL,.55f}, {VIB,.25f}, {VBR,.35f}, {MOD,2}, {MDD,.7f}, {RVM,.4f}, {VOL,0.573f} } },
        { "VOX Choir Oohs",      { {ENG,3}, {VOW,.85f}, {BRT,.1f}, {ATK,.4f}, {SUS,1}, {REL,.6f}, {VIB,.2f}, {VBR,.35f}, {MOD,2}, {MDD,.7f}, {RVM,.4f}, {VOL,0.665f} } },
        { "VOX Eee Pad",         { {ENG,3}, {VOW,.5f}, {BRT,.2f}, {ATK,.5f}, {SUS,1}, {REL,.6f}, {VIB,.15f}, {MOD,1}, {MDD,.5f}, {RVM,.4f}, {VOL,0.465f} } },
        { "VOX Angel",           { {ENG,3}, {RNG,2}, {VOW,.25f}, {BRT,.1f}, {ATK,.5f}, {SUS,1}, {REL,.7f}, {VIB,.3f}, {VBR,.4f}, {MOD,2}, {MDD,.8f}, {DLM,.2f}, {RVM,.5f}, {VOL,0.552f} } },
    };
    return p;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout KeysKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using R = juce::NormalisableRange<float>;
    const R u (0.f, 1.f);
    auto f = [&] (int i, const juce::String& name, R r)
        { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { paramIds[i], 1 }, name, r, kDefault[i], attrs (i == CUT ? 1 : 0))); };
    auto c = [&] (int i, const juce::String& name, juce::StringArray s)
        { p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { paramIds[i], 1 }, name, s, (int) kDefault[i])); };

    c (ENG, "Motor", { "Electric Piano", "Organ", "Analog Keys", "Choir" });
    c (RNG, "Octava", { "-1 oct", "0", "+1 oct" });
    f (ATK, "Ataque", u);  f (DEC, "Decay", u);  f (SUS, "Sustain", u);  f (REL, "Release", u);
    f (VEL, "Velocity", u);  f (VIB, "Vibrato", u);  f (VBR, "Vibrato vel.", u);
    c (FMR, "FM ratio", { "1", "2", "3", "3.5", "4", "7", "14", "0.5" });
    f (FMI, "FM indice", u);  f (FMV, "Vel > indice", u);  f (FMD, "Decay indice", u);  f (TNE, "Tine", u);  f (BRK, "Bark", u);
    for (int i = 0; i < 9; ++i) f (DB0 + i, "Drawbar " + juce::String (i + 1), u);
    c (PRC, "Percusion", { "Off", "2da rapida", "3ra rapida", "2da lenta", "3ra lenta" });
    f (CLK, "Click", u);
    c (WAV, "Onda", { "Saw", "Pulso", "Saw+Pulso" });
    f (ENS, "Ensemble", u);  f (CUT, "Corte", R (40.f, 18000.f, 0.f, 0.3f));  f (RES, "Resonancia", u);
    f (FEA, "Env filtro", u);  f (FDC, "Decay filtro", u);  f (KEY, "Keytrack", u);
    f (VOW, "Vocal", u);  f (BRT, "Aire", u);
    f (DRV, "Drive", u);  f (TONE, "Tono", R (-1.f, 1.f));
    c (MOD, "Modulacion", { "Off", "Chorus", "Ensemble", "Phaser", "Tremolo", "Rotary lento", "Rotary rapido" });
    f (MDR, "Mod vel.", u);  f (MDD, "Mod prof.", u);
    c (DLT, "Delay tiempo", { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2" });
    f (DLM, "Delay mix", u);  f (DLF, "Delay fdbk", u);
    f (RVM, "Reverb mix", u);  f (RVS, "Reverb size", u);  f (RVD, "Reverb damp", u);
    f (VOL, "Master", u);
    return { p.begin(), p.end() };
}

KeysKomodoAudioProcessor::KeysKomodoAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", layout())
{
    for (int i = 0; i < NP; ++i) pp[i] = apvts.getRawParameterValue (paramIds[i]);
    sinTable();                                           // construye la tabla fuera del hilo de audio
}

bool KeysKomodoAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void KeysKomodoAudioProcessor::prepareToPlay (double s, int)
{
    sr = s; invSr = 1.f / (float) s;
    chorus.prepare (s); ensemble.prepare (s); phaser.prepare (s); trem.prepare (s); rotary.prepare (s);
    delay.prepare (s); reverb.setSampleRate (s); reverb.reset(); reverbWasOn = false;
    for (auto& v : voices) v = Voice();
    sustain = false; bend = 0.f; lfoPh = 0.f; toneLp = 0.f; lastMod = -1;
}

// ---------------------------------------------------------------- programas
int KeysKomodoAudioProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String KeysKomodoAudioProcessor::getProgramName (int i)
{
    const auto& p = presets();
    return (i >= 0 && i < (int) p.size()) ? juce::String (p[(size_t) i].name) : juce::String();
}

void KeysKomodoAudioProcessor::setCurrentProgram (int i)
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

void KeysKomodoAudioProcessor::handleAsyncUpdate()
{
    const int p = pendingProgram.exchange (-1);
    if (p >= 0) setCurrentProgram (juce::jmin (p, getNumPrograms() - 1));
}

void KeysKomodoAudioProcessor::doPanic()
{
    for (auto& v : voices) v = Voice();
    sustain = false;
    chorus.clear(); ensemble.clear(); phaser.clear(); rotary.clear(); delay.clear(); reverb.reset();
}

// ---------------------------------------------------------------- MIDI / voces
void KeysKomodoAudioProcessor::noteOn (int n, float vel)
{
    auto* v = allocateVoice (voices, n);
    *v = Voice();
    v->note = n; v->vel = vel; v->hz = (float) juce::MidiMessage::getMidiNoteInHertz (n); v->age = ++ageCounter;
    for (auto& ph : v->p) ph = (eng == 0 || eng == 1) ? 0.f : std::abs (xorNoise (rngState));   // fases al azar solo en analogico/coro
    v->amp.on(); v->flt.on();
}

void KeysKomodoAudioProcessor::noteOff (int n)
{
    for (auto& v : voices)
        if (v.note == n && ! v.released && ! v.free())
        {
            if (sustain) v.sustained = true;
            else { v.amp.off(); v.flt.off(); v.released = true; }
        }
}

void KeysKomodoAudioProcessor::releaseAll()
{
    for (auto& v : voices) { v.amp.off(); v.flt.off(); v.released = true; v.sustained = false; }
}

void KeysKomodoAudioProcessor::handle (const juce::MidiMessage& m)
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
void KeysKomodoAudioProcessor::readParams()
{
    auto g = [this] (int i) { return pp[i]->load(); };
    const int newEng = (int) g (ENG);
    if (newEng != eng) { eng = newEng; releaseAll(); }
    mod = (int) g (MOD);
    rangeMul = std::exp2 (g (RNG) - 1.f);
    velS = g (VEL);
    vibCents = g (VIB) * 40.f + modWheel * 50.f;
    lfoInc = (2.f + g (VBR) * 6.f) * invSr;

    const float tick = (float) sr / (float) kChunk;
    ampP = makeEnvSec (T (g (ATK)), T (g (DEC)), g (SUS), T (g (REL)), tick);
    gain = g (VOL) * g (VOL) * 2.f;

    static const float ratios[8] { 1.f, 2.f, 3.f, 3.5f, 4.f, 7.f, 14.f, 0.5f };
    fmRatio = ratios[juce::jlimit (0, 7, (int) g (FMR))];
    fmI = g (FMI);  fmV = g (FMV);  fmTine = g (TNE);  fmBark = g (BRK);
    fmIdxCoef = 1.f - std::exp (-4.6f / (T (g (FMD)) * tick));
    tineCoef = 1.f - std::exp (-4.6f / (0.22f * tick));

    for (int i = 0; i < 9; ++i) { const float v = g (DB0 + i); dbg[i] = v < 0.01f ? 0.f : std::exp2 (-(1.f - v) * 4.f); }
    prc = (int) g (PRC);
    percCoef = 1.f - std::exp (-4.6f / ((prc >= 3 ? 0.7f : 0.22f) * tick));
    clickAmt = g (CLK) * 0.5f;
    clickCoef = 1.f - std::exp (-4.6f / (0.012f * tick));

    wav = (int) g (WAV);  ensCents = g (ENS) * 18.f;
    cut = g (CUT);  res = g (RES);  fea = g (FEA);  keyAmt = g (KEY);
    fltP = makeEnvSec (0.002f, juce::jmax (0.02f, T (g (FDC))), 0.15f, T (g (REL)), tick);

    // coro: formantes interpolados entre a-e-i-o-u
    static const float vow[5][3] { { 800.f, 1150.f, 2900.f }, { 400.f, 1700.f, 2600.f }, { 270.f, 2140.f, 2950.f },
                                   { 450.f, 800.f, 2830.f },  { 325.f, 700.f, 2700.f } };
    const float pos = juce::jlimit (0.f, 1.f, g (VOW)) * 3.999f;
    const int i0 = (int) pos;  const float fr = pos - (float) i0;
    for (int j = 0; j < 3; ++j)
    {
        const float fc = juce::jlimit (80.f, 0.2f * (float) sr, vow[i0][j] + (vow[i0 + 1][j] - vow[i0][j]) * fr);
        fF[j] = 2.f * std::sin (3.14159265f * fc * invSr);
    }
    brt = g (BRT);

    drv = g (DRV);  tone = g (TONE);  mdr = g (MDR);  mdd = g (MDD);
    dlm = g (DLM);  dlf = g (DLF) * 0.85f;  rvm = g (RVM);
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos2 = ph->getPosition())
            if (auto b = pos2->getBpm()) bpm = *b;
    static const double beats[6] { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
    dlTarget = (float) juce::jmin (beats[juce::jlimit (0, 5, (int) g (DLT))] * 60.0 / juce::jmax (20.0, bpm) * sr, sr * 2.15);
}

// Suma una voz en out[0..n). Envolventes y parametros 1 vez por tick (16 muestras), rampa lineal dentro.
void KeysKomodoAudioProcessor::renderVoice (Voice& v, float* out, int n)
{
    const float* Tb = sinTable();
    const float ae = v.amp.next (ampP);
    const float fe = v.flt.next (fltP);
    const float f  = v.hz * rangeMul * std::exp2 ((bend * 200.f + lfo * vibCents) * (1.f / 1200.f));
    const float a0 = v.gAmp, a1 = ae * (1.f - velS * (1.f - v.vel));
    const float inv = 1.f / (float) n, da = (a1 - a0) * inv;
    float a = a0;

    switch (eng)
    {
        case 0:   // ---------------- piano electrico: 2 pares FM (cuerpo + tine) + pickup
        {
            v.envI += (0.18f - v.envI) * fmIdxCoef;
            v.tineE -= v.tineE * tineCoef;
            const float vs = 1.f - fmV + fmV * v.vel;
            const float IA = fmI * 0.55f * vs * v.envI, IB = 0.3f * vs * v.tineE;
            const float dc = f * invSr, dm = dc * fmRatio, dt = dc * 14.f;
            const float drive = 1.f + fmBark * vs * 4.f, asym = fmBark * 0.25f, norm = 0.6f / (1.f + fmBark * 1.5f);
            const float tn = fmTine * v.tineE;
            float p0 = v.p[0], p1 = v.p[1], p2 = v.p[2];
            for (int i = 0; i < n; ++i)
            {
                p0 = wrap1 (p0 + dc);  p1 = wrap1 (p1 + dm);  p2 = wrap1 (p2 + dt);
                float y = sinLU (Tb, p0 + IA * sinLU (Tb, p1)) + tn * sinLU (Tb, p0 + IB * sinLU (Tb, p2));
                y = clip3 (y * drive + asym * y * y) * norm;
                a += da;
                out[i] += y * a;
            }
            v.p[0] = p0; v.p[1] = p1; v.p[2] = p2;
            break;
        }
        case 1:   // ---------------- organo: drawbars aditivos (armonicos sobre una fase base de 16')
        {
            static const int K[9] { 1, 3, 2, 4, 6, 8, 10, 12, 16 };
            const float fb = f * 0.5f, lim = 0.45f * (float) sr;
            int kk[9], cnt = 0;  float gl[9], sumG = 0.f;
            for (int i = 0; i < 9; ++i)
                if (dbg[i] > 0.f && fb * (float) K[i] < lim) { kk[cnt] = K[i]; gl[cnt] = dbg[i]; sumG += dbg[i]; ++cnt; }
            const int pk = (prc == 1 || prc == 3) ? 4 : 6;
            const bool usePerc = prc > 0 && fb * (float) pk < lim;
            v.percE -= v.percE * percCoef;
            v.click -= v.click * clickCoef;
            const float scl = 0.45f / (0.5f + sumG * 0.5f);
            const float pa = 0.6f * v.percE, ck = clickAmt * v.click;
            const float dc = fb * invSr;
            float p0 = v.p[0];
            for (int i = 0; i < n; ++i)
            {
                p0 = wrap1 (p0 + dc);
                float y = 0.f;
                for (int j = 0; j < cnt; ++j) y += gl[j] * sinLU (Tb, p0 * (float) kk[j]);
                if (usePerc) y += pa * sinLU (Tb, p0 * (float) pk);
                y *= scl;
                if (ck > 0.0005f) y += ck * xorNoise (rngState);
                a += da;
                out[i] += y * a;
            }
            v.p[0] = p0;
            break;
        }
        case 2:   // ---------------- analogico: hasta 3 osciladores detunados + ladder LP24
        {
            const float oct = fea * 5.f * fe + keyAmt * (float) (v.note - 60) * (1.f / 12.f) + velS * (v.vel - 0.5f) * 1.2f;
            const float fc = juce::jlimit (20.f, 0.25f * (float) sr, cut * std::exp2 (oct));
            const float x = 6.2831853f * fc * invSr;
            const float g1 = x / (1.f + x), k = res * 3.8f, inScale = 0.3f * (1.f + k * 0.25f);
            const bool three = ensCents > 0.05f;
            const float d1 = std::exp2 (ensCents * (1.f / 1200.f)), d2 = 1.f / d1;
            const float dt0 = juce::jmin (f * invSr, 0.45f), dt1 = juce::jmin (dt0 * d1, 0.45f), dt2 = juce::jmin (dt0 * d2, 0.45f);
            const float gO = three ? 0.62f : 1.f;
            float g = v.g < 0.f ? g1 : v.g;
            const float dg = (g1 - g) * inv;
            float p0 = v.p[0], p1 = v.p[1], p2 = v.p[2], st[4] { v.st[0], v.st[1], v.st[2], v.st[3] };
            for (int i = 0; i < n; ++i)
            {
                p0 += dt0; if (p0 >= 1.f) p0 -= 1.f;
                float s = anaWave (wav, p0, dt0);
                if (three)
                {
                    p1 += dt1; if (p1 >= 1.f) p1 -= 1.f;
                    p2 += dt2; if (p2 >= 1.f) p2 -= 1.f;
                    s += anaWave (wav, p1, dt1) + anaWave (wav, p2, dt2);
                }
                g += dg;  a += da;
                out[i] += ladderTick (s * gO * inScale, st, g, k, 0) * a * 2.4f;
            }
            v.p[0] = p0; v.p[1] = p1; v.p[2] = p2;
            for (int j = 0; j < 4; ++j) v.st[j] = st[j];
            v.g = g1;
            break;
        }
        default:  // ---------------- coro: sierra + soplido por 3 formantes (SVF)
        {
            const float dt0 = juce::jmin (f * invSr, 0.45f);
            float p0 = v.p[0], sv[6];
            for (int j = 0; j < 6; ++j) sv[j] = v.sv[j];
            for (int i = 0; i < n; ++i)
            {
                p0 += dt0; if (p0 >= 1.f) p0 -= 1.f;
                const float src = 0.9f * (2.f * p0 - 1.f - blep (p0, dt0)) + brt * 0.6f * xorNoise (rngState);
                float y = 0.f;
                for (int j = 0; j < 3; ++j)
                {
                    float& lo = sv[j * 2]; float& bp = sv[j * 2 + 1];
                    lo += fF[j] * bp;
                    const float hi = src - lo - fQ[j] * bp;
                    bp += fF[j] * hi;
                    y += fG[j] * fQ[j] * bp;
                }
                a += da;
                out[i] += y * 1.6f * a;
            }
            v.p[0] = p0;
            for (int j = 0; j < 6; ++j) v.sv[j] = sv[j];
            break;
        }
    }
    v.gAmp = a1;
    if (v.amp.st == 0) { v.note = -1; v.released = false; v.sustained = false; for (auto& q : v.st) q = 0.f; for (auto& q : v.sv) q = 0.f; }
}

void KeysKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int total = b.getNumSamples();
    if (b.getNumChannels() < 2) { b.clear(); return; }
    if (panicFlag.exchange (false)) doPanic();
    keyboardState.processNextMidiBuffer (midi, 0, total, true);
    readParams();

    float* L = b.getWritePointer (0);
    float* R = b.getWritePointer (1);

    auto it = midi.begin();
    int pos = 0;
    while (pos < total)
    {
        while (it != midi.end() && (*it).samplePosition <= pos) { handle ((*it).getMessage()); ++it; }
        int next = juce::jmin (total, pos + kChunk);
        if (it != midi.end()) next = juce::jmin (next, (*it).samplePosition);
        const int n = next - pos;

        lfoPh = wrap1 (lfoPh + lfoInc * (float) n);
        lfo = sinLU (sinTable(), lfoPh);

        std::fill (L + pos, L + next, 0.f);
        for (auto& v : voices)
            if (! v.free()) renderVoice (v, L + pos, n);
        pos = next;
    }

    // ---- efectos: drive -> tono -> modulacion (estereo) -> delay -> reverb
    const float gIn = 1.f + drv * 6.f, gOut = 1.f / (1.f + drv * 1.2f);
    const float lpc = 1.f - std::exp (-6.2831853f * 800.f * invSr), tt = tone * 0.8f;
    for (int i = 0; i < total; ++i)
    {
        float m = L[i] * gain;
        if (drv > 0.001f) m = clip3 (m * gIn) * gOut;
        toneLp += lpc * (m - toneLp);
        L[i] = toneLp * (1.f - tt) + (m - toneLp) * (1.f + tt);
    }

    if (mod != lastMod)
    {
        chorus.clear(); ensemble.clear(); phaser.clear(); rotary.clear();
        lastMod = mod;
    }
    switch (mod)
    {
        case 1:  std::copy (L, L + total, R); chorus.process (L, R, total, 1 + juce::jmin (2, (int) (mdd * 3.f))); break;
        case 2:  ensemble.process (L, R, total, 0.4f + mdd * 0.9f, 0.5f + mdr * 1.5f); break;
        case 3:  phaser.process (L, R, total, 0.1f + mdr * mdr * 6.f, mdd); break;
        case 4:  trem.process (L, R, total, 0.5f + mdr * 9.f, mdd); break;
        case 5:  rotary.process (L, R, total, false, 0.3f + 0.7f * mdd); break;
        case 6:  rotary.process (L, R, total, true, 0.3f + 0.7f * mdd); break;
        default: std::copy (L, L + total, R); break;
    }

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

juce::AudioProcessorEditor* KeysKomodoAudioProcessor::createEditor() { return new ScaledEditor<KeysUi> (*this); }

void KeysKomodoAudioProcessor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    st.setProperty ("prog", curProg, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, d);
}

void KeysKomodoAudioProcessor::setStateInformation (const void* d, int sz)
{
    if (auto xml = getXmlFromBinary (d, sz))
        if (xml->hasTagName (apvts.state.getType()))
        {
            curProg = juce::jlimit (0, getNumPrograms() - 1, xml->getIntAttribute ("prog", 0));
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeysKomodoAudioProcessor(); }
