#pragma once
#include "Common/KomodoDsp.h"

// Orden = indices de parametros (el editor los recorre en este orden)
enum { RNG, SAW, PUL, PW, PWM, SUB, NOI, LR, LP,
       CUT, RES, ENV, KEY, LFF, GL, VOL,
       FA, FD, FS, FR, AA, AD, AS, AR,
       HPF, MODE, CHO, NP };
inline const char* const paramIds[NP] = {
       "rng","saw","pul","pw","pwm","sub","noi","lr","lp",
       "cut","res","env","key","lff","gl","vol",
       "fa","fd","fs","fr","aa","ad","as","ar",
       "hpf","mode","cho" };

constexpr int kVoices = 6;     // como el Juno-60

// ---------------------------------------------------------------- voz
struct Voice
{
    int note = -1;
    bool released = false, sustained = false;
    unsigned age = 0;
    float vel = 1.f, tgtF = 110.f, curF = 110.f;
    float ph = 0.f, sph = 0.f;                          // fase oscilador y sub
    float hpL = 0.f, s1 = 0.f, s2 = 0.f, s3 = 0.f, s4 = 0.f;
    float gAmp = 0.f, g = 0.1f;                         // valores del tick anterior (para rampa)
    Env amp, flt;

    bool free() const noexcept { return amp.st == 0; }
};

// ---------------------------------------------------------------- procesador
class PolyKomodoAudioProcessor : public juce::AudioProcessor
{
public:
    PolyKomodoAudioProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "PolyKomodo"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return curProg; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void readParams();
    void handle (const juce::MidiMessage&);
    void noteOn (int n, float vel);
    void noteOff (int n);
    void releaseAll();
    Voice* allocate (int n);
    void renderVoice (Voice&, float* out, int n);

    std::atomic<float>* pp[NP] {};
    Voice voices[kVoices];
    Chorus chorus;
    int held[32] {}, nHeld = 0, curProg = 0, mode = 0, lastMode = 0, cho = 0;
    unsigned ageCounter = 0, rngState = 22222u;
    bool sustain = false;
    double sr = 44100.0;
    float invSr = 1.f / 44100.f, bend = 0.f, modWheel = 0.f, lfoPh = 0.f, lfo = 0.f;

    // parametros ya convertidos (se calculan una vez por bloque)
    float lsaw = 0, lpul = 0, lsub = 0, noi = 0, pwBase = 0.5f, pwmAmt = 0, vibCents = 0, lfoInc = 0;
    float cut = 2000, res = 0, envAmt = 0, keyAmt = 0, lffAmt = 0, glideC = 1, rangeMul = 1, hpa = 0, gain = 0;
    bool hpOn = false;
    EnvP ampP, fltP;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyKomodoAudioProcessor)
};
