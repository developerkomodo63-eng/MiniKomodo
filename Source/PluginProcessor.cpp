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
    0.f, .3f, 0.f, .6f };                                  // GL LR LA VOL

float T (float x) { return 0.002f + x * x * 3.f; }          // 0..1 -> segundos (igual que la version web)

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
} // namespace

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;

juce::AudioProcessorValueTreeState::ParameterLayout MiniKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using R = juce::NormalisableRange<float>;
    const R u (0.f, 1.f);
    auto f = [&] (int i, const char* name, R r)
        { p.push_back (std::make_unique<APF> (juce::ParameterID { paramIds[i], 1 }, name, r, kDefault[i])); };
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
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void MiniKomodoAudioProcessor::prepareToPlay (double s, int)
{
    sr = s; invSr = 1.f / (float) s;
    chorus.prepare (s);
    for (auto& v : voices) v = Voice();
    nHeld = 0; sustain = false; lfoPh = 0.f; bend = 0.f;
    lastMode = mode = (int) pp[MODE]->load();
}

// ---------------------------------------------------------------- MIDI / voces
void MiniKomodoAudioProcessor::noteOn (int n, float vel)
{
    const float hz = (float) juce::MidiMessage::getMidiNoteInHertz (n);
    const float vg = 0.65f + 0.35f * vel;

    if (mode == 1)                                                        // mono legato, prioridad a la ultima nota
    {
        auto& v = voices[0];
        const bool wasHeld = nHeld > 0;
        for (int i = 0; i < nHeld; ++i) if (held[i] == n) { std::memmove (held + i, held + i + 1, sizeof (int) * (size_t) (nHeld - i - 1)); --nHeld; break; }
        if (nHeld < 32) held[nHeld++] = n;
        v.tgtF = hz; v.note = n; v.vel = vg; v.released = false; v.age = ++ageCounter;
        if (v.free()) v.curF = hz;                                        // sin glide desde el silencio
        if (! wasHeld) { v.amp.on(); v.flt.on(); }                        // legato: no retrigger
        return;
    }

    auto* v = allocateVoice (voices, n);
    v->note = n; v->vel = vg; v->tgtF = v->curF = hz;
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

// Suma una voz en out[0..n). Control (frecuencia, filtro, envolventes) 1 vez por tick, rampa lineal dentro.
void MiniKomodoAudioProcessor::renderVoice (Voice& v, float* out, int n)
{
    v.curF += (v.tgtF - v.curF) * glideC;
    const float base = v.curF * std::exp2 ((bend * 200.f + lfo * vibCents) * (1.f / 1200.f));
    float dt[3];
    for (int k = 0; k < 3; ++k) dt[k] = juce::jmin (base * mul[k] * invSr, 0.45f);

    const float a0 = v.gAmp, a1 = v.amp.next (ampP) * v.vel;
    const float fe = v.flt.next (fltP);
    const float oct = amt * 5.f * fe + lfo * lffAmt * 2.f + keyAmt * (float) (v.note - 60) * (1.f / 12.f);
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

void MiniKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
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
        pos = next;
    }
}

juce::AudioProcessorEditor* MiniKomodoAudioProcessor::createEditor() { return new MiniKomodoEditor (*this); }

void MiniKomodoAudioProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}
void MiniKomodoAudioProcessor::setStateInformation (const void* d, int sz)
{
    if (auto xml = getXmlFromBinary (d, sz))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MiniKomodoAudioProcessor(); }
