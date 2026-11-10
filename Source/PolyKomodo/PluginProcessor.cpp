#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// valores por defecto, en el mismo orden que el enum
const float kDefault[NP] = {
    1.f, .8f, 0.f, .5f, 0.f, .3f, 0.f, .3f, 0.f,          // RNG SAW PUL PW PWM SUB NOI LR LP
    2000.f, .2f, .4f, .5f, 0.f, 0.f, .6f,                  // CUT RES ENV KEY LFF GL VOL
    .01f, .35f, .3f, .25f, .01f, .3f, .8f, .25f,           // FA FD FS FR AA AD AS AR
    0.f, 0.f, 0.f };                                       // HPF MODE CHO

float T (float x) { return 0.001f + x * x * x * 8.f; }     // 0..1 -> segundos

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
        { "Bass Sub",       { {RNG,0}, {SAW,.5f}, {SUB,.9f}, {CUT,450}, {RES,.1f}, {ENV,.3f}, {KEY,.3f}, {FD,.2f}, {FS,.1f}, {FR,.08f}, {AA,0}, {AD,.25f}, {AS,.85f}, {AR,.08f}, {MODE,1} } },
        { "Bass Pluck",     { {RNG,1}, {SAW,.9f}, {PUL,.3f}, {SUB,.5f}, {CUT,320}, {RES,.3f}, {ENV,.75f}, {KEY,.4f}, {FD,.18f}, {FS,0}, {FR,.1f}, {AD,.2f}, {AS,.2f}, {AR,.1f}, {MODE,1} } },
        { "Bass Resonant",  { {RNG,0}, {SAW,1}, {SUB,.5f}, {CUT,280}, {RES,.65f}, {ENV,.8f}, {KEY,.3f}, {FD,.25f}, {FS,.1f}, {FR,.1f}, {AS,.9f}, {AR,.08f}, {GL,.1f}, {MODE,1} } },
        { "Bass Square",    { {RNG,0}, {SAW,0}, {PUL,1}, {PW,.5f}, {SUB,.5f}, {CUT,700}, {RES,.2f}, {ENV,.3f}, {FD,.3f}, {FS,.4f}, {AR,.08f}, {MODE,1} } },
        { "Bass Wide",      { {RNG,0}, {SAW,.7f}, {SUB,.6f}, {CUT,650}, {RES,.2f}, {ENV,.3f}, {KEY,.4f}, {FD,.3f}, {FS,.4f}, {AR,.1f}, {CHO,1} } },
        // ---- Lead
        { "Lead Saw",       { {RNG,1}, {SAW,1}, {SUB,0}, {CUT,3500}, {RES,.3f}, {ENV,.3f}, {KEY,.8f}, {FD,.4f}, {FS,.6f}, {AS,.9f}, {AR,.15f}, {GL,.1f}, {LP,.05f}, {LR,.45f}, {MODE,1} } },
        { "Lead PWM",       { {RNG,1}, {SAW,0}, {PUL,1}, {PW,.5f}, {PWM,.6f}, {LR,.3f}, {SUB,.2f}, {CUT,4500}, {RES,.2f}, {ENV,.2f}, {KEY,.8f}, {AS,.9f}, {AR,.15f}, {CHO,1}, {MODE,1} } },
        { "Lead Bright",    { {RNG,1}, {SAW,.8f}, {PUL,.6f}, {PW,.45f}, {SUB,0}, {CUT,6000}, {RES,.45f}, {ENV,.5f}, {KEY,.8f}, {FD,.3f}, {FS,.5f}, {AS,.9f}, {AR,.15f}, {GL,.05f}, {MODE,1} } },
        { "Lead Soft",      { {RNG,1}, {SAW,.5f}, {PUL,.5f}, {PW,.35f}, {SUB,.1f}, {CUT,2500}, {RES,.15f}, {ENV,.25f}, {KEY,.7f}, {AA,.05f}, {AS,.9f}, {AR,.2f}, {LP,.1f}, {LR,.5f}, {GL,.15f}, {CHO,2}, {MODE,1} } },
        { "Lead Octave",    { {RNG,1}, {SAW,1}, {SUB,.8f}, {CUT,3000}, {RES,.25f}, {ENV,.3f}, {KEY,.7f}, {AS,.9f}, {AR,.12f}, {GL,.05f}, {MODE,1} } },
        // ---- Pad
        { "Pad Warm",       { {RNG,1}, {SAW,.7f}, {PUL,.5f}, {PW,.5f}, {PWM,.5f}, {LR,.15f}, {SUB,.2f}, {CUT,1400}, {RES,.15f}, {ENV,.2f}, {FA,.35f}, {FD,.5f}, {FS,.7f}, {FR,.55f}, {AA,.45f}, {AD,.5f}, {AS,.85f}, {AR,.6f}, {CHO,2} } },
        { "Pad Strings",    { {RNG,1}, {SAW,1}, {SUB,0}, {CUT,2200}, {RES,.1f}, {ENV,.15f}, {FA,.4f}, {FD,.5f}, {FS,.8f}, {FR,.5f}, {AA,.4f}, {AD,.5f}, {AS,.9f}, {AR,.5f}, {HPF,1}, {CHO,2} } },
        { "Pad Glass",      { {RNG,2}, {SAW,0}, {PUL,1}, {PW,.4f}, {PWM,.8f}, {LR,.2f}, {SUB,0}, {CUT,5000}, {RES,.3f}, {ENV,.1f}, {FA,.3f}, {FS,.8f}, {AA,.3f}, {AS,.9f}, {AR,.6f}, {HPF,1}, {CHO,1} } },
        { "Pad Dark",       { {RNG,1}, {SAW,.6f}, {PUL,.6f}, {SUB,.6f}, {CUT,600}, {RES,.25f}, {ENV,.5f}, {FA,.6f}, {FD,.6f}, {FS,.5f}, {FR,.6f}, {AA,.5f}, {AS,.9f}, {AR,.65f}, {CHO,2} } },
        { "Pad Air",        { {RNG,1}, {SAW,.5f}, {PUL,.5f}, {PW,.3f}, {PWM,.3f}, {NOI,.15f}, {CUT,1800}, {RES,.35f}, {ENV,.3f}, {FA,.5f}, {FD,.6f}, {FS,.6f}, {FR,.6f}, {AA,.5f}, {AS,.9f}, {AR,.7f}, {LR,.2f}, {CHO,1} } },
        // ---- Brass
        { "Brass Classic",  { {RNG,1}, {SAW,1}, {SUB,.3f}, {CUT,700}, {RES,.15f}, {ENV,.75f}, {KEY,.5f}, {FA,.06f}, {FD,.35f}, {FS,.6f}, {FR,.12f}, {AA,.03f}, {AD,.3f}, {AS,.9f}, {AR,.1f} } },
        { "Brass Section",  { {SAW,.9f}, {PUL,.4f}, {PW,.5f}, {SUB,.2f}, {CUT,800}, {RES,.15f}, {ENV,.7f}, {FA,.1f}, {FD,.35f}, {FS,.6f}, {FR,.15f}, {AA,.05f}, {AS,.9f}, {AR,.12f}, {CHO,1} } },
        { "Brass Stab",     { {SAW,1}, {SUB,.2f}, {CUT,600}, {RES,.2f}, {ENV,.85f}, {FA,0}, {FD,.18f}, {FS,.3f}, {FR,.1f}, {AA,0}, {AD,.25f}, {AS,.6f}, {AR,.08f} } },
        { "Brass Soft",     { {SAW,.8f}, {SUB,.2f}, {CUT,500}, {RES,.1f}, {ENV,.6f}, {FA,.15f}, {FD,.4f}, {FS,.6f}, {FR,.2f}, {AA,.1f}, {AS,.9f}, {AR,.15f}, {CHO,2} } },
        { "Brass Synth",    { {SAW,.8f}, {PUL,.6f}, {PW,.45f}, {PWM,.2f}, {SUB,.2f}, {CUT,1000}, {RES,.3f}, {ENV,.7f}, {FA,.1f}, {FD,.4f}, {FS,.5f}, {FR,.15f}, {AA,.04f}, {AS,.9f}, {AR,.12f}, {HPF,1}, {CHO,1} } },
    };
    return p;
}
} // namespace

// ---------------------------------------------------------------- parametros
juce::AudioProcessorValueTreeState::ParameterLayout PolyKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using R = juce::NormalisableRange<float>;
    auto f = [&] (int i, const char* name, R r)
        { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { paramIds[i], 1 }, name, r, kDefault[i])); };
    auto c = [&] (int i, const char* name, juce::StringArray s)
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
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void PolyKomodoAudioProcessor::prepareToPlay (double s, int)
{
    sr = s; invSr = 1.f / (float) s;
    chorus.prepare (s);
    for (auto& v : voices) { v = Voice(); }
    nHeld = 0; sustain = false; lfoPh = 0.f; bend = 0.f;
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

// ---------------------------------------------------------------- MIDI / voces
Voice* PolyKomodoAudioProcessor::allocate (int n) { return allocateVoice (voices, n); }

void PolyKomodoAudioProcessor::noteOn (int n, float vel)
{
    const float hz = (float) juce::MidiMessage::getMidiNoteInHertz (n);
    const float vg = 0.65f + 0.35f * vel;

    if (mode == 1)                                                        // mono legato, prioridad a la ultima nota
    {
        auto& v = voices[0];
        const bool wasHeld = nHeld > 0;
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { held[i] = held[--nHeld]; break; }
        if (nHeld < 32) held[nHeld++] = n;
        v.tgtF = hz; v.note = n; v.vel = vg; v.released = false; v.age = ++ageCounter;
        if (v.free()) v.curF = hz;                                        // sin glide desde el silencio
        if (! wasHeld) { v.amp.on(); v.flt.on(); }
        return;
    }

    auto* v = allocate (n);
    v->note = n; v->vel = vg; v->tgtF = v->curF = hz;
    v->released = v->sustained = false; v->age = ++ageCounter;
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
    if (m.isNoteOn())            noteOn (m.getNoteNumber(), m.getFloatVelocity());
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
    cut = g (CUT); res = g (RES); envAmt = g (ENV); keyAmt = g (KEY); lffAmt = g (LFF);
    rangeMul = std::exp2 (g (RNG) - 1.f);
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

// Suma una voz en out[0..n). Los parametros de control se actualizan 1 vez por tick (16 muestras)
// y se interpolan linealmente dentro del tick: barato y sin "zipper noise".
void PolyKomodoAudioProcessor::renderVoice (Voice& v, float* out, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float f   = v.curF * rangeMul * std::exp2 ((bend * 200.f + lfo * vibCents) * (1.f / 1200.f));
    const float dt  = juce::jmin (f * invSr, 0.45f);
    const float sdt = dt * 0.5f;
    const float pw  = juce::jlimit (0.05f, 0.95f, pwBase + lfo * pwmAmt * 0.4f);
    const float dc  = 2.f * pw - 1.f;

    const float a0 = v.gAmp, a1 = v.amp.next (ampP) * v.vel;
    const float fe = v.flt.next (fltP);
    const float oct = envAmt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f);
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

void PolyKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int total = b.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, total, true);

    readParams();
    if (mode != lastMode) { releaseAll(); lastMode = mode; }

    float* L = b.getWritePointer (0);
    float* R = b.getNumChannels() > 1 ? b.getWritePointer (1) : nullptr;
    float mono[kChunk], cl[kChunk], cr[kChunk];

    auto it = midi.begin();
    int pos = 0;
    while (pos < total)
    {
        while (it != midi.end() && (*it).samplePosition <= pos) { handle ((*it).getMessage()); ++it; }
        int next = juce::jmin (total, pos + kChunk);
        if (it != midi.end()) next = juce::jmin (next, (*it).samplePosition);
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
        pos = next;
    }
}

juce::AudioProcessorEditor* PolyKomodoAudioProcessor::createEditor() { return new PolyKomodoEditor (*this); }

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
