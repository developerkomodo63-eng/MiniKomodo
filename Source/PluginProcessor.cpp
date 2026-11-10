#include "PluginProcessor.h"
#include "PluginEditor.h"

static float T (float x) { return 0.002f + x * x * 3.f; }   // 0..1 -> segundos (igual que la version web)

static float blep (float t, float dt)                          // PolyBLEP: reduce aliasing
{
    if (t < dt)        { t /= dt;               return t + t - t * t - 1.f; }
    if (t > 1.f - dt)  { t = (t - 1.f) / dt;    return t * t + t + t + 1.f; }
    return 0.f;
}

static float wave (int w, float t, float dt)
{
    switch (w)
    {
        case 0: return 1.f - 4.f * std::abs (t - 0.5f);                  // triangulo
        case 1: return 2.f * t - 1.f - blep (t, dt);                      // sierra
        case 2: return -(2.f * t - 1.f - blep (t, dt));                   // sierra inversa
        default:
        {
            const float pw = (w == 3 ? 0.5f : w == 4 ? 0.25f : 0.1f);    // cuadrada / pulso ancho / angosto
            float v = t < pw ? 1.f : -1.f;
            v += blep (t, dt);
            v -= blep (std::fmod (t + 1.f - pw, 1.f), dt);
            return v;
        }
    }
}

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;

juce::AudioProcessorValueTreeState::ParameterLayout MiniKomodoAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    juce::StringArray rng { "32'", "16'", "8'", "4'", "2'" },
        wav { "Triangulo", "Sierra", "Sierra inv", "Cuadrada", "Pulso ancho", "Pulso angosto" };

    for (int i = 0; i < 3; ++i)
        p.push_back (std::make_unique<APC> (juce::ParameterID { "r" + juce::String (i), 1 }, "Osc " + juce::String (i + 1) + " rango", rng, 2));
    for (int i = 0; i < 3; ++i)
        p.push_back (std::make_unique<APC> (juce::ParameterID { "w" + juce::String (i), 1 }, "Osc " + juce::String (i + 1) + " onda", wav, 1));

    auto f = [&] (const char* id, const char* name, juce::NormalisableRange<float> r, float def)
        { p.push_back (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def)); };
    using R = juce::NormalisableRange<float>;

    f ("d1", "Osc2 fino", R (-100.f, 100.f), 0.f);   f ("d2", "Osc3 fino", R (-100.f, 100.f), 0.f);
    f ("l0", "Nivel 1", R (0.f, 1.f), 1.f);          f ("l1", "Nivel 2", R (0.f, 1.f), 0.7f);
    f ("l2", "Nivel 3", R (0.f, 1.f), 0.f);          f ("nz", "Ruido", R (0.f, 1.f), 0.f);
    f ("cut", "Corte", R (40.f, 18000.f, 0.f, 0.3f), 1500.f);
    f ("res", "Resonancia", R (0.f, 1.f), 0.25f);    f ("amt", "Cant. env", R (0.f, 1.f), 0.5f);
    f ("fa", "Filtro A", R (0.f, 1.f), 0.01f);       f ("fd", "Filtro D", R (0.f, 1.f), 0.35f);
    f ("fs", "Filtro S", R (0.f, 1.f), 0.2f);
    f ("aa", "Vol A", R (0.f, 1.f), 0.01f);          f ("ad", "Vol D", R (0.f, 1.f), 0.3f);
    f ("as", "Vol S", R (0.f, 1.f), 0.8f);
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dec", 1 }, "Decay on", true));
    f ("gl", "Glide", R (0.f, 1.f), 0.f);            f ("lr", "LFO vel.", R (0.f, 1.f), 0.3f);
    f ("la", "LFO vibrato", R (0.f, 1.f), 0.f);      f ("vol", "Master", R (0.f, 1.f), 0.6f);
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
    sr = s;
    ladder.prepare ({ s, 16u, 1u });
    ladder.setMode (juce::dsp::LadderFilter<float>::Mode::LPF24);   // 24 dB/oct, estilo Moog
    ladder.reset();
    ampEnv.setSampleRate (s);  fltEnv.setSampleRate (s);
    ampEnv.reset();  fltEnv.reset();
    held.clear();
}

void MiniKomodoAudioProcessor::handle (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        const bool first = held.empty();
        held.erase (std::remove (held.begin(), held.end(), n), held.end());
        held.push_back (n);
        tgtF = (float) juce::MidiMessage::getMidiNoteInHertz (n);
        if (first && ! ampEnv.isActive()) curF = tgtF;                // sin glide desde el silencio
        if (first) { ampEnv.noteOn(); fltEnv.noteOn(); }              // legato: no retrigger
    }
    else if (m.isNoteOff())
    {
        held.erase (std::remove (held.begin(), held.end(), m.getNoteNumber()), held.end());
        if (held.empty()) { ampEnv.noteOff(); fltEnv.noteOff(); }
        else tgtF = (float) juce::MidiMessage::getMidiNoteInHertz (held.back());   // prioridad a la ultima nota
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear(); ampEnv.reset(); fltEnv.reset();
    }
}

void MiniKomodoAudioProcessor::render (float* out, int n)
{
    auto g = [this] (int i) { return pp[i]->load(); };
    const float oct[3] { g (R0) - 2.f, g (R1) - 2.f, g (R2) - 2.f };
    const float det[3] { 0.f, g (D1), g (D2) };
    const float lv[3]  { g (L0), g (L1), g (L2) };
    const int   wf[3]  { (int) g (W0), (int) g (W1), (int) g (W2) };
    const float tc = g (GL) * g (GL) * 0.4f;
    const float gc = tc < 0.002f ? 1.f : 1.f - std::exp (-1.f / (tc * (float) sr));
    const float lfoInc = (0.5f + g (LR) * 9.f) / (float) sr, lfoCents = g (LA) * 60.f;
    const float vol = g (VOL) * g (VOL), noise = g (NZ) * 0.8f;

    float fe = 0.f;
    for (int i = 0; i < n; ++i)
    {
        curF += (tgtF - curF) * gc;
        lfoPh += lfoInc; if (lfoPh >= 1.f) lfoPh -= 1.f;
        const float lfo = std::sin (6.2831853f * lfoPh) * lfoCents;

        float s = 0.f;
        for (int k = 0; k < 3; ++k)
        {
            const float f  = curF * std::exp2 (oct[k] + (det[k] + lfo) / 1200.f);
            const float dt = juce::jmin (f / (float) sr, 0.45f);
            ph[k] += dt; if (ph[k] >= 1.f) ph[k] -= 1.f;
            s += lv[k] * wave (wf[k], ph[k], dt);
        }
        s += noise * (rng.nextFloat() * 2.f - 1.f);
        tmp[i] = s * 0.4f;
        ae[i]  = ampEnv.getNextSample();
        fe     = fltEnv.getNextSample();
    }

    // Filtro a "control rate" (cada 16 muestras): barato y suficiente
    const float fc = juce::jmin (g (CUT) * std::exp2 (g (AMT) * 5.f * fe), 0.45f * (float) sr);
    ladder.setCutoffFrequencyHz (fc);
    ladder.setResonance (g (RES) * 0.95f);
    float* ch[1] { tmp };
    juce::dsp::AudioBlock<float> blk (ch, 1, (size_t) n);
    ladder.process (juce::dsp::ProcessContextReplacing<float> (blk));

    for (int i = 0; i < n; ++i) out[i] = tmp[i] * ae[i] * vol;
}

void MiniKomodoAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int total = b.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, total, true);

    auto g = [this] (int i) { return pp[i]->load(); };
    const float rel = g (DEC) > 0.5f ? T (g (AD)) : 0.015f;
    ampEnv.setParameters ({ T (g (AA)), T (g (AD)), g (AS), rel });
    fltEnv.setParameters ({ T (g (FA)), T (g (FD)), g (FS), rel });

    float* out = b.getWritePointer (0);
    auto it = midi.begin();
    int pos = 0;
    while (pos < total)
    {
        while (it != midi.end() && (*it).samplePosition <= pos) { handle ((*it).getMessage()); ++it; }
        int next = juce::jmin (total, pos + 16);
        if (it != midi.end()) next = juce::jmin (next, (*it).samplePosition);
        render (out + pos, next - pos);
        pos = next;
    }
    for (int c = 1; c < b.getNumChannels(); ++c) b.copyFrom (c, 0, b, 0, 0, total);
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
